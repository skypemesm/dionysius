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
using namespace std;


class SignalingFunctions {

private:
	int hellosent, hellorecvd, helloacksent, helloackrecvd, byesent , byerecvd, byeacksent, byeackrecvd;


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
	}

	int signaling()
	{

		//wait for sometime- this is to mitigate the deadlock in case
		//both endpoints start signaling at same time and send hello packets to each other

		sendHelloMessage();


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
		if ( helloacksent == 0 )
			return sendHelloAckMessage();
		else
			return 0;
	}
	int receivedByeMessage()
	{
		byerecvd = 1;
		return sendHelloAckMessage();

	}
	int receivedByeAckMessage()
	{
		byeackrecvd = 1;
		//if they do and if we have not sent out a helloack earlier
		if ( helloacksent == 0 )
			return sendByeAckMessage();
		else
		{
			//TODO:stop session.
			return 0;
		}
	}

	//send message functions
	int sendHelloMessage()
	{

		//If we have received the helloack or hello message, then we should not send this message
		if (hellosent == 1 || hellorecvd == 1 || helloacksent == 1 || helloackrecvd == 1)
			return -1;

		string options = "YES, YES, YES,YES, DEFAULT, DEFAULT, DEFAULT";
		// PSP YES/NO, CBP YES/NO, EBP YES/NO, VITP YES/NO, PSP_ALGO, CBP_ALGO, EBP_ALGO

		SRPPMessage srpp_msg = srpp::create_srpp_message(options);
		srpp_msg.srpp_header.pt = 69;
		srpp_msg.srpp_header.srpp_signalling = 12;


		srpp::send_message(&srpp_msg);

		hellosent = 1;
		return 0;
	}

	int sendHelloAckMessage()
	{

		//If we have not received the hello or have sent the helloack or hello message, then we should not send this message
		if (hellorecvd != 1 || helloacksent == 1 || helloackrecvd == 1)
			return -1;

		string options = "YES, YES, YES, DEFAULT, DEFAULT, DEFAULT";

		SRPPMessage srpp_msg = srpp::create_srpp_message(options);
		srpp_msg.srpp_header.pt = 69;
		srpp_msg.srpp_header.srpp_signalling = 13;

		srpp::send_message(&srpp_msg);

		helloacksent = 1;
		return 0;
	}

	int sendByeMessage()
		{
			SRPPMessage srpp_msg = srpp::create_srpp_message("");
			srpp_msg.srpp_header.pt = 69;
			srpp_msg.srpp_header.srpp_signalling = 22;

			srpp::send_message(&srpp_msg);

			byesent = 1;

			return 0;
		}

	int sendByeAckMessage()
		{
			SRPPMessage srpp_msg = srpp::create_srpp_message("");
			srpp_msg.srpp_header.pt = 69;
			srpp_msg.srpp_header.srpp_signalling = 23;

			srpp::send_message(&srpp_msg);

			byeacksent = 1;
			return 0;
		}




};
