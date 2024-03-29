/**
 * This class represents the structure of a SRPP message
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */
#ifndef SRPP_MESSAGE_HPP
#define SRPP_MESSAGE_HPP

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdint.h>
#include <vector>
#include <sys/time.h>
#include <netinet/in.h>

#ifndef MAXPAYLOADSIZE
	#define MAXPAYLOADSIZE     1464 //16384           // 16384 bytes
#endif

using namespace std;

typedef struct SRPPHeader {

#if __BYTE_ORDER == __BIG_ENDIAN

	  uint16_t		version:2;	/* protocol version */
	  uint16_t		p:1;		/* padding flag */
	  uint16_t		x:1;		/* header extension flag */
	  uint16_t		cc:4;		/* CSRC count */
	  uint16_t		m:1;		/* marker bit */
	  uint16_t		pt:7;		/* payload type */

#else
	  uint16_t		cc:4;		/** CSRC count             */
	  uint16_t		x:1;		/** padding flag           */
	  uint16_t		p:1;		/** header extension flag  */
	  uint16_t		version:2;	/** protocol version       */
	  uint16_t		pt:7;		/** payload type           */
	  uint16_t		m:1;		/** marker bit             */

#endif

	  uint16_t		seq;		/** srpp sequence number        */

	  uint32_t		ts;			/** srpp timestamp              */
	  uint32_t		ssrc;		/** synchronization source */

	  uint32_t		csrc[15];		/** contributing sources  */


	  uint16_t		defined_by_profile;   /** SET THE EXTENSION HEADER **/
	  uint16_t		extension_header;
	  uint32_t		srpp_signalling;		/** rtp extension flag for srpp */

} SRPPHeader ;

typedef struct SRPPEncrypted {
	vector<char> 		original_payload;  					 /** original rtp/srtp payload **/
    vector<char> 		srpp_padding;       				 /** padding (can maximum be full packet)**/
	uint32_t			pad_count;    					 /** srpp pad count **/

#if __BYTE_ORDER == __BIG_ENDIAN
	uint16_t			original_padding_bit:1;			 /** original packet's padding bit  **/
	uint16_t			dummy_flag:15;					 /** Dummy flag for srpp packet **/
#else
	uint16_t			dummy_flag:15;					 /** Dummy flag for srpp packet **/
	uint16_t			original_padding_bit:1;			 /** original packet's padding bit  **/
#endif

	uint16_t 			original_seq_number;			 /** Original packet's seq. number **/

} SRPPEncrypted ;

extern int lastSequenceNo;
extern uint32_t srppssrc;

class SRPPMessage {

public:
	SRPPHeader srpp_header;						/** SRPP Header **/
	SRPPEncrypted 	encrypted_part;				/** This is the encrypted part of the packet **/
    uint32_t		authentication_tag;				/** Authentication Tag  **/

    SRPPMessage()
    	  {

    		  srpp_header.version = 2;
    		  srpp_header.p = 0;
    		  srpp_header.x = 1;
    		  srpp_header.cc = htons(15);
    		  srpp_header.m = 0;
    		  srpp_header.pt = 121;
    		  srpp_header.seq = ++lastSequenceNo;
    		  srpp_header.defined_by_profile = 9999;  /** SRPP PROFILE NOT DEFINED YET :P **/
    		  srpp_header.extension_header = 1;

    		  timeval a;
    		  gettimeofday(&a, NULL);
    		  srpp_header.ts = 1000000*a.tv_sec + a.tv_usec;
    		  srpp_header.ssrc = srppssrc;

    		  encrypted_part.pad_count = 0;
    		  encrypted_part.original_padding_bit = 0;
    		  encrypted_part.dummy_flag = 0;
    		  encrypted_part.original_seq_number = 0;

    		  for (int i = 0;i<15; i++)
    			  srpp_header.csrc[i] = 0;

    		  authentication_tag = 0;

    	  }

