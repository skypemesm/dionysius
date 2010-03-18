/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok mailto: <chaser@soft-industry.com>
 */


#ifndef	__ZFONEG_CONFIG_H__
#define __ZFONEG_CONFIG_H__

#include <zrtp.h>

#define	ZFONE_SAS_SCHEME_COUNT	2

#define	SECRET_NEVER_EXPIRES	0xFFFFFFFF
#define	SECRET_DEFAULT_EXPIRE	30

#define ZRTP_COMP_COUNT			5

#define PROTO_UDP				17
#define PROTO_TCP				6


#define ZRTP_MAX_SIP_PORTS_FOR_SCAN 15
#define ZRTP_SIP_PORT_DESC_MAX_SIZE 48

#define ZRTP_BIT_SIP_SCAN_PORTS	0x01
#define ZRTP_BIT_SIP_SCAN_UDP	0x02
#define ZRTP_BIT_SIP_SCAN_TCP	0x04

#define ZRTP_BIT_RTP_DETECT_SIP 0x01
#define ZRTP_BIT_RTP_DETECT_RTP 0x02

typedef enum zfone_detection_mode
{
    ZFONE_DETECT_SIP_REQUEST_MODE	= 0,
    ZFONE_DETECT_SIP_RESPONSE_MODE	= 1,
    ZFONE_DETECT_SIP_AUTO_MODE		= 2,
    ZFONE_DETECT_SIP_MODES_COUNT	    
} zfone_detection_mode_t;

typedef struct zfone_sip_port
{
    unsigned short port;
    unsigned short proto;
    char	   desc[ZRTP_SIP_PORT_DESC_MAX_SIZE];
} zfone_sip_port_t;

typedef struct zfone_sniff_params
{
    zfone_sip_port_t		sip_ports[ZRTP_MAX_SIP_PORTS_FOR_SCAN];
    unsigned char			sip_scan_mode;
    unsigned char       	rtp_detection_mode;
} zfone_sniff_params_t;

typedef struct zfone_params
{
    zrtp_profile_t			zrtp;
    zfone_sniff_params_t	sniff;
    unsigned char			is_debug;
    unsigned char			print_debug;
    unsigned char			alert;
	unsigned char			is_ec;
	uint16_t				license_mode;
	uint16_t				hear_ctx;
} zfone_params_t;

zfone_params_t 				params;

// add value to profile element
int addParam(zrtp_crypto_comp_t type, int aValue);

// insert value to profile element with given priority
int insertParam(zrtp_crypto_comp_t type, char* aValue, int aPriority);

// clear all profile elements
void clearParams();

#endif
