/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 * Nikolay Popok <chaser@soft-industry.com>
 */

#if ZRTP_PLATFORM != ZP_WIN32_KERNEL
	#include <sys/time.h>
	#include <unistd.h>

	#include "sfiles.h"
	#include "zfoned_ctrl.h"
	#include "zfoned_commands.h"
#endif

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone commands"

zrtp_conn_info_t	conn_info_list[ZFONE_MAX_CONNECTIONS_COUNT];
uint16_t			conn_info_list_size = 0;


//==============================================================================
// INPUT commangs handlers
//==============================================================================


//------------------------------------------------------------------------------
int comm_secure(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zfone_ctx_t* session = manager.get_session(&manager, cmd->session_id);
    if (session)
    {
		int i;
		for (i = 0; i < MAX_SDP_RTP_CHANNELS; i++)
		{
	  		if ( session->streams[i].type != ZFONE_SDP_MEDIA_TYPE_UNKN )
				zrtp_stream_secure(session->streams[i].zrtp_stream);
		}
    }
    else
    {
        ZRTP_LOG(1, (_ZTU_, "zfoned ctrl_secure: zrtp stream not found.\n"));
    }

	return 0;
}

//------------------------------------------------------------------------------
int comm_clear(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zfone_ctx_t* session = manager.get_session(&manager, cmd->session_id);
	if ( session )
    {
		int i;
		for (i = 0; i < MAX_SDP_RTP_CHANNELS; i++)
		{
	  		if ( session->streams[i].type != ZFONE_SDP_MEDIA_TYPE_UNKN )
				zrtp_stream_clear(session->streams[i].zrtp_stream);
		}

		/*ZSTR_SET_EMPTY(session->sas1);
		ZSTR_SET_EMPTY(session->sas2);		*/

		ZRTP_LOG(3, (_ZTU_, "zfoned test (comm_clear): clearing sases\n"));
		cb_refresh_conns();
	}
    else
    {
        ZRTP_LOG(1, (_ZTU_, "zfoned ctrl_clear: zrtp stream not found.\n"));
    }
	return 0;
}

//------------------------------------------------------------------------------
int comm_set_verified(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
	zfone_ctx_t* session = NULL;
	if ( in_size < sizeof(zrtp_cmd_t) + sizeof(zrtp_cmd_set_verified_t) )
		return -1;
	
    session = manager.get_session(&manager, cmd->session_id);
	if (session)
    {
		char buf[ZFONE_SESSION_ID_SIZE*2+2];		

		zrtp_cmd_set_verified_t* cmd_verified = (zrtp_cmd_set_verified_t*) ZFONE_CMD_EXT(cmd);

		zrtp_verified_set( session->zrtp_ctx->zrtp,
						   &session->zrtp_ctx->zid,
						   &session->zrtp_ctx->peer_zid,
						   (uint8_t)cmd_verified->is_verified );

		ZRTP_LOG(3, (_ZTU_, "zfoned ctrl_set_verified: VERIFIED=%i for ctx%p  sips %s\n",
    					cmd_verified->is_verified,
						session,
						hex2str((const char*)cmd->session_id,
						ZFONE_SESSION_ID_SIZE, buf, sizeof(buf))));
    }
    else
    {
		char buf[ZFONE_SESSION_ID_SIZE*2+2];        
		ZRTP_LOG(1, (_ZTU_, "zfoned ctrl_verified: zrtp session %s not found.\n",
						hex2str((const char*)cmd->session_id, ZFONE_SESSION_ID_SIZE, buf, sizeof(buf))));
    }

	return 0;
}

