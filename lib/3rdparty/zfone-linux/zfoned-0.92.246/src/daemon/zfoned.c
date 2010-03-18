/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <string.h>
#include <zrtp.h>

#include "zfone.h"

#define _ZTU_ "zfone main"

struct zfone_configurator	zfone_cfg;
struct zfone_heurist 		heurist;
struct zfone_manager		manager;
struct zfone_siprocessor 	siproc;

zrtp_zid_t					zid;			//!< local side zid, not for router versions
zrtp_global_t 			*zrtp_global;	//!< zrtp context for global data storing
zfone_cfg_error_t 			config_state;	//!< define configuration process status


/*
 * \brief restore ZID from file
 */
static int get_zid(const char *path, zrtp_zid_t zid, zrtp_global_t * zrtp_global);

extern int  zfoned_start_engine();
extern void zfoned_stop_engine();

extern void zrtp_protocol_event_callback(zrtp_stream_t *ctx, zrtp_protocol_event_t event);
extern void zrtp_security_event_callback(zrtp_stream_t *ctx, zrtp_security_event_t event);
extern int zrtp_send_rtp_callback(const zrtp_stream_t* ctx, char* rtp_packet, unsigned int rtp_packet_length);

//==============================================================================
//    Zfone daemon main part
//==============================================================================


