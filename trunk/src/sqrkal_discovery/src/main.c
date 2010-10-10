/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <signal.h>
#include <pthread.h>

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_	"zfone main"

//-------------------------------------------------------------------------------
void* sig_handler(void* arg)
{
	int sig;
	sigset_t signal_set;
	
	for(;;)
	{
		// Wait for any and all signals
		sigfillset(&signal_set);
		sigwait(&signal_set, &sig);
		
		// Stop daemon on all terminated and core signals
		if ( (sig != SIGCHLD) && (sig != SIGCONT) && (sig != SIGSTOP) && (sig != SIGTSTP) &&
			(sig != SIGTTIN) && (sig != SIGTTOU) && (sig != SIGURG) && (sig != SIGWINCH) )
		{			
			// Core signal has been acptured - send notification to GUI
			if ( (sig == SIGQUIT) || (sig == SIGILL) || (sig == SIGABRT) ||
				(sig == SIGFPE) || (sig == SIGSEGV) || (sig == SIGBUS) ||
				(sig == SIGSYS) || (sig == SIGTRAP) || (sig == SIGXCPU) ||
				(sig == SIGXFSZ) || (sig == SIGIOT) )
			{
				ZRTP_LOG(3, (_ZTU_, "ZFONED sigINT: handling core signal", sig));
				send_crash_cmd();			
			}
			else
			{
				ZRTP_LOG(3, (_ZTU_, "ZFONED sigINT: handling terminate signal", sig));
				send_stop_cmd();
			}
			
			zfone_stop();
			zfone_down();
		}
	}
	return (void*)0;
}


//-------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	int res = 0;
	sigset_t signal_set;
	pthread_t sig_thread;
	
    if ( daemon(1, 1) < 0)
		return -1;

	// Block all signals and  create a single signals handling thread
	sigfillset(&signal_set);
	pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
	pthread_create(&sig_thread, NULL, sig_handler, NULL);


	fclose(stderr);

	// First of all initialize logger 
	zrtp_log_set_log_engine(zfone_log_write);
    zrtp_init_loggers(LOG_PATH);
	
	res = init_ctrl_server();
	if (0 != res)
	{
		ZRTP_LOG(1, (_ZTU_, "ZFONED main: Error number:%d during zfone control server starting.", res));
		return res;
	}
    
    // Create, initialize and start VoIP deamon
    res = zfone_init();
    if (0 != res)
    {
		ZRTP_LOG(1, (_ZTU_, "ZFONED main: Error number:%d during zfoned initialization.", res));
		return res;
    }
    
	res = zfone_start();
    if (0 != res)
    {
		ZRTP_LOG(1, (_ZTU_, "ZFONED main: Error number:%d during zfoned starting.", res));
		return res;
    }
		
    run_ctrl_server();
	
    return 0;
}
