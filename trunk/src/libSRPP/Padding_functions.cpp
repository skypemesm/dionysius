/***
 *  \file Padding_functions.cpp
 *
 *  This file implements the functions needed for padding rtp/srtp messages to
 *  corresponding srpp packets
 *
 *  Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#include "Padding_functions.h"

using namespace std;

/**
 * Default constructor .. initializes the dummy cache
 */
PaddingFunctions::PaddingFunctions()
{
	unsigned char buff[MAXPAYLOADSIZE];
	int thisdummysize;

	//put random data in dummy cache
	for(int i = 0; i < MAXDUMMYCACHESIZE; i++)
	{
		SRPPMessage* thisdummy = new SRPPMessage(buff);
		thisdummysize = srpp::srpp_rand(1, MAXPAYLOADSIZE);

		for (int j = 0; j < thisdummysize; j++)
			thisdummy->encrypted_part.original_payload[j] = srpp::srpp_rand(1,255) ^ srpp::srpp_rand(0,65536); // characters xored with some random number

		dummy_cache[i] = *thisdummy;

	}


}

/** Performs packet size padding i.e. increases the size of the given SRPP message with raw RTP/SRTP data
 *  using some padding algorithm
 */
int PaddingFunctions::packet_size_padding()
	{
	}

/**
 * Performs Current Burst PAdding i.e. increases the current burst by adding some dummy packets
 * based on some CBP algorithm
 */
int PaddingFunctions::current_burst_padding()
	{
	}

/**
 * Performs Extra Burst Padding i.e. adds extra bursts of dummy packets in periods of silence
 * based on some EBP algorithm
 */
int PaddingFunctions::extra_burst_padding()
{
}

/**
 * Performs Variable Interarrival Time Padding i.e. adds random delay so as to disturb the distribution of original Packet
 * InterArrival times.
 */
int PaddingFunctions::variable_interarrival_time_padding()
{


}


/**
 * Generates a Dummy packet by randomly selecting a particular packet from the dummy cache
 */
SRPPMessage PaddingFunctions::generate_dummy_pkt()
{

}

/**
 * Generates a Dummy packet of a particular size by randomly selecting a particular packet from the dummy cache
 */
SRPPMessage PaddingFunctions::generate_dummy_pkt(int size)
{

}

/** Generates a character array containing some dummy payload data (Useful for PSP algos)**/
char*	PaddingFunctions::generate_dummy_data(int size)
{

}

/** This functions handles adding of a newly generated SRPPMessage to the Dummy cache **/
int PaddingFunctions::add_to_dummy_cache(SRPPMessage * srpp_msg)
{

}




