/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov <egm@soft.cn.ua>, <m.yegorov@gmail.com>
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */


 /* This is a copy for zfoned_commands.h just without functions declarations */

#ifndef __ZFONEG_COMMANDS_H__
#define __ZFONEG_COMMANDS_H__


#define ZRTP_CMD_MAGIC0	0x0
#define ZRTP_CMD_MAGIC1	0xffffffff

#define ZRTP_CMD_STR_LENGTH	128
/* max command length */
#define ZRTP_MAX_CMD_LENGTH	10240

#include <zrtp.h>

//#define ZFONE_STREAM_ID_SIZE 2
//! Zfone media stream uniqe identifier: hash(zfone_session_id, stream number in list)
//typedef unsigned char zfone_stream_id_t[ZFONE_STREAM_ID_SIZE];

typedef	unsigned short stream_id_t;

#define ZFONE_SESSION_ID_SIZE 4
//! Zfone session uniqe identifier: hash(first ZFONE_SESSION_ID_SIZE bites from sip_session_id)
typedef unsigned char zfone_session_id_t[ZFONE_SESSION_ID_SIZE];

typedef struct zrtp_streams_info
{
    uint16_t 		state;			//!< ZRTP protocol state (zrtp_state_t)
    uint16_t		type;			//!< // TODO: what is this?

    uint8_t			allowclear;		//! staysecure flag for current connection
    uint8_t			is_mitm;		//! man in the middle flag
	uint8_t			rx_state;
	uint8_t			tx_state;

    uint32_t		disclose_bit;	//! peer disclose flag
    zrtp_uchar4_t 	pkt;			//! publik keys exchange scheme for current connection

	int16_t			pbx_reg_ok;
	uint16_t		hear_ctx;

    uint32_t 		cache_ttl;		//! cache TTL for current ZRTP peer
	uint32_t		passive_peer;
} zrtp_streams_info_t;

#define ZFONE_MAX_CONNECTIONS_COUNT	4

/*
 * \brief Structure for Zfone/ZRTP connection state descryption
 */
typedef struct zrtp_conn_info
{
	zfone_session_id_t 	session_id;			//! ID of necessary Zfone/ZRTP session
    zrtp_string128_t	name;				//! Name/URI assoceated with this session
    zrtp_string16_t		zrtp_peer_client;	//! other-side's ZRTP realization name
    zrtp_string8_t		zrtp_peer_version;	//! other-side's ZRTP version    
    unsigned int		secure_since;		//! secure since value
    uint32_t			is_verified;		//! SAS verification flag (1 - is verified)		 
	zrtp_string16_t		sas1;
	zrtp_string16_t		sas2;	

    uint32_t			is_autosecure;	//! autosecure flag for current connection

    zrtp_uchar4_t		cipher;			//! chiper type for current connection
    zrtp_uchar4_t		atl;			//! SRTP auth tag length for current connection
    zrtp_uchar4_t 		sas_scheme;		//! SAS scheme for current connection
    zrtp_uchar4_t 		hash;			//! HASH calculation scheme for current connection

	uint32_t			matches;
	uint32_t			cached;
	zrtp_streams_info_t	streams[ZRTP_MAX_STREAMS_PER_SESSION];
} zrtp_conn_info_t;

typedef struct zrtp_cmd_update_streams
{
    uint32_t 			count;
    zrtp_conn_info_t	list[ZFONE_MAX_CONNECTIONS_COUNT];
} zrtp_cmd_update_streams_t;

/*!
    \brief base command structure
*/
typedef struct zrtp_cmd
{
    uint32_t		imagic[2];

    unsigned int	code;		/*!< command code */
    zfone_session_id_t 	session_id;	/*!< local ip for addressing */
    stream_id_t		stream_id;	/*!< local RTP port for addressing */

    short			pad;

    unsigned int	length;	/*!< extension length */
    char		data[0];/*!< pointer to special data (usage: a = (struct a_t*) (&cmd->data) ) */
}zrtp_cmd_t;


//void zrtp_init_cmd(zrtp_cmd_t *c, unsigned int ip, unsigned short port, unsigned int code);

/* SET_VERIFIDE command special data */
typedef struct zrtp_cmd_set_verified
{
    unsigned int 	is_verified;			/* verified flag */
}zrtp_cmd_set_verified_t;

/* SET_NAME command special data */
typedef struct zrtp_cmd_set_name
{
    unsigned int 	name_length;			/* name or url length */
    char 		name[ZRTP_CMD_STR_LENGTH];	/* name or SIP url */
}zrtp_cmd_set_name_t;

/* SEND_VERSION command special data */
#define ZFONEG_VERSION_SIZE 8
typedef struct zrtp_cmd_send_version
{
    unsigned int	is_manually;			/* force check flag (in spite of check for updates period) */
    unsigned int	is_expired;			/* expired flag */
    unsigned int 	curr_version;			/* current software version */
    unsigned int 	new_version;			/* version available at the server */
    unsigned int 	curr_build;			/* current software build number */
    unsigned int 	new_build;			/* build number available at the server */
        
    unsigned int 	url_length;			/* url length */
    char 		url[ZRTP_CMD_STR_LENGTH];	/* url for updates */
    
    char		zrtp_version[ZFONEG_VERSION_SIZE];
}zrtp_cmd_send_version_t;

