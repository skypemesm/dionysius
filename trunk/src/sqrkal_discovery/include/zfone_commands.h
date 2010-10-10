/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 * Nikolay Popok <chaser@soft-industry.com>
 */

#ifndef __ZFONED_CMD_COMMON_H__
#define __ZFONED_CMD_COMMON_H__

#include <zrtp.h>

#include "zfone_types.h"
#include "zfone_updater.h"

//! These values separate command packets in TCP stream (for Unix platforms only)
#define ZRTP_CMD_MAGIC0	0x0	
#define ZRTP_CMD_MAGIC1	0xffffffff

//! Max string length in command packet
#define ZRTP_CMD_STR_LENGTH	128 
//! Max command length of command body
#define ZRTP_MAX_CMD_LENGTH	2048


//! base command structure
typedef struct zrtp_cmd
{
    uint32_t			imagic[2];		//!< Separator for TCP streams
    uint32_t			code;			//!< Command code
    zfone_session_id_t 	session_id;		//!< Zfone/ZRTP session ID 
    uint16_t			stream_type;	//! Zfone media stream type (Video, Audio)
    short				pad;
    unsigned int		length;			//!< extension length
} zrtp_cmd_t;

#define ZFONE_CMD_EXT(x)	(((char*)x) + sizeof(zrtp_cmd_t))

//! SET_VERIFIDE command special data. (for input commands only)
typedef struct zrtp_cmd_set_verified
{
    unsigned int 	is_verified;		//!< verified flag (1 - enabled, 0 - disabled)
}zrtp_cmd_set_verified_t;


//! SET_NAME command special data (for input commands only)
typedef struct zrtp_cmd_set_name
{
    unsigned int 	name_length;				//!< Name length
    char 			name[ZRTP_CMD_STR_LENGTH];	//!< NAME associated with session 
}zrtp_cmd_set_name_t;


//! SEND_VERSION command special data (for input commands only) 
typedef struct zrtp_cmd_send_version
{
    unsigned int	is_manually;	//!< force check flag (in spite of check for updates period)
    unsigned int	is_expired;		//!< expired flag (1 - software expired )
    unsigned int 	curr_version;	//!< current software version
    unsigned int 	new_version;	//!< version available at the server
    unsigned int 	curr_build;		//!< current software build number
    unsigned int 	new_build;		//!< build number available at the server			
    unsigned int 	url_length;		//!< URL for updates (specifed on server)
    char 			url[ZRTP_CMD_STR_LENGTH];
    char			zrtp_version[8];
} zrtp_cmd_send_version_t;


//! ERROR command special data (for input commands only) 
typedef struct zrtp_cmd_error
{
    uint32_t 		error_code;	//! ZRTP error code
}zrtp_cmd_error_t;


//!  Enumeration for RTP/SRTP activiti indication 
typedef enum zfoned_packets
{
    ZFONED_NO_ACTIV			= 50,	//!< No RTP activity
    ZFONED_ALERTED_NO_ACTIV = 51,   //!< No RTP activity
    ZFONED_DROP_ACTIV		= 52,	//!< Row, unprotected RTP detected
    ZFONED_PASS_ACTIV		= 53	//!< Secure RTP detected
} zfoned_packets_t;


//! command special data
typedef struct zrtp_cmd_zrtp_packet
{
    unsigned int 	direction;		//!< 0 - output, 1- input
    unsigned int 	packet_type;	//!< zrtp packet type
}zrtp_cmd_zrtp_packet_t;

//! alert sound playing command
typedef struct zrtp_cmd_alert
{
    unsigned int	alert_type;		//!< zrtp_alert_t alert sound type
}zrtp_cmd_alert_t;

/* hear ctx command */
typedef struct zrtp_cmd_hear_ctx
{
	unsigned int	disable_zrtp;
} zrtp_cmd_hear_ctx_t;

typedef struct zrtp_cmd_set_ips
{
    uint32_t		ip_count;
    uint32_t		ips[ZFONE_MAX_INTERFACES_COUNT];
    uint8_t			flags[ZFONE_MAX_INTERFACES_COUNT];
} zrtp_cmd_set_ips_t;

/*!
 * \brief ZRTP stream information
 */
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

/*
 *\brief Command for sessions info transfer
 */
typedef struct zrtp_cmd_update_streams
{
	uint32_t 			count;
	zrtp_conn_info_t	list[ZFONE_MAX_CONNECTIONS_COUNT];
} zrtp_cmd_update_streams_t;

typedef struct zrtp_cmd_set_notify
{
	unsigned char      state;      /* 1 if ON and 0 is OFF */
} zrtp_cmd_set_notify_t;


/*!
   \brief  Control command types. 
   \warning Don't change comand codes !!!
*/
enum control_commands
{
     UPDATE_STREAMS	= 0
    ,GO_SECURE		= 1
    ,GO_CLEAR		= 2
	,REFRESH		= 3 	// for refreshing GUI state in unix
	,SET_VERIFIED	= 4
    ,SET_NAME		= 5
    ,SEND_VERSION	= 6 	// sends version to GUI
    ,GET_VERSION	= 7 	// force daemon to update version (unix)
    ,CRASHED		= 8 	// unix
    ,STOPPED		= 9
    ,STARTED		= 10
    ,SET_PREF		= 11
    ,ERROR			= 12   
    ,ZRTP_PACKET	= 13
    ,PLAY_ALERT		= 14 	// mac daemon sends this
    ,CLEAN_LOGS		= 15
    ,GET_DEFAULTS	= 16
    ,RESET_DAEMON	= 17
    ,SET_IPS		= 18
	,GET_PREF		= 19
	,SET_NOTIFY		= 20 	//win driver feature
	,RESET_ZID		= 21
	,GET_CACHE_INFO	= 22
	,SET_CACHE_INFO	= 23
	,REMEMBER_CALL  = 24
	,HEAR_CTX		= 25
	,ZRTP_NR_CMDS
};

int comm_secure(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
int comm_clear(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
int comm_set_verified(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
int comm_set_name(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
int comm_clean_logs(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
int comm_set_pref(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
int comm_set_ips(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);

void check_for_updates(int force);
int force_check_for_updates(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
void zfone_check4updates_done(zfone_version_t *version, int result, unsigned int force);
	
/*!
 * \brief Refresh connections information list
 * Just refresh information. After this function all get_streams_info() can
 * be used to get connections list.
 */
void comm_refresh_conns();

/*!
 * \brief Return connections info
 * This function fills \c cmd structure with connections array. After function
 * call, number of elements  will be available at cmd->count.
 * \param cmd - buffer for streams info storing
 * \return 
 *		- total size of returned information in bytes (array + count)
 */
int get_streams_info(zrtp_cmd_update_streams_t *cmd, int out_size);


void cb_refresh_conns();
void cb_rtp_activity(zfone_stream_t *stream, unsigned int direction, unsigned int code);
void cb_is_in_state_error(zrtp_stream_t *stream_ctx);
void send_streamless_error(uint32_t error);

int comm_set_cache_info(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
int comm_remember_call(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);
int comm_hearctx(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size);

/* callback type for async cmd notifying */
typedef int (*voip_ctrl_callback)(zrtp_cmd_t *, uint32_t, uint32_t);

#endif //__ZFONED_CMD_COMMON_H__
