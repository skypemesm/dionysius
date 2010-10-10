/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONE_UPDATER_H__
#define __ZFONE_UPDATER_H__

#include <zrtp.h>

#define GET_RESPONSE_ATTEMPT_COUNT	2
#define GET_RESPONSE_TIMEOUT		700

//! Structure for software version describing
typedef struct zfone_version
{
    zrtp_string128_t	version_str;//! version string X.YY, where X - major version and YY - minor
    zrtp_string128_t	url;		//! URL for updates
    int 				maj_version;//! major version number
    int 				sub_version;//! minor version number
    int 				version_int;//! full version in integer format
    int 				build;		//! build number
} zfone_version_t;

/*!
 * \brief notice about updates checking finishing
 * zfone_updater#check calls this function when updates checking thread is finished
 * \param version - filled version structure
 * \param result - updates checking status 
 */
extern void zfone_check4updates_done( zfone_version_t *version,
									  int result,
									  unsigned int force );

/*!
 * \brief prepare updater for make connections
 * \params ip - server IP in host mode
 * \params name - server name 
 * \params path - full path for http request
 * \return 0 on success and -1 on error
 */
void zfone_check4updates( uint32_t ip,
						  const char *name,
						  const char *path,
						  int forces );

#endif //__ZFONE_UPDATER_H__

