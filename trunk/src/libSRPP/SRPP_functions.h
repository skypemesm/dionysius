/**
 * This contains the declaration of all the SRPP functions.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "SRPPMessage.hpp"
#include "SRPPSession.hpp"

using namespace std;


	//initialize stuff
	int init_SRPP();

	//create SRPP session
	SRPPSession create_SRPPSession(string address,int port);

	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp();

	//convert a SRPP packet back to RTP packet
	int srpp_to_rtp();

	// Convert a SRTP packet to SRPP packet
	SRPPMessage srtp_to_srpp();

	//Convert a SRPP packet back to SRTP
	int srpp_to_srtp();

	//Create a SRPP Message with the data and encrypt it and return it
	SRPPMessage create_and_encrypt_srpp(string data, CryptoProfile * crypto);

	// Only create a SRPP Message and return it.
	SRPPMessage create_srpp_message(string data);

	// Encrypt the given SRPP packet
	SRPPMessage encrypt_srpp(
			SRPPMessage * original_pkt,
			CryptoProfile * crypto,
			SRPPSession * srtp_session);

	//Decrypt the given SRPP packet
	SRPPMessage decrypt_srpp(
			SRPPMessage * encrypted_pkt,
			CryptoProfile * crypto,
			SRPPSession * srtp_session);

