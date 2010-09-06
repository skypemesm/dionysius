/**
 * \file srpp_timer.hpp
 *
 * This file implements the functionalities of the timers used in SRPP
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#ifndef SRPP_TIMER_H
#define SRPP_TIMER_H


#include <iostream>
#include <sys/time.h>
#include <csignal>
#include <string>

#include "Padding_functions.h"

class SRPPTimer {

private:
	 static int packet_interval,silence_interval, packet_counter, silence_counter;
	 static bool packet_ON,silence_ON;
	 static struct itimerval pkt_tout_val,pkt_tin_val,silence_tout_val,silence_tin_val;


	 //Static Handler of the packet timer function
	 static void timer_fired(int i);
	 static void pkt_timer_fired();
	 static void silence_timer_fired();


	// Stop the Timer
	static void stopTimer(int i);

public:
	SRPPTimer(){}
	SRPPTimer(int packet_timer_interval,int silence_packet_interval);

	~SRPPTimer(){}

	//Start and set The timer to a given interval
	int setTimer();
	static int setPacketTimer(int packet_timer_interval);
	static int setSilenceTimer(int silence_timer_interval);

		/**
		 * Start: obviously start the individual timers
		 * Pause: obviously pause the individual timers
		 * To unpause, simply call the corresponding start functions
		 *
		 */
		static int start_packet();
		static int start_silence();
		static int pause_packet();
		static int pause_silence();
		static int reset_packet();
		static int reset_silence();

		//Pause the timer. This function might just not be needed at all.
		// Start the timer to unpause
		static int pauseTimer();
		static int startTimer();

};



#endif
