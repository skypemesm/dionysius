/***
 *  \file sdp_srpp.hpp
 *
 *  This class provides the elements required for SRPP negotiation
 *  through SIP/SDP
 *
 *  Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */
#ifndef SDP_SRPP_HPP
#define SDP_SRPP_HPP

#include <string>
#include <cstdlib>

class sdp_srpp {

	int valid;
	int srpp_active;
	string options;
	int key;
	int maxpacketlength; //maxpacketsize

public:
	/**
	 * THIS RETURNS THE STRING THAT NEEDS TO GO INTO sdp OFFER OR ANSWER
	 */
	string get_sdp_srpp()
	{
		string thissdp = "a=srpp\na=srppmap:";
		thissdp.append(options);

		return thissdp;
	}

	/** deactivate srpp **/
	int deactivate_srpp()
	{
		srpp_active = 0;
		return 0;
	}

	/** activate srpp **/
	int activate_srpp()
	{
		srpp_active = 1;
		return 0;
	}

	int is_active()
	{
		return srpp_active;
	}

	int is_valid()
	{
		return valid;
	}

	/**
	 * GETTER Methods to APPLY the options received so that SRPP can now start
	 */
	int getKey()
	{
		return key;
	}

	int getMaxPacketSize()
	{
		return maxpacketlength;
	}

	/**
	 * THIS PROCESSES SDP for SRPP
	 * @param int offer_or_answer: 0 if this is a SDP offer and 1 if SDP answer
	 */
	int process_sdp_srpp(int offer_or_answer,string thisoptions)
	{
		options = thisoptions;

		//extract key from the payload
		string thiskey = options.substr(0,options.find_first_of(','));
		key = atoi(thiskey.c_str());


		cout << "KEY RECVD: " << thiskey << endl;

		thiskey = options.substr(
						 options.find_first_of(',')+1,
						 options.substr(options.find_first_of(',')+1,options.length()).find_first_of(',') );

			//set maxpacketsize to min(maxpacketsize, received maxpayloadsize)
		if (maxpacketlength > atoi(thiskey.c_str())) {

			maxpacketlength = atoi(thiskey.c_str()) ;

		}

		cout << "MAXPAYLOADSIZE RECEIVED: " << thiskey << endl;


		// !!!!!!!!!!!!!!!!!!!!!!! PROCESS OTHER PARAMS HERE AS NECESSARY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//If anything is differently handled in the SDP OFFER THAN ANSWER---
		if (offer_or_answer == 0) // OFFER
		{

		}
		else //ANSWER
		{
		}

		if ( key < 0 || maxpacketlength < 0)
		{
			valid = 0;
		}
		else
		{
			valid = 1;
		}
		return 0;
	}





};

#endif
