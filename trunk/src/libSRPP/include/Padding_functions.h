/***
 *  \file Padding_functions.cpp
 *
 *  This file declares the functions needed for padding rtp/srtp messages to
 *  corresponding srpp packets
 *
 *  Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#ifndef PADDING_FUNCTIONS_H
#define PADDING_FUNCTIONS_H

#include "SRPPMessage.hpp"
#include "Padding_Algorithms.h"

#define	MAXDUMMYCACHESIZE	99       /** dummy cache of 100 srpp packets **/

class PaddingFunctions {

public:
	PaddingFunctions();
	~PaddingFunctions();

	int pad(SRPPMessage * srpp_msg);

	int packet_size_padding(SRPPMessage * srpp_msg);
	int current_burst_padding();
	int extra_burst_padding();
	int variable_interarrival_time_padding();

	int unpad(SRPPMessage * srpp_msg);

	static SRPPMessage generate_dummy_pkt();
	static SRPPMessage generate_dummy_pkt(int size);
	static string	generate_dummy_data(int size);

	static int add_to_dummy_cache(SRPPMessage srpp_msg);

private:

	static SRPPMessage dummy_cache[100];
	PaddingAlgos algos;









};

#endif
