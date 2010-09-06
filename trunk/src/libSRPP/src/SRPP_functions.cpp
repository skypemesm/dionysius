/**
 * This contains the implementation of all the SRPP functions.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "SRPP_functions.h"
#include "srpp_timer.h"
#include "SRPPSession.hpp"

using namespace std;

namespace srpp {

/** The session to which this is tied to **/
SRPPSession * srpp_session;
PaddingFunctions padding_functions;

	//initialize stuff
	int init_SRPP(){
		//Initialize any padding parameters necessary.
		padding_functions = *(new PaddingFunctions());

		//Initialize the SRPP parameter..


		cout << "SRPP initiated" << endl;
		return 0;
	}

	//create SRPP session
	SRPPSession * create_session(string address, int port){
		srpp_session = new SRPPSession(address,port,port,PACKET_INTERVAL_TIME, SILENCE_INTERVAL_TIME);
		return srpp_session;
	}

	//start the srpp session
	int start_session(){
		//call signaling so that we negotiate the padding parameters

		//then we start the srpp session
		srpp_session->start_session();
	}

	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp(RTPMessage * rtp_msg){

		cout << "in funccc : seq_num" << rtp_msg->rtp_header.seq << endl;
		cout << "in funccc : payload" << rtp_msg->payload << endl;


		//Create a SRPPMessage with the data from RTP packet
		SRPPMessage srpp_msg = create_srpp_message(rtp_msg->payload);

		srpp_msg.srpp_header.version = rtp_msg->rtp_header.version;
		srpp_msg.encrypted_part.original_padding_bit = rtp_msg->rtp_header.p;
		srpp_msg.srpp_header.cc = rtp_msg->rtp_header.cc;
		srpp_msg.srpp_header.x = rtp_msg->rtp_header.x;
		srpp_msg.srpp_header.m = rtp_msg->rtp_header.m;
		srpp_msg.encrypted_part.original_seq_number = rtp_msg->rtp_header.seq;
		srpp_msg.srpp_header.pt = rtp_msg->rtp_header.pt;
		srpp_msg.srpp_header.ts = rtp_msg->rtp_header.ts;
		srpp_msg.srpp_header.ssrc = rtp_msg->rtp_header.ssrc;

		for (int i = 0; i<10; i++)
			srpp_msg.srpp_header.csrc[i] = rtp_msg->rtp_header.csrc[i];

		srpp_msg.srpp_header.srpp_signalling = 0;

		srpp_msg.encrypted_part.dummy_flag = 0;

		//Pad the SRPPMessage
		padding_functions.pad(&srpp_msg);

		//Encrypt the message


		//Return

		cout << "THIS SRPP MSG HAS VALUE:" << srpp_msg.encrypted_part.original_payload << endl;
		cout << "THIS SRPP MSG HAS Pad count:" << srpp_msg.encrypted_part.pad_count << endl;
		cout << "THIS SRPP MSG HAS Padding bytes:" << srpp_msg.encrypted_part.srpp_padding << endl;


		return srpp_msg;

	}

	//convert a SRPP packet back to RTP packet
	RTPMessage srpp_to_rtp(SRPPMessage * srpp_msg){

		//Decrypt the SRPP Message

		//Unpad the SRPP Message
		padding_functions.unpad(srpp_msg);
/*

		cout << "in funccc : seq_num" << rtp_msg->rtp_header.seq << endl;
		cout << "in funccc : payload" << rtp_msg->payload << endl;
*/


		//Create a RTPMessage with the data from SRPP packet
		RTPMessage rtp_msg = create_rtp_message(srpp_msg->encrypted_part.original_payload);

		rtp_msg.rtp_header.version = srpp_msg->srpp_header.version;
		rtp_msg.rtp_header.p = srpp_msg->encrypted_part.original_padding_bit;
		rtp_msg.rtp_header.cc = srpp_msg->srpp_header.cc;
		rtp_msg.rtp_header.x = srpp_msg->srpp_header.x;
		rtp_msg.rtp_header.m = srpp_msg->srpp_header.m ;
		rtp_msg.rtp_header.seq = srpp_msg->encrypted_part.original_seq_number;
		rtp_msg.rtp_header.pt= srpp_msg->srpp_header.pt ;
		rtp_msg.rtp_header.ts = srpp_msg->srpp_header.ts ;
		rtp_msg.rtp_header.ssrc= srpp_msg->srpp_header.ssrc;

		for (int i = 0; i<10; i++)
			rtp_msg.rtp_header.csrc[i]= srpp_msg->srpp_header.csrc[i];


		//Return

		cout << "THIS RTP MSG HAS VALUE:" << rtp_msg.payload << endl;


		return rtp_msg;

}

	// Convert a SRTP packet to SRPP packet
	SRPPMessage srtp_to_srpp(RTPMessage * srtp_msg){

		//Create a SRPPMessage

		//Pad the SRPPMessage

		//Encrypt the message

		//Return

	}

	//Convert a SRPP packet back to SRTP
	RTPMessage srpp_to_srtp(SRPPMessage * srpp_msg){
		// Unpad the SRPPMessage

		// Create a SRTPMessage

		// Return


	}

	//Create a SRPP Message with the data and encrypt it and return it
	SRPPMessage create_and_encrypt_srpp(string data, CryptoProfile * crypto, SRPPSession* srpp_session1){

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

	// Only create a RTP Message and return it.
		RTPMessage create_rtp_message(string data){

			unsigned char buff[65536];
			RTPMessage* rtp_msg = new RTPMessage(buff);

			//put data, if any, in the payload
			if(!data.empty())
				data.copy((rtp_msg->payload),data.length(),0);

			return *rtp_msg;

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

	//Get the padding functions object used here
	PaddingFunctions* get_padding_functions(){
		return &(padding_functions);
	}

// Pseudo-Random number between min and max
	int srpp_rand(int min,int max){

			srand(time(NULL));

			return ((rand() % max) + min);

		}

} // end of namespace
