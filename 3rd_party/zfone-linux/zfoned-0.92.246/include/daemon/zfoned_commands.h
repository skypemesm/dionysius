/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 * Max Yegorov <egm@soft.cn.ua>, <m.yegorov@gmail.com>
 */

#ifndef __ZFONED_COMMANDS_H__
#define __ZFONED_COMMANDS_H__

#include <zrtp.h>

#include "zfone_types.h"
#include "zfone_commands.h"

void init_ctrl_callbacks();
void deinit_ctrl_callbacks();
void register_zrtp_callbacks();

void send_update_streams_info();

void send_stop_cmd();
void send_crash_cmd();
void send_start_cmd();
//void cb_rtp_activity(zfone_stream_t *zstream, unsigned int direction, unsigned int type);

void cb_send_version( unsigned int curr_version, unsigned int new_version,
		      unsigned int curr_build, unsigned int new_build,
		      const char* url, int url_length, int is_expire, int is_manually );
void send_set_ips(const uint32_t *array, const uint8_t *flags, unsigned int count);
//void send_streamless_error(uint8_t error);
void cb_refresh_conns();

void process_cmd(zrtp_cmd_t *cmd, int in_size);

#endif //__ZFONED_COMMANDS_H__
