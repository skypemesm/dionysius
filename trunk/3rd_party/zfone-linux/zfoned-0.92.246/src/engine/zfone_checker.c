/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>

#if ZRTP_PLATFORM != ZP_WIN32_KERNEL
	#include <pthread.h>	/* for threading */
	#include <unistd.h>		/* for sleep() */     
#endif

#include "zfone.h"

#define _ZTU_ "zfone checker"

static int zfone_checker_running = 0;

#if ZRTP_PLATFORM == ZP_WIN32_KERNEL    
    PVOID  zfone_schecker_thread;
#endif


#if ZRTP_PLATFORM == ZP_WIN32_KERNEL
	#define SLEEP(x) NdisMSleep(x)
#else
	#define SLEEP(x) usleep(x)
#endif

extern int zfone_call_finish_check(struct zfone_heurist *heurist);
extern int zfone_clean_dead_sessions(struct zfone_heurist *heurist);
extern void _zfone_print_heurist_state(struct zfone_heurist *heurist);


/*----------------------------------------------------------------------------*/
void check_stream(zfone_stream_t* stream)
{
    uint64_t time_diff, time_now = zrtp_time_now(); 
	time_diff = time_now - stream->last_inc_activity;
	if ( (stream->rx_state > ZFONED_ALERTED_NO_ACTIV) && (time_diff >= ZFONED_RTP_ACTIVITY_TIMEOUT) )
    {
        stream->rx_state = ZFONED_NO_ACTIV;
        cb_rtp_activity(stream, ZFONE_IO_IN, ZFONED_NO_ACTIV);
    }
	else if ( (stream->rx_state == ZFONED_NO_ACTIV) && (time_diff >= ZFONED_RTP_ACTIVITY_ALERT_TIMEOUT) )
	{
        stream->rx_state = ZFONED_ALERTED_NO_ACTIV;
        cb_rtp_activity(stream, ZFONE_IO_IN, ZFONED_ALERTED_NO_ACTIV);
	}
	
	time_diff = time_now - stream->last_out_activity;
    if ( (stream->tx_state > ZFONED_ALERTED_NO_ACTIV) && (time_diff >= ZFONED_RTP_ACTIVITY_TIMEOUT) )
	{
        stream->tx_state = ZFONED_NO_ACTIV;
        cb_rtp_activity(stream, ZFONE_IO_OUT, ZFONED_NO_ACTIV);
    }
    else if ( (stream->tx_state == ZFONED_NO_ACTIV) && (time_diff >= ZFONED_RTP_ACTIVITY_ALERT_TIMEOUT) )
	{
        stream->tx_state = ZFONED_ALERTED_NO_ACTIV;
        cb_rtp_activity(stream, ZFONE_IO_OUT, ZFONED_ALERTED_NO_ACTIV);
    }
}


/*----------------------------------------------------------------------------*/
#if   (ZRTP_PLATFORM == ZP_WIN32_KERNEL)
	static void doit(void *param)
#else
	static void *doit(void *param)
#endif
{
    unsigned int	count = 0;
    
#if ZRTP_PLATFORM == ZP_WIN32_KERNEL	
	__try
	{
#endif
		while (zfone_checker_running)
		{
			SLEEP(ZFONED_TIME_QUANTUM*1000);
			if (count % ZFONED_RTP_ACTIVITY_INTERVAL == 0)	
	  			manager.for_each_stream(&manager, check_stream);

			if(0 == count % ZFONED_HEURIST_CALL_CHECK)
				heurist.clean_closed_calls(&heurist);			

			if(0 == count % ZFONED_HEURIST_RTPS_CHECK)
				heurist.clean_dead_streams(&heurist);			
#if 1 // Display heurist internal state
			if(0 == count % ZFONED_HEURIST_RTPSHOW_CHECK)
			{
				zrtp_mutex_lock(heurist.protector);
				heurist.show_state(&heurist);
				zrtp_mutex_unlock(heurist.protector);
			}			
#endif

#if   (ZRTP_PLATFORM != ZP_WIN32_KERNEL)
			if (count % ZFONED_INTERFACES_TIMEOUT == 0)
			{
	  			if ( zfone_manager_refresh_ip_list() )
	  				send_set_ips(interfaces, flags, interfaces_count);
			}
#endif		

			count++;
		}
#if ZRTP_PLATFORM == ZP_WIN32_KERNEL	
    }
   	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ZRTP_LOG(1, (_ZTU_, "zfoned: (doit)"
						" EXCEPTION %x.\n", GetExceptionCode()));
	}	
