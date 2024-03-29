/**
 * This contains the declaration of all the SRPP functions.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#ifndef SRPP_FUNCTIONS_H
#define  SRPP_FUNCTIONS_H

#include "SRPPMessage.hpp"
#include "rtp.hpp"
#include "srtp.hpp"
#include "CryptoProfile.hpp"
#include "Padding_functions.h"
#include "sdp_srpp.hpp"



#include <ctime>
#include <cstdlib>


#define PACKET_INTERVAL_TIME	300	           /** This is time interval in ms we wait before we start current burst padding  **/
#define SILENCE_INTERVAL_TIME	400            /** This is time interval in ms we wait in silence before we start extra burst padding  **/

class SRPPSession;									/** Forward Declaration  **/

namespace srpp {


	//initialize stuff
	int init_SRPP();

	//verify if SRPP has been disabled or not: Returns 0, if disabled
	int SRPP_Enabled();

	//create SRPP session
	SRPPSession* create_session(string address, int port, CryptoProfile crypto);

	//starts the srpp session
	int start_session();
	int start_session(sdp_srpp sdp);

	//stops the srpp session
	int stop_session();
	void stop_abnormally(int i);

	//Signaling start
	int signaling();

	// convert a RTP packet to SRPP packet
	SRPPMessage rtp_to_srpp(RTPMessage* rtp_msg);
	SRPPMessage rtp_to_srpp(RTP_Header rtp_hdr, char* buf, int length);

	//convert a SRPP packet back to RTP packet
	RTPMessage srpp_to_rtp(SRPPMessage* srpp_msg);

	// Convert a SRTP packet to SRPP packet
	SRPPMessage srtp_to_srpp(SRTPMessage* srtp_msg);

	//Convert a SRPP packet back to SRTP
	SRTPMessage srpp_to_srtp(SRPPMessage* srpp_msg);
	int srpp_to_srtp(SRPPMessage * srpp_msg, char * buff,int length);

	//Create a SRPP Message with the data and encrypt it and return it
	SRPPMessage create_and_encrypt_srpp(string data);

	// Only create a SRPP Message and return it.
	SRPPMessage create_srpp_message(string data);

	// Only create a RTP Message and return it.
	RTPMessage create_rtp_message(string data);
	// Only create a SRTP Message and return it.
	SRTPMessage create_srtp_message(string data);

	// Encrypt the given SRPP packet
	SRPPMessage encrypt_srpp(SRPPMessage * original_pkt);

	//Decrypt the given SRPP packet
	SRPPMessage decrypt_srpp(SRPPMessage * encrypted_pkt);

	//Get the padding functions object used here
	PaddingFunctions* get_padding_functions();

	//Get the current Session object
	SRPPSession * get_session();

	/** Utility functions **/

	// Pseudo-Random number between min and max
	int srpp_rand(int min,int max);

	//USed by the interior functions to send a specific message or receive a message
	int send_message(SRPPMessage* msg);
	SRPPMessage receive_message();
	SRPPMessage processReceivedData(char * buff, int bytes_read);
	int setSendFunctor(int (*process_func)(char*,int)); // Pass a process function or functor which takes in message and length of message and returns and int status
	int setReceiveFunctor(SRPPMessage (*process_func)()); // Pass a process function or functor which takes in message and length of message and returns and int status

	// parse the received message ... returns -1 if its a media packet.. and 1 if its a signaling packet (whose corresponding handler is called)
	int isSignalingMessage (SRPPMessage * message);
	int isSignalingMessage (char * buff);

	//Check whether the signaling is complete
	 int isSignalingComplete();
	 int setSignalingComplete();

	 //verify if we need to look for signaling and enabling srpp still
 	  int verifySignalling(char * buff);

	 //Check whether media session is complete
	  int isMediaSessionComplete();

	  //Set the encryption in the session
	  int setKey(int key);
	  //Get the encryption in the session
	  int getKey();
	  //Get the maximum payload size in the session
	  int getMaxPayloadSize();

	  //Set Full Bandwidth option
	  int set_full_bandwidth();
	  int set_burst_padding();
	  int set_gradual_ascent();
	  int set_min_prob_bin();
	  int set_small_perturbation();

	  int set_starting_sequenceno(int seq_no);
	  int set_packet_to_send();


	  //reset timers
	  int resetPacketTimer();
	  int resetSilenceTimer();

	  int disable_srpp();
	  int enable_srpp();
	  
	  
}


#endif
