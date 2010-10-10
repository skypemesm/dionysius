/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONE_SIPROCESSOR_H__
#define __ZFONE_SIPROCESSOR_H__

#include <zrtp.h>

#include "zfone_types.h"

//------------------------------------------------------------------------------
//     SDP related definitions
//------------------------------------------------------------------------------

//! type SIP message body
typedef enum 
{
    ZFONE_SIP_BODY_UNKNOWN	= 0,//!< unsupported protocol
    ZFONE_SIP_BODY_SDP		= 1	//!< SDP protocol
} zfone_sip_body_type_t;

//! Structure for IP addreses and ports
typedef struct 
{
    uint32_t	base;		//!< unicast IP addr/UDP port in host mode, or multicaset base address  
    uint32_t	range;		//!< addresses range for multicast, or 0 for unicast
} zfone_sdp_addr_t;

/*!
 * \brief structure of Media strem
 * Used for defining SDP media strem sections. Now contains just a set of main
 * options which allow to extract RTP stream from UDP trafic. Some other fields 
 * can be added in the future.
 */
typedef struct
{
    zfone_media_type_t		type;		//!< stream type (audio, video, data).
    
    zfone_sdp_addr_t    	contact;	//!< Contact field (if no "c=" at Session section)	
    zfone_sdp_addr_t		rtp_ports;	//!< RTP ports set
    zfone_sdp_addr_t		rctp_ports;	//!< sometimes RTCP ports can be specifed at SDP

	uint32_t				extra;
	uint32_t				ice_ip;
	uint32_t				ice_port;
	zfone_sdp_dir_attr_t	dir_attr;
} zfone_sdp_stream_t;


#define ZFONE_SDP_CODECS_MAX	10
typedef struct zfone_sdp_codecs
{
	uint32_t	type[ZFONE_SDP_CODECS_MAX];
	uint32_t	code[ZFONE_SDP_CODECS_MAX];
	uint8_t		count;
} zfone_sdp_codecs_t;


//! SDP message structure
typedef struct zfone_sdp_message
{
    uint16_t			version;	//!< SDP version
    zfone_sdp_addr_t	contact;	//!< Connection data. (Internet type and IP4 supported only)
    zfone_sdp_stream_t  streams[MAX_SDP_RTP_CHANNELS]; //!< media streams
	uint16_t			active_streams; //!< number of media streams ftreams
	uint8_t				is_zrtp_present;
	uint32_t			zrtp_offset;
	zfone_sdp_codecs_t	codecs;
	uint32_t			extra;
} zfone_sdp_message_t;


//------------------------------------------------------------------------------
//     SIP related definitions
//------------------------------------------------------------------------------


#define SIP2_HDR "SIP/2.0"
#define SIP2_HDR_LEN (strlen (SIP2_HDR))

// buffer size for the cseq_method name
#define MAX_CSEQ_METHOD_SIZE	16

// define buffer size for CallID name
#define MAX_CALL_ID_SIZE 	128

// define buffer size for SIP URL 
#define MAX_SIP_URI_SIZE 	128

// sip session ID length in bytes
#define SIP_SESSION_ID_SIZE 	16


#define MIN_SIP_MESSAGE_SIZE	30
#define MIN_SIP_START_LINE_SIZE 11

/*!
 * \brief  SIP methods codes assotiated with \ref zfone_sip_methods
 * \warning! Don't fprget to change \ref zfone_sip_methods at zfone_siprocessor.c
 *           In case of this enumeration expansion
 */
typedef enum
{
    ZFONE_SIP_METHOD_INVALID	= 0,
    ZFONE_SIP_METHOD_ACK 		= 1,
    ZFONE_SIP_METHOD_BYE 		= 2,
    ZFONE_SIP_METHOD_CANCEL 	= 3,
    ZFONE_SIP_METHOD_INVITE 	= 4
} zfone_sip_methods_t;

/*!
 * \brief  enum for SIP headers processed by Zfone
 * \warning! Don't fprget to change \ref zfone_sip_headers at zfone_siprocessor.c
 *           In case of this enumeration expansion
 */