//------------------------------------------------------------------------------
int comm_set_name(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zfone_ctx_t *ctx;   
    
	if ( in_size < sizeof(zrtp_cmd_t) + sizeof(zrtp_cmd_set_name_t) )
		return -1;
	
	ctx = manager.get_session(&manager, cmd->session_id); 
	if (ctx)
    {
		zrtp_status_t res = 0;
		zrtp_cmd_set_name_t* cmd_set_name = (zrtp_cmd_set_name_t*) ZFONE_CMD_EXT(cmd);
		uint16_t size = 0;
		
		if (cmd_set_name->name_length > sizeof(cmd_set_name->name))
			cmd_set_name->name_length = sizeof(cmd_set_name->name);
		
		//WIN64
		size = (sizeof(ctx->name.buffer) <= cmd_set_name->name_length) ? (uint16_t)sizeof(ctx->name.buffer) - 1 :
								(uint16_t)cmd_set_name->name_length;
		
		zrtp_memcpy(ctx->name.buffer, cmd_set_name->name, size);
		ctx->name.buffer[size] = 0;
		ctx->name.length = size;
		
		{
			char buf[ZFONE_SESSION_ID_SIZE*2+2];
			ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl_set_name(): changing RS name=%s length=%d for ctx%p sips %s\n",
							ctx->name.buffer,
							ctx->name.length,
							ctx,
							hex2str((const char*)cmd->session_id, ZFONE_SESSION_ID_SIZE, buf, sizeof(buf))));
		}
		res = zrtp_def_cache_put_name( ZSTR_GV(ctx->zrtp_ctx->zid),
								   ZSTR_GV(ctx->zrtp_ctx->peer_zid),
								   ZSTR_GV(ctx->name) );
		if (zrtp_status_ok != res)
			ZRTP_LOG(1, (_ZTU_, "zfoned ctrl_set_name: Can't save cache name. errno=%d.\n", res));
    }
    else
    {
		char buf[ZFONE_SESSION_ID_SIZE*2+2];
        ZRTP_LOG(1, (_ZTU_, "zfoned ctrl_set_name: zrtp session %s not found.\n",
		        hex2str((const char*)cmd->session_id, ZFONE_SESSION_ID_SIZE, buf, sizeof(buf))));
    }

	return 0;
}

//------------------------------------------------------------------------------
int comm_clean_logs(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zrtp_log_truncate();
    ZRTP_LOG(3, (_ZTU_, "zfoned ctrl_clean_logs: logs truncated\n"));
	return 0;
}

//------------------------------------------------------------------------------
int comm_set_pref(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    unsigned int state = ZRTP_CONFIG_ERROR;

	ZRTP_LOG(3, (_ZTU_, "zfoned ctrl_set_pref: got SET_ZRTP_PREF - configuring library.\n"));
    
    // update config options at first
    state = zfone_cfg.update_params((const char*)ZFONE_CMD_EXT(cmd), cmd->length,  &zfone_cfg);
    if ( ZRTP_CONFIG_OK != state )
    {
		send_streamless_error(ZFONE_ERROR_WRONG_CONFIG);
		return 0;
    }
    
    // configure Global ZRTP params
    state = zfone_cfg.configure_global(&zfone_cfg);
    if ( ZRTP_CONFIG_OK != state )
		send_streamless_error(ZFONE_ERROR_WRONG_CONFIG);
    else
		zfone_cfg.storage(&zfone_cfg); // save pref to the file

	return 0;
}

//------------------------------------------------------------------------------
int comm_set_ips(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zrtp_cmd_set_ips_t *info;
	
	if (in_size < sizeof(zrtp_cmd_t) + sizeof(zrtp_cmd_set_ips_t) )
		return -1;
	
	info = (zrtp_cmd_set_ips_t *) ZFONE_CMD_EXT(cmd);
    
	// todo: thread-unsafe! can be a couse of problems during RTP detection
    zrtp_memcpy(interfaces, info->ips, sizeof(interfaces));
    zrtp_memcpy(flags, info->flags, sizeof(flags));
    interfaces_count = info->ip_count;

	return 0;
}

//-----------------------------------------------------------------------------
int comm_set_cache_info(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zrtp_cache_info_t *cmd_cache = NULL;
	if (in_size < sizeof(zrtp_cmd_t) + sizeof(zrtp_cache_info_t))	
		return -1;

    cmd_cache = (zrtp_cache_info_t*) ZFONE_CMD_EXT(cmd);
	zrtp_set_cache_info(cmd_cache);

	return 0;
}

//-----------------------------------------------------------------------------
int comm_remember_call(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zfone_ctx_t* session = manager.get_session(&manager, cmd->session_id);

	ZRTP_LOG(3, (_ZTU_, "ZFONED comm_remember_call(): User has accepted enrollment ritual for session %p.\n", session));
    if (session)
    {
		char buff[128+ZFONE_TRUSTEDMITM_PREFIX_LENGTH];
		int i;
		for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
		{
	  		if ( session->streams[i].type != ZFONE_SDP_MEDIA_TYPE_UNKN )
				zrtp_register_with_trusted_mitm(session->streams[i].zrtp_stream);
		}		
		
		// Mark PBX secrets with special prefix and store default name to the cache
		zrtp_memset(buff, 0, sizeof(buff));
		zrtp_memcpy(buff, ZFONE_TRUSTEDMITM_PREFIX, ZFONE_TRUSTEDMITM_PREFIX_LENGTH);		
		zrtp_memcpy(buff+ZFONE_TRUSTEDMITM_PREFIX_LENGTH, session->name.buffer, session->name.length);
		session->name.length += ZFONE_TRUSTEDMITM_PREFIX_LENGTH;
		zrtp_memcpy(session->name.buffer, buff, session->name.length);

		zrtp_def_cache_put_name( ZSTR_GV(session->zrtp_ctx->zid),
								  ZSTR_GV(session->zrtp_ctx->peer_zid),								  
								  ZSTR_GV(session->name) );

		zrtp_def_cache_put_name( ZSTR_GV(session->zrtp_ctx->zid),
							 ZSTR_GV(session->zrtp_ctx->peer_zid),								  
							 ZSTR_GV(session->name) );

		ZRTP_LOG(3, (_ZTU_, "ZFONED comm_remember_call(): remember default trusted MiTM name %s.\n", buff));

		cb_refresh_conns();
    }
	
	return 0;
}

