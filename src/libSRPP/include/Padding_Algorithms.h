/***
 *  \file Padding_algorithms.h
 *
 *  This file implements the algorithms needed for different padding techniques
 *
 *  ADD YOUR NEW PADDING ALGORITHMS HERE...
 *
 *  Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#ifndef PADDING_ALGORITHMS_H
#define PADDING_ALGORITHMS_H

#include <string>

#include "SRPPMessage.hpp"

using namespace std;


class PaddingAlgos {


public:

	enum psp_algo_type { DEFAULT_PSP };
	enum cbp_algo_type { DEFAULT_CBP };
	enum ebp_algo_type { DEFAULT_EBP };
	enum vitp_algo_type { DEFAULT_VITP };

	PaddingAlgos(){}
	~PaddingAlgos(){}


	int psp_pad_algo(psp_algo_type atype,SRPPMessage * srpp_msg);
	int cbp_pad_algo(cbp_algo_type atype);
	int ebp_pad_algo(ebp_algo_type atype);
	int vitp_pad_algo(vitp_algo_type atype);



private:
	int cbp_packet_count;

	int default_psp_pad_algo(SRPPMessage * srpp_msg);
	int default_cbp_pad_algo();
	int default_ebp_pad_algo();
	int default_vitp_pad_algo();


};

#endif