typedef enum
{
    ZFONE_SIP_HEADER_UNKNOWN		= 0,
    ZFONE_SIP_HEADER_CALLID			= 1,
    ZFONE_SIP_HEADER_CSEQ			= 2,
    ZFONE_SIP_HEADER_CONTENT_LENGTH	= 3,
    ZFONE_SIP_HEADER_CONTENT_TYPE	= 4,
    ZFONE_SIP_HEADER_FROM			= 5,
    ZFONE_SIP_HEADER_TO				= 6,
    ZFONE_SIP_HEADER_AGENT			= 7
} zfone_sip_headers_t;


//! SIP "CSeq" structure
typedef struct
{
    uint32_t	seq;				//!< single decimal sequence number
    uint16_t	name;				//!< request method
} zfone_sip_seq_t;


//! Structure for SIP contacts : FROM, TO, Contact etc..
typedef struct
{
    char	uri[MAX_SIP_URI_SIZE];	//! SIP URI
    char	name[MAX_SIP_URI_SIZE];	//! Display name
} zfone_sip_contact;

/*
 * \brief SIP packet type
 * It's either a SIP Request, a SIP Response, or another type of packet.
 */
typedef enum
{
    ZFONE_SIP_UNKNOWN 	= 0,	//!< Not SIP 
    ZFONE_SIP_ERROR 	= 1, 	//!< Some ERROR in Status_lien or Reqest-Line parsing
    ZFONE_SIP_RESPONSE 	= 2,	//!< SIP Response packet
    ZFONE_SIP_REQUEST 	= 3  	//!< SIP Reqest packet
} zfone_sip_type_t;

//! SIP Start line. One structure for reqests and responses is used.
typedef struct zfone_start_line
{
    uint16_t	type;			//! message type: request/response
    uint16_t	version;		//! version format:  major*10+minor (ex. 2.0: 2*10+0=21)
    uint32_t	method;			//! request "Method" or response "Reason-Phrase" \sa zfone_sip_method_t
    uint32_t	status_code;	//! Response "Status-Code"
	uint8_t		is_voicemail;
} zfone_start_line_t;


/*!
 * \brief VoIP clients requere special detection schemes
 * This enum lists several VoIP clients which requere some tricks in RTP
 * detection. We take this value from SIP and than use it during analyzing
 */
typedef enum zfone_voip_agent
{
    ZFONE_AGENT_UNKN	= 0,
    ZFONE_AGENT_DEFAULT	= 1,
    ZFONE_AGENT_GIZMO	= 2,
    ZFONE_AGENT_TIVI	= 3,
    ZFONE_AGENT_ICHAT	= 4
} zfone_voip_agent_t;

#define ZFONE_AGENT_GIZMO_NAME	"Gizmo"
#define ZFONE_AGENT_TIVI_NAME	"tivi"
#define ZFONE_AGENT_ICHAT_NAME  "Viceroy"

//! SIP message
typedef struct
{
    zfone_start_line_t		start_line;		//!< SIP initial line
    zfone_sip_contact		from;			//!< "From" fiels parse results
    zfone_sip_contact		to;				//!< "To" fields parse results
    char call_id[MAX_CALL_ID_SIZE];			//!< Call-Id (rfc 3665)
    zfone_sip_seq_t			cseq;			//!< CSeq (rfc 3665)
    zfone_sip_body_type_t	ctype;			//!< Content-Type 
    uint32_t 				clength; 		//!< Content-Length in bytes
    zfone_voip_agent_t		user_agent;		//!< User agent type extracted form SIP "User-Agent" header
    
    zfone_sdp_message_t 	body;			//!< SIP body: SDP message in our case

    uint8_t session_id[SIP_SESSION_ID_SIZE];//!< session ID = hash(Call-Id, local URI, remote URI)    
    uint32_t				parsed_flags;	//!< parsed flags. (for internal use only)
	
	uint32_t				content_length_start;

    zfone_packet_t*			packet;
} zfone_sip_message_t;


