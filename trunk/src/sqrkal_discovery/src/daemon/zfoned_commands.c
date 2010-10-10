/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok <chaser@soft-industry.com>
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>
#include "zfone.h"

#include <sys/time.h>

#define _ZTU_ "zfoned commands"

static const unsigned int expiration_date_sec =  1234094906 + 60*60*24*20; //date +%s
static const unsigned int expiration_interval = 60*60*24;
static unsigned int expiration_last_check = 0;

void cb_refresh_conns();

//------------------------------------------------------------------------------
static void send_pref()
{
    ZRTP_LOG(3, (_ZTU_, "zfoned send_pref: send PREFERENCES command to GUI.\n"));
    send_cmd(SET_PREF, NULL, ZFONE_SDP_MEDIA_TYPE_UNKN, &zfone_cfg.params, sizeof(zfone_params_t));
}

static int ctrl_refresh(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zrtp_print_log_delim(3, LOG_START_SELECT, "restoring GUI");

    // get configuration state
    switch ( zfone_get_config_state() )
    {
	case ZRTP_CONFIG_NO: // send notification about conf file missing	
	    ZRTP_LOG(3, (_ZTU_, "zfoned ctrl_refresh: send notification"
									   " about configuration file missing.\n"));
	    send_streamless_error(ZFONE_ERROR_NO_CONFIG);
	    zfone_reset_config_state();
		break;

	case ZRTP_CONFIG_ERROR: // send notification about errors in conf file
	    ZRTP_LOG(3, (_ZTU_, "zfoned ctrl_refresh: send notification about"
									   " errors in configuration file.\n"));
	    send_streamless_error(ZFONE_ERROR_WRONG_CONFIG);
	    zfone_reset_config_state();
		break;

	default:
		break;
    }
    // send preferences
    send_pref();
    
    // and then update current interfaces list
    send_set_ips(interfaces, flags, interfaces_count);

    cb_refresh_conns();
	zfone_checker_reset_rtp_activity();
    
    // chech for updates and send necessary commands to GUI
    check_for_updates(0);    
	
	return 0;
}

//------------------------------------------------------------------------------
static int ctrl_stop(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    ZRTP_LOG(3, (_ZTU_, "zfoned ctrl_stop: got STOPPED from GUI.\n"));
    
    // send STOPPED command to GUI for featback
    send_stop_cmd();
	    
    // stopping daemon
    zfone_stop();
    zfone_down();
	
	return 0;
}

//------------------------------------------------------------------------------
static int ctrl_reset(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    ZRTP_LOG(3, (_ZTU_, "zfoned ctrl_refresh: got REFRESH from GUI.\n"));
    siproc.refresh(&siproc);
	heurist.reset(&heurist);

	return 0;
}

//------------------------------------------------------------------------------
static int ctrl_get_defaults(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zfone_cfg.load_defaults(&zfone_cfg);
    send_cmd( GET_DEFAULTS,
			  NULL,
			  ZFONE_SDP_MEDIA_TYPE_UNKN,
			  &zfone_cfg.params,
			  sizeof(zfone_params_t) );
	return 0;
}

static int ctrl_get_cache_info(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
	zrtp_cache_info_t *cmd_cache = zrtp_sys_alloc(sizeof(zrtp_cache_info_t));	
	if (!cmd_cache) 
		return -1;
		
    zrtp_get_cache_info(cmd_cache);
    send_cmd( SET_CACHE_INFO,
			  NULL,
			  0,
			  cmd_cache,
			  sizeof(cmd_cache->count) + (cmd_cache->count * sizeof(zrtp_cache_info_record_t)) );
	zrtp_sys_free(cmd_cache);

	return 0;
}

//------------------------------------------------------------------------------
void init_ctrl_callbacks()
{
    register_ctrl_callback(GO_SECURE,      comm_secure);
    register_ctrl_callback(GO_CLEAR,       comm_clear);
    register_ctrl_callback(REFRESH,        ctrl_refresh);
    register_ctrl_callback(SET_VERIFIED,   comm_set_verified);
    register_ctrl_callback(SET_NAME,       comm_set_name);        
    register_ctrl_callback(GET_VERSION,    force_check_for_updates);
    register_ctrl_callback(SET_PREF,       comm_set_pref);
    register_ctrl_callback(GET_DEFAULTS,   ctrl_get_defaults);
    register_ctrl_callback(CLEAN_LOGS,     comm_clean_logs);
    register_ctrl_callback(STOPPED,        ctrl_stop);            
    register_ctrl_callback(RESET_DAEMON,   ctrl_reset);
    register_ctrl_callback(SET_IPS,        comm_set_ips);
    register_ctrl_callback(GET_CACHE_INFO, ctrl_get_cache_info);
    register_ctrl_callback(SET_CACHE_INFO, comm_set_cache_info);
    register_ctrl_callback(REMEMBER_CALL,  comm_remember_call);
    register_ctrl_callback(HEAR_CTX,	   comm_hearctx);
}


