/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONE_TYPES_H__
#define __ZFONE_TYPES_H__

#include <zrtp.h>

#if ZRTP_PLATFORM == ZP_LINUX
    #include <libipq.h>
#elif ZRTP_PLATFORM == ZP_DARWIN
    #include <sys/types.h>
    #include <sys/socket.h>
	#include <netinet/in.h>
#elif ZRTP_PLATFORM == ZP_WIN32_KERNEL
#else
    #error "can't detect platform type"
#endif

#define VOIP_SIP_PORT				5060
#define VOIP_SIP_PORT_GIZMO1		64064
#define VOIP_SIP_PORT_GIZMO2		6325


#if (ZRTP_PLATFORM == ZP_WIN32_KERNEL)
	// Defines for IPv4
	struct in_addr
	{
		uint32_t		s_addr;
	};
	
	struct sockaddr_in
	{
	    uint16_t		sin_port;
	    struct in_addr	sin_addr;	    
	};
	
	#define EF_ADDR_SIZE	6

	#pragma	pack(push, 1)
	struct eth_hdr
	{
	    unsigned char  dst[EF_ADDR_SIZE];
	    unsigned char  src[EF_ADDR_SIZE];
	    unsigned short type;
	};
	#pragma	pack(pop)
#endif


//! Type for direction decription (packets, connections etc.)
typedef enum
{
    ZFONE_IO_IN			= 0,	//!< input packet/connection
    ZFONE_IO_OUT		= 1,	//!< output packet/connection
    ZFONE_IO_UNKNOWN	= 2,	//!< unknown direction
	ZFONE_IO_COUNR		= 3
} zfone_direction_t;
extern char* zfone_direction_names[ZFONE_IO_COUNR];


#ifndef MTU_PACKET_SIZE
    // You can re-define your own max ip packet size
    #define MTU_PACKET_SIZE 	2000
#endif

#ifndef MAX_PACKET_SIZE
    //You can re-define your own max ip packet size
    #if ZRTP_PLATFORM == ZP_WIN32_KERNEL
	#define MAX_PACKET_SIZE 	MTU_PACKET_SIZE
    #else
	#define MAX_PACKET_SIZE 	6000
    #endif
#endif

#define MAX_SDP_RTP_CHANNELS 2

/*
 * \brief Media stream type
 * List of media types, specifed in RFC 2327 par 6.0: "audio", "video", "data" etc
 * We are defining just RTP based media types: audio and video.
 */
typedef enum 
{    
    ZFONE_SDP_MEDIA_TYPE_UNKN	= 0,	//!< unsupported media type
    ZFONE_SDP_MEDIA_TYPE_VIDEO	= 1,	//!< video stream "video"
    ZFONE_SDP_MEDIA_TYPE_AUDIO	= 2,	//!< audio stream "audio"
	ZFONE_SDP_MEDIA_TYPE_ZRTP	= 3,
	ZFONE_SDP_MEDIA_TYPE_COUNT	= 4
} zfone_media_type_t;
extern char* zfone_media_type_names[ZFONE_SDP_MEDIA_TYPE_COUNT];

typedef enum
{
	ZFONE_SDP_DIR_ATTR_SENDRECV = 0, //default
	ZFONE_SDP_DIR_ATTR_RECVONLY,
	ZFONE_SDP_DIR_ATTR_SENDONLY,
	ZFONE_SDP_DIR_ATTR_INACTIVE
} zfone_sdp_dir_attr_t;


//! RTP media stream definition
typedef struct zfone_hstream zfone_hstream_t;
typedef struct zfone_media_stream
{	
    zfone_media_type_t  type;			//!< stream data type {video, audio}
    		
    uint16_t			local_rtp;		//!< local-side RTP port
    uint16_t			local_rtcp;		//!< local-side RTCP port
    uint16_t			remote_rtp;		//!< remote-side RTP port
    uint16_t			remote_rtcp;	//!< remote-side RTCP port
	uint32_t			local_ip;
	uint32_t			remote_ip;		//!< remote-side IP address
	uint32_t			ssrc;

	zfone_sdp_dir_attr_t dir_attr;

    zfone_hstream_t 	*hstream;
} zfone_media_stream_t;


typedef struct zfone_ctx zfone_ctx_t;

//! Zfone media stream identifier size in bytes
#define ZFONE_STREAM_ID_SIZE 2
//! Zfone media stream uniqe identifier: hash(zfone_session_id, stream number in list)
typedef unsigned char zfone_stream_id_t[ZFONE_STREAM_ID_SIZE];

//! zfone media stream structure
typedef struct zfone_stream
{    
    zfone_stream_id_t	 id;			//!< media-stream identifier
	zfone_media_stream_t in_media;
	zfone_media_stream_t out_media;
	zfone_media_type_t	 type;	
	zrtp_stream_t		*zrtp_stream;
	zfone_ctx_t			*zfone_ctx;		//!< zfone session context
	uint8_t				alert_code;
	uint32_t			alert_ssrc;		
    uint64_t			last_inc_activity;
    uint64_t			last_out_activity;
    uint8_t				rx_state;
    uint8_t				tx_state;
	uint32_t			pbx_reg_ok;
	uint8_t				hear_ctx;
	mlist_t				_mlist;
} zfone_stream_t;

