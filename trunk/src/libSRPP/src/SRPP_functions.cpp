/**
 * This contains the implementation of all the SRPP functions.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "../include/SRPP_functions.h"
#include "../include/srpp_timer.h"
#include "../include/SRPPSession.hpp"
#include "../include/Signaling_functions.hpp"

#include <errno.h>
#include <cstring>
#include <sys/time.h>


using namespace std;

int lastSequenceNo = 10000;
int maxpacketsize = MAXPAYLOADSIZE;
uint32_t srppssrc = srpp::srpp_rand(2^17,2^32);
int rtpSequenceNo = 0, srtpSequenceNo = 0;
int nonsrpp_message_count = 0;
int sending_packet_now = 0;
int max_nonsrpp_messages = 1000; // 1000 non-srpp messages can be received before we infer that srpp signaling is not possible
int packet_to_send;


namespace srpp {

/** The session to which this is tied to **/
SRPPSession * srpp_session;
PaddingFunctions padding_functions;
SignalingFunctions signaling_functions;
int srpp_enabled = 3;
int signaling_by_srpp = 1;

int (*send_processor)(char*,int);
SRPPMessage (*receive_processor)();
int send_external = 0, receive_external = 0;



	//initialize stuff
	int init_SRPP(){
		//Initialize any padding parameters necessary.
		padding_functions = *(new PaddingFunctions());

		//Initialize the SRPP parameter..


		//cout << "\nSRPP initiated inside CCRTP stack." << endl;
		cout << "SRPP initiated in sqrkal" << endl;
		return 0;
	}

	int SRPP_Enabled()
	{
		return srpp_enabled;
	}

	//create SRPP session
	SRPPSession * create_session(string address, int port, CryptoProfile crypto){

		srpp_session = new SRPPSession(address,port,port,PACKET_INTERVAL_TIME, SILENCE_INTERVAL_TIME, crypto);
		return srpp_session;
	}

	//start the srpp session
	int start_session(){

	 //SIGNALING NOT YET DONE.. IN-BAND SIGNALING TO COMMENCE NOW
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
	 srpp_enabled = 1;


	  signal(SIGINT,srpp::stop_abnormally);
	 return 0;

	}

	int start_session(sdp_srpp sdp)
	{


		// DONE THROUGH SIP
		signaling_by_srpp = 0;

		if (sdp.is_active() == 0)
		{
			srpp_enabled = 0;
			return stop_session();
		}

		setKey(sdp.getKey());
		srpp_session->maxpacketsize = sdp.getMaxPacketSize();
		maxpacketsize = sdp.getMaxPacketSize();
		signaling_functions.setSignalingComplete(1);

		//then we start the srpp session

		 srpp_enabled = 1;

		 signal(SIGINT,srpp::stop_abnormally);

		return srpp_session->start_session();
	}


	void stop_abnormally(int i)
	{
		cout << "SOMehting onaosdun\n";
		stop_session();
	}

	//stOP the srpp session
	int stop_session(){
		cout << "------------------------------------------------------------------------------------\n\n";

		//we stop the srpp session object
		srpp_session->stop_session();

		if (srpp_enabled == 0)
		{
			cout << "SRPP IS NOT SUPPORTED AT THE OTHER END. \n";
			return -10;
		}

		if (signaling_by_srpp == 1)
		{
			//We send BYE message
			signaling_functions.sendByeMessage();
			receive_message();
		}

		if(isMediaSessionComplete() == 1)
		{
			return 1;
		}
		else
		{
			cout << "ERROR IN STOPPING THE MEDIA SESSION.." << endl;
			return -1;
		}

		signal(SIGINT,SIG_DFL);
	}

	//Signaling start
	int signaling()
	{
		if (signaling_by_srpp == 1)
			return signaling_functions.signaling();
		else
			return 0;

	}


	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp(RTPMessage * rtp_msg){

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

		for (int i = 0; i< ntohs(rtp_msg->rtp_header.cc); i++)
			srpp_msg.srpp_header.csrc[i] = rtp_msg->rtp_header.csrc[i];

		srpp_msg.srpp_header.srpp_signalling = 0;

		srpp_msg.encrypted_part.dummy_flag = 0;

		//Pad the SRPPMessage
		padding_functions.pad(&srpp_msg);
		srpp_msg.encrypted_part.pad_count = srpp_msg.encrypted_part.srpp_padding.size();

		//Encrypt the message
		encrypt_srpp(&srpp_msg);

		//Return
		return srpp_msg;

	}

	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp(RTP_Header rtp_header, char* buff, int length){


		//Create a SRPPMessage with the data from RTP packet
		string str(buff,length);
		SRPPMessage srpp_msg = create_srpp_message(str);

		srpp_msg.srpp_header.version = rtp_header.version;
		srpp_msg.encrypted_part.original_padding_bit = rtp_header.p;
		srpp_msg.srpp_header.cc = rtp_header.cc;
		srpp_msg.srpp_header.x = rtp_header.x;
		srpp_msg.srpp_header.m = rtp_header.m;
		srpp_msg.encrypted_part.original_seq_number = rtp_header.seq;
		srpp_msg.srpp_header.pt = rtp_header.pt;
		srpp_msg.srpp_header.ts = rtp_header.ts;
		srpp_msg.srpp_header.ssrc = rtp_header.ssrc;

		for (int i = 0; i< ntohs(rtp_header.cc); i++)
			srpp_msg.srpp_header.csrc[i] = rtp_header.csrc[i];

		srpp_msg.srpp_header.srpp_signalling = 0;

		srpp_msg.encrypted_part.dummy_flag = 0;

		//Pad the SRPPMessage
		padding_functions.pad(&srpp_msg);
		srpp_msg.encrypted_part.pad_count = srpp_msg.encrypted_part.srpp_padding.size();

		//Encrypt the message
		encrypt_srpp(&srpp_msg);

		//Return
		return srpp_msg;

	}

	//convert a SRPP packet back to RTP packet
	RTPMessage srpp_to_rtp(SRPPMessage * srpp_msg){

		//Decrypt the SRPP Message
		decrypt_srpp(srpp_msg);


		//Unpad the SRPP Message
		RTPMessage rtp_msg;
		if (padding_functions.unpad(srpp_msg) == -1 )
		{
			//Create a RTPMessage with the data from SRPP packet
			cout << "Dummy Message\n" << "\n";
			rtp_msg = create_rtp_message("");
		}
		else
		{
			//Create a RTPMessage with the data from SRPP packet
		    string data (srpp_msg->encrypted_part.original_payload.begin(),srpp_msg->encrypted_part.original_payload.end());
			rtp_msg = create_rtp_message(data);
		}

		rtp_msg.rtp_header.version = srpp_msg->srpp_header.version;
		rtp_msg.rtp_header.p = srpp_msg->encrypted_part.original_padding_bit;
		rtp_msg.rtp_header.cc = srpp_msg->srpp_header.cc;
		rtp_msg.rtp_header.x = srpp_msg->srpp_header.x;
		rtp_msg.rtp_header.m = srpp_msg->srpp_header.m ;
		rtp_msg.rtp_header.seq = srpp_msg->encrypted_part.original_seq_number;
		rtp_msg.rtp_header.pt= srpp_msg->srpp_header.pt ;
		rtp_msg.rtp_header.ts = srpp_msg->srpp_header.ts ;
		rtp_msg.rtp_header.ssrc= srpp_msg->srpp_header.ssrc;

		for (int i = 0; i< ntohs(srpp_msg->srpp_header.cc); i++)
			rtp_msg.rtp_header.csrc[i]= srpp_msg->srpp_header.csrc[i];


		return rtp_msg;

}

	// Convert a SRTP packet to SRPP packet
	SRPPMessage srtp_to_srpp(SRTPMessage * srtp_msg){

		char * data = (char *) srtp_msg;
		string str(data);

		//Create a SRPPMessage with the data from SRTP packet
		SRPPMessage srpp_msg = create_srpp_message(str);

		srpp_msg.srpp_header.version = srtp_msg->srtp_header.version;
		srpp_msg.srpp_header.cc = srtp_msg->srtp_header.cc;
		srpp_msg.srpp_header.x = srtp_msg->srtp_header.x;
		srpp_msg.srpp_header.m = srtp_msg->srtp_header.m;
		srpp_msg.srpp_header.pt = srtp_msg->srtp_header.pt;
		srpp_msg.srpp_header.ts = srtp_msg->srtp_header.ts;
		srpp_msg.srpp_header.ssrc = srtp_msg->srtp_header.ssrc;

		for (int i = 0; i< ntohs(srtp_msg->srtp_header.cc); i++)
			srpp_msg.srpp_header.csrc[i] = srtp_msg->srtp_header.csrc[i];

		srpp_msg.srpp_header.srpp_signalling = 0;

		srpp_msg.encrypted_part.dummy_flag = 0;

		//Pad the SRPPMessage
		padding_functions.pad(&srpp_msg);
		srpp_msg.encrypted_part.pad_count = srpp_msg.encrypted_part.srpp_padding.size();

		//Encrypt the message
		encrypt_srpp(&srpp_msg);

		//Return
		return srpp_msg;

	}

	//Convert a SRPP packet back to SRTP
	SRTPMessage srpp_to_srtp(SRPPMessage * srpp_msg){

		//Decrypt the SRPP Message
		decrypt_srpp(srpp_msg);


		//Unpad the SRPP Message
		SRTPMessage *srtp_msg;
		if (padding_functions.unpad(srpp_msg) > 0 )
		{
			//Create a SRTPMessage with the data from SRPP packet
			string data (srpp_msg->encrypted_part.original_payload.begin(),srpp_msg->encrypted_part.original_payload.end());
			srtp_msg = (SRTPMessage *)data.c_str();
		}

		return *srtp_msg;
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

		SRPPMessage srpp_msg;

		//put data, if any, in the payload
		if(!data.empty())
		{		srpp_msg.encrypted_part.original_payload = vector<char>(data.begin(),data.end());	}

		return srpp_msg;

	}

	// Only create a RTP Message and return it.
		RTPMessage create_rtp_message(string data){

			RTPMessage rtp_msg;

			//put data, if any, in the payload
			if(!data.empty())
				data.copy((rtp_msg.payload),data.length(),0);

			rtp_msg.payload[data.length()] = '\0';

			return rtp_msg;

		}

	// Encrypt the given SRPP packet
	SRPPMessage encrypt_srpp(SRPPMessage * original_pkt)
	{
		//TODO::: USE CRYPTO
		cout << "KEY:" << srpp_session->encryption_key << endl;
		int val = srpp_rand(0,2^15);
		val |= 1;

		original_pkt->encrypted_part.dummy_flag &= val;

		original_pkt->encrypt(srpp_session->encryption_key);
		return *original_pkt;

	}

	//Decrypt the given SRPP packet
	SRPPMessage decrypt_srpp(SRPPMessage * encrypted_pkt)
	{
		//TODO::: USE CRYPTO
		encrypted_pkt->decrypt(srpp_session->encryption_key);
		encrypted_pkt->encrypted_part.dummy_flag &= 1;

		cout << " REMOVED extra: dummy=" << encrypted_pkt->encrypted_part.dummy_flag << endl;
		return *encrypted_pkt;
	}

	// Pass a process function or functor which takes in message and length of message and returns and int status
	int setSendFunctor(int (*process_func)(char*,int))
	{
		send_processor = process_func;
		send_external = 1;
	}

	// Pass a process function or functor which takes in message and length of message and returns and int status
	int setReceiveFunctor(SRPPMessage (*process_func)())
	{
		receive_processor = process_func;
		receive_external = 1;
	}


	int send_message(SRPPMessage * message)
	{
		packet_to_send = 1;

		if (message->encrypted_part.original_payload.size() == 0)
			return 0;

		int size = sizeof(message->srpp_header) + message->encrypted_part.original_payload.size()  +
					message->encrypted_part.srpp_padding.size() + 3* sizeof(uint32_t);

/*
		cout << sizeof(message->srpp_header) << "::" << message->encrypted_part.original_payload.size()   << "::" <<
							message->encrypted_part.srpp_padding.size()  << "::" << 3* sizeof(uint32_t) << ":: " << message->encrypted_part.pad_count << endl;

*/
		char * buff = new char[size];
		if (isSignalingMessage(message) == 1)
		{
			message->srpp_to_network(buff, 0);
		} else {
			message->srpp_to_network(buff, srpp_session->encryption_key);
		}

		//cout << "SENT " << size << " bytes to "<< inet_ntoa(srpp_session->sender_addr.sin_addr)
		//		<< ":" << ntohs(srpp_session->sender_addr.sin_port) <<"\n\n" ;

		if (send_external == 1) // We have a sender functor to process the sending action.
		{
			return send_processor(buff,size); //message,length
		}

		int byytes = sendto(srpp_session->sendersocket, buff, size, 0,
							              (struct sockaddr *)&(srpp_session->sender_addr), sizeof(struct sockaddr));
		if (byytes < 0)
			cout << "ERROR IN SENDING DATA: " << strerror(errno)<< endl;

		string str (message->encrypted_part.original_payload.begin(),message->encrypted_part.original_payload.end());
		cout << "\nWriting " << byytes << " bytes \""
						//<< str << "\" to other endpoint at "
						<< inet_ntoa(srpp_session->sender_addr.sin_addr) << ":"
						<< ntohs(srpp_session->sender_addr.sin_port) << endl << endl;

		return byytes;
	}

