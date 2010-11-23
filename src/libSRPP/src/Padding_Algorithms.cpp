/***
 *  \file Padding_algorithms.cpp
 *
 *  This file implements the algorithms needed for different padding techniques
 *
 *  ADD YOUR NEW PADDING ALGORITHMS HERE...
 *
 *  Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#include <iostream>
#include "../include/Padding_Algorithms.h"
#include "../include/SRPP_functions.h"
#include "../include/Padding_functions.h"


using namespace std;
extern int packet_to_send;

int cbp_packet_count = 0;
int is_full_bandwidth = 0;

	/** Sets the behavior to pad all packets to maximum packet size or full bandwidth **/
	int PaddingAlgos::set_full_bandwidth()
	{
		is_full_bandwidth = 1;
		cout << "Padding options set to FULL BANDWIDTH padding.\n";
	}

	/** Redirects to the specified Packet Size Padding algo **/
	int PaddingAlgos::psp_pad_algo(psp_algo_type atype,SRPPMessage * srpp_msg)
	{
		/**
		 * The algorithm included here must pad the packet to a calculated size
		 */

		if (atype == DEFAULT_PSP)
			return default_psp_pad_algo(srpp_msg);
		else
			return -1;

	}

	/** Redirects to the specified Packet Size Padding algo
	 *
	 *  This is called whenever the packet timer fires. We must see that the algo does this:
	 *     - determine if we need to send a dummy packet based on burst length
	 *     - if we need to send a dummy packet, reset ptimer and silence timer and send dummy, if we have not received a legit
	 *       packet yet
	 *     - else do nothing really.
	 * **/
	int PaddingAlgos::cbp_pad_algo(cbp_algo_type atype)
	{
		if (atype == DEFAULT_CBP)
					return default_cbp_pad_algo();
		else
			return -1;
	}

	/** Redirects to the specified Packet Size Padding algo
	 *
	 *  This is called whenever the silence timer fires. We must see that the algo does this:
	 *     - determine if we need to send a dummy packet based on silence burst length
	 *     - if we need to send a dummy packet, reset ptimer and silence timer and send dummy, if we have not received a legit
	 *       packet yet
	 *     - else do nothing really.
	 */
int PaddingAlgos::ebp_pad_algo(ebp_algo_type atype)
	{

		if (atype == DEFAULT_EBP)
						return default_ebp_pad_algo();
		else
			return -1;
	}

	/** Redirects to the specified Packet Size Padding algo **/
	int PaddingAlgos::vitp_pad_algo(vitp_algo_type atype)
	{
		if (atype == DEFAULT_VITP)
						return default_vitp_pad_algo();
		else
			return -1;
	}



	int PaddingAlgos::default_psp_pad_algo(SRPPMessage * srpp_msg)
	{

		// I will get a random extra size and add the extra bytes to the packet
		int extra_size = 1366 - (srpp_msg->encrypted_part.original_payload.size());

		if (is_full_bandwidth == 0)
			extra_size = srpp::srpp_rand(1,extra_size);

		string status = PaddingFunctions::generate_dummy_data(extra_size);

		if (status.length() < 0)
				cout << "ERROR IN GENERATING DUMMY DATA.." << endl;

		srpp_msg->encrypted_part.srpp_padding = vector<char>(status.begin(),status.end());
		srpp_msg->encrypted_part.pad_count = extra_size;

		//reset both packet and silence timers
		srpp::resetPacketTimer();
		srpp::resetSilenceTimer();

		return 0;
	}

	int PaddingAlgos::default_cbp_pad_algo()
	{
		int calculated_burst_dummies = srpp::srpp_rand(0,10); // THIS IS WHAT WE WILL CALCULATE BASED ON CURRENT BURST SIZE

		cout << "Sending " << calculated_burst_dummies << "packets\n";
		cbp_packet_count=0;
		if ((++cbp_packet_count) <= calculated_burst_dummies)
		{
			SRPPMessage dummy_msg = PaddingFunctions::generate_dummy_pkt();
			//cout << "Sequence Number of Dummy packet: " << dummy_msg.get_sequence_number() << endl;
			srpp::encrypt_srpp(&dummy_msg);

			//check if we already have a packet to send
			//send if NO
			if (packet_to_send == 0){
				srpp::send_message(&dummy_msg);
			} else
			{
				packet_to_send = 0;
				cout << "Waiting for.." << packet_to_send << endl;

			}

			//reset both packet and silence timers
			srpp::resetPacketTimer();
			srpp::resetSilenceTimer();


		} else // We are done sending packets
		{
			//reset packet timer
			srpp::resetPacketTimer();

		}


		return 0;
	}

	int PaddingAlgos::default_ebp_pad_algo()
	{
		cout << "I will send one packet" << endl;
		SRPPMessage dummy_msg = PaddingFunctions::generate_dummy_pkt();
		cout << "Sequence Number of Dummy packet: " << dummy_msg.get_sequence_number()
				<< " SIZE:" << dummy_msg.encrypted_part.original_payload.size() << endl;
		srpp::encrypt_srpp(&dummy_msg);

		//check if we already have a packet to send
		//send if NO
		srpp::send_message(&dummy_msg);
		//Reset packet and silence timers

		return 0;
	}

	int PaddingAlgos::default_vitp_pad_algo()
	{
		//Add delays


		return 0;
	}

