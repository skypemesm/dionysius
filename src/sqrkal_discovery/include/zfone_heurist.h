/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 * Vitaly Rozhkov <v.rozhkov@soft-industry.com> <vitaly.rozhkov@googlemail.com>
 */

#ifndef __ZFONE_HEURIST__H__
#define __ZFONE_HEURIST__H__

#include <zrtp.h>

#include "zfone_siprocessor.h"
#include "zrtp_poll.h"


//#define ZFONE_USE_HEURIST_DEBUG 1


//=============================================================================
//  Data types and definitions for heuristic analysis
//=============================================================================


#define	ZFONED_RTP_PACKET_MAX_SCORE		100

#define ZFONE_SYMP_TRANSP_COST			((uint32_t)(ZFONED_RTP_PACKET_MAX_SCORE * 0.3))
#define ZFONE_SYMP_RTP_NONALERT_COST	((uint32_t)(ZFONED_RTP_PACKET_MAX_SCORE * 0.2))
#define ZFONE_SYMP_RTP_STATIC_COST		((uint32_t)(ZFONED_RTP_PACKET_MAX_SCORE * 0.3))
#define	ZFONED_RTP_PACKET_ACCEPT_BORDER	75

#define ZFONE_SYMP_SEQ_COST				((uint32_t)(ZFONED_RTP_PACKET_MAX_SCORE * 1))
#define	ZFONED_RTP_STREAM_ACCEPT_BORDER	 ZFONED_RTP_PACKET_MAX_SCORE

//time in ms
#define ZFONE_CALL_SIP_BYE_DELTA 		100
#define ZFONE_CALL_REMOVE_TIME			8000
#define ZFONE_CALL_REMOVE_SIP_COEF		5
#define ZFONE_CALL_REMOVE_BYE_COEF		6

#define ZFONE_SESSION_SLEEP_TIME 		2000
#define ZFONE_SESSION_REMOVE_TIME 		10000
#define ZFONE_SESSION_REMOVE_SIP_COEF	2

#define ZFONE_SIP_LINK_WINDOW			5000

#define ZFONE_CONCURENT_SIP_TIMEOUT		3000

//WARNING: Use even values only! 
#define ZFONE_HEURIST_MIN_SEQUENTIAL 	4

//#define ZFONE_SIP_BYE_HISTORY_DEPTH	10

#define ZFONE_SKIP_BEFORE_ICE_COUNT		20
#define ZFONE_SKIP_AFTER_BYE_TIMEOUT	5000

typedef enum zfone_symptom_status
{
    ZFONE_SYMP_STATUS_ERROR		= 0,
    ZFONE_SYMP_STATUS_MATCH		= 1,
    ZFONE_SYMP_STATUS_DIST		= 2
} zfone_symptom_status_t;

typedef enum zfone_static_symptom_type
{    
    ZFONE_SYMP_TRANSPORT_CHECK	= 0,
	ZFONE_SYMP_SKIPALERT_CHECK	= 1,
	//ZFONE_SYMP_HELLO_CHECK		= 2,
	ZFONE_SYMP_SRTP_CHECK		= 2,
	ZFONE_SYMP_SCOUNT			= 3

} zfone_static_symptom_type_t;

typedef enum zfone_dynam_symptom_type
{    	
	ZFONE_SYMP_DRTP_CHECK		= 0,
	ZFONE_SYMP_DCOUNT			= 1
} zfone_dynam_symptom_type_t;

typedef enum zfone_confirm_mode
{
	ZFONE_CONF_MODE_UNKN		= 0,
	ZFONE_CONF_MODE_CONFIRM		= 1,
	ZFONE_CONF_MODE_LINK		= 2,
	ZFONE_CONF_MODE_CLOSED		= 3,
	ZFONE_CONF_MODE_COUNT		= 4		
} zfone_confirm_mode_t;
extern char* zfone_confirm_mode_names[ZFONE_CONF_MODE_COUNT];

// !warning: don't change order of these definitions we use >, < operations
typedef enum zfone_prtps_state
{
	ZFONE_PRTPS_STATE_PASIVE	= 0,	//! When session have been inactive for a ZFONE_PRTPS_PASIVE_TIMEOUT seconds since ZFONE_PRTPS_STATE_SLEEP
	ZFONE_PRTPS_STATE_ACTIVE	= 1,	//! activated on first packet passed all static tests	
	ZFONE_PRTPS_STATE_PRE_READY = 2,	//! when session total score >=  ZFONED_RTP_STREAM_ACCEPT_BORDER/2
	ZFONE_PRTPS_STATE_READY		= 3,
	ZFONE_PRTPS_STATE_PRE_ESTABL= 4,	//! when session total score > ZFONED_RTP_STREAM_ACCEPT_BORDER but foe "one-way" media
	ZFONE_PRTPS_STATE_ESTABL	= 5,	//! when session total score > ZFONED_RTP_STREAM_ACCEPT_BORDER
	ZFONE_PRTPS_STATE_CLOSED	= 6,
	ZFONE_PRTPS_STATE_COUNT		= 7
} zfone_prtps_state_t;
extern char* zfone_prtps_state_names[ZFONE_PRTPS_STATE_COUNT];


