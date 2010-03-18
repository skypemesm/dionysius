/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>

#include "zfone.h"

#define _ZTU_ "zfone manager"

extern zrtp_global_t 	*zrtp_global;	//!< zrtp context for global data storing


//------------------------------------------------------------------------------
static int create(struct zfone_manager *manager, zrtp_zid_t *zid)
{    
	ZRTP_LOG(3, (_ZTU_, "zfone_manager creating: initialization process for manager=%p...\n", manager));
    
    if (!manager->inited)
    {
		int i =0;

#if (ZRTP_PLATFORM != ZP_WIN32_KERNEL)
		/* zid will be read from registry for windows driver, its empty for now.*/
		zrtp_memcpy(&manager->zid, zid, sizeof(zrtp_zid_t));
#endif

		// Clearing detection ports space	
		zrtp_memset(manager->local_ports, 0, sizeof(manager->local_ports));
		for (i=0; i<ZFONE_PORTS_SPACE_SIZE; i++)		
			manager->local_ports[i].proto = voip_proto_UNKN;		
		
		// Init zfone contexts and streams lists
		init_mlist(&manager->ctx_head);
		init_mlist(&manager->streams_head);
		
		manager->inited = 1;
    }
    
    return 0;
}

//------------------------------------------------------------------------------
static void destroy(struct zfone_manager *manager)
{
    ZRTP_LOG(3, (_ZTU_, "zfone_manager destroy: deinitialization for manager=%p...\n", manager));
    
    if (manager->inited)
    {
		// destroy all allocated contexts and streams
		mlist_t *node=NULL, *tmp= NULL;

		manager->inited = 0;

		mlist_for_each_safe(node, tmp, &manager->ctx_head)
			zrtp_sys_free(mlist_get_struct(zfone_ctx_t, mlist, node));

		init_mlist(&manager->ctx_head);
		init_mlist(&manager->streams_head);
    }
}

//------------------------------------------------------------------------------
static zfone_ctx_t* get_session(struct zfone_manager *manager, zfone_session_id_t id)
{
    mlist_t *node = NULL;
    zfone_ctx_t *ctx = NULL;
    
    mlist_for_each(node, &manager->ctx_head)
    {
		ctx = mlist_get_struct(zfone_ctx_t, mlist, node);
		if ( !memcmp(ctx->id, id, ZFONE_SESSION_ID_SIZE) )
			return ctx;
    }
    
    return NULL;
}

//------------------------------------------------------------------------------
static zfone_ctx_t* create_session( struct zfone_manager *manager,
									const unsigned char *source,
									uint32_t source_len,
				    			    const char *uri,
									uint32_t uri_len,
									zfone_voip_agent_t luac,
									zfone_voip_agent_t ruac )
{
	zfone_session_id_t id;
    zfone_ctx_t *new_ctx = NULL;
    uint32_t i = 0;
	
	// Generate session ID as a hash of source string. When we have ID we may check
	// for duplicated sessions.
    {
		sha256_ctx  sha_ctx;
		uint8_t	sha_buff[64];
	
		sha256_begin(&sha_ctx);
		sha256_hash(source, source_len, &sha_ctx);
		sha256_end(sha_buff, &sha_ctx);
		zrtp_memcpy(id, sha_buff, ZFONE_SESSION_ID_SIZE);
    }

	if (NULL != get_session(manager, id))
	{
		ZRTP_LOG(1, (_ZTU_, "zfone manager: Sessiuon with such ID has already existed!\n"));
		return NULL;
	}    
	
	// Create and initialize new session structure. Prepare streams contexts for usage
	new_ctx = (zfone_ctx_t*) zrtp_sys_alloc(sizeof(zfone_ctx_t));
    if (!new_ctx)
    {
		ZRTP_LOG(1, (_ZTU_, "zfone manager: Can't allocate memory for new Zfone context!\n"));
		return NULL;
    }

    zrtp_memset(new_ctx, 0, sizeof(zfone_ctx_t));
	zrtp_memcpy(new_ctx->id, id, ZFONE_SESSION_ID_SIZE);

	//ZSTR_SET_EMPTY(new_ctx->sas1);
	//ZSTR_SET_EMPTY(new_ctx->sas2);

    for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
    {
		new_ctx->streams[i].in_media.type = ZFONE_SDP_MEDIA_TYPE_UNKN;
		new_ctx->streams[i].out_media.type = ZFONE_SDP_MEDIA_TYPE_UNKN;
		new_ctx->streams[i].type = ZFONE_SDP_MEDIA_TYPE_UNKN;
		new_ctx->streams[i].zfone_ctx = new_ctx;
    }
    
    ZSTR_SET_EMPTY(new_ctx->name);
	if ( uri_len )
	{
  		new_ctx->remote_uri.length = (uri_len < ZRTP_STRING64) ? uri_len : ZRTP_STRING64;
  		zrtp_memcpy(new_ctx->remote_uri.buffer, uri, new_ctx->remote_uri.length);
	}

	new_ctx->luac = luac;
	new_ctx->ruac = ruac;

	// Create and initialize ZRTP session context
	// todo: check
	new_ctx->zrtp_ctx = &new_ctx->zrtp_ctx_holder;
	/*
    new_ctx->zrtp_ctx = (zrtp_session_t*) zrtp_sys_alloc(sizeof(zrtp_session_t));
    if (!new_ctx->zrtp_ctx)
    {
		ZRTP_LOG(1, (_ZTU_, "zfone manager: Can't allocate memory for new ZRTP crypto-context!\n"));
		zrtp_sys_free(new_ctx);
  		return NULL;
    }
	*/	
	
    if (zrtp_status_ok != zrtp_session_init(zrtp_global, 
											&zfone_cfg.params.zrtp,
											manager->zid,
											1,
											&new_ctx->zrtp_ctx))
    {
		ZRTP_LOG(1, (_ZTU_, "zfoned manager: Error during ZRTP context initialization!\n"));
		// todo: check
		//zrtp_sys_free(new_ctx->zrtp_ctx);
		zrtp_sys_free(new_ctx);
		return NULL;
	}
	new_ctx->zrtp_ctx->usr_data = new_ctx;

    mlist_add(&manager->ctx_head, &new_ctx->mlist);

    return new_ctx;
}

