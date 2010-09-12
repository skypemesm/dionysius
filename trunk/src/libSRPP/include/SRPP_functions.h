/**
 * This contains the declaration of all the SRPP functions.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#ifndef SRPP_FUNCTIONS_H
#define  SRPP_FUNCTIONS_H

#include "SRPPMessage.hpp"
#include "rtp.hpp"
#include "CryptoProfile.hpp"
#include "Padding_functions.h"



#include <ctime>
#include <cstdlib>


#define PACKET_INTERVAL_TIME	10000	           /** This is time interval in ms we wait before we start current burst padding  **/
#define SILENCE_INTERVAL_TIME	35000              /** This is time interval in ms we wait in silence before we start extra burst padding  **/

class SRPPSession;									/** Forward Declaration  **/

namespace srpp {


	//initialize stuff
	int init_SRPP();

	//create SRPP session
	SRPPSession* create_session(string address, int port, CryptoProfile crypto);

	//starts the srpp session
	int start_session();

	//stops the srpp session
	int stop_session();

	//Signaling start
	int signaling();

	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp(RTPMessage* rtp_msg);

	//convert a SRPP packet back to RTP packet
	RTPMessage srpp_to_rtp(SRPPMessage* srpp_msg);

	// Convert a SRTP packet to SRPP packet
	SRPPMessage srtp_to_srpp(RTPMessage* srtp_msg);

	//Convert a SRPP packet back to SRTP
	RTPMessage srpp_to_srtp(SRPPMessage* srpp_msg);

	//Create a SRPP Message with the data and encrypt it and return it
	SRPPMessage create_and_encrypt_srpp(string data);

	// Only create a SRPP Message and return it.
	SRPPMessage create_srpp_message(string data);

	// Only create a SRPP Message and return it.
	RTPMessage create_rtp_message(string data);

	// Encrypt the given SRPP packet
	SRPPMessage encrypt_srpp(SRPPMessage * original_pkt);

	//Decrypt the given SRPP packet
	SRPPMessage decrypt_srpp(SRPPMessage * encrypted_pkt);

	//Get the padding functions object used here
	PaddingFunctions* get_padding_functions();

	/** Utility functions **/

	// Pseudo-Random number between min and max
	int srpp_rand(int min,int max);

	//USed by the interior functions to send a specific message or receive a message
	int send_message(SRPPMessage* msg);
	SRPPMessage receive_message();

	// parse the received message ... returns -1 if its a media packet.. and 1 if its a signaling packet (whose corresponding handler is called)
	int isSignalingMessage (SRPPMessage * message);

	//Check whether the signaling is complete
	 int isSignalingComplete();

	 //Check whether media session is complete
	  int isMediaSessionComplete();

	  //Set the encryption in the session
	  int setKey(int key);
}


#endif
