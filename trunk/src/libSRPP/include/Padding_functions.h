/***
 *  \file Padding_functions.cpp
 *
 *  This file declares the functions needed for padding rtp/srtp messages to
 *  corresponding srpp packets
 *
 *  Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */



#ifndef SRPP_MESSAGE_HPP
	#define SRPP_MESSAGE_HPP
	#include "SRPPMessage.hpp"
#endif
#ifndef SRPP_FUNCTIONS_H
	#define  SRPP_FUNCTIONS_H
	#include "SRPP_functions.h"
#endif
#ifndef PADDING_ALGORITHMS_H
	#define PADDING_ALGORITHMS_H
	#include "Padding_Algorithms.h"
#endif

#define	MAXDUMMYCACHESIZE	100       /** dummy cache of 100 srpp packets **/
using namespace std;

class PaddingFunctions {

public:
	PaddingFunctions();
	int packet_size_padding();
	int current_burst_padding();
	int extra_burst_padding();
	int variable_interarrival_time_padding();



private:

	SRPPMessage dummy_cache[100];


	SRPPMessage generate_dummy_pkt();
	SRPPMessage generate_dummy_pkt(int size);
	char*	generate_dummy_data(int size);

	int add_to_dummy_cache(SRPPMessage * srpp_msg);






};
