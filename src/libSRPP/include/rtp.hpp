/**
 * \file rtp.hpp
 * This file implements the major functions of rtp.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#ifndef RTP_MESSAGE_HPP
#define RTP_MESSAGE_HPP

extern int rtpSequenceNo;

struct RTP_Header{
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


class RTPMessage {
public:
	struct RTP_Header rtp_header;						/** RTP Header **/
	char payload[MAXPAYLOADSIZE];

   RTPMessage()
    	  {

    		  rtp_header.version = 2;
    		  rtp_header.p = 1;
    		  rtp_header.x = 0;
    		  rtp_header.cc = 15;
    		  rtp_header.m = 0;
    		  rtp_header.pt = 0;
    		  rtp_header.seq = ++rtpSequenceNo;
    		  rtp_header.ts = 0;
    		  rtp_header.ssrc = 0;


    	  }
	  ~RTPMessage()
	  {

	  }

	  int print()
	  {

		  cout << "\n****************** RTP PACKET *******************************\n";
		  cout << "Version:" << rtp_header.version << endl;
		  cout << "Padding Bit:" << rtp_header.p << endl;
		  cout << "Extension Bit:" << rtp_header.x << endl;
		  cout << "CSRC Count:" << rtp_header.cc << endl;
		  cout << "Marker Bit:" << rtp_header.m << endl;
		  cout << "Payload Type:" << rtp_header.pt << endl;
		  cout << "Sequence Number:" << rtp_header.seq << endl;
		  cout << "Timestamp:" << rtp_header.ts << endl;
		  cout << "SSRC:" << rtp_header.ssrc << endl;

		  //csrc
		  cout << "CSRCs:";
		  for (int i = 0; i<rtp_header.cc; i++)
			  cout << rtp_header.csrc[i] << ",";

		  cout << "\n";

		  //payload
		  cout << payload << endl;

		  cout << "************************************************************\n";

	  }
};


#endif
