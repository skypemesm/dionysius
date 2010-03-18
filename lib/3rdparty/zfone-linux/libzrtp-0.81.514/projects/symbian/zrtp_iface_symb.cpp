/*
 * Copyright (c) 2006-2009 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * For licensing and other legal details, see the file zrtp_legal.c.
 */

#include <charconv.h>
#include <stdarg.h>
#include <sys/time.h>

#include <e32msgqueue.h>

#include <zrtp.h>
#include <zrtp_iface_builtin.h>

//-----------------------------------------------------------------------------
zrtp_status_t zrtp_mutex_init(zrtp_mutex_t **mutex)
{
	RMutex *rmutex = new RMutex();
	rmutex->CreateLocal();
	*mutex = (zrtp_mutex_t*) rmutex;
	return zrtp_status_ok;	
}

zrtp_status_t zrtp_mutex_lock(zrtp_mutex_t* mutex)
{
	RMutex *rmutex = (RMutex *) mutex;
	rmutex->Wait();
	return zrtp_status_ok;	
}

zrtp_status_t zrtp_mutex_unlock(zrtp_mutex_t* mutex)
{
	RMutex *rmutex = (RMutex *) mutex;
	rmutex->Signal();
	return zrtp_status_ok;	
}

zrtp_status_t zrtp_mutex_destroy(zrtp_mutex_t* mutex)
{
	RMutex *rmutex = (RMutex *) mutex;
	if ( rmutex )
	{
		rmutex->Close();
		delete rmutex;
	}
	return zrtp_status_ok;	
}

//-----------------------------------------------------------------------------
zrtp_status_t zrtp_sem_init(zrtp_sem_t** sem, uint32_t value, uint32_t limit)
{
	RSemaphore *rsem = new RSemaphore();
	rsem->CreateLocal(value);
	*sem = (zrtp_sem_t*) rsem;
	return zrtp_status_ok;	
}

zrtp_status_t zrtp_sem_destroy(zrtp_sem_t* sem)
{
	RSemaphore *rsem = (RSemaphore *) sem;
	if (rsem) {
		rsem->Close();
		delete rsem;
	}
	return zrtp_status_ok;	   
}

zrtp_status_t zrtp_sem_wait(zrtp_sem_t* sem)
{
	RSemaphore *rsem = (RSemaphore *) sem;
	rsem->Wait();
	return zrtp_status_ok;	   
}
zrtp_status_t zrtp_sem_trtwait(zrtp_sem_t* sem)
{
	RSemaphore *rsem = (RSemaphore *) sem;
	rsem->Wait(1000);
	return zrtp_status_ok;	   
}
zrtp_status_t zrtp_sem_post(zrtp_sem_t* sem)
{
	RSemaphore *rsem = (RSemaphore *) sem;
	rsem->Signal();
	return zrtp_status_ok;	   
}

//-----------------------------------------------------------------------------
int zrtp_sleep(unsigned int msec)
{
   TTimeIntervalMicroSeconds32 i(msec *1000);
   User::After(i);
   return 0;
}
/*
typedef struct{
  zrtp_thread_routine_t start_routine;
  void *arg;
}TH_CREATE_STRUCT;

static int th_fnc(void *p){
   
   TH_CREATE_STRUCT *s=(TH_CREATE_STRUCT*)p;
   s->start_routine(s->arg);
   delete s;
   return 0;
}
*/
int zrtp_thread_create(zrtp_thread_routine_t start_routine, void *arg){
//int zrtp_thread_create(void *(*start_routine)(void *), void *arg){
   
   RThread h;
   TBuf<64> thName=_L("zrtp_thread");

   //TH_CREATE_STRUCT *s=new TH_CREATE_STRUCT;
  // s->start_routine = start_routine;
   //s->arg = arg;
   h.Create(thName, start_routine, KDefaultStackSize*2, NULL, arg) ;   
   h.Resume();
   h.Close();
   
   return 0;
}

