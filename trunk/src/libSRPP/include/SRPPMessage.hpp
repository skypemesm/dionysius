/**
 * This class represents the structure of a SRPP message
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include <string>
#include <stdint.h>

#ifndef MAXPAYLOADSIZE
	#define MAXPAYLOADSIZE     16384           // 16384 bytes
#endif

using namespace std;

typedef struct SRPPHeader {
	  uint16_t		version:2;	/** protocol version       */
	  uint16_t		p:1;		/** padding flag           */
	  uint16_t		x:1;		/** header extension flag  */
	  uint16_t		cc:4;		/** CSRC count             */
	  uint16_t		m:1;		/** marker bit             */
	  uint16_t		pt:7;		/** payload type           */
	  uint16_t		seq;		/** srpp sequence number        */
	  uint32_t		ts;			/** srpp timestamp              */
	  uint32_t		ssrc;		/** synchronization source */

	  uint32_t		csrc[10];		/** contributing sources  */
	  uint32_t		srpp_signalling;		/** rtp extension flag for srpp */


} SRPPHeader ;

typedef struct SRPPEncrypted {
	char 		original_payload[MAXPAYLOADSIZE];   /** original rtp/srtp payload **/
    char 		srpp_padding[MAXPAYLOADSIZE];       /** padding (can maximum be full packet)**/
	uint32_t		pad_count;    					/** srpp pad count **/
	uint16_t		original_padding:1;				/** original packet's padding bit  **/
	uint16_t		dummy_flag:15;					/** Dummy flag for srpp packet **/
	uint16_t 		original_seq_number;			/** Original packet's seq. number **/

} SRPPEncrypted ;


class SRPPMessage {

public:
	SRPPHeader srpp_header;						/** SRPP Header **/
	SRPPEncrypted 	encrypted_part;				/** This is the encrypted part of the packet **/
    uint32_t		authentication_tag;				/** Authentication Tag  **/



    SRPPMessage(unsigned char * buff)
	  {

		  srpp_header.version = 2;
		  srpp_header.p = 1;
		  srpp_header.x = 0;
		  srpp_header.cc = 0;
		  srpp_header.m = 0;
		  srpp_header.pt = 0;
		  srpp_header.seq = 0;
		  srpp_header.ts = 0;
		  srpp_header.ssrc = 0;

		  encrypted_part.pad_count = 0;
		  encrypted_part.original_padding = 0;
		  encrypted_part.dummy_flag = 0;
		  encrypted_part.original_seq_number = 0;

		  authentication_tag = 0;
	  }

	  ~SRPPMessage()
	  {

	  }
};