//==============================================================================
//         Utility functions
//==============================================================================


//------------------------------------------------------------------------------
void send_stop_cmd()
{    
    send_cmd(STOPPED, NULL, ZFONE_SDP_MEDIA_TYPE_UNKN, NULL, 0);
}

//------------------------------------------------------------------------------
void send_crash_cmd()
{
    send_cmd(CRASHED, NULL, ZFONE_SDP_MEDIA_TYPE_UNKN, NULL, 0);
}

//------------------------------------------------------------------------------
void send_set_ips(const uint32_t *ips, const uint8_t *flags, unsigned int count)
{
    zrtp_cmd_set_ips_t cmd;

    memcpy(cmd.ips, ips, sizeof(cmd.ips));
    memcpy(cmd.flags, flags, sizeof(cmd.flags));
    cmd.ip_count = count;
    
    send_cmd(SET_IPS, NULL, ZFONE_SDP_MEDIA_TYPE_UNKN, &cmd, sizeof(cmd));
}

//------------------------------------------------------------------------------
void send_start_cmd()
{
    send_cmd(STARTED, NULL, ZFONE_SDP_MEDIA_TYPE_UNKN, NULL, 0);
}

//------------------------------------------------------------------------------
void send_streamless_error(uint32_t error)
{
    zrtp_cmd_error_t cmd;
    cmd.error_code = error;
    send_cmd(ERROR, NULL, ZFONE_SDP_MEDIA_TYPE_UNKN, &cmd, sizeof(cmd));
}

//-----------------------------------------------------------------------------
void cb_is_in_state_error(zrtp_stream_t *stream_ctx)
{
    zfone_ctx_t* ctx = (zfone_ctx_t*) stream_ctx->session->usr_data;
	zfone_stream_t* zstream = (zfone_stream_t*) stream_ctx->usr_data;
    zrtp_cmd_error_t cmd;
	memset(&cmd, 0, sizeof(cmd));
	    
    ZRTP_LOG(1, (_ZTU_, "ZFONED cb_is_in_state_error: ERROR CODE=%d.\n",
					stream_ctx->last_error));
	ZRTP_LOG(3, (_ZTU_, "ZFONED cb_is_in_state_error: zrtp_state=%d. %d\n",
					stream_ctx->state, stream_ctx->last_error));

	cmd.error_code = stream_ctx->last_error;	
    send_cmd(ERROR, ctx->id, zstream->type, &cmd, sizeof(cmd));
		
	cb_refresh_conns();	
}

//------------------------------------------------------------------------------
void cb_rtp_activity(zfone_stream_t *zstream, unsigned int direction, unsigned int type)
{
    zrtp_cmd_zrtp_packet_t cmd_packet;
    zfone_ctx_t* ctx = zstream->zfone_ctx;

    memset(&cmd_packet, 0, sizeof(cmd_packet));
    cmd_packet.direction = direction;
    cmd_packet.packet_type = type;

    send_cmd(ZRTP_PACKET, ctx->id, zstream->type, &cmd_packet, sizeof(cmd_packet));

	if (type == ZFONED_ALERTED_NO_ACTIV && direction == ZFONE_IO_OUT &&
		 zfone_cfg.params.alert && zstream->zrtp_stream->state == ZRTP_STATE_SECURE)
	{
		zstream->alert_code = ZRTP_ALERT_PLAY_NOACTIVE;
		zrtp_play_alert(zstream);
	}
}

//------------------------------------------------------------------------------
void cb_refresh_conns()
{
    zrtp_cmd_update_streams_t *cmd = zrtp_sys_alloc(sizeof(zrtp_cmd_update_streams_t));

	comm_refresh_conns();

	get_streams_info(cmd, sizeof(zrtp_cmd_update_streams_t));	
    send_cmd( UPDATE_STREAMS,
			  NULL,
			  0,
			  cmd, sizeof(cmd->count) + cmd->count * sizeof(zrtp_conn_info_t) );
	zrtp_sys_free(cmd);
}

//------------------------------------------------------------------------------
void cb_send_version( unsigned int curr_version, unsigned int new_version,
					  unsigned int curr_build, unsigned int new_build,
					  const char* url, int url_length, int is_expire, int is_manually )
{
    zrtp_cmd_send_version_t cmd;

    memset(&cmd, 0, sizeof(cmd));

    ZRTP_LOG(3, (_ZTU_, "ZFONED cb_send_version: sent SEND_VERSION command"
								   " to GUI (currv=%u, newv=%u is expare:%d.).\n",
		curr_version, new_version, is_expire ));
    ZRTP_LOG(3, (_ZTU_, "ZFONED cb_send_version: sent BUILD to GUI (currv=%u, newv=%u).\n",
		curr_build, new_build));

    cmd.is_manually	= is_manually;
    cmd.is_expired	= is_expire;
    cmd.curr_version	= curr_version;
    cmd.new_version  = new_version;
    cmd.curr_build	= curr_build;
    cmd.new_build	= new_build;

    cmd.url_length = (url_length < ZRTP_CMD_STR_LENGTH) 
					  ? url_length
					  : ZRTP_CMD_STR_LENGTH;
    strncpy(cmd.url, url, cmd.url_length);

	memcpy(cmd.zrtp_version, ZRTP_VERSION, sizeof(ZRTP_VERSION));
  
    send_cmd(SEND_VERSION, 0, 0, &cmd, sizeof(cmd));
}