//------------------------------------------------------------------------------
int zfone_init()
{
    int res = 0;	// variable for zfoned functions status
    char date[32], start_msg[128];
	zrtp_config_t zrtp_conf;

	// already done?
    //zrtp_init_loggers(LOG_PATH);
	
    // Create and initialize configurator
    res = zfone_configurator_ctor(&zfone_cfg);
    if (ZRTP_CONFIG_OK != res)
    {
		zrtp_print_log_console(3, "ZFONED create: can't create configurator. (code=%d)", res);
		return -1;
    }
    
    res = zfone_cfg.create(&zfone_cfg, CONFIG_PATH);
    if (ZRTP_CONFIG_OK != res)
    {
		zrtp_print_log_console(3, "ZFONED create: can't initialize configurator. (code=%d)", res );
		return -2;
    }
    
    // Load options from configuration file
    zrtp_set_dafault_logs(&zfone_cfg);
	    
    config_state = zfone_cfg.load(&zfone_cfg);
    if (ZRTP_CONFIG_NO == config_state)
    {
		ZRTP_LOG(2, (_ZTU_,"ZFONED create: CONFIGURATION FILE HAVEN'T BEEN FOUND. Default preferences are used.\n"));
		ZRTP_LOG(2, (_ZTU_,"ZFONED create: Send notification to GUI when connected...\n"));
    }
    if (ZRTP_CONFIG_OK != config_state)
    {
		ZRTP_LOG(2, (_ZTU_,"ZFONED create: INFO! CONFIGURATION FILE IS INCORRECT. Default preferences are used.\n"));
		ZRTP_LOG(2, (_ZTU_,"ZFONED create: INFO! Send notification to GUI when connected...\n"));
		zfone_cfg.load_defaults(&zfone_cfg);
    }
    
    zrtp_print_log_delim(3, LOG_SPACE, "");
    zrtp_print_log_delim(3, LOG_SPACE, "");

    memset(date, 0, sizeof(date));    
    snprintf(start_msg, sizeof(start_msg), " ZFONED started successfully at %s", zrtph_get_time_str(date, (int)sizeof(date)) );
    zrtp_print_log_delim(3, LOG_LABEL, start_msg);
    snprintf( start_msg,
			  sizeof(start_msg),
			  "ZRTP proto/library: %s/%s DAEMON VERSION: %d.%d.%d", ZRTP_PROTOCOL_VERSION, LIBZRTP_VERSION_STR,
			  ZFONE_MAJ_VERSION, ZFONE_SUB_VERSION, ZFONE_BUILD_NUMBER );
    
	zrtp_print_log_delim(3, LOG_START_SUBSECTION, start_msg);

    zrtp_print_log_delim(3, LOG_START_SECTION, "ZFONED INITIALIZATION");


    // Initialize ZRTP library    
	zrtp_memset(&zrtp_conf, 0, sizeof(zrtp_config_t));
	zrtp_config_defaults(&zrtp_conf);
	memcpy(zrtp_conf.client_id, ZFONE_CLIENT_ID, sizeof(zrtp_client_id_t));
	zrtp_conf.lic_mode = zfone_cfg.params.license_mode;
	
	zrtp_zstrncpyc(ZSTR_GV(zrtp_conf.def_cache_path), CACHE_PATH, strlen(CACHE_PATH));
	
	zrtp_conf.cb.event_cb.on_zrtp_protocol_event = zrtp_protocol_event_callback;
	zrtp_conf.cb.event_cb.on_zrtp_security_event = zrtp_security_event_callback;	
	zrtp_conf.cb.misc_cb.on_send_packet = zrtp_send_rtp_callback;
    if (zrtp_status_ok != zrtp_init(&zrtp_conf, &zrtp_global))
    {
		zrtp_print_log_console(1, "ZFONED voipd_init: Error during ZRTP init\n");
		return -3;
    }
    
    // Load ZID
    if ( 0 > get_zid(ZID_PATH, zid, zrtp_global) )
    {
		zrtp_print_log_console(1, "ZFONED create: can't get ZID from %s.\n", ZID_PATH);
		return -4;
    }    

	if ( 0 != (res = zrtph_init_alert(SNDS_DIR)))
	{
		zrtp_print_log_console(1, "ZFONED create: can't initialize alerts system. (code=%d)", res);
		return -5;
	}
	
	// It's time to upload random seed computed in previous sessions and initalize our RNG with this value
	{
		FILE *noncef = fopen(ZFONE_ENTROPY_PATH, "rb");
		
		ZRTP_LOG(3, (_ZTU_, "ZFONED creeate: uploading initial seed for RNG from %s.\n", ZFONE_ENTROPY_PATH));
		if (noncef)
		{
			char buff[ZFONE_ENTROPY_SIZE];
			if (1 != fread(buff, ZFONE_ENTROPY_SIZE, 1, noncef))
				zrtp_entropy_add(zrtp_global, (const unsigned char*)buff, ZFONE_ENTROPY_SIZE);
			fclose(noncef);				
			ZRTP_LOG(3, (_ZTU_, "ZFONED creeate: %d bytes of entropy were added to the RNG routing.\n", ZFONE_ENTROPY_SIZE));
		}
		else
		{
			ZRTP_LOG(3, (_ZTU_, "ZFONED creeate: uploading initial seed for RNG - NO data is available.\n"));
		}
	}
		    
    // Create and initialize zfone connection MANAGER
	zfone_manager_initialize_ip_list();
    if (0 != (res = zfone_manager_ctor(&manager)))
    {
		zrtp_print_log_console(1, "ZFONED create: can't create connection manager. (code=%d)\n", res);
		return -6;
    }
    if (0 != (res = manager.create(&manager, &zid)))
    {
		zrtp_print_log_console(1, "ZFONED create: can't initialize connection-manager. (code=%d)\n", res );
		return -7;
    }
    
    // Create and initialize zfone NETWORKER	
	if (0 != (res = zfone_network_init(IPF_PATH) < 0))
	{
		zrtp_print_log_console(1, "ZFONED create: can't initialize network. (code=%d)\n", res);
		return -8;
	}    

    // Create and initialize zfone SIP PROCESSOR
    if (0 != (res = zfone_siproc_ctor(&siproc)))
    {
		zrtp_print_log_console(1, "ZFONED create: can't create siprocessor. (code=%d)", res);
		return -9;
    }
    if (0 != (res = siproc.create(&siproc, &zid)))
    {
		zrtp_print_log_console(1, "ZFONED create: can't initialize siprocessor. (code=%d)", res);
		return -10;
    }    
    
    // Create and initialize zfone HEURIST
    if (0 != (res = zfone_heurist_ctor(&heurist)))
    {
		zrtp_print_log_console(1, "ZFONED create: can't create heurist. (code=%d)", res);
		return -11;
    }
    if (0 != (res = heurist.create(&heurist)))
    {
		zrtp_print_log_console(1, "ZFONED create: can't initialize heurist. (code=%d)", res);
		return -12;
    }
    
    // Create and start zfone SCHEDULLER
    if (0 != (res = zfone_checker_start()))
    {
		zrtp_print_log_console(1, "ZFONED create: can't create rtp checker. (code=%d)", res);
		return -13;
    }
	            
    zrtp_print_log_delim(3, LOG_END_SECTION, "ZFONED INITIALIZATION");
    zrtp_print_log_delim(3, LOG_SPACE, "");

    return 0;
}

//------------------------------------------------------------------------------
void zfone_down()
{
    // save config options to file and destroy configurator
    zfone_cfg.storage(&zfone_cfg);        
    zfone_cfg.destroy(&zfone_cfg);

	zfone_network_down();	    
    
    siproc.destroy(&siproc);    
    heurist.destroy(&heurist);
    manager.destroy(&manager);

	zfone_checker_stop();

	// It's time to store some amount of entropy computed during this zfone session.
	// We will use it on next zfone start to improve quality of default OS RNG.
	{
		FILE *noncef = fopen(ZFONE_ENTROPY_PATH, "wb+");		
		if (noncef)
		{
			unsigned char buff[ZFONE_ENTROPY_SIZE];
			zrtp_randstr(zrtp_global, buff, ZFONE_ENTROPY_SIZE);
			fwrite(buff, ZFONE_ENTROPY_SIZE, 1, noncef);
			fclose(noncef);
			ZRTP_LOG(3, (_ZTU_, "ZFONED down: storing %d bytes of entropy to %s.\n", ZFONE_ENTROPY_SIZE, ZFONE_ENTROPY_PATH));
		}
	}

    // Deinitialize ZRTP library
    zrtp_down(zrtp_global);
    
    // down link to GUI
    stop_ctrl_server();

    // close ZRTP logger
    zrtp_logger_reset();
}

