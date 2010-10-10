/**
 * \file rtp.hpp
 * This file implements the major functions of rtp.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#ifndef SRTP_MESSAGE_HPP
#define SRTP_MESSAGE_HPP

extern int srtpSequenceNo;

struct SRTP_Header{

	  uint16_t		version:2;	/** protocol version       */
	  uint16_t		p:1;		/** padding flag           */
	  uint16_t		x:1;		/** header extension flag  */
	  uint16_t		cc:4;		/** CSRC count             */
	  uint16_t		m:1;		/** marker bit             */
	  uint16_t		pt:7;		/** payload type           */
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
};


#endif
