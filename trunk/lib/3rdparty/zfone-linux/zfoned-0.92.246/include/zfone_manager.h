/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONE_MANAGER_H__
#define __ZFONE_MANAGER_H__

#include <zrtp.h>

#include "zfone_types.h"
#include "zfone_siprocessor.h"

#include "zfone_cfg.h"

#define ZFONE_RTP_REPLACE_TIMEOUT	60

#define ZRTP_BIT_SIP_NONE			0x00
#define ZRTP_BIT_SIP_UDP			0x01
#define ZRTP_BIT_SIP_TCP			0x02


/*!
  * /brief  Traceable interfaces
  * Array of the IP addresses, processed as local. Before start VoIP packet
  * direction you must full this array with ip_add() function. All IP's trored
  * in network byte-order
  */
extern uint32_t 	interfaces[ZFONE_MAX_INTERFACES_COUNT];
extern uint8_t 		flags[ZFONE_MAX_INTERFACES_COUNT];
extern uint32_t 	interfaces_count;

int zfone_manager_initialize_ip_list();
int zfone_manager_add_ip(uint32_t ip, int is_static);
int zfone_manager_remove_ip(uint32_t ip);
int zfone_manager_refresh_ip_list();

int zfone_manager_prepare_packet(zfone_packet_t* raw_packet);

typedef void zfone_manager_for_each(zfone_ctx_t* ctx);
typedef void zfone_manager_for_each_stream(zfone_stream_t* stream);

typedef struct zfone_manager zfone_manager_t;

//! connection manager
struct zfone_manager
{
    uint32_t		  inited;
	zrtp_zid_t		  zid;
    mlist_t			  ctx_head;
	mlist_t			  streams_head;
    zfone_port_wrap_t local_ports[ZFONE_PORTS_SPACE_SIZE]; // SIP detection ports-space in host-mode byte-order	
    
    int   (*create)(struct zfone_manager* manager, zrtp_zid_t *zid);
    void  (*destroy)(struct zfone_manager* manager);
        
	void  (*config_sip)( struct zfone_manager* manager,
						 struct zfone_configurator* config );

    zrtp_voip_proto_t (*detect_proto)( struct zfone_manager* manager,
									   zfone_packet_t* packet );

	/*!
	 * \brief Creates and initializes Zfone and ZRTP sessions structures
	 * \param source - source buffer to create session ID. It may be SIP ID or
	 *		some other unique session data;
	 * \param source_len - ID sources lengths in bytes;
	 * \param uri - pointer to the SIP URI if alailable, or NULL in other case;
	 * \param uri_len - SIP URI lengths in bytes. If SIP isn't available - use 0;
	 * \param check_ssrc - pointe ZFone to use outgoing RTP SSRC as a part of the
	 *		primary key for stream addressing. (see zfone_ctx_t::check_ssrc).
	 * \return 
	 *	- pointer to the new ZFone session in case of success or NULL in other case.	 
	 */	
	zfone_ctx_t* (*create_session)( struct zfone_manager* manager,
									const unsigned char* source, uint32_t source_len, 
									const char* uri, uint32_t uri_len,
									zfone_voip_agent_t luac,
									zfone_voip_agent_t ruac );	

    zfone_ctx_t* (*get_session)( struct zfone_manager* manager,
								 zfone_session_id_t id );
    void (*destroy_session)( struct zfone_manager* manager,
							 zfone_ctx_t* ctx );
    void (*for_each_session)( struct zfone_manager* manager,
							  zfone_manager_for_each* func );	

	/*!
	 * \brief Creates Zfone stream
	 * add_stream() creates one more ZFone stream within session \c ctx. New stream
	 * is initialized by data from media context and insetred into the detection map.
	 * This allows fast RTP/RTCP extraction from the UDP/TCPO channel by means of
	 * detect_proto(). Besides Zfone stream, ZRTP crypto stream is created and
	 * started as well.
	 * \param ctx - ZFone session to attach stream for;
	 * \param mstream - media context for the new stream initialization.
	 * \return
	 *	- pointer to the just created stream context in cas of success;
	 *	- NULL if some errors occured during initialization. (see logs).
	 */
    zfone_stream_t* (*add_stream)( struct zfone_manager *manager,
								   zfone_ctx_t *ctx,
								   zfone_hstream_t *hastream);

    zfone_stream_t* (*remove_stream)( struct zfone_manager* manager,
									  zfone_stream_t* stream );									  

	void 			(*for_each_stream)( struct zfone_manager* manager,
										zfone_manager_for_each_stream* func );	
};

/*!
 * \brief create and initialize zfone manager
 */
extern int zfone_manager_ctor(struct zfone_manager* manager);
extern struct zfone_manager manager;

#endif //__ZFONE_MANAGER_H__

