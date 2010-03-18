/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>
#include "zfone.h"


#define CONVINT(val, is_htonl) is_htonl ? zrtp_hton32(val) : zrtp_ntoh32(val)
#define CONVINT_(val, is_htonl) val = is_htonl ? zrtp_hton32(val) : zrtp_ntoh32(val)
#define CONVSHORT(val, is_htonl) is_htonl ? zrtp_hton16(val) : zrtp_ntoh16(val)
#define CONVSHORT_(val, is_htonl) val = CONVSHORT(val, is_htonl)

//------------------------------------------------------------------------------
static void convert(zrtp_cmd_t* cmd, int is_htonl)
{
    unsigned int  code = is_htonl ? cmd->code : zrtp_ntoh32(cmd->code);

    // abstract command part convertion
    cmd->stream_type	= CONVSHORT(cmd->stream_type, is_htonl);
    cmd->code		= CONVINT(cmd->code, is_htonl);
    cmd->length		= CONVINT(cmd->length, is_htonl);

    // special command part convertion
    switch ( code )
    {
	case UPDATE_STREAMS:
	{
	    unsigned int i, j, count;
	    zrtp_cmd_update_streams_t *update_cmd = (zrtp_cmd_update_streams_t *) ZFONE_CMD_EXT(cmd);

	    count = update_cmd->count;
	    CONVINT_(update_cmd->count, is_htonl);
	    count = is_htonl ? count : update_cmd->count;	    
	    for (i=0; i < count; i++)
	    {
			zrtp_conn_info_t *info = &update_cmd->list[i];
			CONVINT_(info->secure_since, is_htonl);			
			CONVSHORT_(info->name.length, is_htonl);
			CONVSHORT_(info->name.max_length, is_htonl);
	  		CONVSHORT_(info->zrtp_peer_client.length, is_htonl);
			CONVSHORT_(info->zrtp_peer_client.max_length,  is_htonl);
			CONVSHORT_(info->zrtp_peer_version.length, is_htonl);
			CONVSHORT_(info->zrtp_peer_version.max_length,  is_htonl);
			CONVSHORT_(info->sas1.length,  is_htonl);
			CONVSHORT_(info->sas1.max_length,  is_htonl);
	  		CONVSHORT_(info->sas2.length,  is_htonl);
			CONVSHORT_(info->sas2.max_length,  is_htonl);
			CONVINT_(info->is_verified, is_htonl);			
			CONVINT_(info->matches, is_htonl);			
			CONVINT_(info->cached, is_htonl);			
			CONVINT_(info->is_autosecure, is_htonl); 
	  		for (j = 0; j < ZRTP_MAX_STREAMS_PER_SESSION; j++)
			{
				zrtp_streams_info_t *stream_ = &info->streams[j];
				CONVSHORT_(stream_->pbx_reg_ok, is_htonl);
				CONVSHORT_(stream_->hear_ctx, is_htonl);
				CONVINT_(stream_->cache_ttl, is_htonl);
				CONVINT_(stream_->passive_peer, is_htonl);
				CONVSHORT_(stream_->state,  is_htonl);
				CONVINT_(stream_->disclose_bit, is_htonl);
				CONVSHORT_(stream_->type,  is_htonl);
			}
	    }
	} break;

    case SET_VERIFIED:
    {
	    zrtp_cmd_set_verified_t* verefied_cmd = (zrtp_cmd_set_verified_t*) ZFONE_CMD_EXT(cmd);
	    verefied_cmd->is_verified	= CONVINT(verefied_cmd->is_verified, is_htonl);
    } break;

	case SET_NAME:
    {
	    zrtp_cmd_set_name_t* name_cmd = (zrtp_cmd_set_name_t*) ZFONE_CMD_EXT(cmd);
	    name_cmd->name_length		= CONVINT(name_cmd->name_length, is_htonl);
    } break;

	case SEND_VERSION:
	{
	    zrtp_cmd_send_version_t* version_cmd = (zrtp_cmd_send_version_t*) ZFONE_CMD_EXT(cmd);
	    version_cmd->is_manually	= CONVINT(version_cmd->is_manually, is_htonl);
	    version_cmd->is_expired		= CONVINT(version_cmd->is_expired, is_htonl);
	    version_cmd->curr_version	= CONVINT(version_cmd->curr_version, is_htonl);
	    version_cmd->new_version	= CONVINT(version_cmd->new_version, is_htonl);
	    version_cmd->curr_build		= CONVINT(version_cmd->curr_build, is_htonl);
	    version_cmd->new_build		= CONVINT(version_cmd->new_build, is_htonl);
	    version_cmd->url_length		= CONVINT(version_cmd->url_length, is_htonl);
	} break;

	case ZRTP_PACKET:
    {
	    zrtp_cmd_zrtp_packet_t* packet_cmd = (zrtp_cmd_zrtp_packet_t*) ZFONE_CMD_EXT(cmd);
	    packet_cmd->direction		= CONVINT(packet_cmd->direction, is_htonl);
	    packet_cmd->packet_type		= CONVINT(packet_cmd->packet_type, is_htonl);
	    
    } break;

	case PLAY_ALERT:
	{
	    zrtp_cmd_alert_t* alert_cmd = (zrtp_cmd_alert_t*) ZFONE_CMD_EXT(cmd);
	    alert_cmd->alert_type		= CONVINT(alert_cmd->alert_type, is_htonl);
	    
	} break;

	case HEAR_CTX:
	{
	    zrtp_cmd_hear_ctx_t* hear_cmd = (zrtp_cmd_hear_ctx_t*) ZFONE_CMD_EXT(cmd);
	    hear_cmd->disable_zrtp = CONVINT(hear_cmd->disable_zrtp, is_htonl);
	}break;
	case GET_DEFAULTS:
	{
		if (!is_htonl)
			break;
	}
	case SET_PREF:
	{
		int i;
		zfone_params_t* c = (zfone_params_t*) ZFONE_CMD_EXT(cmd);
		for (i = 0; i < ZRTP_MAX_SIP_PORTS_FOR_SCAN; i++)
		{
			CONVSHORT_(c->sniff.sip_ports[i].port, is_htonl);
			CONVSHORT_(c->sniff.sip_ports[i].proto, is_htonl);
		}
		CONVINT_(c->zrtp.cache_ttl, is_htonl);
		CONVSHORT_(c->license_mode, is_htonl);
		CONVSHORT_(c->hear_ctx, is_htonl);
	} break;

	case SET_IPS:
	{
		int i;
		zrtp_cmd_set_ips_t* c = (zrtp_cmd_set_ips_t*) ZFONE_CMD_EXT(cmd);
		CONVINT_(c->ip_count, is_htonl);
		for (i = 0; i < ZFONE_MAX_INTERFACES_COUNT; i++)
		{
			CONVINT_(c->ips[i], is_htonl);
		}
	} break;
  case SET_CACHE_INFO:
	{
		zrtp_cache_info_t* info = (zrtp_cache_info_t*)ZFONE_CMD_EXT(cmd);
		int i, count = info->count;
		CONVINT_(info->count, is_htonl);
		if (!is_htonl)
			count = info->count; 
		for (i = 0; i < count; i++)
		{
			zrtp_cache_info_record_t *record = &info->list[i];
			CONVINT_(record->time_created, is_htonl);
			CONVINT_(record->time_accessed, is_htonl);
		}
	}break;
	default:
		break;
    }// command type switch    
}

//------------------------------------------------------------------------------
void zfone_cmd_hton(zrtp_cmd_t* cmd)
{
    if ( cmd )
	convert(cmd, 1); 
}

//------------------------------------------------------------------------------
void zfone_cmd_ntoh(zrtp_cmd_t* cmd)
{
    if ( cmd ) 
	convert(cmd, 0);
}

