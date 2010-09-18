/**
 * This contains the implementation of all the SRPP functions.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "SRPP_functions.h"
#include "srpp_timer.h"
#include "SRPPSession.hpp"
#include "Signaling_functions.hpp"
#include <errno.h>
#include <cstring>
#include <sys/time.h>


using namespace std;

int lastSequenceNo = 0;
int rtpSequenceNo = 0;

namespace srpp {

/** The session to which this is tied to **/
SRPPSession * srpp_session;
PaddingFunctions padding_functions;
SignalingFunctions signaling_functions;

	//initialize stuff
	int init_SRPP(){
		//Initialize any padding parameters necessary.
		padding_functions = *(new PaddingFunctions());

		//Initialize the SRPP parameter..


		cout << "SRPP initiated" << endl;
		return 0;
	}

	//create SRPP session
	SRPPSession * create_session(string address, int port, CryptoProfile crypto){

		srpp_session = new SRPPSession(address,port,port,PACKET_INTERVAL_TIME, SILENCE_INTERVAL_TIME, crypto);
		return srpp_session;
	}

	//start the srpp session
	int start_session(){
		//So here we signal first..
		if (signaling() < 0)
		{
			cout << " SRPP not supported in the other endpoint. " << endl;
			//show message in the GUI, and exit
			return -1;
		}

		//If we are able to set the get the proper srpp parameters from the other endpoint
		//and set it in our session, then we start the session.

		//then we start the srpp session
		srpp_session->start_session();
	}

	//stOP the srpp session
	int stop_session(){
		cout << "------------------------------------------------------------------------------------\n\n";

		//we stop the srpp session object
		srpp_session->stop_session();

		//We send BYE message
		signaling_functions.sendByeMessage();
		receive_message();

		if(isMediaSessionComplete() == 1)
		{
			return 1;
		}
		else
		{
			cout << "ERROR IN STOPPING THE MEDIA SESSION.." << endl;
			return -1;
		}
	}

	//Signaling start
	int signaling()
	{
		signaling_functions.signaling();
	}


	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp(RTPMessage * rtp_msg){

/*		cout << "in funccc : seq_num" << rtp_msg->rtp_header.seq << endl;
		cout << "in funccc : payload" << rtp_msg->payload << endl;*/


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
		srpp_msg.encrypted_part.pad_count = srpp_msg.encrypted_part.srpp_padding.size();

		//Encrypt the message
		//encrypt_srpp(&srpp_msg);

		//Return

	/*	cout << "THIS SRPP MSG HAS VALUE:" << srpp_msg.encrypted_part.original_payload << endl;
		cout << "THIS SRPP MSG HAS Pad count:" << srpp_msg.encrypted_part.pad_count << endl;
		cout << "THIS SRPP MSG HAS Padding bytes:" << srpp_msg.encrypted_part.srpp_padding << endl;
*/

		return srpp_msg;

	}

	//convert a SRPP packet back to RTP packet
	RTPMessage srpp_to_rtp(SRPPMessage * srpp_msg){

		//Decrypt the SRPP Message
		//decrypt_srpp(srpp_msg);


		//Unpad the SRPP Message
		RTPMessage rtp_msg;
		if (padding_functions.unpad(srpp_msg) == -1 )
		{
			//Create a RTPMessage with the data from SRPP packet
			rtp_msg = create_rtp_message("");
		}
		else
		{
			//Create a RTPMessage with the data from SRPP packet
		    string data (srpp_msg->encrypted_part.original_payload.begin(),srpp_msg->encrypted_part.original_payload.end());
			rtp_msg = create_rtp_message(data);
		}
/*

		cout << "in funccc : seq_num" << rtp_msg->rtp_header.seq << endl;
		cout << "in funccc : payload" << rtp_msg->payload << endl;
*/




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


		//cout << "THIS RTP MSG HAS VALUE:" << rtp_msg.payload << endl;


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
	SRPPMessage create_and_encrypt_srpp(string data){

		//create the message
		SRPPMessage* srpp_msg = new SRPPMessage();

		if(!data.empty())
		{	srpp_msg->encrypted_part.original_payload = vector<char>(data.begin(),data.end());	}


		//encrypt the message
		*srpp_msg = encrypt_srpp(srpp_msg);

		return *srpp_msg;

	}

	// Only create a SRPP Message and return it.
	SRPPMessage create_srpp_message(string data){

		SRPPMessage* srpp_msg = new SRPPMessage();

		//put data, if any, in the payload
		if(!data.empty())
		{		srpp_msg->encrypted_part.original_payload = vector<char>(data.begin(),data.end());	}

		return *srpp_msg;

	}

	// Only create a RTP Message and return it.
		RTPMessage create_rtp_message(string data){

			RTPMessage* rtp_msg = new RTPMessage();

			//put data, if any, in the payload
			if(!data.empty())
				data.copy((rtp_msg->payload),data.length(),0);

			rtp_msg->payload[data.length()] = '\0';

			return *rtp_msg;

		}

	// Encrypt the given SRPP packet
	SRPPMessage encrypt_srpp(SRPPMessage * original_pkt)
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
	SRPPMessage decrypt_srpp(SRPPMessage * encrypted_pkt)
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

		timeval a;
		gettimeofday(&a, NULL);
		srand(1000*a.tv_sec + a.tv_usec);      // SEED ON MICROSECONDS

			return ((rand() % max) + min);

		}


