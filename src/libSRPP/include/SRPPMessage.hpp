/**
 * This class represents the structure of a SRPP message
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */
#ifndef SRPP_MESSAGE_HPP
#define SRPP_MESSAGE_HPP

#include <iostream>
#include <cstdio>

#include <cstring>
#include <string>
#include <stdint.h>
#include <vector>

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
	string 		original_payload;  					 /** original rtp/srtp payload **/
    string 		srpp_padding;       				 /** padding (can maximum be full packet)**/
	uint32_t		pad_count;    					 /** srpp pad count **/
	uint16_t		original_padding_bit:1;			 /** original packet's padding bit  **/
	uint16_t		dummy_flag:15;					 /** Dummy flag for srpp packet **/
	uint16_t 		original_seq_number;			 /** Original packet's seq. number **/

} SRPPEncrypted ;

extern int lastSequenceNo;

class SRPPMessage {

public:
	SRPPHeader srpp_header;						/** SRPP Header **/
	SRPPEncrypted 	encrypted_part;				/** This is the encrypted part of the packet **/
    uint32_t		authentication_tag;				/** Authentication Tag  **/

    SRPPMessage()
    	  {

    		  srpp_header.version = 2;
    		  srpp_header.p = 1;
    		  srpp_header.x = 1;
    		  srpp_header.cc = 10;
    		  srpp_header.m = 0;
    		  srpp_header.pt = 0;
    		  srpp_header.seq = ++lastSequenceNo;
    		  srpp_header.ts = 0;
    		  srpp_header.ssrc = 0;

    		  encrypted_part.pad_count = 0;
    		  encrypted_part.original_padding_bit = 0;
    		  encrypted_part.dummy_flag = 0;
    		  encrypted_part.original_seq_number = 0;

    		  encrypted_part.original_payload.resize(MAXPAYLOADSIZE);
    		  encrypted_part.srpp_padding.resize(MAXPAYLOADSIZE);

    		  encrypted_part.original_payload = "";
    		  encrypted_part.srpp_padding = "";

    		  authentication_tag = 0;

    	  }

   int srpp_to_network(char * buff)
	  {

//	   printf("%d\n", buff);

	    SRPPHeader* srpp_header1 = (SRPPHeader *) buff;
	    *srpp_header1 = srpp_header;

//	    printf("%d || %d\n", srpp_header1,sizeof(SRPPHeader));
		char* data = (char *) &buff[sizeof(SRPPHeader)];

//		printf("%d :: %d\n", data, encrypted_part.original_payload.length());

		//copy the payload
		strcpy(data, encrypted_part.original_payload.c_str());

		//copy the padding
		data = &data[encrypted_part.original_payload.length()];

//		printf("%d %d\n", data,encrypted_part.pad_count);

		strcpy(data, encrypted_part.srpp_padding.c_str());
		data = &data[encrypted_part.pad_count];

//		printf("%d %d\n", data, 8/sizeof(char));

		//copy other bits
		memcpy(data, (const char *)&encrypted_part.pad_count, 8);

		//copy the tag
		data += 8/sizeof(char);

//		printf("%d\n", data);

		uint32_t * thisnow = (uint32_t *)data ;
		*thisnow = authentication_tag;
		thisnow ++ ;

//		printf("%d\n", thisnow);


//		cout << "-----------------------------------\n";
		return 0;

	  }

   int network_to_srpp(char * buff, int bytes)
	  {

//	   printf("%d %d\n", buff, bytes);

	    SRPPHeader* srpp_header1 = (SRPPHeader *) buff;
	    srpp_header = *srpp_header1;

//		cout << "VERSIO : " << srpp_header.version << endl;

	    char* data = (char *) &buff[bytes];

//	    printf("%d\n", data);

	    //copy the tag
		data -= 4/sizeof(char);

//	    printf("%d\n", data);

		uint32_t * thisnow = (uint32_t *)data ;

//	    printf("%d\n", thisnow);
		authentication_tag = *thisnow;

//		cout << "Authentication : " << authentication_tag << endl;

		thisnow -=2;
//		printf("%d\n", thisnow);

		//copy other bits
		memcpy((char *)&encrypted_part.pad_count, (const char *)thisnow, 8);

//		cout << encrypted_part.pad_count << "::" << encrypted_part.dummy_flag << "::" << encrypted_part.original_seq_number << endl;

		//copy the padding bytes
		data = (char *)thisnow;
		data -= encrypted_part.pad_count;
//		printf("%d \n", data);

		encrypted_part.srpp_padding = string ((const char *)data,encrypted_part.pad_count);
//		cout << encrypted_part.srpp_padding << endl;

		char * thisnow1 = &buff[sizeof(srpp_header)];
		bytes = (data - thisnow1);

//		cout << "NO OF BYTES " << bytes << endl;


		//copy the payload
		encrypted_part.original_payload = string((const char *)thisnow1,bytes);

//		cout << "PAYLOAD: " << encrypted_part.original_payload  << endl;
		return 0;

	  }

	  ~SRPPMessage()
	  {

	  }

	  //Get the sequence number
	  int get_sequence_number()
	  {
		  return srpp_header.seq;
	  }

	  //Get the SSRC

};

#endif