//------------------------------------------------------------------------------
int comm_hearctx(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
	zfone_ctx_t* session;
	
	if (in_size < sizeof(zrtp_cmd_t) + sizeof(zrtp_cmd_hear_ctx_t) )
		return -1;
	
	session	= manager.get_session(&manager, cmd->session_id);
	
	if ( session )
    {
		int i;
		for (i = 0; i < MAX_SDP_RTP_CHANNELS; i++)
		{
	  		if ( session->streams[i].type == cmd->stream_type )
			{
				zrtp_cmd_hear_ctx_t *extcmd = (zrtp_cmd_hear_ctx_t *) ZFONE_CMD_EXT(cmd);
				session->streams[i].hear_ctx = extcmd->disable_zrtp;
				break;
			}
		}
		cb_refresh_conns();
	}
    else
        ZRTP_LOG(1, (_ZTU_, "zfoned comm_hearctx: zrtp session was not found.\n"));
	return 0;
}


//==============================================================================
// OUTPUT commands handlers
//==============================================================================


//-----------------------------------------------------------------------------
static void conn_handler(zfone_ctx_t* ctx)
{
    zrtp_conn_info_t* conn_info = NULL;
    zrtp_session_t*  zrtp_ctx = NULL;
    int i = 0;	

    if ((conn_info_list_size >= ZFONE_MAX_CONNECTIONS_COUNT) || !ctx)
    {
		conn_info_list_size = 0;
		ZRTP_LOG(1, (_ZTU_, "ZFONED conn_handler(): ctx is %p\n", ctx));
		return;
    }

    conn_info = &conn_info_list[conn_info_list_size];
    zrtp_ctx = ctx->zrtp_ctx;

	memset(conn_info, 0, sizeof(zrtp_conn_info_t));
	ZSTR_SET_EMPTY(conn_info->name);
	ZSTR_SET_EMPTY(conn_info->zrtp_peer_client);
	ZSTR_SET_EMPTY(conn_info->zrtp_peer_version);
	ZSTR_SET_EMPTY(conn_info->sas1);
	ZSTR_SET_EMPTY(conn_info->sas2);
	
	
	// Send name if possible; if name doesn't available - send URI
    if (ctx->name.length > 0)    
		zrtp_zstrcpy(ZSTR_GV(conn_info->name), ZSTR_GV(ctx->name));		        
    else	
		zrtp_zstrcpy(ZSTR_GV(conn_info->name), ZSTR_GV(ctx->remote_uri));		    
/*
	ZRTP_LOG(3, (_ZTU_, "ZFONED conn_handler(): NAME=%s length=%d.", ctx->name.buffer, ctx->name.length));
	ZRTP_LOG(3, (_ZTU_, "ZFONED conn_handler(): URI=%s length=%d.", ctx->remote_uri.buffer, ctx->remote_uri.length));
	ZRTP_LOG(3, (_ZTU_, "ZFONED conn_handler(): sending %s=%s length=%d to GUI.",
					(ctx->name.length > 0) ? "name" : "URI",
					conn_info->name.buffer,
					conn_info->name.length));
*/
    zrtp_memcpy(conn_info->session_id, ctx->id, sizeof(zfone_session_id_t));

	if (zrtp_ctx)
    {
		zrtp_session_info_t session_info;		
		
		// Get "secure since", "verified" and SAS values for SECURE state		
		zrtp_def_cache_get_since( ZSTR_GV(zrtp_ctx->zid),
								  ZSTR_GV(zrtp_ctx->peer_zid),
								  &conn_info->secure_since);
		
		zrtp_session_get(zrtp_ctx, &session_info);
		
	
		conn_info->is_verified  = session_info.sas_is_verified;
		conn_info->matches = zrtp_ctx->secrets.matches_curr;
		conn_info->cached = zrtp_ctx->secrets.cached_curr;	

		conn_info->is_autosecure = zrtp_ctx->profile.autosecure;

		// Get crypto-options of current session
		if (zrtp_ctx->blockcipher)
			zrtp_memcpy(conn_info->cipher, zrtp_ctx->blockcipher->base.type, ZRTP_COMP_TYPE_SIZE);
		if (zrtp_ctx->authtaglength)
			zrtp_memcpy(conn_info->atl,    zrtp_ctx->authtaglength->base.type, ZRTP_COMP_TYPE_SIZE);
		if (zrtp_ctx->sasscheme)
			zrtp_memcpy(conn_info->sas_scheme,  zrtp_ctx->sasscheme->base.type, ZRTP_COMP_TYPE_SIZE);
		if (zrtp_ctx->hash)
			zrtp_memcpy(conn_info->hash,   zrtp_ctx->hash->base.type, ZRTP_COMP_TYPE_SIZE);

		// Then copy information about every stream within the session
  		for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++) 
  		{
			zrtp_streams_info_t *stream_info = &conn_info->streams[i];		
			zrtp_stream_t *zrtp_stream   = &zrtp_ctx->streams[i];		

			stream_info->state = zrtp_stream->state;
	
			if (zrtp_stream->state == ZRTP_STATE_NONE || zrtp_stream->state == ZRTP_STATE_ACTIVE)
				continue;
			
			if (!conn_info->sas1.length && zrtp_stream->state == ZRTP_STATE_SECURE &&
				(zrtp_stream->session->zrtpsess.length > 0))
			{
				zrtp_zstrcpy(ZSTR_GV(conn_info->sas1), ZSTR_GV(zrtp_stream->session->sas1));
				zrtp_zstrcpy(ZSTR_GV(conn_info->sas2), ZSTR_GV(zrtp_stream->session->sas2));
			}

			if ( !conn_info->zrtp_peer_client.length )
			{
				zrtp_memcpy( conn_info->zrtp_peer_client.buffer,
							 zrtp_stream->messages.peer_hello.cliend_id,
							 sizeof(zrtp_uchar12_t) );
				conn_info->zrtp_peer_client.length  = sizeof(zrtp_client_id_t);
				zrtp_memcpy( conn_info->zrtp_peer_version.buffer,
							 zrtp_stream->messages.peer_hello.version,
							 sizeof(zrtp_uchar4_t) );
				conn_info->zrtp_peer_version.length = ZRTP_VERSION_SIZE;
			}

			stream_info->type = ((zfone_stream_t *) zrtp_stream->usr_data)->type;
			
			if (zrtp_stream->session->secrets.cached & ZRTP_BIT_PBX)
				stream_info->pbx_reg_ok = -1;
			else
				stream_info->pbx_reg_ok = ((zfone_stream_t *) zrtp_stream->usr_data)->pbx_reg_ok;
			  			
			// Get FLAGS
  			stream_info->rx_state =	((zfone_stream_t *) zrtp_stream->usr_data)->rx_state;
  			stream_info->tx_state =	((zfone_stream_t *) zrtp_stream->usr_data)->tx_state;			
			stream_info->hear_ctx = ((zfone_stream_t *) zrtp_stream->usr_data)->hear_ctx;
			stream_info->allowclear = (zrtp_stream->allowclear && zrtp_ctx->profile.allowclear);
			stream_info->disclose_bit = zrtp_stream->peer_disclose_bit;
			
			if (zrtp_stream->pubkeyscheme)
				zrtp_memcpy(stream_info->pkt,    zrtp_stream->pubkeyscheme->base.type, ZRTP_COMP_TYPE_SIZE);
  		
			stream_info->cache_ttl = zrtp_stream->cache_ttl;
			stream_info->is_mitm = (zrtp_stream->mitm_mode == ZRTP_MITM_MODE_RECONFIRM_CLIENT);
			stream_info->passive_peer = zrtp_stream->peer_passive;
		}
	}

	conn_info_list_size++;
}

void comm_refresh_conns()
{
    zrtp_memset(conn_info_list, 0, sizeof(conn_info_list));
    conn_info_list_size = 0;
    
    manager.for_each_session(&manager, conn_handler); 
}

//-----------------------------------------------------------------------------
int get_streams_info(zrtp_cmd_update_streams_t *cmd, int out_size)
{
	int size = conn_info_list_size * sizeof(zrtp_conn_info_t);
	if ( out_size < size )	
		return -1;
	
	cmd->count = conn_info_list_size;
	zrtp_memcpy(cmd->list, conn_info_list, size);	

	return size + sizeof(cmd->count);
}
