/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone handler"

extern zrtp_global_t 		*zrtp_global;

//-----------------------------------------------------------------------------
void zfone_heurist_action_handler(zfone_hresult_t* result, zfone_heurist_action_t action)
{
	zfone_hstream_t *hstream = result->hstream;
	zfone_ctx_t	*ctx 		 = result->zfone_session;

#if ZRTP_PLATFORM == ZP_WIN32_KERNEL
	// simple attemt to protect driver from crash
	set_zrtp_ok(-1);	
	//NdisMSleep(100000); //TODO: Can't use Sleep there but we still need some protection
#endif

	switch (action)
	{
	case ZFONED_SIPS_CREATE:
	{
		zfone_sip_session_t *sips = NULL;
		if (result->hstream->in_stream)
			sips = (result->hstream->in_stream->call->psips) ? result->hstream->in_stream->call->psips->sips : NULL;
		else if (result->hstream->out_stream)
			sips = (result->hstream->out_stream->call->psips) ? result->hstream->out_stream->call->psips->sips : NULL;

		zrtp_print_log_delim(3, LOG_START_SUBSECTION, "NEW RTP CALL WAS DETECTED");
  			
	  	// create user profile
		// If configuration error - restore default preferences and tro to configure session one more time
		if (zrtp_status_ok != zrtp_profile_check(&zfone_cfg.params.zrtp, zrtp_global))
		{	
			ZRTP_LOG(1, (_ZTU_, "zfoned: Configuration error. Default options will be used.\n"));
			zfone_cfg.load_defaults(&zfone_cfg);
			send_streamless_error(ZFONE_ERROR_WRONG_CONFIG);
		}

		if (sips)
		{
			size_t i = strlen(sips->remote_name.name);
			
			// Creating ZFone session for SIP confirmed call: all necessary
			// information are available from SIP. Use ID and URI from SIP messages.
			ctx = manager.create_session( &manager,
										  sips->id,
										  SIP_SESSION_ID_SIZE,
					  					  i ? sips->remote_name.name : sips->remote_name.uri, 
					  					  i ? i : strlen(sips->remote_name.uri),
										  sips->local_agent,
										  sips->remote_agent );
									  	  
		}
		else
		{
			// Call was detected using RTP heuristics only and there are no any
			// additional information. Use any unicue data as sources for session ID
			// and don't specify URI at all.
			ctx = manager.create_session( &manager,
										  (unsigned char*) hstream, sizeof(zfone_hstream_t),								  		  
									  	  NULL, 0,
										  ZFONE_AGENT_UNKN, ZFONE_AGENT_UNKN );									  	  
		}
				
	  	if (!ctx)
  		{
			ZRTP_LOG(1, (_ZTU_, "ZFONED engine: Can't create Zfone context.\n"));
			break;
  		}
			
		// Attach first stream to the session. We don't check return code because all
		// error handlers for add_stream() are included.
		result->hstream->zfone_stream = manager.add_stream(&manager, ctx, result->hstream);
		if ( !result->hstream->zfone_stream )
		{
			ZRTP_LOG(1, (_ZTU_, "ZFONED engine: Can't attach Zfone context.\n"));
			manager.destroy_session(&manager, ctx);
		}
		else
		{
			result->zfone_session = ctx;
		}		

		zrtp_print_log_delim(3, LOG_END_SUBSECTION, "NEW RTP CALL WAS DETECTED");
	} break;

	case ZFONE_HEURIST_ACTION_ADD:
		zrtp_print_log_delim(3, LOG_START_SUBSECTION, "NEW RTP CALL WAS ADDED");
		if (ctx && hstream)
		{
			result->hstream->zfone_stream = manager.add_stream(&manager, ctx, hstream);
		}
		zrtp_print_log_delim(3, LOG_END_SUBSECTION, "NEW RTP CALL WAS ADDED");
		break;

	case ZFONE_HEURIST_ACTION_REMOVE:
		zrtp_print_log_delim(3, LOG_START_SUBSECTION, "NEW RTP STREAM WAS REMOVED");
		if (ctx && hstream && hstream->zfone_stream)
		{
			manager.remove_stream(&manager, hstream->zfone_stream);
		}	
		zrtp_print_log_delim(3, LOG_END_SUBSECTION, "NEW RTP STREAM WAS REMOVED");
		break;

	case ZFONE_HEURIST_ACTION_CHANGE:
		zrtp_print_log_delim(3, LOG_START_SUBSECTION, "NEW RTP CALL WAS UPDATED");

		if (hstream && ctx && hstream->zfone_stream)
		{
			zfone_stream_t *stream  = hstream->zfone_stream;
			zfone_media_stream_t *in_media = hstream->in_stream ? &hstream->in_stream->media : NULL;
			zfone_media_stream_t *out_media = hstream->out_stream ? &hstream->out_stream->media : NULL;

			// Print out transport options before the modification
			char ipbuff1[25]; char ipbuff2[25];
			ZRTP_LOG(3, (_ZTU_, "zfone engine original transport options:\n"));
			ZRTP_LOG(3, (_ZTU_, "IN \t\t %s:%d<--> %s:%d ssrc=%u.\n",
			zfone_ip2str(ipbuff1, 25, zrtp_ntoh32(stream->in_media.local_ip)), zrtp_ntoh16(stream->in_media.local_rtp),
			zfone_ip2str(ipbuff2, 25, zrtp_ntoh32(stream->in_media.remote_ip)), zrtp_ntoh16(stream->in_media.remote_rtp),
			zrtp_ntoh32(stream->in_media.ssrc)));
			ZRTP_LOG(3, (_ZTU_, "OUT \t %s:%d<--> %s:%d ssrc=%u.\n\n",
			zfone_ip2str(ipbuff1, 25, zrtp_ntoh32(stream->out_media.local_ip)), zrtp_ntoh16(stream->out_media.local_rtp),
			zfone_ip2str(ipbuff2, 25, zrtp_ntoh32(stream->out_media.remote_ip)), zrtp_ntoh16(stream->out_media.remote_rtp),
			zrtp_ntoh32(stream->out_media.ssrc)));

			//
			// Looking for changes in incoming media-stream options
			// We supose that only remote IP and RTP ports may be changed
			//
			if ((stream->in_media.type == ZFONE_SDP_MEDIA_TYPE_UNKN) && in_media) // incoming stream was added
			{				
				zrtp_memcpy(&stream->in_media, in_media, sizeof(zfone_media_stream_t));				
			}			
			else if (in_media)
			{
				if (in_media->remote_ip != stream->in_media.remote_ip)
					stream->in_media.remote_ip = in_media->remote_ip;					

				if (in_media->remote_rtp != stream->in_media.remote_rtp)
				{
					stream->in_media.remote_rtp = in_media->remote_rtp;
					stream->in_media.remote_rtcp = zrtp_ntoh16(in_media->remote_rtp) + 1;
				}

				if (in_media->local_rtp != stream->in_media.local_rtp)
					stream->in_media.local_rtp = in_media->local_rtp;

				if (in_media->ssrc != stream->in_media.ssrc)
					stream->in_media.ssrc = in_media->ssrc;					
			}

			if ((stream->out_media.type == ZFONE_SDP_MEDIA_TYPE_UNKN) && out_media) // outgoing stream was added
			{				
				zrtp_memcpy(&stream->out_media, out_media, sizeof(zfone_media_stream_t));
				stream->zrtp_stream->media_ctx.ssrc = out_media->ssrc;
			}
			else if (out_media)
			{
				if (out_media->remote_ip != stream->out_media.remote_ip)				
					stream->out_media.remote_ip = out_media->remote_ip;

				if (out_media->remote_rtp != stream->out_media.remote_rtp)				
				{
					stream->out_media.remote_rtp = out_media->remote_rtp;
					stream->out_media.remote_rtcp = zrtp_ntoh16(out_media->remote_rtp) + 1;
				}

				if (out_media->local_rtp != stream->out_media.local_rtp)
					stream->out_media.local_rtp = out_media->local_rtp;

				if (out_media->ssrc != stream->out_media.ssrc)
				{
					stream->out_media.ssrc = out_media->ssrc;
					stream->zrtp_stream->media_ctx.ssrc = out_media->ssrc;
				}
			}

			ZRTP_LOG(3, (_ZTU_, "zfone engine after the modification:\n")); 
			ZRTP_LOG(3, (_ZTU_, "IN \t %s:%d<--> %s:%d ssrc=%u.\n",
			zfone_ip2str(ipbuff1, 25, zrtp_ntoh32(stream->in_media.local_ip)), zrtp_ntoh16(stream->in_media.local_rtp),
			zfone_ip2str(ipbuff2, 25, zrtp_ntoh32(stream->in_media.remote_ip)), zrtp_ntoh16(stream->in_media.remote_rtp),
			zrtp_ntoh32(stream->in_media.ssrc)));
			ZRTP_LOG(3, (_ZTU_, "OUT \t\t %s:%d<--> %s:%d ssrc=%u.\n",
			zfone_ip2str(ipbuff1, 25, zrtp_ntoh32(stream->out_media.local_ip)), zrtp_ntoh16(stream->out_media.local_rtp),
			zfone_ip2str(ipbuff2, 25, zrtp_ntoh32(stream->out_media.remote_ip)), zrtp_ntoh16(stream->out_media.remote_rtp),
			zrtp_ntoh32(stream->out_media.ssrc)));
		}
				
		zrtp_print_log_delim(3, LOG_END_SUBSECTION, "NEW RTP CALL WAS UPDATED");
		break;

	case ZFONE_HEURIST_ACTION_CLOSE:	
	    zrtp_print_log_delim(3, LOG_START_SUBSECTION, "RTP CALL WAS CLOSED");
		if (ctx)
		{			
			manager.destroy_session(&manager, ctx);			
			manager.config_sip(&manager, &zfone_cfg);
				 
			// Flush ZRTP cache to the hard drive to prevent RS unsynchronisation
			// in case of emergency stopping (doen't affect windows build
			zrtp_def_cache_store(zrtp_global);
		}
		zrtp_print_log_delim(3, LOG_END_SUBSECTION, "RTP CALL WAS CLOSED");
		break;

	default: break;
	} // switch(action);

#if ZRTP_PLATFORM == ZP_WIN32_KERNEL
	set_zrtp_ok(1);
#endif

	cb_refresh_conns();
}

