/**
 * \file srpp_timer.hpp
 *
 * This file implements the functionalities of the timers used in SRPP
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "../include/srpp_timer.h"
#include "../include/SRPP_functions.h"

#include <cstdlib>

using namespace std;

	int SRPPTimer::packet_counter = 0;
	int SRPPTimer::silence_counter = 0;

	int SRPPTimer::packet_interval = 0;
	bool SRPPTimer::packet_ON = false;
	struct itimerval SRPPTimer::pkt_tout_val;
	struct itimerval SRPPTimer::pkt_tin_val;

	int SRPPTimer::silence_interval = 0;
	bool SRPPTimer::silence_ON = false;
	struct itimerval SRPPTimer::silence_tout_val;
	struct itimerval SRPPTimer::silence_tin_val;

	//Static handler for timer function
	void SRPPTimer::timer_fired(int i)
	{
		signal(i,timer_fired);
		packet_counter ++;
		silence_counter ++;

		if (packet_counter == packet_interval && packet_ON == true)
			pkt_timer_fired();
		if (silence_counter == silence_interval && silence_ON == true)
			silence_timer_fired();

		signal(i,timer_fired);
	}

	//Static Handler of the packet timer function
	 void SRPPTimer::pkt_timer_fired()
	 {
           cout << "Packet Timer fired..." << endl;

			PaddingFunctions * padding_functions = srpp::get_padding_functions();
			padding_functions->current_burst_padding();

           reset_packet();
	 }

	 //Static Handler of the silence timer function
	 void SRPPTimer::silence_timer_fired()
	 {
			cout << "Silence Timer fired..." << endl;

			PaddingFunctions * padding_functions = srpp::get_padding_functions();
			padding_functions->extra_burst_padding();
			reset_silence();
	 }

	// Stop the Timer
	void SRPPTimer::stopTimer(int i)
	{
		pauseTimer();
		cout << "All SRPP Timers stopped..." << endl;
		signal(SIGINT,SIG_DFL);
	}

	//Constructor
	SRPPTimer::SRPPTimer(int packet_timer_interval, int silence_timer_interval)
	{
		setPacketTimer(packet_timer_interval); // set the packet interval
		setSilenceTimer(silence_timer_interval); //set the silence interval
		//startTimer();       // BUT DO NOT START PACKET OR SILENCE, it will be set when the session starts
		setTimer();           //start the regular timer though.

	}

	//Set Packet timer interval
	int SRPPTimer::setPacketTimer(int packet_timer_interval)
	{
		packet_interval = packet_timer_interval;

		return 0;
	}

	//Set Packet timer interval
	int SRPPTimer::setSilenceTimer(int silence_timer_interval)
	{
		silence_interval = silence_timer_interval;
		return 0;
	}

	//Start and set The timer to a given interval
	int SRPPTimer::setTimer()
	{
		  pkt_tin_val.it_interval.tv_sec = 0;
  		  pkt_tin_val.it_interval.tv_usec = 0;
  		  pkt_tin_val.it_value.tv_sec = 0;
  		  pkt_tin_val.it_value.tv_usec = 0;



		  pkt_tout_val.it_interval.tv_sec = 0;
		  pkt_tout_val.it_interval.tv_usec = 1000;  // 1ms
		  pkt_tout_val.it_value.tv_sec = 0;
		  pkt_tout_val.it_value.tv_usec = 1000;    // 1ms

		  if (setitimer(ITIMER_REAL, &pkt_tout_val, &pkt_tin_val) < 0)
			  cerr << " ERROR: Unable to set the Timer ... " << endl;

		  signal(SIGALRM,timer_fired);
		  signal(SIGINT,SRPPTimer::stopTimer);
			return 0;
}

	/**
	 * Start: obviously start the individual timers
	 * Pause: obviously pause the individual timers
	 * Reset: reset the counters so that the timers just start from that time onwards
	 * To unpause, simply call the corresponding start functions
	 *
	 */
	int SRPPTimer::start_packet()
	{
		packet_ON = true;
		return 0;
	}
	int SRPPTimer::start_silence()
	{
		silence_ON = true;
		return 0;
	}
	int SRPPTimer::pause_packet()
	{
		packet_ON = false;
		return 0;
	}
	int SRPPTimer::pause_silence()
	{
		silence_ON = false;
		return 0;
	}
	int SRPPTimer::reset_packet()
	{
		packet_counter = 0;
		return 0;
	}
	int SRPPTimer::reset_silence()
	{
		silence_counter = 0;
		return 0;
	}
	//Pause the timer. This function might just not be needed at all.
	// Start the timer to unpause
	int SRPPTimer::pauseTimer()
	{
		packet_ON = false;
		silence_ON = false;
		cout << "Paused both the timers" << endl;
		return 0;
	}

	int SRPPTimer::startTimer()
	{
		packet_ON = true;
		silence_ON = true;

		cout << "Started both the timers" << endl;
		return 0;
	}

