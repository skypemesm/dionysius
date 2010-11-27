/**
 * \file srtp.hpp
 * This file implements the major functions of srtp.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#ifndef SRTP_MESSAGE_HPP
#define SRTP_MESSAGE_HPP

#include "rtp.hpp"

extern int srtpSequenceNo;

struct SRTP_Header{

#if __BYTE_ORDER == __BIG_ENDIAN
	  uint16_t		version:2;	/** protocol version       */
	  uint16_t		p:1;		/** padding flag           */
	  uint16_t		x:1;		/** header extension flag  */
	  uint16_t		cc:4;		/** CSRC count             */
	  uint16_t		m:1;		/** marker bit             */
	  uint16_t		pt:7;		/** payload type           */
#else
	  uint16_t		cc:4;		/** CSRC count             */
	  uint16_t		x:1;		/** header extension flag  */
	  uint16_t		p:1;		/** padding flag           */
	  uint16_t		version:2;	/** protocol version       */
	  uint16_t		pt:7;		/** payload type           */
	  uint16_t		m:1;		/** marker bit             */
#endif
	  uint16_t		seq;		/** sequence number        */
	  uint32_t		ts;			/** timestamp              */
	  uint32_t		ssrc;		/** synchronization source */
	  uint32_t		csrc[15];		/** contributing sources  */
};


class SRTPMessage {
public:
	struct SRTP_Header srtp_header;						/** SRTP Header **/
	char payload[MAXPAYLOADSIZE];
	uint32_t mki;
	uint32_t authentication_tag;

   SRTPMessage()
    	  {

    		  srtp_header.version = 2;
    		  srtp_header.p = 1;
    		  srtp_header.x = 0;
    		  srtp_header.cc = 15;
    		  srtp_header.m = 0;
    		  srtp_header.pt = 0;
    		  srtp_header.seq = ++srtpSequenceNo;
    		  srtp_header.ts = 0;
    		  srtp_header.ssrc = 0;

    		  mki = 0;
    		  authentication_tag = 0;


    	  }
	  ~SRTPMessage()
	  {

	  }

	/*  // Convert a extract rtp packet to network format
	  int srtp_to_network(char * srtp_buff, int length)
	  {

		    SRTP_Header* srtp_header1 = (SRTP_Header *) rtp_buff;
		    *srtp_header1 = srtp_header;
			char* data = (char *) (srtp_buff + sizeof(SRTP_Header) - 4*(15-ntohs(srtp_header.cc)));
			//copy the payload
			memcpy(data, payload, length);

			uint32_t* thisone = (uint32_t*)(data+length);
			*thisone = htons(mki);
			thisone++;
			*thisone = htons(authentication_tag);

			return 0;

	  }*/

	  int print()
	  {

		  cout << "\n****************** SRTP PACKET *******************************\n";
		  cout << "Version:" << srtp_header.version << endl;
		  cout << "Padding Bit:" << srtp_header.p << endl;
		  cout << "Extension Bit:" << srtp_header.x << endl;
		  cout << "CSRC Count:" << srtp_header.cc << endl;
		  cout << "Marker Bit:" << srtp_header.m << endl;
		  cout << "Payload Type:" << srtp_header.pt << endl;
		  cout << "Sequence Number:" << srtp_header.seq << endl;
		  cout << "Timestamp:" << srtp_header.ts << endl;
		  cout << "SSRC:" << srtp_header.ssrc << endl;

		  //csrc
		  cout << "CSRCs:";
		  for (int i = 0; i< ntohs(srtp_header.cc); i++)
			  cout << srtp_header.csrc[i] << ",";


		  cout << "\n";

		  //payload
		  cout << payload << endl;

		  cout << "************************************************************\n";

	  }

};


#endif
