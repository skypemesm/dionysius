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
	int hellosent,hellorecvd,helloacksent, helloackrecvd, byesent, byerecvd, byeacksent, byeackrecvd;


public:

	int sendHelloMessage()
	{
		SRPPMessage * srpp_msg = srpp::create_srpp_message("");
		srpp_msg->srpp_header.pt = 69;
		srpp_msg->srpp_header.srpp_signalling = 12;

		string options = "YES, YES, YES, DEFAULT, DEFAULT, DEFAULT";
		srpp_msg->encrypted_part.original_payload = options.c_str();


	}



};
