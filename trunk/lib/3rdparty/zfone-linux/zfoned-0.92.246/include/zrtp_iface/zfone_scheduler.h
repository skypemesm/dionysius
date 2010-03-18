/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Viktor Krikun <v.krikun@soft-industry.com>
 */

#ifndef __ZFONE_SCHEDULER_H__
#define __ZFONE_SCHEDULER_H__

void zrtp_on_cancel_call_later_callback(zrtp_stream_t* ctx, zrtp_retry_task_t* ztask);
void zrtp_on_call_later_callback( zrtp_stream_t *ctx, zrtp_retry_task_t* ztask);

#endif //__ZFONE_SCHEDULER_H__
