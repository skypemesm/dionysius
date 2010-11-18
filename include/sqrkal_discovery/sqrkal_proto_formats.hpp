/*
 * This header file contains all the protocols' headers of the SQRKal app
 * IP, UDP, SIP, SDP
 *
 * @author Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#ifndef SQRKAL_PROTO_FORMATS_HPP
#define SQRKAL_PROTO_FORMATS_HPP


#ifndef IP_HEADER_DEFD
#define IP_HEADER_DEFD
/**
 * IP Header
 */
typedef struct IP_header
  {
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t	 		version:4;
    uint8_t 		ihl:4;
#else
    uint8_t 		ihl:4;
    uint8_t 		version:4;
#endif

    uint8_t			tos;
    uint16_t		tot_len;
    uint16_t		id;
    uint16_t		frag_off;
    uint8_t			ttl;
    uint8_t			protocol;
    uint16_t		check;
    uint32_t		saddr;
    uint32_t		daddr;
  } IP_Header;
#endif


/**
 * TCP Header
 */
struct TCP_Header
  {
    uint16_t source;
    uint16_t destination;
    uint32_t seq;
    uint32_t ack_seq;

#if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t doff:4;
    uint16_t res1:4;
    uint16_t res2:2;
    uint16_t urg:1;
    uint16_t ack:1;
    uint16_t psh:1;
    uint16_t rst:1;
    uint16_t syn:1;
    uint16_t fin:1;
#  else
    uint16_t res1:4;
    uint16_t doff:4;
    uint16_t fin:1;
    uint16_t syn:1;
    uint16_t rst:1;
    uint16_t psh:1;
    uint16_t ack:1;
    uint16_t urg:1;
    uint16_t res2:2;
#  endif

    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};


/**
 * UDP Header
 *
 */

struct UDP_Header
{
  uint16_t source;
  uint16_t destination;
  uint16_t length;
  uint16_t checksum;
};




#endif
