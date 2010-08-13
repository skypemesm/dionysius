/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 * Nikolay Popok <chaser@soft-industry.com>
 */

#ifndef __ZFONED_CFG_H__
#define __ZFONED_CFG_H__

#include <zrtp.h>

#include "zfone_types.h"
#include "zfone_log.h"
 

#if ZRTP_PLATFORM != ZP_WIN32_KERNEL
	#define CONFIG_FILE_NAME_SIZE	ZFONE_MAX_FILENAME_SIZE
#else
	#include <ndis.h>
	
	#define LOG_PATH				"C:\\zrtp.log" //TODO: don't use this path
	#define CONFIG_REGISTRY_PATH	L"\\Registry\\Machine\\SOFTWARE\\Zfone\\Config"
	#define	REGISTRY_PATH_SIZE		256
#endif

typedef enum zfone_cfg_error
{
    ZRTP_CONFIG_ERROR	= -1,
    ZRTP_CONFIG_OK		= 0,
    ZRTP_CONFIG_NO		= 1
} zfone_cfg_error_t;


//=============================================================================
//    Base params definition					      						  
//=============================================================================


#define SECRET_NEVER_EXPIRES		0xFFFFFFFF

#define ZRTP_MAX_SIP_PORTS_FOR_SCAN 15
#define ZRTP_SIP_PORT_DESC_MAX_SIZE 48

#define ZRTP_BIT_SIP_SCAN_PORTS		0x01
#define ZRTP_BIT_SIP_SCAN_UDP		0x02
#define ZRTP_BIT_SIP_SCAN_TCP		0x04

#define ZRTP_BIT_RTP_DETECT_SIP		0x01
#define ZRTP_BIT_RTP_DETECT_RTP		0x02

typedef struct zfone_sip_port
{
    unsigned short 		port;
    unsigned short 		proto;
    char	   			desc[ZRTP_SIP_PORT_DESC_MAX_SIZE];
} zfone_sip_port_t;

typedef struct zfone_sniff_params
{
    zfone_sip_port_t	sip_ports[ZRTP_MAX_SIP_PORTS_FOR_SCAN];
    unsigned char		sip_scan_mode;
	unsigned char		rtp_detection_mode;
} zfone_sniff_params_t;

typedef struct zfone_params
{
	zrtp_profile_t			zrtp;
	zfone_sniff_params_t	sniff;
	uint8_t					is_debug; 
	uint8_t					print_debug;
	uint8_t					alert;
	uint8_t					is_ec;
	uint16_t				license_mode;
	uint16_t				hear_ctx;
} zfone_params_t;


//=============================================================================
//    Action definitions part												  
//=============================================================================


typedef struct zfone_logger_info
{
	char			file_name[FILES_NAME_SIZE];
    int				log_mode;
} zfone_logger_info_t;

/*!
 * \brief zfone configurator
 */
struct zfone_configurator
{
    zfone_params_t	params;								//!< configuration params
   
#if ZRTP_PLATFORM != ZP_WIN32_KERNEL
    pthread_mutex_t	lock;        						//!< lock to serialize access to the @fds
    char			file_name[CONFIG_FILE_NAME_SIZE];	//!< full path to the configuration file

    zfone_logger_info_t	loggers[LOG_ELEMS_COUNT];
    int				loggers_count;

    unsigned int	update_timeout;
    unsigned int	storing_period;
#else
    WCHAR			registry_path[REGISTRY_PATH_SIZE];
#endif
    unsigned int	max_size;
    
    int   (*create)(struct zfone_configurator* config, void* fname);
    void  (*destroy)(struct zfone_configurator* config);
    
    int   (*load)(struct zfone_configurator* config);
    void  (*load_defaults)(struct zfone_configurator* config);
    int   (*storage)(const struct zfone_configurator* config);
    
    int   (*configure_global)(struct zfone_configurator* config);    
    int	  (*update_params)(const char* params, int params_length, struct zfone_configurator* config);
};


extern void zrtp_set_dafault_logs(struct zfone_configurator* config);
extern int zfone_configurator_ctor(struct zfone_configurator* config);
extern struct zfone_configurator zfone_cfg;

#endif //__ZFONED_CFG_H__