SRPPMessage receive_message()
	{

		if (receive_external == 1) // We have a sender functor to process the sending action.
			{
				return receive_processor(); //message,length
			}

		int addr_len = sizeof(struct sockaddr);

		cout << "\nWaiting for messages ... " << endl;


		//TODO: WE NEED TO ADD A SELECT OR POLL SO AS TO AVOID WAITING FOR A LONG TIME
		char buff[65535];
		int bytes_read = recvfrom(srpp_session->receiversocket,buff,sizeof(buff),0,
						(struct sockaddr *)&(srpp_session->sender_addr),
						(socklen_t *)&addr_len);

		if (bytes_read < 0)
					cout << "ERROR IN RECEIVING DATA: " << strerror(errno)<< endl;
		else {
		 cout << "Read " << bytes_read << " bytes from the other endpoint at "
						<< inet_ntoa(srpp_session->sender_addr.sin_addr) << ":"
						<< ntohs(srpp_session->sender_addr.sin_port ) << endl;

		 return processReceivedData(buff, bytes_read);
		}

	}

SRPPMessage processReceivedData(char * buff, int bytes_read)
	{

		SRPPMessage srpp_msg = srpp::create_srpp_message("");
		if (signaling_by_srpp == 0 && signaling_functions.isSignalingComplete() == 1)
		{
			//signaling done entirely through sip
			srpp_msg.network_to_srpp(buff,bytes_read, srpp_session->encryption_key);
			return srpp_msg;
		}

		//verify if we need to look for signaling and enabling srpp still
		if (isSignalingMessage(buff) == 1 || (verifySignalling(buff) == 0 && srpp_enabled == 0))
		{
			if (isSignalingMessage(buff) == 1)
				srpp_msg.network_to_srpp(buff,bytes_read, 0);
			else
				srpp_msg.network_to_srpp(buff,bytes_read, srpp_session->encryption_key);


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

						thiskey = options.substr(
						                 options.find_first_of(',')+1,
						                 options.substr(options.find_first_of(',')+1,options.length()).find_first_of(',') );

						//set maxpacketsize to min(maxpacketsize, received maxpayloadsize)
						if (srpp_session->maxpacketsize > atoi(thiskey.c_str())) {
							srpp_session->maxpacketsize = atoi(thiskey.c_str()) ;
							maxpacketsize = srpp_session->maxpacketsize;
						}

						cout << "MAXPAYLOADSIZE RECEIVED: " << thiskey << endl;
						signaling_functions.receivedHelloMessage();

					}
					else if (srpp_msg.srpp_header.srpp_signalling == 13)
					{
						cout << "Received HELLO ACK message " << endl;

						//extract maxpayloadsize from the payload
						string options (srpp_msg.encrypted_part.original_payload.begin(),srpp_msg.encrypted_part.original_payload.end());
						string thiskey = options.substr(0,options.find_first_of(','));

						//set maxpacketsize to min(maxpacketsize, received maxpayloadsize)
						if (srpp_session->maxpacketsize > atoi(thiskey.c_str()))
						{
								srpp_session->maxpacketsize = atoi(thiskey.c_str()) ;
								maxpacketsize = srpp_session->maxpacketsize;
						}

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
	}

 // parse the received message ... returns -1 if its a media packet.. and 1 if its a signaling packet (whose corresponding handler is called)
 int isSignalingMessage (SRPPMessage * message)
 {

	 if (message->srpp_header.srpp_signalling == 0 and message->srpp_header.pt != 124) //NOT A SIGNALING MESSAGE
		 return -1;
	 else if(message->srpp_header.srpp_signalling != 0 and message->srpp_header.pt == 124)
		 return 1;

	 return 0;
 }

 // parse the received message ... returns -1 if its a media packet.. and 1 if its a signaling packet (whose corresponding handler is called)
  int isSignalingMessage (char * buff1)
  {
	  char buff[sizeof(SRPPHeader)+10];
	  memcpy(buff,buff1,sizeof(SRPPHeader));

	  SRPPHeader* srpp_header1 = (SRPPHeader *) buff;
      SRPPHeader srpp_header = *srpp_header1;

	  cout << "PAYLOAD TYPE :" << srpp_header.pt << " SIGNALING:" << srpp_header.srpp_signalling << "\n";

	  if (srpp_header.srpp_signalling == 0 and srpp_header.pt != 124) //NOT A SIGNALING MESSAGE
 		 return -1;
 	 else if(srpp_header.srpp_signalling !=0 and srpp_header.pt == 124){
 		 return 1;
 	 }

 	 return 0;
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
		return 0;
 }

//verify if we need to look for signaling and enabling srpp still
int verifySignalling(char * buff)
 {
	// Are we waiting for a HELLO ACK MESSAGE? We need to check that the reply is SRPP
		// or not. Otherwise, we need to disable SRPP
		if (signaling_functions.isHelloSent() == 1)
		{
		// CHECK IF WE HAVE SRPP EXTENSION IN THE RECEIVED MESSAGE.
		// IF NOT, WE DISABLE SRPP AND STOP RECEIVING HERE.
		SRPPHeader* srpp_header1 = (SRPPHeader *) buff;
		SRPPHeader srpp_header = *srpp_header1;


		if (srpp_header.srpp_signalling != 13 && srpp_header.pt != 124) //not helloack signaling message
		{
			// NOT SRPP MESSAGE.
			++nonsrpp_message_count;
			if (nonsrpp_message_count > max_nonsrpp_messages)
			{
				// STOP SESSIONS ETC AFTER 100 such messages. ?? CHANGE IF REQD. ??
				srpp_enabled = 0;
				return stop_session();

			}
			else
				return -1;
		}

	}
	else
		return 0;
 }


	//Get the padding functions object used here
	PaddingFunctions* get_padding_functions(){
		return &(padding_functions);
	}

	// Get the current session object
	SRPPSession * get_session(){
		return srpp_session;
	}

	// Pseudo-Random number between min and max
	int srpp_rand(int min,int max){

		if (max == 0)
			return 0;

		timeval a;
		gettimeofday(&a, NULL);
		srand(1000000*a.tv_sec + a.tv_usec);      // SEED ON MICROSECONDS

			return ((rand() % max) + min);

		}

	int resetPacketTimer()
	{
		return srpp_session->srpp_timer->reset_packet();
	}
	int resetSilenceTimer()
	{
		return srpp_session->srpp_timer->reset_silence();

	}

	 //Get the encryption in the session
	  int getKey()
	  {
		 if(srpp_session)
		  return srpp_session->encryption_key;
		 else
			 return -1;
	  }

	  //Get the maximum payload size in the session
	  int getMaxPayloadSize(){
		  return MAXPAYLOADSIZE;
	  }

	  int disable_srpp()
	  {
		srpp_enabled = 0;
		return 0;
	  }

	  int enable_srpp()
	  {
		srpp_enabled = 1;
		return 0;
	  }

	  int setSignalingComplete()
	  {
		  return signaling_functions.setSignalingComplete(1);
	  }


	  int set_full_bandwidth()
	  {
		  return padding_functions.set_full_bandwidth();
	  }
} // end of namespace