enum
{
	ZFONE_ERROR_NO_CONFIG = 0x500,
	ZFONE_ERROR_WRONG_CONFIG,
	ZFONE_NO_ZID,	
	ZFONE_ZRTP_INIT_ERROR,
	ZFONE_ERROR_SAS		
};

/* ERROR command special data */
typedef struct zrtp_cmd_error
{
    uint32_t 	error_code;				/* zrtp error code */
}zrtp_cmd_error_t;

/*  command special data */
typedef struct zrtp_cmd_zrtp_packet
{
    unsigned int 	direction;			/*0 - output, 1- input */
    unsigned int 	packet_type;			/* zrtp packet type */
} zrtp_cmd_zrtp_packet_t;

/* alert sound playing command*/
typedef struct zrtp_cmd_alert
{
    unsigned int        alert_type;                     /* zrtp_alert_t alert sound type */
}zrtp_cmd_alert_t;
    
typedef struct zrtp_cmd_hear_ctx
{
	unsigned int	disable_zrtp;
} zrtp_cmd_hear_ctx_t;

/* callback type for async cmd notifying */
typedef void voip_ctrl_callback(zrtp_cmd_t *cmd);

#define	ZFONE_MAX_INTERFACES_COUNT	20

typedef struct zrtp_cmd_set_ips
{
    uint32_t	ip_count;
    uint32_t	ips[ZFONE_MAX_INTERFACES_COUNT];
    uint8_t	flags[ZFONE_MAX_INTERFACES_COUNT];
} zrtp_cmd_set_ips_t;

/*!
    Control command types. 
*/
/*Do not change values*/
enum control_commands
{
     UPDATE_STREAMS	= 0
    ,GO_SECURE		= 1
    ,GO_CLEAR		= 2
    ,REFRESH		= 3
    ,SET_VERIFIED	= 4
    ,SET_NAME		= 5
    ,SEND_VERSION	= 6
    ,GET_VERSION	= 7
    ,CRASHED		= 8
    ,STOPPED		= 9
    ,STARTED		= 10
    ,SET_PREF		= 11
    ,ERROR			= 12
    ,ZRTP_PACKET	= 13
    ,PLAY_ALERT		= 14
    ,CLEAN_LOGS		= 15
    ,SET_DEFAULTS	= 16
    ,RESET_DAEMON	= 17
    ,SET_IPS		= 18
    ,GET_PREF		= 19
    ,SET_NOTIFY		= 20
    ,RESET_ZID		= 21
	,GET_CACHE_INFO = 22
	,SET_CACHE_INFO = 23
	,REMEMBER_CALL  = 24
	,HEAR_CTX		= 25
	,ZRTP_NR_CMDS
};

/* parametsrs structure from zfoned_cfg.h */

typedef enum 
{    
    ZFONE_SDP_MEDIA_TYPE_UNKN	= 0,	//!< unsupported media type
    ZFONE_SDP_MEDIA_TYPE_VIDEO	= 1,	//!< video stream "video"
    ZFONE_SDP_MEDIA_TYPE_AUDIO	= 2,	//!< audio stream "audio"
} zfone_media_type_t;

typedef enum zfoned_packets
{
    ZFONED_NO_ACTIV	    	= 50,
    ZFONED_NO_ALERTED_ACTIV	= 51,
    ZFONED_DROP_ACTIV		= 52,
    ZFONED_PASS_ACTIV		= 53
} zfoned_packets_t;

//! Type for direction decription (packets, connections etc.)
typedef enum
{
    ZFONE_IO_IN			= 0,	//!< input packet/connection
    ZFONE_IO_OUT		= 1,	//!< output packet/connection
    ZFONE_IO_UNKNOWN	= 2	//!< unknown direction
} zfone_direction_t;

#define ZRTP_MAX_CACHE_INFO_RECORD	50
/*
 * Cache info record is used to store brief information about cache entry
 */
typedef struct zrtp_cache_info_record
{
	zrtp_cache_id_t		id;								/* cache element identifier */
	char				name[ZFONE_CACHE_NAME_LENGTH];  /* name of the user associated with this cache entry */
	uint8_t				verified;						/* is verified */		
	uint32_t			time_created;					/* time when cache entry was created*/ 
	uint32_t			time_accessed;					/* the last access time */ 
	uint8_t				oper;							/* operation which performed over entry (modification or deleting or smth else)*/
	uint8_t				is_mitm;
	uint8_t				is_expired;
} zrtp_cache_info_record_t;

/*
 *	Structure is used for storing array of cache info records, its used in 
 *  zrtp_get_cache_info and zrtp_set_cache_info
*/
typedef struct zrtp_cache_info
{
    uint32_t				 count;			/* count of record in the array */
    zrtp_cache_info_record_t list[ZRTP_MAX_CACHE_INFO_RECORD]; /* array of cache info records */
} zrtp_cache_info_t;


zrtp_cmd_set_ips_t			ips_holder;
extern zrtp_cache_info_t	global_cache_cmd;

#define	IP_FLAG_AUTO_VALUE	0
#define	IP_FLAG_MANUAL_VALUE	1

#endif //__ZFONEG_COMMANDS_H__