int send_message(SRPPMessage * message)
	{

		int size = sizeof(message->srpp_header) + message->encrypted_part.original_payload.size()  +
					message->encrypted_part.srpp_padding.size() + 3* sizeof(uint32_t);

		cout << sizeof(message->srpp_header) << "::" << message->encrypted_part.original_payload.size()   << "::" <<
							message->encrypted_part.srpp_padding.size()  << "::" << 3* sizeof(uint32_t) << ":: " << message->encrypted_part.pad_count << endl;

		char * buff = new char[size];
		message->srpp_to_network(buff);

cout << size << " bytes\n\n" ;
		int byytes = sendto(srpp_session->sendersocket, buff, size, 0,
							              (struct sockaddr *)&(srpp_session->sender_addr), sizeof(struct sockaddr));
		if (byytes < 0)
			cout << "ERROR IN SENDING DATA: " << strerror(errno)<< endl;


	/*	cout << "\nWriting " << byytes << " bytes \""
				//		<< message->encrypted_part.original_payload << "\" to other endpoint at "
						<< inet_ntoa(srpp_session->sender_addr.sin_addr) << ":"
						<< ntohs(srpp_session->sender_addr.sin_port) << endl << endl;*/

		return byytes;
	}

SRPPMessage receive_message()
	{
		int addr_len = sizeof(struct sockaddr);
		SRPPMessage srpp_msg = srpp::create_srpp_message("");

		cout << "\nWaiting for messages ... " << endl;


		//TODO: WE NEED TO ADD A SELECT OR POLL SO AS TO AVOID WAITING FOR A LONG TIME
		char buff[MAXPAYLOADSIZE];
		int bytes_read = recvfrom(srpp_session->receiversocket,buff,sizeof(buff),0,
						(struct sockaddr *)&(srpp_session->sender_addr),
						(socklen_t *)&addr_len);

		if (bytes_read < 0)
					cout << "ERROR IN RECEIVING DATA: " << strerror(errno)<< endl;

		srpp_msg.network_to_srpp(buff,bytes_read);

	/*	cout << "Read " << bytes_read << " bytes from the other endpoint at "
				<< inet_ntoa(srpp_session->sender_addr.sin_addr) << ":"
				<< ntohs(srpp_session->sender_addr.sin_port ) << endl;
*/
			//Set the sender_addr's port correctly
			srpp_session->sender_addr.sin_port = htons(ntohs(srpp_session->sender_addr.sin_port) + 2);


		// If this is a signaling message, point to the signaling handler
			if (isSignalingMessage(&srpp_msg) == 1)
			{

				if (srpp_msg.srpp_header.srpp_signalling == 12)
				{
					cout << "\n\n----------------------------------------------------------------------------------------------\n";
					cout << "Received HELLO message " << endl;

					//extract key from the payload
					string options (srpp_msg.encrypted_part.original_payload.begin(),srpp_msg.encrypted_part.original_payload.end());
					string thiskey = options.substr(0,options.find_first_of(','));
					setKey(atoi(thiskey.c_str()));

					cout << "KEY RECVD: " << thiskey << endl;
					signaling_functions.receivedHelloMessage();

				}
				else if (srpp_msg.srpp_header.srpp_signalling == 13)
				{
					cout << "Received HELLO ACK message " << endl;

					if (signaling_functions.receivedHelloAckMessage() < 0)
						cout << "ERROR IN PROCESSING HELLOACK" << endl;

					if (signaling_functions.isSignalingComplete() == 1)
					{
						cout << "\n\n SIGNALING IS COMPLETE NOW. STARTING MEDIA SESSION with key " <<
												srpp_session->encryption_key << "... \n\n" ;
						cout << "-----------------------------------------------------------------------------------------------\n";
					}

				}
				else if (srpp_msg.srpp_header.srpp_signalling == 22)
				{
					cout << "\n\n------------------------------------------------------------------------------------------------\n";
					cout << "Received BYE message " << endl;
					signaling_functions.receivedByeMessage();
				}
				else if (srpp_msg.srpp_header.srpp_signalling == 23)
				{
					cout << "Received BYE ACK message " << endl;
					signaling_functions.receivedByeAckMessage();

					if (signaling_functions.isSessionComplete() == 1)
					{
						cout << "\n\n SESSION IS COMPLETE NOW. EXITING NOW... \n\n" ;
						cout << "-----------------------------------------------------------------------------------------------\n";
					}

				}

			}
		else
		{

			//cout << "Not a signaling message" << endl;
			return srpp_msg;
		}
	}

 // parse the received message ... returns -1 if its a media packet.. and 1 if its a signaling packet (whose corresponding handler is called)
 int isSignalingMessage (SRPPMessage * message)
 {

	 if (message->srpp_header.srpp_signalling == 0 and message->srpp_header.pt != 69) //NOT A SIGNALING MESSAGE
		 return -1;
	 else if(message->srpp_header.srpp_signalling !=0 and message->srpp_header.pt == 69)
		 return 1;
 }


 //Check whether the signaling is complete
 int isSignalingComplete()
 {
	 return signaling_functions.isSignalingComplete();
 }

 //Check whether media session is complete
 int isMediaSessionComplete()
 {
	 return signaling_functions.isSessionComplete();
 }

 int setKey(int key)
 {
	 srpp_session->encryption_key = key;
 }


} // end of namespace
