/***
 *  \file Padding_functions.cpp
 *
 *  This file implements the functions needed for padding rtp/srtp messages to
 *  corresponding srpp packets
 *
 *  Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#include <iostream>
#include "../include/Padding_functions.h"
#include "../include/SRPP_functions.h"


using namespace std;

SRPPMessage PaddingFunctions::dummy_cache[100];
extern int maxpacketsize;

/**
 * Default constructor .. initializes the dummy cache
 */
PaddingFunctions::PaddingFunctions()
{

	int thisdummysize;

	//put random data in dummy cache
	for(int i = 0; i < MAXDUMMYCACHESIZE; i++)
	{
		SRPPMessage* thisdummy = new SRPPMessage();
		//cout << "MAXPACKETSIZE HERE: " << maxpacketsize << endl;
		thisdummysize = srpp::srpp_rand(1, maxpacketsize);

		for (int j = 0; j < thisdummysize; j++)
			thisdummy->encrypted_part.original_payload.push_back( srpp::srpp_rand(1,255) ^ srpp::srpp_rand(0,65536)); // characters xored with some random number

		thisdummy->encrypted_part.dummy_flag = 1;
		dummy_cache[i] = *thisdummy;

		delete thisdummy;

	}


}
//Destructor,ofcourse
PaddingFunctions::~PaddingFunctions()
{
	//delete dummy_cache;
}

/** THe MAIN PAD FUNCTION WHICH CALLS THE OTHER PADDING FUNCTIONS **/
int PaddingFunctions::pad(SRPPMessage * srpp_msg)
{
	/**Sender Padding Algorithm**/

	//pad PSP everytime.
	packet_size_padding(srpp_msg);

	// set dummy flag
		srpp_msg->encrypted_part.dummy_flag = 0;

	//COIN TOSS AND add this to the dummy cache, if HEADS.
	if (srpp::srpp_rand(0,10) <= 5 )
	{
		SRPPMessage new_dummy (*srpp_msg);
		PaddingFunctions::add_to_dummy_cache(new_dummy);
	}

	return 0;
}


/** Performs packet size padding i.e. increases the size of the given SRPP message with raw RTP/SRTP data
 *  using some padding algorithm
 *  @param rtp : bool which is true if the packet is RTP and false if it is SRTP
 */
int PaddingFunctions::packet_size_padding(SRPPMessage * srpp_msg)
{
		algos.psp_pad_algo(PaddingAlgos::DEFAULT_PSP, srpp_msg);
		return 0;
}

/**
 * Performs Current Burst PAdding i.e. increases the current burst by adding some dummy packets
 * based on some CBP algorithm
 */
int PaddingFunctions::current_burst_padding()
{
	cout << "Starting to current burst pad the stream now" << endl;
	algos.cbp_pad_algo(PaddingAlgos::DEFAULT_CBP);
	return 0;
}

/**
 * Performs Extra Burst Padding i.e. adds extra bursts of dummy packets in periods of silence
 * based on some EBP algorithm
 */
int PaddingFunctions::extra_burst_padding()
{
	cout << "Starting to extra burst pad the stream now" << endl;
	algos.ebp_pad_algo(PaddingAlgos::DEFAULT_EBP);
	return 0;
}

/**
 * Performs Variable Interarrival Time Padding i.e. adds random delay so as to disturb the distribution of original Packet
 * InterArrival times.
 */
int PaddingFunctions::variable_interarrival_time_padding()
{

	algos.vitp_pad_algo(PaddingAlgos::DEFAULT_VITP);
	return 0;

}


int PaddingFunctions::unpad(SRPPMessage * srpp_msg)
{

	//discard if dummy and remove padding
	if (srpp_msg->encrypted_part.dummy_flag == 1)
			return -1;
	else
	{
		//remove the extra padding
		srpp_msg->encrypted_part.srpp_padding.clear();
		srpp_msg->encrypted_part.pad_count = 0;
		return 1;
	}

}



/**
 * Generates a Dummy packet by randomly selecting a particular packet from the dummy cache
 */