#endif

#if (ZRTP_PLATFORM == ZP_WIN32_KERNEL)
	PsTerminateSystemThread(STATUS_SUCCESS);	
#else
	return NULL;
#endif
}
             
//-----------------------------------------------------------------------------
int zfone_checker_start()
{
#if ZRTP_PLATFORM == ZP_WIN32_KERNEL
	HANDLE zfone_schecker_handle = NULL;
#endif
    zfone_checker_running = 1;

#if ZRTP_PLATFORM == ZP_WIN32_KERNEL	
	if (STATUS_SUCCESS != PsCreateSystemThread( &zfone_schecker_handle,
												THREAD_ALL_ACCESS,
												0, 0, 0,
												doit, NULL) )
	{
		ZRTP_LOG(1, (_ZTU_, "zfoned: (checker.start) PsCreateSystemThread failed!\n"));
		return -1;
	}			

				
	if (STATUS_SUCCESS != ObReferenceObjectByHandle( zfone_schecker_handle,
													 THREAD_ALL_ACCESS,
													 NULL,
													 KernelMode,
													 &zfone_schecker_thread,
													 NULL) )
	{
		ZRTP_LOG(1, (_ZTU_, "zfoned: (checker.start) Can't get thread object for the next destroying.\n"));
		zfone_schecker_thread = NULL;				
		ZwClose(zfone_schecker_handle);		
		return -2;
	}

	// We don't need this handler any longer - let's close it right now
	// and don't wait till don_pipeline (had problems with this in the past)
	ZwClose(zfone_schecker_handle);

	return 0;
#else
	pthread_t thread;
    return pthread_create(&thread, NULL, doit, NULL);
#endif
}

//-----------------------------------------------------------------------------
void zfone_checker_stop()
{
	zfone_checker_running = 0;
#if ZRTP_PLATFORM == ZP_WIN32_KERNEL	
	if (zfone_schecker_thread)
	{						
		KeWaitForSingleObject(zfone_schecker_thread, Executive, KernelMode, TRUE, NULL);
		ObDereferenceObject(zfone_schecker_thread);
	}
#endif
};

/*----------------------------------------------------------------------------*/
void zfone_checker_reg_rtp(zfone_packet_t *packet, int dir, int passed)
{
    uint8_t state = passed ? ZFONED_PASS_ACTIV : ZFONED_DROP_ACTIV;
    zfone_stream_t *stream = packet->stream;

	if ( !stream )
	{
		ZRTP_LOG(1, (_ZTU_, "[checker::reg_rtp] : can find stream.\n"));
		return;
	}
	
	if (dir == ZFONE_IO_IN)
	{
		stream->last_inc_activity = zrtp_time_now();
		if (stream->rx_state <= ZFONED_ALERTED_NO_ACTIV || state != stream->rx_state)
		{
			stream->rx_state = state;
	  		cb_rtp_activity(stream, dir, state);
		}
    }
    else if (dir == ZFONE_IO_OUT)
    {
		stream->last_out_activity = zrtp_time_now();
		if (stream->tx_state <= ZFONED_ALERTED_NO_ACTIV || state != stream->tx_state)
		{
	  		stream->tx_state = state;
	  		cb_rtp_activity(stream, dir, state);
		}
    }
}

//-----------------------------------------------------------------------------
void reset_rtp(zfone_stream_t* stream)
{
    stream->rx_state = ZFONED_NO_ACTIV;
    stream->tx_state = ZFONED_NO_ACTIV;
}

void zfone_checker_reset_rtp_activity()
{
	manager.for_each_stream(&manager, reset_rtp);
}