typedef struct zfone_prob_rtps zfone_prob_rtps_t;
typedef struct zfone_rtpcheck_info
{
	zfone_packet_t*			packet;
    zrtp_rtp_hdr_t*			rtp_hdr;
	zfone_media_type_t		type;
	zfone_prob_rtps_t*		prtps;
	uint32_t				score;
	mpoll_t					_mpoll;
} zfone_rtpcheck_info_t;

typedef struct zfone_hs_node
{
	mlist_t					head;
} zfone_hs_node_t;

typedef struct
{
	uint16_t				max_seq;
	uint16_t				bad_seq;
	uint32_t				cycles;
	uint32_t				received;
	uint32_t				received_prior;
	uint32_t				expected_prior;
	int32_t					probation;
	uint32_t				max_ts;
	uint32_t				tmp_score;
} zfone_seq_data_t;

typedef struct zfone_prob_sips zfone_prob_sips_t;
typedef struct zfone_prob_rtp_call zfone_prob_rtp_call_t;
struct zfone_prob_rtps
{
	zfone_prtps_state_t		state;
	zfone_media_stream_t	media;
	zfone_direction_t		direction;
	uint8_t					codec;

	uint32_t				total_score;	
	uint64_t				timestamp; 
	uint64_t				pre_ready_timestamp;
	zfone_seq_data_t 		seq_data;

	uint8_t					pos;
	zfone_sip_id_t			sip_id;
	zfone_prob_sips_t		*psips;
	zfone_confirm_mode_t	conf_mode;
	zfone_prob_rtp_call_t*	call;

	uint16_t				skip_before_ice;

	mlist_t					mlist_map;
	mpoll_t					_mpoll; 		//! keep it at the end of the structure
};

struct zfone_hstream
{
	zfone_prob_rtps_t		*in_stream;
	zfone_prob_rtps_t		*out_stream;
	zfone_media_type_t  	type;
	zfone_stream_t			*zfone_stream;
	zfone_prtps_state_t		state;
};

struct zfone_prob_rtp_call
{
	zfone_hstream_t			streams[MAX_SDP_RTP_CHANNELS];	
	zfone_confirm_mode_t	conf_mode;
	zfone_sip_id_t			sip_id;
	zfone_prob_sips_t		*psips;
	zfone_ctx_t				*zfone_session;
	uint64_t				in_touch;	//!< touch time for incoming stream since the Epoch in milliseconds
	uint64_t				out_touch;	//!< touch time for outgoing stream since the Epoch in milliseconds
	mlist_t					mlist;
};

struct zfone_prob_sips
{
	zfone_sip_session_t		*sips;
    zfone_prob_rtp_call_t	*call;	

	/*!
	 *\brief last SIP update timestamp.
	 * At the beginning of the call it is equal to timestamp of the very first
	 * SIP INVITE. We update this timestamp with every next SIP reINVITE.
	 */
	uint64_t				created_at;
	mpoll_t					_mpoll;
};

typedef struct zfone_heurist_bye
{
	zfone_media_stream_t	media;	
	uint64_t				created_at;
	mlist_t					_mlist;
} zfone_heurist_bye_t;

typedef struct zfone_hresult
{
	zfone_ctx_t				*zfone_session;
	zfone_hstream_t			*hstream;
} zfone_hresult_t;

typedef enum zfone_heurist_action
{
	ZFONE_HEURIST_ACTION_NOP 	= 0,
	ZFONE_HEURIST_ACTION_CREATE	= 1,
	ZFONE_HEURIST_ACTION_ADD	= 2,
	ZFONE_HEURIST_ACTION_REMOVE	= 3,
	ZFONE_HEURIST_ACTION_CHANGE	= 4,
	ZFONE_HEURIST_ACTION_CLOSE	= 5,
	ZFONE_HEURIST_ACTION_COUNT	= 6
} zfone_heurist_action_t;

//! Return statuses for heurist processing routine
typedef enum zfone_heurist_status
{
	zfone_hs_error				= 0,	//! Error was occured during packet processing (see debug logging)
	zfone_hs_nop				= 1,	//! Positive result, but any changes in "Heurist" state	
	zfone_hs_touched			= 2,	//! Stream/call touch time was updated. (any actions)
	zfone_hs_oneway_created		= 3,	//! "One-way" media call was created_at
	zfone_hs_full_created		= 4,	//! Full, "two-ways" media call was created_at
    zfone_hs_full_updated		= 5,	//! "One-way" call was upgated to "Full" by adding missed stream
	zfone_hs_forwarded			= 6,	//! Call may be updated to "Full" but this action can't be performed
										//  on current runlevel. deep_analyse() should be called secondarily
										//  but from "safe" thread with lower run-level. 
	zfone_hs_prtp				= 7,	//! Positive result for fast analyzer. Such a packet should be inspected
										//  by dynamic tests in deep_analyze();
	zfone_hs_skip				= 8,	//! Packet was skipped by ICE handling engine or after SIP BYE to
										//  prevent Heurist engine from unsuccessful transitions;
	zfone_hs_count				= 9
} zfone_heurist_status_t;