   int srpp_to_network(char * buff, int key)
	  {

	    SRPPHeader* srpp_header1 = (SRPPHeader *) buff;

	    *srpp_header1 = srpp_header;

	    //----------FORMAT NETWORK BYTE ORDER TO INTS-------------
	    srpp_header1->seq = htons(srpp_header.seq);
	    srpp_header1->ts = htonl(srpp_header.ts);
	    srpp_header1->ssrc = htonl(srpp_header.ssrc);

	    //cout << "CC:" << ntohs(srpp_header.cc) <<":" << srpp_header.cc << endl;
	    int i = 0;
	    for (; i< ntohs(srpp_header.cc); i++)
	    	srpp_header1->csrc[i] = htonl(srpp_header.csrc[i]);
	    for (; i< 15; i++){
					srand(srpp_header.ts+i*50);
	    	    	srpp_header1->csrc[i] = rand()*(2^32)+1;
	    }


	    srpp_header1->defined_by_profile = htons(srpp_header.defined_by_profile);
	    srpp_header1->extension_header = htons(srpp_header.extension_header);

	    srpp_header1->srpp_signalling = htonl(srpp_header.srpp_signalling);
	    // -------------------------------------------------------


		char* data = (char *) &buff[sizeof(SRPPHeader)];
		//printf("\n1:%u %u %d\n",buff,data,data-buff);

		//copy the payload
		string str (encrypted_part.original_payload.begin(),encrypted_part.original_payload.end());
		strcpy(data, str.c_str());

		//copy the padding
		data = &data[str.length()];
		//printf("\n2:%u %u %d\n",buff,data,data-buff);

		if (key > 0 && srpp_header.srpp_signalling == 0)
		{
			encrypted_part.pad_count ^= key;
		}

	if ( encrypted_part.pad_count > 0 && srpp_header.srpp_signalling == 0)
	 {
		string str_pad (encrypted_part.srpp_padding.begin(),encrypted_part.srpp_padding.end());
		strcpy(data, str_pad.c_str());
		data = &data[encrypted_part.pad_count];
	 }
	//cout << "PAD:" << encrypted_part.pad_count << "Actual:" << encrypted_part.srpp_padding.size() << "\n";
	//printf("\n3:%u %u %d\n",buff,data,data-buff);

		//copy other bits
		if (key > 0 && srpp_header.srpp_signalling == 0)
		{
			encrypted_part.pad_count ^= key;
		}

		memcpy(data, (const char *)&encrypted_part.pad_count, 8);
		//printf("\n4:%u %u %d\n",buff,data,data-buff);

		//copy the tag
		data += 8/sizeof(char);

		//cout << "Authentication Tag :" << authentication_tag << "\n";
		//printf("\n4:%u %u %d\n",buff,data,data-buff);

		uint32_t * thisnow = (uint32_t *)data ;
		//printf("\n5:%u %u %d\n",buff,thisnow,((char*)thisnow)-buff);
		*thisnow = htonl(authentication_tag);
		thisnow ++ ;

		//printf("\n6:%u %u %d\n",buff,thisnow,((char*)thisnow)-buff);

		return 0;

	  }