//------------------------------------------------------------------------------
static void destroy_session(struct zfone_manager* manager, zfone_ctx_t* ctx)
{
    // destroy ZRTP crypto context, remove session from list and free memory
    if (ctx)
    {
		int i = 0;
		for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
		{
			zfone_stream_t* s;
			s =  &ctx->streams[i];
			if (s->type != ZFONE_SDP_MEDIA_TYPE_UNKN)
				manager->remove_stream(manager, s);
		}

		if (ctx->zrtp_ctx)
		{	
			zrtp_session_down(ctx->zrtp_ctx);
		}

		mlist_del(&ctx->mlist);
		zrtp_sys_free(ctx);
    }
}

//------------------------------------------------------------------------------    
static void for_each_session( struct zfone_manager* manager,
							  zfone_manager_for_each* func )
{
    mlist_t *node = NULL, *tmp = NULL;	
    zfone_ctx_t *ctx = NULL;
    
    mlist_for_each_safe(node, tmp, &manager->ctx_head)
    {
		ctx = mlist_get_struct(zfone_ctx_t, mlist, node);
		func(ctx);
    }
}

//------------------------------------------------------------------------------    
static void for_each_stream( struct zfone_manager *manager,
							 zfone_manager_for_each_stream *func )
{
    mlist_t *node = NULL, *tmp = NULL;    
    zfone_ctx_t* ctx = NULL;
    
    mlist_for_each_safe(node, tmp, &manager->ctx_head)
    {
		int i;
		ctx = mlist_get_struct(zfone_ctx_t, mlist, node);
	
		for (i=0; i < MAX_SDP_RTP_CHANNELS; i++)
		{
	  		if ( ctx->streams[i].type != ZFONE_SDP_MEDIA_TYPE_UNKN )
	  		{
				func(&ctx->streams[i]);
	  		}
		}
    }
}

