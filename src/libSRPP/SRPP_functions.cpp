/**
 * This contains the implementation of all the SRPP functions.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */


#ifndef SRPP_FUNCTIONS_H
	#define  SRPP_FUNCTIONS_H
	#include "SRPP_functions.h"
#endif

using namespace std;

namespace srpp {

	//initialize stuff
	int init_SRPP(){
		//Initialize any padding parameters necessary.

		return 0;
	}

	//create SRPP session
	SRPPSession create_SRPPSession(string address,int port){

	}

	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp(RTPMessage * rtp_msg){

	}

	//convert a SRPP packet back to RTP packet
	RTPMessage srpp_to_rtp(SRPPMessage * srpp_msg){

	}

	// Convert a SRTP packet to SRPP packet
	SRPPMessage srtp_to_srpp(RTPMessage * srtp_msg){

	}

	//Convert a SRPP packet back to SRTP
	RTPMessage srpp_to_srtp(SRPPMessage * srpp_msg){

	}

	//Create a SRPP Message with the data and encrypt it and return it
	SRPPMessage create_and_encrypt_srpp(string data, CryptoProfile * crypto, SRPPSession* srpp_session){

		//create a buffer
		unsigned char buff[65536];
		//create the message
		SRPPMessage* srpp_msg = new SRPPMessage(buff);

		//put data, if any, in the payload
		if(!data.empty())
			data.copy((srpp_msg->encrypted_part.original_payload),data.length(),0);


		//encrypt the message
		*srpp_msg = encrypt_srpp(srpp_msg, crypto,srpp_session);

		return *srpp_msg;

	}

	// Only create a SRPP Message and return it.
	SRPPMessage create_srpp_message(string data){

		unsigned char buff[65536];
		SRPPMessage* srpp_msg = new SRPPMessage(buff);

		//put data, if any, in the payload
		if(!data.empty())
			data.copy((srpp_msg->encrypted_part.original_payload),data.length(),0);

		return *srpp_msg;

	}

	// Encrypt the given SRPP packet
	SRPPMessage encrypt_srpp(
			SRPPMessage * original_pkt,
			CryptoProfile * crypto,
			SRPPSession * srpp_session)
	{
		//TODO::: USE CRYPTO

		SRPPEncrypted * to_encrypt = &(original_pkt->encrypted_part);

		//Encrypt the "encrypted_part" of the SRPP Packet.
	//		cout << "DATA:" << to_encrypt->original_payload << endl;

		//encrypt each unsigned int part with the key from the session
		unsigned int * orig = reinterpret_cast<unsigned int *>(to_encrypt);
		int i = 0;
		for (i = 0;i<= (sizeof(*to_encrypt)/sizeof(unsigned int));i++)
		{
			(*(orig+i)) ^= srpp_session->encryption_key;
		}

//		cout << "ENCRYPTED DATA:" << to_encrypt->original_payload << endl;

		return *original_pkt;

	}

	//Decrypt the given SRPP packet
	SRPPMessage decrypt_srpp(
			SRPPMessage * encrypted_pkt,
			CryptoProfile * crypto,
			SRPPSession * srpp_session)
	{
		//TODO::: USE CRYPTO

		SRPPEncrypted * to_decrypt = &(encrypted_pkt->encrypted_part);

		//Encrypt the "encrypted_part" of the SRPP Packet.
		//		cout << "DATA:" << to_decrypt->original_payload << endl;

		//encrypt each unsigned int part with the key from the session
		unsigned int * orig = reinterpret_cast<unsigned int *>(to_decrypt);
		int i = 0;
		for (i = 0;i<= (sizeof(*to_decrypt)/sizeof(unsigned int));i++)
		{
			(*(orig+i)) ^= srpp_session->encryption_key;
		}

		//cout << "DECRYPTED DATA:" << to_decrypt->original_payload << endl;


		return *encrypted_pkt;
	}


} // end of namespace
