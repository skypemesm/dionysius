/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONE_IFACE_H__
#define __ZFONE_IFACE_H__

#include <zrtp.h>

extern void zrtph_play_alert(zrtp_session_t* ctx);

extern int zrtph_send_ip(char* packet, unsigned int length, unsigned int to);

char* zrtph_get_time_str(char* buff, int size);

#endif //__ZFONE_IFACE_H__