//------------------------------------------------------------------------------
//     SIP state-machine definitions
//------------------------------------------------------------------------------


//! define SIP state-machine states
typedef enum zfone_sip_states
{
    ZFONE_SIP_STATE_DOWN	= 0,	//!< when have no dialogs
    ZFONE_SIP_STATE_EARLY	= 1,	//!< INVITE was sent/received but not confirmed yet
    ZFONE_SIP_STATE_ESTABL	= 2,	//!< INVITE was confirmed by OK (received/sent depends on direction)
    ZFONE_SIP_STATE_CONFIRM	= 3,	//!< ACK was sent/received.
	ZFONE_SIP_STATE_COUNT	= 4
} zfone_sip_states_t;

#define SIP_TIMEOUT_PROOT_T1 	60	//!< SIP protocol timeout to handle broken connections (in seconds)
#define SIP_TIMEOUT_CLIENT_T2	300	//!< timeout to close unconfirmed SIP sessions (without ACK)									
#define SIP_TIMEOUT_UNUSED_T3	600	//!< timeout to handle unexpected VoIP clients behaveour (in seconds)
#define SIP_TIMESTAMP_ESTABL	0

typedef uint8_t zfone_sip_id_t[SIP_SESSION_ID_SIZE];

//! SIP session definition
struct zfone_sip_session
{
    zfone_sip_id_t		id;			//!< session ID = hash(Call-Id, local URI, remote URI)
    zfone_sip_states_t	state;		//!< current session state
    zfone_direction_t	direction;	//!< session direction {input - INVITE was received, output - sent}
    uint32_t			last_seq;	//!< sequence number of last valid incoming request
    
    zfone_sip_contact	local_name;	//!< local-side name 
    zfone_sip_contact	remote_name;//!< remite-side name
    zfone_voip_agent_t	local_agent;//!< local user agent type
    zfone_voip_agent_t	remote_agent;//!< local user agent type
        	
    zfone_media_stream_t  conf_streams[MAX_SDP_RTP_CHANNELS];
	uint32_t			conf_count;
    zfone_media_stream_t  new_streams[MAX_SDP_RTP_CHANNELS];
	uint32_t			new_count;    
    uint32_t			established;
	uint64_t			createdat;	//! creation time in miliseconds, touched by the first SIP INVITE
    uint32_t			timestamp;

	void*				*psips;		//! back-pointer to the zfone heurist psips
    uint32_t			is_active;	//!< activity flag. We are marking unused structures instead of destroying    
	uint32_t			out_extra;
	uint32_t			in_extra;

	uint8_t				is_ice_enabled;

	mlist_t				mlist;
};


//------------------------------------------------------------------------------
//     SIP processor engine
//------------------------------------------------------------------------------

int zfone_sip_parse(zfone_packet_t* packet);
int zfone_sip_insert_tag( zrtp_zid_t *zid, 
						  char *hash_buff, 
						  uint32_t hash_buff_length,
						  zfone_sip_message_t* sipmsg,
						  zfone_packet_t* packet );

struct zfone_siprocessor
{	
	uint8_t 	inited; 	//!< Initialization flag.
	mlist_t 	sip_head;	//!< Sip sessions list header
	zrtp_zid_t	zid;		//!< local side zid (for ZRTP SDP tag)

    int   (*create)(struct zfone_siprocessor* siproc, zrtp_zid_t *zid);
    void  (*destroy)(struct zfone_siprocessor* siproc);	
    void  (*refresh)(struct zfone_siprocessor* siproc);    
    int   (*process)(struct zfone_siprocessor* siproc, zfone_packet_t* packet);	
};


extern int zfone_siproc_ctor(struct zfone_siprocessor* siproc);
extern struct zfone_siprocessor siproc;

#define ZFONE_PRINT_SIPID(id, buff) \
	hex2str((const char*)id, SIP_SESSION_ID_SIZE, buff, sizeof(buff))

#endif //__ZFONE_SIPROCESSOR_H__

