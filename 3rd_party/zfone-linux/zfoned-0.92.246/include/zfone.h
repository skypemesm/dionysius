/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONE_H__
#define __ZFONE_H__

#include <zrtp.h>

#include "zfone_system.h"
#include "zfone_types.h"

#include "zfone_cfg.h"
#include "zfone_utils.h"
#include "zfone_manager.h"
#include "zfone_heurist.h"
#include "zfone_siprocessor.h"

#include "zfone_scheduler.h"
#include "zfone_checker.h"

#include "zfone_commands.h"
#include "zfone_log.h"
#include "zfone_alert.h"
#include "zfone_cache.h"

#include "zfone_updater.h"

#if ZRTP_PLATFORM == ZP_LINUX || ZRTP_PLATFORM == ZP_DARWIN
	#include "sfiles.h"
	#include "zfoned_cmdconv.h"
	#include "zfoned_sockets.h"
	#include "zfoned_commands.h"
	#include "zfoned_network.h"
	#include "zfoned_ctrl.h"

	#include <netinet/in.h>
	#include <ifaddrs.h>
#else
  #include "zrtp_driver.h"
#endif


#define ZFONE_VERSION_SIZE	8
#define ZFONE_MAC_CLIENT_ID "Mac ZfoneEC 0.92"
#define ZFONE_WIN_CLIENT_ID	"Win ZfoneEC 0.92"
#define ZFONE_LIN_CLIENT_ID	"lin ZfoneEC 0.92"

#if ZRTP_PLATFORM == ZP_WIN32_KERNEL
	#define ZFONE_MAJ_VERSION	0
	#define ZFONE_SUB_VERSION	92
	#define ZFONE_BUILD_NUMBER	218
	#define ZFONE_CLIENT_ID		ZFONE_WIN_CLIENT_ID

#elif ZRTP_PLATFORM == ZP_LINUX
	#define ZFONE_MAJ_VERSION	0
	#define ZFONE_SUB_VERSION	92
	#define ZFONE_BUILD_NUMBER	246
	#define ZFONE_CLIENT_ID		ZFONE_LIN_CLIENT_ID

#elif ZRTP_PLATFORM == ZP_DARWIN
	#define ZFONE_MAJ_VERSION	0
	#define ZFONE_SUB_VERSION	92
	#define ZFONE_BUILD_NUMBER	265
	#define ZFONE_CLIENT_ID		ZFONE_MAC_CLIENT_ID

#else
	#error "Unown platform - can't define version number"
#endif


#define ZFONE_VERSION ((1000L * ZFONE_MAJ_VERSION) + ZFONE_SUB_VERSION) 


#define ZRTP_SYMP_MIN_RTP_LENGTH 12 + 2			// so small because of iChat 4.0 and DTMF
#define ZRTP_SYMP_MAX_RTP_LENGTH 12 + 2500
#define ZRTP_SYMP_MIN_RTP_PORT	 1030

#define ZFONE_ENTROPY_SIZE		 256

#define ZFONE_TRUSTEDMITM_PREFIX		"MiTM "
#define ZFONE_TRUSTEDMITM_PREFIX_LENGTH  5

#undef ZFONE_ENABLE_DEBUG

/*!
 * \brief Initializes ZFone daemon/driver routine
 * Creates and initalizes all subsystems and components. After call of this
 * function ZFone is ready to be started by zfone_start() functions.
 * \return:
 *	`	- 0 in case of success
 *		- -1 in case of error. See debug output for more information
 */
int   zfone_init();

/*! Release all allocated by zfone_init() respurces  */
void  zfone_down();

/*!
 * \brief Starts ZFone Network filter.
 * After call of zfone_start() ZFone is ready for VoIP calls encryption
 */
int   zfone_start();

/*! Stops ZFone Network Filter */
void  zfone_stop();


/*!
 * \brief Return status of configuration file reading and parsing
 * Used my ZFone daemon/driver to alarm users about problems with the configuration
 * file. 
 * \return one of \ref zfone_cfg_error_t error codes
 */
zfone_cfg_error_t zfone_get_config_state();

/*!
 * \brief reset status of configuration file reading and parsing back to dewfault value
 * After sending an alarm message to the GUI, engine grops alarm event by means
 * of this function. It prevents GUI from displaying multiple alarms.
 */
void zfone_reset_config_state();


#endif //__ZFONE_H__
