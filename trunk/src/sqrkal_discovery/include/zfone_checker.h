/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 * Nikolay Popok <chaser@soft-industry.com>
 */

#ifndef __ZFONED_RTP_CHECKER_H__
#define __ZFONED_RTP_CHECKER_H__

#include "zrtp.h"
#include "zfone_manager.h"

//500ms
#define ZFONED_RTP_ACTIVITY_TIMEOUT		500L

//5s
#define ZFONED_RTP_ACTIVITY_ALERT_TIMEOUT	5000L

#define ZFONED_TIME_QUANTUM		200L

//200ms
#define ZFONED_RTP_ACTIVITY_INTERVAL	1L

//10s
#define ZFONED_INTERFACES_TIMEOUT	50L

//200 ms
#define ZFONED_HEURIST_CALL_CHECK 1L
//1s
#define ZFONED_HEURIST_RTPS_CHECK 5L
//10s
#define ZFONED_HEURIST_RTPSHOW_CHECK 50L


int zfone_checker_start();
void zfone_checker_stop();
void zfone_checker_reg_rtp(zfone_packet_t *packet, int is_inc, int passed);
void  zfone_checker_reset_rtp_activity();

#endif /*__ZFONED_SCHEDULER_H__*/