#if ZRTP_PLATFORM == ZP_DARWIN
void zrtph_alert_play(zfone_stream_t* stream)
{
    zrtp_cmd_alert_t    cmd_alert;
    memset(&cmd_alert, 0, sizeof(zrtp_cmd_alert_t));		
			    
    switch (stream->alert_code)
    {
    case ZRTP_ALERT_PLAY_SECURE:
		cmd_alert.alert_type = ZRTP_ALERT_PLAY_SECURE;
		break;
	case ZRTP_ALERT_PLAY_CLEAR:
		cmd_alert.alert_type = ZRTP_ALERT_PLAY_CLEAR;
        break;
	case ZRTP_ALERT_PLAY_ERROR:    
		cmd_alert.alert_type = ZRTP_ALERT_PLAY_ERROR;
        break;
	case ZRTP_ALERT_PLAY_NOACTIVE:
		cmd_alert.alert_type = ZRTP_ALERT_PLAY_NOACTIVE;
        break;
    default:   
	   ZRTP_LOG(1, (_ZTU_, "zrtp_play_alart: ERROR! Unsupported alert tupe:%d.\n", stream->alert_code));
       return;       
    }
	
    send_cmd(PLAY_ALERT, 0, stream->type, &cmd_alert, sizeof(cmd_alert));
}
#endif

//------------------------------------------------------------------------------
void check_for_updates(int force)
{
    struct timeval	time_now;
    gettimeofday(&time_now, 0);    
    
    if (!force)
    {
		cb_send_version(ZFONE_VERSION, 0, ZFONE_BUILD_NUMBER, 0, "", 0, 0, 0); // send current version only
		if ((time_now.tv_sec - expiration_last_check) < expiration_interval)
			return;
    }
    
    expiration_last_check = time_now.tv_sec;
	zfone_check4updates( zfone_str2ip("69.49.175.144"),
						 "philzimmermann.com",
						 "",
						 force );
}

//------------------------------------------------------------------------------
int force_check_for_updates(zrtp_cmd_t *cmd, uint32_t in_size, uint32_t out_size)
{
    zrtp_print_log_delim(3, LOG_START_SUBSECTION, "Checking for updates");
    check_for_updates(1);
	return 0;
}

//------------------------------------------------------------------------------
void zfone_check4updates_done(zfone_version_t *version, int result, unsigned int force)
{
	int is_expired = 0;
    struct timeval	time_now;
    gettimeofday(&time_now, 0);
    
    if (0 > result)
    {
		// no connection with Zfone server - send error mesage to GUI
        ZRTP_LOG(1, (_ZTU_, "zfoned check_for_updates: checking canceled"
									   " - can't connect to the zrtp server.\n"));
		zrtp_print_log_delim(3, LOG_END_SUBSECTION, "Checking for updates");
		cb_send_version(ZFONE_VERSION, 0, ZFONE_BUILD_NUMBER, 0, 0, 0, 2, force);
		return;
    }
    
    ZRTP_LOG(3, (_ZTU_, "zfoned check_for_updates: version on the server"
								   " is %s/%d, url for updates:%s build %d.\n",
		    version->version_str.buffer, version->version_int, version->url.buffer, version->build));
    
    // first of all check date
    if ((unsigned int)time_now.tv_sec > expiration_date_sec)
    {
        // expiration date is up - check for new version
        if ( (version->version_int > ZFONE_VERSION) || 
	     ((version->version_int == ZFONE_VERSION) && (version->build > ZFONE_BUILD_NUMBER)) )
        {
            ZRTP_LOG(3, (_ZTU_, "zfoned check_for_updates: Time is out"
										   " and new version is available.\n"));
			ZRTP_LOG(3, (_ZTU_, "zfoned check_for_updates: Send new"
										   " version to GUI with command STOP!\n"));
			is_expired =1;
        }
		else
		{
			ZRTP_LOG(3, (_ZTU_, "zfoned check_for_updates: Time is out"
							" (now=%lu ed=%u), but new version isn't availbale.\n",
							time_now.tv_sec, expiration_date_sec));
		}
    }
    zrtp_print_log_delim(3, LOG_END_SUBSECTION, "Checking for updates");

    cb_send_version( ZFONE_VERSION, version->version_int,
					 ZFONE_BUILD_NUMBER, version->build,
					 version->url.buffer, version->url.length, is_expired, force);
    
    if (is_expired)	// stop daemon if current Zfone version is expired
	zfone_stop();
}
