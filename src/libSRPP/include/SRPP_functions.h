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


#define PACKET_INTERVAL_TIME	5000	           /** This is time interval in ms we wait before we start current burst padding  **/
#define SILENCE_INTERVAL_TIME	15000              /** This is time interval in ms we wait in silence before we start extra burst padding  **/

class SRPPSession;									/** Forward Declaration  **/

namespace srpp {


	//initialize stuff
	int init_SRPP();

	//create SRPP session
	SRPPSession* create_session(string address, int port);

	//starts the srpp session
	int start_session();

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
	SRPPMessage create_and_encrypt_srpp(string data, CryptoProfile * crypto, SRPPSession* srpp_session1);

	// Only create a SRPP Message and return it.
	SRPPMessage create_srpp_message(string data);

	// Only create a SRPP Message and return it.
	RTPMessage create_rtp_message(string data);

	// Encrypt the given SRPP packet
	SRPPMessage encrypt_srpp(
			SRPPMessage * original_pkt,
			CryptoProfile * crypto,
			SRPPSession * srpp_session);

	//Decrypt the given SRPP packet
	SRPPMessage decrypt_srpp(
			SRPPMessage * encrypted_pkt,
			CryptoProfile * crypto,
			SRPPSession * srpp_session);

	//Get the padding functions object used here
	PaddingFunctions* get_padding_functions();

	/** Utility functions **/

	// Pseudo-Random number between min and max
	int srpp_rand(int min,int max);


}


#endif
