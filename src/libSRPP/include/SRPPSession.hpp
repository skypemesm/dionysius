/**
 *
 * This class contains all the information related to a SRPP Session
 * like the addresses of the endpoints, level of padding, and other parameters.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#ifndef SRPP_SESSION_HPP
#define SRPP_SESSION_HPP


#include <iostream>
#include <string>
#include <ctime>

#include <srpp_timer.h>

using namespace std;

class SRPPSession
{

public:
	string receiverIP, startTime;
	int receiverPort,myPort, lastSeqNo;
	time_t start_time;
	double currentBurstTime;
	unsigned int encryption_key;

	SRPPTimer * srpp_timer;

	/**
	 * @param receiverIP: IP of the receiver
	 * @param receiverPort: Port of the client
	 * @param myPort: port at which my server is running
	 * @param PACKET_INTERVAL_ This is time interval in ms we wait before we start current burst padding
	 * @param SILENCE_INTERVAL_	This is time interval in ms we wait in silence before we start extra burst padding
	 * @start time
	 */
	SRPPSession(
			string thisreceiverIP,
			int thisreceiverPort,
			int thismyPort,
			int PACKET_INTERVAL_,
			int SILENCE_INTERVAL_
			)
	{
		receiverIP = thisreceiverIP;
		receiverPort = thisreceiverPort;
		myPort = thismyPort;

		//get present date time
		struct tm * timeinfo;
		time ( &start_time );
		timeinfo = localtime ( &start_time );

		startTime = asctime(timeinfo);

		encryption_key = 1655;

		lastSeqNo = 0;
		currentBurstTime = 0;

		srpp_timer = new SRPPTimer(PACKET_INTERVAL_, SILENCE_INTERVAL_);
	}

	//We are done signaling.. start the session and timers
	int start_session()
	{
		// now enable the packets based on which one is required.
		srpp_timer->start_packet();
		srpp_timer->start_silence();
	}


	//Increase the sequence number
	int increment_seq()
	{
		return ++lastSeqNo;
	}

	//clear the sequence number
	int clear_seq()
	{
		return (lastSeqNo = 0);
	}

	//increment the burst time
	void increment_bursttime()
	{
		time_t timenow;
		time(&timenow);
		currentBurstTime += difftime(timenow,start_time);
	}

	//clear the burst time
	int clear_currentBurstTime()
	{
		return (currentBurstTime = 0);
	}
};



#endif