//------------------------------------------------------------------------------    
int zfone_start()
{
    zrtp_print_log_delim(3, LOG_START_SECTION, "ZFONED STARTING");
    
	// Try to remove rules if they are present in ipfilter tables for some reason,
	// Looks like we don't need this trick on Mac OS because we use ipfw API
#if ZRTP_PLATFORM != ZP_DARWIN
    zfone_network_add_rules(0, voip_proto_UDP);
    zfone_network_add_rules(0, voip_proto_TCP);	
#endif
	
    // Configuring SIP/RTP detection unite
    if (zfone_manager_refresh_ip_list())
		send_set_ips(interfaces, flags, interfaces_count);	
    
    manager.config_sip(&manager, &zfone_cfg);
        
    // Runing main processing loop
    if (0 != zfoned_start_engine())
    {
		zrtp_print_log_console(1, "ZFONED start: can't start main procesing loop.\n");
		return -1;
    }
    
    // Inserting rules to ipfilter and start packet snifing 
    if (0 != zfone_network_add_rules(1, voip_proto_UDP))
    {
		zrtp_print_log_console(1, "ZFONED start: can't insert UDP rules to ipfilter.\n");
		return -2;
    }
    if (zfone_cfg.params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_TCP)
    {
		if (0 != zfone_network_add_rules(1, voip_proto_TCP))
		{
		    zrtp_print_log_console(1, "ZFONED start: can't insert TCP rules to ipfilter.\n");
		    return -3;
		}
    }    
    
    zrtp_print_log_delim(3, LOG_END_SECTION, "ZFONED STARTING");
    zrtp_print_log_delim(3, LOG_SPACE, "");

    ZRTP_LOG(3, (_ZTU_, "================================================================\n"));

    return 0;
}

//------------------------------------------------------------------------------    
void zfone_stop()
{
    zrtp_print_log_delim(3, LOG_START_SECTION, "stopping IP filter");
    
	zfone_network_add_rules(0, voip_proto_UDP);
	zfone_network_add_rules(0, voip_proto_TCP);
	// Try to remove rules if they are present in ipfilter tables for some reason,
	// Looks like we don't need this trick on Mac OS because we use ipfw API
#if ZRTP_PLATFORM != ZP_DARWIN
//	printf("zfoned: Strings below are not a reason for excitement. Zfone makes such\n");
//	printf("zfoned: tricks to keep your firewall tables clean in case of zfone crashes.\n");
	zfone_network_add_rules(0, voip_proto_UDP);
	zfone_network_add_rules(0, voip_proto_TCP);
#endif
	
    zfoned_stop_engine();
}


//------------------------------------------------------------------------------    
zfone_cfg_error_t zfone_get_config_state()
{
    return config_state;
}

void zfone_reset_config_state()
{
    config_state = 0;
}


/*----------------------------------------------------------------------------*/
static int get_zid(const char *path, zrtp_zid_t zid, zrtp_global_t * zrtp_global)
{
	FILE* zidf = fopen(path, "r+");
    
	ZRTP_LOG(3, (_ZTU_, "zrtph_get_zid: reading of ZID from '%s' started\n", path));
    
    if ( zidf )
    {
		ZRTP_LOG(3, (_ZTU_, "zrtph_get_zid: ZID file opened. Reading string.\n"));
		fseek(zidf, 0, SEEK_END);
		if ( sizeof(zrtp_zid_t) == ftell(zidf) )
		{
		    fseek(zidf, 0, SEEK_SET);
		    fread(zid, sizeof(zrtp_zid_t), 1, zidf);
		    fclose(zidf);
	    
		    ZRTP_LOG(3, (_ZTU_, "zrtph_get_zid: ZID has been read\n"));
		}
		else
		{
			fclose(zidf);
		    ZRTP_LOG(3, (_ZTU_, "zrtph_get_zid: wrong ZID file size - generate new.\n"));
		}
    }
    else
    {    
		ZRTP_LOG(3, (_ZTU_, "zrtph_get_zid: generating ZID.\n"));
		zrtp_randstr(zrtp_global, zid, sizeof(zrtp_zid_t));
      
		ZRTP_LOG(3, (_ZTU_, "zrtph_get_zid: writing new ZID to file.\n"));
		zidf = fopen(path, "w");
		if (!zidf)
		{
		    ZRTP_LOG(1, (_ZTU_, "zrtph_get_zid: Can't create file: '%s' for ZID generation.\n", path));
		    return -1;
		}
		fwrite(zid, sizeof(zrtp_zid_t), 1, zidf);
		fclose(zidf);
		ZRTP_LOG(3, (_ZTU_, "get_zid: ZID has been created and saved.\n"));
    }

    return 0;
}