   /**
    * Converts the network packet to corresponding SRPP packet and
    * decrypt the pad_count if its not a signaling message
    */
   int network_to_srpp(char * buff, int bytes, int key)
	  {

	   /*for (int i = 0; i<bytes; i++)
		   printf("%x ",buff[i]);*/

	   SRPPHeader* srpp_header1 = (SRPPHeader *) buff;
	    srpp_header = *srpp_header1;

	    //----------FORMAT FROM NETWORK BYTE ORDER TO INTS-------------
	    srpp_header.seq = ntohs(srpp_header1->seq);
	    srpp_header.ts = ntohl(srpp_header1->ts);
	    srpp_header.ssrc = ntohl(srpp_header1->ssrc);

	    for (int i = 0; i< srpp_header.cc; i++)
	    	srpp_header.csrc[i] = ntohl(srpp_header1->csrc[i]);

	    srpp_header.defined_by_profile = ntohs(srpp_header1->defined_by_profile);
	    srpp_header.extension_header = ntohs(srpp_header1->extension_header);

	    srpp_header.srpp_signalling = ntohl(srpp_header1->srpp_signalling);
	    // -------------------------------------------------------

	    char* data = (char *) &buff[bytes];
	    //printf ("\n%u %u %d\n",buff,data,data-buff);

	    //copy the tag
		data -= 4/sizeof(char);
		uint32_t * thisnow = (uint32_t *)data ;

		authentication_tag = ntohl(*thisnow);

		//printf("Authentication Tag: %d\n",authentication_tag);

		thisnow -= 2;

		//copy other bits
		memcpy((char *)&encrypted_part.pad_count, (const char *)thisnow, 8);

		//printf("Pad Count: %d\n",encrypted_part.pad_count);

		if (srpp_header.srpp_signalling == 0 && key > 0){
			encrypted_part.pad_count ^= key;
		}

		//printf("Pad Count: %d\n",encrypted_part.pad_count);
		if(encrypted_part.pad_count < 0 || encrypted_part.pad_count > 1500)
			{cout << "NOT A SRPP Packet\n\n"; return -1;}


     if (encrypted_part.pad_count > 0 && srpp_header.srpp_signalling == 0)
     {
		//copy the padding bytes
		data = (char *)thisnow;
		data -= encrypted_part.pad_count;

		//string str_pad = string ((const char *)data,encrypted_part.pad_count);
		//encrypted_part.srpp_padding = vector<char>(str_pad.begin(),str_pad.end());
		encrypted_part.pad_count ^= key;

     }
     else
     {
    	 encrypted_part.pad_count = 0;
     }

		char * thisnow1 = &buff[sizeof(srpp_header)];

		bytes = (data - thisnow1);


		//copy the payload
		string str = string((const char *)thisnow1,bytes);
		encrypted_part.original_payload = vector<char>(str.begin(),str.end());

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

	  //ENCRYPT THE MESSAGE USING A KEY
	  int encrypt( unsigned int key)
	  {
		  //encrypt the payload
		  for (int i = 0; i< (int)encrypted_part.original_payload.size(); i++)
			  encrypted_part.original_payload[i] ^= key;

		  //encrypt the padding
		  for (int i = 0; i< (int)encrypted_part.srpp_padding.size(); i++)
			  encrypted_part.srpp_padding[i] ^= key;

		  //encrypt the other flags
		  encrypted_part.original_seq_number ^= key;
		  encrypted_part.dummy_flag ^= key;
		  encrypted_part.pad_count ^= key;
		  encrypted_part.original_padding_bit ^= key;
			return 0;

	  }

	  //DECRYPT THE MESSAGE USING A KEY
	  int decrypt( unsigned int key)
	  {
		  //decrypt the payload
		  for (int i = 0; i< (int)encrypted_part.original_payload.size(); i++)
			  encrypted_part.original_payload[i] ^= key;


		  //decrypt the padding
		  for (int i = 0; i< (int)encrypted_part.srpp_padding.size(); i++)
			  encrypted_part.srpp_padding[i] ^= key;


		  //decrypt the other flags
		  encrypted_part.original_seq_number ^= key;
		  encrypted_part.dummy_flag ^= key;
		  encrypted_part.pad_count ^= key;
		  encrypted_part.original_padding_bit ^= key;
			return 0;

	  }

	  // Print the details of this message
	  int print()
	  {
		  //Print the header
		 cout << "---------------------------------------------------------------------------------\n";
		 cout << "SRPP Header:" << endl;
		 cout << "Version: " << srpp_header.version << endl ;
		 cout << "Padding bit: " <<  srpp_header.p << endl;
		 cout << "Extension bit: " <<  srpp_header.x << endl;
		 cout << "CC bit: " <<  srpp_header.cc << endl;
		 cout << "Marking bit: " <<  srpp_header.m << endl;
		 cout << "Sequence Number: " <<  srpp_header.seq << endl;
		 cout << "TimeStamp: " <<  srpp_header.ts << endl;
		 cout << "SSRC: " <<  srpp_header.ssrc << endl;

		  //Print the payload
		 string str (encrypted_part.original_payload.begin(), encrypted_part.original_payload.end());
		 cout << "Original Payload: " <<  str << endl;

		  // Print the padding bits
		 string str_pad (encrypted_part.srpp_padding.begin(), encrypted_part.srpp_padding.end());
		 cout << "SRPP Padding bits: " <<  str_pad << endl;

		 cout << "Defined by Profile: " <<  srpp_header.defined_by_profile << endl;
		 cout << "Extension: " <<  srpp_header.extension_header << endl;
		 cout << "Signaling:" << srpp_header.srpp_signalling << endl;

		  //Print the flags

		 cout << "Pad count: " <<  encrypted_part.pad_count << endl;
		 cout << "Original Sequence Number: " <<  encrypted_part.original_seq_number << endl;
		 cout << "Original Padding bit: " <<  encrypted_part.original_padding_bit << endl;
		 cout << "Dummy Flag: " <<  encrypted_part.dummy_flag << endl;

		  //Print the authentication tag
		   cout << "Authentication Tag: " <<  authentication_tag << endl;
			 cout << "---------------------------------------------------------------------------------\n\n";

			 return 0;
	  }

};

#endif