SRPPMessage PaddingFunctions::generate_dummy_pkt()
{

	int dummy_index = srpp::srpp_rand(0,MAXDUMMYCACHESIZE);
	dummy_cache[dummy_index].srpp_header.seq = lastSequenceNo++;

	int pay_size = dummy_cache[dummy_index].encrypted_part.original_payload.size();
	int pad_size = dummy_cache[dummy_index].encrypted_part.srpp_padding.size();

	if (pay_size + pad_size > 800	)
	{
		//vect.erase(vect.begin()+(vect.size() - 5 ),vect.end());
		dummy_cache[dummy_index].encrypted_part.original_payload.erase(
				dummy_cache[dummy_index].encrypted_part.original_payload.begin()+0.5*(pay_size),
				dummy_cache[dummy_index].encrypted_part.original_payload.end());

		dummy_cache[dummy_index].encrypted_part.srpp_padding.erase(
						dummy_cache[dummy_index].encrypted_part.srpp_padding.begin()+0.5*(pad_size),
						dummy_cache[dummy_index].encrypted_part.srpp_padding.end());

		/*cout << dummy_cache[dummy_index].encrypted_part.original_payload.size() << "::"
				<< dummy_cache[dummy_index].encrypted_part.srpp_padding.size() << endl;*/

	}

	return dummy_cache[dummy_index];
}

/**
 * Generates a Dummy packet of a particular size by randomly selecting a particular packet from the dummy cache
 */
SRPPMessage PaddingFunctions::generate_dummy_pkt(int size)
{
	int dummy_index = srpp::srpp_rand(0,MAXDUMMYCACHESIZE);
	//dummy_cache[dummy_index].srpp_header.seq = lastSequenceNo++;

	int pay_size = dummy_cache[dummy_index].encrypted_part.original_payload.size();
	int pad_size = dummy_cache[dummy_index].encrypted_part.srpp_padding.size();

	if (pay_size > size	)
	{
		//vect.erase(vect.begin()+(vect.size() - 5 ),vect.end());
		dummy_cache[dummy_index].encrypted_part.original_payload.erase(
				dummy_cache[dummy_index].encrypted_part.original_payload.begin()+size,
				dummy_cache[dummy_index].encrypted_part.original_payload.end());

		dummy_cache[dummy_index].encrypted_part.srpp_padding.clear();
		dummy_cache[dummy_index].encrypted_part.pad_count = 0;

	}
	else if (pad_size > size - pay_size)
	{
		dummy_cache[dummy_index].encrypted_part.srpp_padding.erase(
				dummy_cache[dummy_index].encrypted_part.srpp_padding.begin()+size-pay_size,
				dummy_cache[dummy_index].encrypted_part.srpp_padding.end());

		dummy_cache[dummy_index].encrypted_part.pad_count = size - pay_size;
	}

	return dummy_cache[dummy_index];
}

/** Generates a character array containing some dummy payload data (Useful for PSP algos)**/
string	PaddingFunctions::generate_dummy_data (int size)
{
	int dummy_index = srpp::srpp_rand(0,MAXDUMMYCACHESIZE);
	int size_recvd = 0, packet_size = 0;
	string return_str;

	SRPPMessage thisdummy;

	while (size_recvd < size)
	{
		thisdummy = dummy_cache[dummy_index];
		packet_size = thisdummy.encrypted_part.original_payload.size();
		if (packet_size > (size-size_recvd))
			packet_size = (size-size_recvd);

		const char * str = (const char *)&(thisdummy.encrypted_part.original_payload);


		for(int i = 0; i< packet_size; i++)
			return_str += str[i];


		size_recvd += packet_size;

	}

	//cout << "Size reqd:" << size << " SIZE GOT:" << return_str.length() << endl;

	return return_str;

}

/** This functions handles adding of a newly generated SRPPMessage to the Dummy cache **/
int PaddingFunctions::add_to_dummy_cache(SRPPMessage srpp_msg)
{
	//We add this message after encrypting the message and also repadding it.

	int dummy_index = srpp::srpp_rand(0,MAXDUMMYCACHESIZE);

	int thisdummysize, j=0;

	//cout << "MAXPACKETSIZE HEREIN: " << maxpacketsize << endl;
	thisdummysize = srpp::srpp_rand(1, 500); // NOTE:: 1000 is given as the maximum packet size of the dummy now.

	for (; j < (int)srpp_msg.encrypted_part.original_payload.size(); j++)
			srpp_msg.encrypted_part.original_payload.at(j) ^= srpp::srpp_rand(0,65536); // original data xored with some random number

	for (; j < thisdummysize; j++)
			srpp_msg.encrypted_part.original_payload.push_back( srpp::srpp_rand(1,255) ^ srpp::srpp_rand(0,65536)); // characters xored with some random number


	srpp_msg.encrypted_part.dummy_flag = 1;

/*	cout << "ADDING A PACKET "<< srpp_msg.encrypted_part.original_payload.size() << " TO THE CACHE with index: " << dummy_index
				<< " and avaliable size " << dummy_cache[dummy_index].encrypted_part.original_payload.size()
				<< " MAX = " << MAXDUMMYCACHESIZE<< "\n\n";*/

	dummy_cache[dummy_index] = srpp_msg;

	return 0;

}

int PaddingFunctions::set_padding_options(int i)
{
	algos.set_options(i);
}