//------------------------------------------------------------------------------
void zfone_sip_action_handler(zfone_sip_session_t* sips, zfone_sip_action_t action)
{
	switch (action)
	{
  	case ZFONED_SIPS_CREATE:
		zrtp_print_log_delim(3, LOG_START_SUBSECTION, "NEW SIP SESSION WAS DETECTED");
#if 0
#warning !!!! "REMOVE AFTER TESTING. IT FORCE SIP BE UNCONFIRMED"
		sips->new_streams[0].local_rtp = sips->new_streams[0].local_rtp+1;
		sips->new_streams[0].remote_rtp = sips->new_streams[0].remote_rtp+1;
		sips->new_streams[1].local_rtp = sips->new_streams[1].local_rtp+1;
		sips->new_streams[1].remote_rtp = sips->new_streams[1].remote_rtp+1;
#warning !!!! "REMOVE AFTER TESTING. IT FORCE SIP BE UNCONFIRMED"
#endif
		if ( 0 != heurist.create_sip_session(&heurist, sips))
		{
			ZRTP_LOG(1, (_ZTU_, "ZFONED engine: heurist.create_sip_session failed with status=%d\n", -1));
		}
		break;
		
	case ZFONED_SIPS_UPDATE:
		zrtp_print_log_delim(3, LOG_START_SUBSECTION, "NEW SIP SESSION WAS UPDATED");
		if ( 0 != heurist.update_sip_session(&heurist, sips) )
		{
			ZRTP_LOG(1, (_ZTU_, "ZFONED engine: heurist.update_sip_session failed with status=%d\n", -1));
		}
		break;
		
	case ZFONED_SIPS_CLOSE:
		zrtp_print_log_delim(3, LOG_START_SUBSECTION, "NEW SIP SESSION WAS CLOSED");
		if ( 0 != heurist.destroy_sip_session(&heurist, sips))
		{
			ZRTP_LOG(1, (_ZTU_, "ZFONED engine: heurist.create_sip_session failed with status=%d\n", -1));
		}
		break;
		
	default:
		break;
	} // switch(action);
}

