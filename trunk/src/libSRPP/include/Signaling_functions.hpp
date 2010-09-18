/**
 * \file SIgnaling_functions.hpp
 *
 * This set of functions are responsible for implementing the state machine
 * which handles the initial signaling part between SRPP endpoints.
 *
 * This signaling happens in-band. Basically, an endpoint sends a HELLOSRPP message
 * and waits for the HELLOACKSRPP message from the other side. This HELLOACKSRPP
 * message is supposed to negotiate the padding parameters between the endpoints.
 *
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */
#include "SRPP_functions.h"
#include "SRPPMessage.hpp"
#include <string>
#include <iostream>
#include <cstdio>

using namespace std;


class SignalingFunctions {

private:
	int hellosent, hellorecvd, helloacksent, helloackrecvd, byesent , byerecvd, byeacksent, byeackrecvd;
	int signaling_complete, session_complete;


public:

	SignalingFunctions ()
	{
		hellosent = 0;
		hellorecvd = 0;
		helloacksent = 0;
		helloackrecvd = 0;
		byesent =0;
		byerecvd =0;
		byeacksent =0;
		byeackrecvd=0;

		signaling_complete = 0;
		session_complete = 0;
	}

	int isSignalingComplete()
	{
		return signaling_complete;
	}

	int isSessionComplete()
		{
			return session_complete;
		}
	int signaling()
	{

		//wait for sometime- this is to mitigate the deadlock in case
		//both endpoints start signaling at same time and send hello packets to each other

		int r = srpp::srpp_rand(5000,10000);
		while (r-- != 0);

		return sendHelloMessage();


	}

	//receive message functions
	int receivedHelloMessage()
	{
		hellorecvd = 1;


		return sendHelloAckMessage();

	}

	int receivedHelloAckMessage()
	{
		helloackrecvd = 1;

		//check to see if the options match or not..

			//if they do and if we have not sent out a helloack earlier
			// if they dont, they we can send a helloack once more, if we can support the new params


			if ( helloacksent == 0 )
			{
				signaling_complete = 1;
				return sendHelloAckMessage();
			}
			else
			{
				signaling_complete = 1;
				return 0;
			}

	// If options do not match
	}
	int receivedByeMessage()
	{
		byerecvd = 1;
		return sendByeAckMessage();

	}
	int receivedByeAckMessage()
	{
		byeackrecvd = 1;
		//if they do and if we have not sent out a byeack earlier
		if ( byeacksent == 0 )
		{
			session_complete = 1;
			return sendByeAckMessage();
		}
		else
		{
			session_complete = 1;
			return 0;
		}
	}

	//send message functions
	int sendHelloMessage()
	{

		//If we have received the helloack or hello message, then we should not send this message
		if (hellosent == 1 || hellorecvd == 1 || helloacksent == 1 || helloackrecvd == 1)
			return -1;

		//generate a key now.
		int key = srpp::srpp_rand(0,65535);
		srpp::setKey(key);
		char buf[5];
		sprintf(buf,"%d",key);
		string options = buf;

		options.append(", YES, YES, YES,YES, DEFAULT, DEFAULT, DEFAULT");


		// PSP YES/NO, CBP YES/NO, EBP YES/NO, VITP YES/NO, PSP_ALGO, CBP_ALGO, EBP_ALGO

		SRPPMessage srpp_msg = srpp::create_srpp_message(options);
		srpp_msg.srpp_header.pt = 69;
		srpp_msg.srpp_header.srpp_signalling = 12;
		srpp_msg.srpp_header.x = 1;

		srpp::send_message(&srpp_msg);

		hellosent = 1;
		srpp::receive_message();

		return 0;

	}

	int sendHelloAckMessage()
	{
		//If we have not sent or received the hello or have sent the helloack message, then we should not send this message
		if ((hellorecvd != 1 && hellosent != 1)|| helloacksent == 1)
			return -1;

		string options = "YES, YES, YES, DEFAULT, DEFAULT, DEFAULT";
		SRPPMessage srpp_msg = srpp::create_srpp_message(options);
		srpp_msg.srpp_header.pt = 69;
		srpp_msg.srpp_header.srpp_signalling = 13;
		srpp_msg.srpp_header.x = 1;

		cout << "Sending a HELLO ACK message now " <<endl;
		srpp::send_message(&srpp_msg);

		helloacksent = 1;

		if (signaling_complete == 0)
			srpp::receive_message();
		// If we do not get a message within a timeout, then signaling failed.

		return 0;
	}

	int sendByeMessage()
		{
			SRPPMessage srpp_msg = srpp::create_srpp_message("");
			srpp_msg.srpp_header.pt = 69;
			srpp_msg.srpp_header.srpp_signalling = 22;
			srpp_msg.srpp_header.x = 1;

			cout << "Sending a BYE message now " << endl;
			srpp::send_message(&srpp_msg);

			byesent = 1;

			return 0;
		}

	int sendByeAckMessage()
		{
			//If we have not sent or received the bye message or have sent the byeack message, then we should not send this message
			if ((byerecvd != 1 && byesent != 1)|| byeacksent == 1)
				return -1;


			SRPPMessage srpp_msg = srpp::create_srpp_message("");
			srpp_msg.srpp_header.pt = 69;
			srpp_msg.srpp_header.srpp_signalling = 23;
			srpp_msg.srpp_header.x = 1;

			cout << "Sending a BYE ACK message now " <<endl;
			srpp::send_message(&srpp_msg);

			byeacksent = 1;
			return 0;
		}




};
