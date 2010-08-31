/**
 * This contains the declaration of all the SRPP functions.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#ifndef SRPP_MESSAGE_HPP
	#define SRPP_MESSAGE_HPP
	#include "SRPPMessage.hpp"
#endif
#include "rtp.hpp"

#ifndef SRPP_SESSION
	#define SRPP_SESSION YES
	#include "SRPPSession.hpp"
#endif
#include "CryptoProfile.hpp"
#include "Padding_functions.h"

using namespace std;

namespace srpp {

	//initialize stuff
	int init_SRPP();

	//create SRPP session
	SRPPSession create_SRPPSession(string address,int port);

	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp(RTPMessage* rtp_msg);

	//convert a SRPP packet back to RTP packet
	RTPMessage srpp_to_rtp(SRPPMessage* srpp_msg);

	// Convert a SRTP packet to SRPP packet
	SRPPMessage srtp_to_srpp(RTPMessage* srtp_msg);

	//Convert a SRPP packet back to SRTP
	RTPMessage srpp_to_srtp(SRPPMessage* srpp_msg);

	//Create a SRPP Message with the data and encrypt it and return it
	SRPPMessage create_and_encrypt_srpp(string data, CryptoProfile * crypto, SRPPSession* srpp_session);

	// Only create a SRPP Message and return it.
	SRPPMessage create_srpp_message(string data);

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


}
