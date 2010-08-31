/**
 * This file implements the major functions of rtp.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#ifndef SRPP_MESSAGE_HPP
	#define SRPP_MESSAGE_HPP
	#include "SRPPMessage.hpp"
#endif


struct RTPHeader{

	  uint16_t		version:2;	/** protocol version       */
	  uint16_t		p:1;		/** padding flag           */
	  uint16_t		x:1;		/** header extension flag  */
	  uint16_t		cc:4;		/** CSRC count             */
	  uint16_t		m:1;		/** marker bit             */
	  uint16_t		pt:7;		/** payload type           */
	  uint16_t		seq;		/** sequence number        */
	  uint32_t		ts;			/** timestamp              */
	  uint32_t		ssrc;		/** synchronization source */
	  uint32_t		csrc[10];		/** contributing sources  */


};


class RTPMessage {
public:
	struct RTPHeader rtp_header;						/** RTP Header **/
	char payload[MAXPAYLOADSIZE];

    RTPMessage(unsigned char * buff)
	  {

		  rtp_header.version = 2;
		  rtp_header.p = 1;
		  rtp_header.x = 0;
		  rtp_header.cc = 0;
		  rtp_header.m = 0;
		  rtp_header.pt = 0;
		  rtp_header.seq = 0;
		  rtp_header.ts = 0;
		  rtp_header.ssrc = 0;

	  }

	  ~RTPMessage()
	  {

	  }
};