void zfone_heurist_action_handler(zfone_hresult_t* result, zfone_heurist_action_t action);

struct zfone_heurist
{
	uint8_t				_is_initiated;
	mlist_t 			bye_head;
	mpoll_t				sips_poll;
	mpoll_t				rtps_poll;
	mlist_t				rtpc_head;
	mpoll_t				info_poll;
	zfone_hs_node_t		rtps_map[ZFONE_PORTS_SPACE_SIZE];
	zrtp_mutex_t		*protector;
	
	uint32_t			a_calls_count;
	uint32_t			v_calls_count;
	    
    int					(*create)(struct zfone_heurist* self);
    void				(*destroy)(struct zfone_heurist* self);
	void 				(*show_state)(struct zfone_heurist *self);
    
    zfone_heurist_status_t (*fast_analyse)( struct zfone_heurist* self,
											zfone_packet_t* packet,
											zrtp_voip_proto_t proto );

	zfone_heurist_status_t (*deep_analyse)( struct zfone_heurist* self,
											zfone_packet_t* packet,
											zrtp_voip_proto_t proto);
	void 				(*clean_dead_streams)(struct zfone_heurist *self);
	void 				(*clean_closed_calls)(struct zfone_heurist *self);

	int					(*create_sip_session)( struct zfone_heurist* self,
											   zfone_sip_session_t* sips );
	int					(*destroy_sip_session)( struct zfone_heurist* self,
												zfone_sip_session_t* sips );
	int					(*update_sip_session)( struct zfone_heurist* self,
											   zfone_sip_session_t* sips );
	int					(*is_sip_session_busy)( struct zfone_heurist* self,
											   zfone_sip_session_t* sips );

	void 				(*reset)(struct zfone_heurist *self);
};


extern int zfone_heurist_ctor(struct zfone_heurist* heurist);
extern struct zfone_heurist heurist;



// ============================================================================
// INTERNAL function used by different parts of Heurist engine
// ============================================================================



typedef zfone_symptom_status_t zfone_rtp_symptom_handler( struct zfone_heurist* heurist,
														  zfone_rtpcheck_info_t* info );
extern zfone_rtp_symptom_handler* _rtp_static_sympt[ZFONE_SYMP_SCOUNT];
extern zfone_rtp_symptom_handler* _rtp_dynam_sympt[ZFONE_SYMP_DCOUNT];


#define ZH_PRINT_PSIPS(psips, buff)\
 psips ? hex2str((void*) psips->sip_id, SIP_SESSION_ID_SIZE, buff, sizeof(buff)) : "NULL"

#define ZH_PRINT_SIPS(sips, buff)\
 sips ? hex2str((void*) sips->sip_id, SIP_SESSION_ID_SIZE, buff, sizeof(buff)) : "NULL"


/*!
 * \brief Marks RTP stream as confirmed by SIP session
 * Links RTP stream structure with SIP session by remote RTP port. Function checks
 * local/(request) and remote/(response) RTP ports from SIP session and compares
 * them with RTP remote port. If any of them are matched - function links prtps
 * with psips.
 * \param prtps - RTP stream for the linking
 * \param psips - SIP session wrapper for the linking
 * \param use_new - points to look for matches in "new streams" space instead of
 *					"confirmed streams" space.
 * \return:
 *	0 - if prtps was confirmed as by SIP;
 * -1 - in pther case.
 */
int _zfone_mark_prtps_by_sip( zfone_prob_rtps_t *prtps,
							  zfone_prob_sips_t *psips,
							  uint8_t use_new );

uint8_t zfone_heurist_check4changes( struct zfone_heurist *heurist,
									 zfone_prob_rtps_t* prtps,
									 zfone_rtpcheck_info_t *info );

zfone_prob_sips_t* _find_appropriate_sips( struct zfone_heurist* self,
										   zfone_prob_rtps_t *prtps,
										   zfone_prob_rtps_t *pair_prtps );

void _zfone_release_prtps( struct zfone_heurist* heurist,
						   zfone_prob_rtps_t *prtps );

/*!
 * \brief Checks RTP activity on closed calls
 * Function returns 1 if RTP packet info received on the same address as just
 * closed SIP call. Heurist uses this function to prevent it's engine from
 * creating useless prtp streams.
 */
int zfone_is_rtp_after_bye( struct zfone_heurist *heurist,
							zfone_rtpcheck_info_t* info );

#endif //__ZFONE_HEURIST__H__
