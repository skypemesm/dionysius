/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONE_ALERT_H__
#define __ZFONE_ALERT_H__

#include "zfone_types.h"

typedef enum zfone_alert_t
{
	ZRTP_ALERT_PLAY_NO = 0,
	ZRTP_ALERT_PLAY_SECURE,
	ZRTP_ALERT_PLAY_CLEAR,
	ZRTP_ALERT_PLAY_ERROR,
	ZRTP_ALERT_PLAY_NOACTIVE
} zfone_alert_t;

int zrtph_init_alert(const char* alert_dir_name);
void zrtp_play_alert(zfone_stream_t* stream);

#endif //__ZFONE_ALERT_H__