void zrtp_protocol_event_callback(zrtp_stream_t *ctx, zrtp_protocol_event_t event)
{
	zfone_ctx_t *conn = ((zfone_stream_t *)ctx->usr_data)->zfone_ctx;
	zfone_stream_t* zstream = (zfone_stream_t*) ctx->usr_data;

    switch (event)
    {
		case ZRTP_EVENT_IS_SECURE_DONE:
			// If there is no name in the case - store SIP URI by default
			// We use this event because in ZRTP_EVENT_IS_SECURE state new RS doesn't
			// exits and we can't store the name of the cace
			if (0 == conn->name.length)
			{
				zrtp_zstrcpy(ZSTR_GV(conn->name), ZSTR_GV(conn->remote_uri));
				zrtp_def_cache_put_name( ZSTR_GV(ctx->session->zid),
									ZSTR_GV(ctx->session->peer_zid),
									ZSTR_GV(conn->name) );
				ZRTP_LOG(3, (_ZTU_, "ZFONED zrtp_event_callback(): name for the RS"
							   " isn't specified - use default one %s.\n", conn->name.buffer));
			}
			break;
		case ZRTP_EVENT_IS_SECURE:
			ZSTR_SET_EMPTY(conn->name);
			zrtp_def_cache_get_name( (const zrtp_stringn_t*)&ctx->session->zid,
									(const zrtp_stringn_t*)&ctx->session->peer_zid, 
									ZSTR_GV(conn->name) );
			if (ctx->session->secrets.wrongs) {
				send_streamless_error(ZFONE_ERROR_SAS);
				zstream->alert_code = ZRTP_ALERT_PLAY_ERROR;
			} else {
				zstream->alert_code = ZRTP_ALERT_PLAY_SECURE;
			}
						
			cb_refresh_conns();	
			zrtp_play_alert(zstream);
			break;
			
		case ZRTP_EVENT_IS_CLIENT_ENROLLMENT:
			((zfone_stream_t *)ctx->usr_data)->pbx_reg_ok = 1;
			cb_refresh_conns();
			break;
		case ZRTP_EVENT_IS_CLEAR:
			zstream->alert_code = ZRTP_ALERT_PLAY_CLEAR;
			zrtp_play_alert(zstream);
		case ZRTP_EVENT_IS_INITIATINGSECURE:
		case ZRTP_EVENT_IS_PENDINGSECURE:
		case ZRTP_EVENT_IS_PENDINGCLEAR:
		case ZRTP_EVENT_NO_ZRTP:
		case ZRTP_EVENT_LOCAL_SAS_UPDATED:
			cb_refresh_conns();
			break;
		default:
			break;
	}
}

void zrtp_security_event_callback(zrtp_stream_t *ctx, zrtp_security_event_t event)
{	
	((zfone_stream_t*)ctx->usr_data)->alert_code = ZRTP_ALERT_PLAY_ERROR;
	zrtp_play_alert(ctx->usr_data);
	cb_is_in_state_error(ctx);
}