//------------------------------------------------------------------------------    
static zfone_stream_t* add_stream( struct zfone_manager* manager,
								   zfone_ctx_t* ctx,
								   zfone_hstream_t *hstream )
{
    zfone_stream_t* stream = NULL;
	zfone_media_stream_t *in_media = hstream->in_stream ? &hstream->in_stream->media : NULL;
	zfone_media_stream_t *out_media = hstream->out_stream ? &hstream->out_stream->media : NULL;
    uint8_t pos = 0;    
	
	{
	char ip_buff1[25];
	char ip_buff2[25];
	ZRTP_LOG(3, (_ZTU_, "zfoned add_stream to the session %p type=%s:\n", ctx, zfone_media_type_names[hstream->type]));
	if (in_media)
	ZRTP_LOG(3, (_ZTU_, "INC local %s:%d:%d remote %s:%d:%d ssrc=%u.\n",
					zfone_ip2str(ip_buff1, 25, zrtp_ntoh32(in_media->local_ip)),
					zrtp_ntoh16(in_media->local_rtp),
					zrtp_ntoh16(in_media->local_rtcp),
					zfone_ip2str(ip_buff2, 25, zrtp_ntoh32(in_media->remote_ip)),
					zrtp_ntoh16(in_media->remote_rtp),
					zrtp_ntoh16(in_media->remote_rtcp),
					zrtp_ntoh32(in_media->ssrc)));
	if (out_media)
	ZRTP_LOG(3, (_ZTU_, "OUT local %s:%d:%d remote %s:%d:%d ssrc=%u.\n",
					zfone_ip2str(ip_buff1, 25, zrtp_ntoh32(out_media->local_ip)),
					zrtp_ntoh16(out_media->local_rtp),
					zrtp_ntoh16(out_media->local_rtcp),
					zfone_ip2str(ip_buff2, 25, zrtp_ntoh32(out_media->remote_ip)),
					zrtp_ntoh16(out_media->remote_rtp),
					zrtp_ntoh16(out_media->remote_rtcp),
					zrtp_ntoh32(out_media->ssrc)));
	}

	//
	// Get next free stream from the sessiuon poll. Initialize stream data by info
	// from the media context. When ZFone context is ready - create ZRTP context
	// and attach it to the ZRTP session, associated with current ZFone session.
	//
    stream = NULL;
    for (pos=0; pos<MAX_SDP_RTP_CHANNELS; pos++)
    {
		if (ctx->streams[pos].type == ZFONE_SDP_MEDIA_TYPE_UNKN)
		{
			stream = &ctx->streams[pos];
			break;
		}
    }
    if (!stream)
    {
		ZRTP_LOG(1, (_ZTU_, "zfone add_stream: can't find one more"
									   " free stream in specifed context\n"));	
		return NULL;
    }
	
    zrtp_memset(stream, 0, sizeof(zfone_stream_t));    
    
    { // generate  stream ID like hash from context IP and stream pos in context list
		sha256_ctx  sha_ctx;
		uint8_t	sha_buff[64];
		sha256_begin(&sha_ctx);
		sha256_hash(ctx->id, ZFONE_STREAM_ID_SIZE, &sha_ctx);
		sha256_hash((unsigned char*) &pos, sizeof(uint32_t), &sha_ctx);
		sha256_end(sha_buff, &sha_ctx);
		zrtp_memcpy(stream->id, sha_buff, ZFONE_STREAM_ID_SIZE);
    }

	stream->zfone_ctx	= ctx;
	stream->type 		= hstream->type;
	if (in_media)
		zrtp_memcpy(&stream->in_media, in_media, sizeof(zfone_media_stream_t));
	if (out_media)
		zrtp_memcpy(&stream->out_media, out_media, sizeof(zfone_media_stream_t));
    stream->rx_state 	= ZFONED_NO_ACTIV;
    stream->tx_state 	= ZFONED_NO_ACTIV; 	

    // Create ZRTP crypto stream and start engine	
	zrtp_stream_attach(ctx->zrtp_ctx, &stream->zrtp_stream);										

	stream->zrtp_stream->usr_data = stream;

	// Now all preaparations were done - start ZRTP protocol
	zrtp_stream_start(stream->zrtp_stream, zrtp_ntoh32(out_media ? out_media->ssrc : ~in_media->ssrc));

	// Add ready to use stream to the detection list. It will allow detector to
	// extract RTP packets from the entire UDP channel.
	mlist_add(&manager->streams_head, &stream->_mlist);

	return stream;
}

//------------------------------------------------------------------------------
static zfone_stream_t* remove_stream( struct zfone_manager* manager,
									  zfone_stream_t* stream )									  
{
	zfone_ctx_t *tmp_ctx = stream->zfone_ctx;
	// Stop ZRTP protocol and release all allocated resources

	if (stream->zrtp_stream)
	{		
		zrtp_stream_stop(stream->zrtp_stream);		
	}		

	// Remove form the detection list
	mlist_del(&stream->_mlist);

    // Clear stream body
	// todo: check this
	/*
    memset(stream, 0, sizeof(zfone_stream_t));
	*/
	stream->zfone_ctx = tmp_ctx;
    stream->type = ZFONE_SDP_MEDIA_TYPE_UNKN;	
    return stream;
}


//------------------------------------------------------------------------------
extern void config_sip(struct zfone_manager* manager, struct zfone_configurator* config);
extern zrtp_voip_proto_t detect_proto(struct zfone_manager* manager, zfone_packet_t* packet);

int zfone_manager_ctor(struct zfone_manager* manager)
{        
    memset(manager, 0, sizeof(struct zfone_manager));    
    
    manager->create 			= create;
    manager->destroy 			= destroy;    
    
    manager->create_session		= create_session;    
    manager->get_session		= get_session;
    manager->destroy_session	= destroy_session;
    manager->for_each_session	= for_each_session;
    manager->for_each_stream	= for_each_stream;

    manager->add_stream			= add_stream;
    manager->remove_stream		= remove_stream;    
    
    manager->config_sip			= config_sip;   
    manager->detect_proto   	= detect_proto;    
    
    return 0;
}

