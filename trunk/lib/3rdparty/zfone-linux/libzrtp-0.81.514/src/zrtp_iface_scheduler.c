/*
 * Copyright (c) 2006-2009 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * For licensing and other legal details, see the file zrtp_legal.c.
 *
 * Viktor Krikun <v.krikun at zfoneproject.com>
 */

#include "zrtp.h"

#if defined(ZRTP_DONT_USE_BUILTIN) && (ZRTP_DONT_USE_BUILTIN == 0)

/* Windows kernel have it's own realization based on kernel timers */
#if (ZRTP_PLATFORM != ZP_WIN32_KERNEL)

#define ZRTP_SCHED_QUEUE_SIZE	ZRTP_MAX_STREAMS_PER_SESSION * 1000
#define ZRTP_SCHED_LOOP_QVANT 20

#define ZRTP_SCHED_SLEEP(count) zrtp_sleep(ZRTP_SCHED_LOOP_QVANT*count);


/** Schedulling tasks structure */
typedef struct 
	{    
		zrtp_stream_t   *ctx;		/** ZRTP stream context associated with the task */
		zrtp_retry_task_t	*ztask;		/** ZRTP stream associated with the task */
		uint64_t			wake_at;	/* Wake time in milliseconds */
		mlist_t				_mlist;
	} zrtp_sched_task_t;

/** Initiation flag. Protection from reinitialization.  (1 if initiated) */
static uint8_t		inited = 0;

/** Sorted by wake time tasks list.  First task to do at the begining */
static mlist_t		tasks_head;

/** Tasks queue protector againts race conditions on add/remove tasks */
static zrtp_mutex_t* protector = NULL;

/** Main queue symaphore */
static zrtp_sem_t* count = NULL;

static uint8_t is_running = 0;
static uint8_t is_working = 0;


/*==========================================================================*/
/*				   	     Platform Dependent Routine                         */
/*==========================================================================*/

#if (ZRTP_PLATFORM == ZP_WIN32) || (ZRTP_PLATFORM == ZP_WINCE)
#include <Windows.h>

int zrtp_sleep(unsigned int msec)
{
	Sleep(msec);
	return 0;
}

int zrtp_thread_create(zrtp_thread_routine_t start_routine, void *arg)
{
	DWORD	dwThreadId;
	HANDLE	handle;
	
	handle = CreateThread(NULL, 0, start_routine, 0, 0, &dwThreadId);
	if (!handle) {
		return -1;
	}
	
	CloseHandle(handle);	
	return 0;
}

#elif (ZRTP_PLATFORM == ZP_LINUX) || (ZRTP_PLATFORM == ZP_DARWIN)
#if ZRTP_HAVE_UNISTD_H == 1
#include <unistd.h>
#else
#error "Used environment dosn't have <unistd.h> - zrtp_scheduler can't be build."
#endif
#if ZRTP_HAVE_PTHREAD_H == 1
#include <pthread.h>
#else
#	error "Used environment dosn't have <pthread.h> - zrtp_scheduler can't be build."
#endif

int zrtp_sleep(unsigned int msec)
{
	return usleep(msec*1000);
}

int zrtp_thread_create(zrtp_thread_routine_t start_routine, void *arg)
{
	pthread_t thread;
	return pthread_create(&thread, NULL, start_routine, NULL);
}
#endif


/*==========================================================================*/
/*				   	     Scheduler Implementation                           */
/*==========================================================================*/

#if   (ZRTP_PLATFORM == ZP_WIN32) || (ZRTP_PLATFORM == ZP_WIN64) || (ZRTP_PLATFORM == ZP_WINCE)
static DWORD WINAPI sched_loop(void* param)
#else
static void* sched_loop(void* param)
#endif
{
	is_working = 1;
	while (is_running)
	{
		zrtp_sched_task_t* task = NULL;		
		zrtp_sched_task_t task2run;
		int ready_2_run = 0;
		mlist_t* node = 0;

		/* Wait for tasks in queue */
		zrtp_sem_wait(count);
		 
		zrtp_mutex_lock(protector);
		
		node = mlist_get(&tasks_head);
		if (!node) {	
			zrtp_mutex_unlock(protector);
			continue;
		}

		task = mlist_get_struct(zrtp_sched_task_t, _mlist, node);
		if (task->wake_at <= zrtp_time_now())
		{
			task2run.ctx = task->ctx;
			task2run.ztask = task->ztask;
			mlist_del(node);
			zrtp_sys_free(task);
			ready_2_run = 1;
		}
		
		zrtp_mutex_unlock(protector);
		
		if (ready_2_run) {
			task2run.ztask->_is_busy = 1;
			task2run.ztask->callback(task2run.ctx, task2run.ztask);
			task2run.ztask->_is_busy = 0;
		} else {
			zrtp_sem_post(count);
		}
		
		ZRTP_SCHED_SLEEP(1);
	}
	
	is_working = 0;
	
#if   (ZRTP_PLATFORM != ZP_WIN32) && (ZRTP_PLATFORM != ZP_WIN64) && (ZRTP_PLATFORM != ZP_WINCE)
	return NULL;
#else
	return 0;	
#endif
}

