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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <csignal>

#include "CryptoProfile.hpp"
#include "srpp_timer.h"

using namespace std;

class SRPPSession
{

public:
	string receiverIP, startTime;
	int receiverPort,myPort;
	time_t start_time;
	double currentBurstTime;
	unsigned int encryption_key;
	CryptoProfile crypto_profile;
	int sendersocket,receiversocket;
	struct sockaddr_in sender_addr , receiver_addr;
	int maxpacketsize;
	bool is_srtp;

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
			int SILENCE_INTERVAL_,
			CryptoProfile thisCryptoProfile
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

		currentBurstTime = 0;

		srpp_timer = new SRPPTimer(PACKET_INTERVAL_, SILENCE_INTERVAL_);
		crypto_profile = thisCryptoProfile;
		maxpacketsize = MAXPAYLOADSIZE;
		is_srtp = 0;
	}

	//Destructors
	~SRPPSession()
	{
		// close socket
		if(close(sendersocket) == -1 || close(receiversocket) == -1)
		{
			cout << "\nCould not close socket\n";
			return ;
		}
	}

	//Set the socket parameters. You must call this function after creating the to and from channels i.e. client and server channels
	int set_sockets(
			int sender_sock, int receiver_sock,
			struct sockaddr_in sender_addrr, struct sockaddr_in receiver_addrr
			)
	{
		sendersocket = sender_sock;
		receiversocket = receiver_sock;
		sender_addr = sender_addrr;
		receiver_addr = receiver_addrr;
		return 0;
	}
	//Set the socket parameters. You must call this function after creating the to and from channels i.e. client and server channels
	int set_sockets(int receiver_sock, struct sockaddr_in receiver_addrr)
	{
		receiversocket = receiver_sock;
		receiver_addr = receiver_addrr;
		return 0;
	}

	//We are done signaling.. start the session and timers
	int start_session()
	{
		// now enable the packets based on which one is required.
		srpp_timer->start_packet();
		srpp_timer->start_silence();
		return 0;
	}

	//We are done signaling.. start the session and timers
	int stop_session()
	{
		srpp_timer->pauseTimer();
		return 0;
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

	//Setter and Getter method for CryptoProfile
	int setCryptoProfile(CryptoProfile thisCryptoProfile)
	{
		crypto_profile = thisCryptoProfile;
		return 0;
	}
	CryptoProfile getCryptoProfile()
	{
		return crypto_profile;
	}


	int getMaxpacketsize()
	{
		return maxpacketsize;
	}

};



#endif