//! Zfone session identifier size in bytes
#define ZFONE_SESSION_ID_SIZE 4
//! Zfone session uniqe identifier: hash(first ZFONE_SESSION_ID_SIZE bites from sip_session_id)
typedef unsigned char zfone_session_id_t[ZFONE_SESSION_ID_SIZE];

//! zfone session context
struct zfone_ctx
{
    zfone_session_id_t	id;			//!< unicue session identifier
    zrtp_string128_t    name;		//!< name attached to other-side ZID peer
    zrtp_string128_t    remote_uri;	//!< name attached to other-side ZID peer
	int					luac;
	int					ruac;
	zrtp_session_t		zrtp_ctx_holder;
	zrtp_session_t*	zrtp_ctx;	//!< ZRTP crypto-context
    zfone_stream_t		streams[MAX_SDP_RTP_CHANNELS];	
#if ZRTP_PLATFORM == ZP_WIN32_KERNEL
    int32_t				_eth_is_valid;
    struct eth_hdr		_eth_hdr;
    void				*adapter;
#endif
	mlist_t	   			mlist;
};

/*!
 * \brief zfone VoIP packet structure
 */
typedef struct zfone_packet
{
    uint8_t			*packet;	//! pointer to the IP/Ether packet body
	uint16_t		proto;		//! transport protocol
    uint16_t		offset;		//! offset to the begining of the VoIP/UDP etc. packet in bytes 
    uint32_t		size;		//! packetr size
	uint8_t			buffer[MAX_PACKET_SIZE+512];
    
#if ZRTP_PLATFORM == ZP_LINUX
    ipq_packet_msg_t*	message;//! ipq message for linux netfiltering 		
#elif ZRTP_PLATFORM == ZP_DARWIN
    struct sockaddr_in 	from;
    socklen_t 		from_length;	
#elif ZRTP_PLATFORM == ZP_WIN32_KERNEL	
#else
	#error "can't detect platform type"
#endif

    uint16_t		external_proto;	//!< packet transport protocol (can be ether) \sa zrtp_voip_proto_t
	uint16_t		voip_proto;		//!< voip packet protocol \sa zrtp_voip_proto_t
    uint8_t			direction;		//!< packet direction
    uint32_t		local_ip;		//!< local IP (from IP header)
    uint16_t		local_port;		//!< local port (from UDP/TCP header)
    uint32_t		remote_ip;		//!< remote IP (from IP header)
    uint16_t		remote_port;	//!< remote port (from UDP/TCP header)
    
    zfone_stream_t*	stream;
    zfone_ctx_t*	ctx;
    void*			extra_data;		//! pointer to the additional packet data (for SIP)
} zfone_packet_t;


#define ZFONE_PORTS_SPACE_SIZE		65535

//! wraper for zfone RTP/RTCP detection ports space
typedef struct zfone_port_wrap
{
    uint8_t				proto;		//!< VoIP protocol
	uint8_t				transport;	//!< transport protocol for SIP    
} zfone_port_wrap_t;


// TODO: cleare fallow definitions
#define	ZFONE_MAX_FILENAME_SIZE	128
enum
{
	ZFONE_ERROR_NO_CONFIG = 0x500,
	ZFONE_ERROR_WRONG_CONFIG,
	ZFONE_NO_ZID,	
	ZFONE_ZRTP_INIT_ERROR,
	ZFONE_ERROR_SAS		
};

#define ZFONE_MAX_INTERFACES_COUNT	20

/*!
 * \brief network protocols enumeration used in libzrtp and libzrtphelper.
 * This enumeration of TCP/IP and VoIP protocols is used for maintenance 
 * independence from system header files and name space standardization.
 */
typedef enum
{
    voip_proto_ERROR	= -1,		//!< wrongly detected protocol
    voip_proto_UNKN		= 0,		//!< unsupported/unknown protocol
    voip_proto_IP		= 4,		//!< IP protocol
    voip_proto_UDP		= 17,		//!< UDP protocol
    voip_proto_TCP		= 6,		//!< TCP protocol
    voip_proto_SIP		= 20,		//!< SIP protocol
    voip_proto_RTP		= 31,		//!< RTP protocol
    voip_proto_RTCP		= 32,		//!< RTCP protocol
    voip_proto_ETHER	= 40,		//!< ethernet protocol
	voip_proto_PSIP		= 42,
	voip_proto_PRTP		= 43,
	voip_proto_PSRTP	= 44,
} zrtp_voip_proto_t;

//! returned values for zfone_siprocessor#process
typedef enum zfone_sip_action
{
    ZFONED_SIPS_NOTHING	 	= 0,	//!< nothing changed
    ZFONED_SIPS_CREATE		= 1,	//!< new SIP session was established
    ZFONED_SIPS_CLOSE		= 2,	//!< SIP session closed
    ZFONED_SIPS_UPDATE		= 3		//!< SIP session updated
} zfone_sip_action_t;

typedef struct zfone_sip_session zfone_sip_session_t;
void zfone_sip_action_handler(zfone_sip_session_t* sips, zfone_sip_action_t action);

#if ZRTP_PLATFORM == ZP_WIN32_KERNEL

typedef struct zfone_adapter_info
{
	int32_t				_eth_is_valid;
    struct eth_hdr		_eth_hdr;
    void				*adapter;
} zfone_adapter_info_t;

#endif


#endif //__ZFONE_TYPES_H__