/*---------------------------------------------------------------------------*/
zrtp_status_t zrtp_def_scheduler_init(zrtp_global_t* zrtp)
{	
	zrtp_status_t status = zrtp_status_ok;
	
	if (inited) {
		return zrtp_status_ok;
	}
	
	do {
		init_mlist(&tasks_head);

		if (zrtp_status_ok != (status = zrtp_mutex_init(&protector))) {
			break;
		}		
		if (zrtp_status_ok != (status = zrtp_sem_init(&count, 0, ZRTP_SCHED_QUEUE_SIZE))) {
			break;
		}

		/* Starting processing loop */
		is_running = 1;
		
		if (0 != zrtp_thread_create(sched_loop, NULL)) {
			zrtp_sem_destroy(count);
			zrtp_mutex_destroy(protector);
			
			status = zrtp_status_fail;
			break;
		}

		inited  = 1;
	} while (0);

	return status;
}

/*---------------------------------------------------------------------------*/
void zrtp_def_scheduler_down()
{	
	mlist_t *node = 0, *tmp = 0;
	
	if (!inited) {
		return;
	}

	/* Stop main thread */		
	is_running = 0;	
	zrtp_sem_post(count);
	while (is_working) {
		ZRTP_SCHED_SLEEP(1);
	}
	
	/* Then destroy tasks queue and realease all other resources */
	zrtp_mutex_lock(protector);

	mlist_for_each_safe(node, tmp, &tasks_head) {
		zrtp_sched_task_t* task = mlist_get_struct(zrtp_sched_task_t, _mlist, node);
		zrtp_sys_free(task);
	}
	init_mlist(&tasks_head);

	zrtp_mutex_unlock(protector);
	
	zrtp_mutex_destroy(protector);
	zrtp_sem_destroy(count);
	
	inited  = 0;
}

/*---------------------------------------------------------------------------*/
void zrtp_def_scheduler_call_later(zrtp_stream_t *ctx, zrtp_retry_task_t* ztask)
{	
	mlist_t *node=0, *tmp=0;			
	mlist_t* last = &tasks_head;

	zrtp_mutex_lock(protector);

	if (!ztask->_is_enabled) {
		zrtp_mutex_unlock(protector);
		return;
	}

	do {
		zrtp_sched_task_t* new_task = zrtp_sys_alloc(sizeof(zrtp_sched_task_t));
		if (!new_task) {	
			break;
		}

		new_task->ctx			= ctx;
		new_task->ztask			= ztask;		
		new_task->wake_at		= zrtp_time_now() + ztask->timeout;
		
		/* Try to find element with later wacked time than we have */
		mlist_for_each_safe(node, tmp, &tasks_head) {
			zrtp_sched_task_t* tmp_task = mlist_get_struct(zrtp_sched_task_t, _mlist, node);
			if (tmp_task->wake_at >= new_task->wake_at) {
				last = node;
				break;
			}
		}

		/*
		 * If packet wasn't inserted (empty queue or all elements are smaller)
		 * Put them to the end of the queue.
		 */
		mlist_insert(last, &new_task->_mlist);		

		zrtp_sem_post(count);
	} while (0);

	zrtp_mutex_unlock(protector);		
}

/*---------------------------------------------------------------------------*/
void zrtp_def_scheduler_cancel_call_later(zrtp_stream_t* ctx, zrtp_retry_task_t* ztask)
{
	mlist_t *node=0, *tmp=0;

	zrtp_mutex_lock(protector);

	mlist_for_each_safe(node, tmp, &tasks_head) {
		zrtp_sched_task_t* task = mlist_get_struct(zrtp_sched_task_t, _mlist, node);
		if ((task->ctx == ctx) && ((task->ztask == ztask) || !ztask)) {
			mlist_del(&task->_mlist);
			zrtp_sys_free(task);
			zrtp_sem_trtwait(count);
			if (ztask) {
				break;
			}
		}
	}

	zrtp_mutex_unlock(protector);	
}

void zrtp_def_scheduler_wait_call_later(zrtp_stream_t* ctx)
{
	while (ctx->messages.hello_task._is_busy) {
		ZRTP_SCHED_SLEEP(1);
	}
	while (ctx->messages.commit_task._is_busy) {
		ZRTP_SCHED_SLEEP(1);
	}
	while (ctx->messages.dhpart_task._is_busy) {
		ZRTP_SCHED_SLEEP(1);
	}
	while (ctx->messages.confirm_task._is_busy) {
		ZRTP_SCHED_SLEEP(1);
	}
	while (ctx->messages.error_task._is_busy) {
		ZRTP_SCHED_SLEEP(1);
	}
	while (ctx->messages.errorack_task._is_busy) {
		ZRTP_SCHED_SLEEP(1);
	}
	while (ctx->messages.goclear_task._is_busy) {
		ZRTP_SCHED_SLEEP(1);	
	}
	while (ctx->messages.dh_task._is_busy) {
		ZRTP_SCHED_SLEEP(1);
	}
}

#endif /* not for windows kernel */

#endif /* ZRTP_DONT_USE_BUILTIN */
