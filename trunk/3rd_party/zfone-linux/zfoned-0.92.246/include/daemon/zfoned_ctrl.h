/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Max Yegorov <egm@soft.cn.ua>, <m.yegorov@gmail.com>
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONED_CTRL_H__
#define __ZFONED_CTRL_H__

#include "zfoned_commands.h"

int init_ctrl_server(void);
extern void run_ctrl_server(void);
void stop_ctrl_server(void);

void register_ctrl_callback(unsigned int callback_type, voip_ctrl_callback cb);

int send_cmd(unsigned int	code
	    ,zfone_session_id_t	session_id
	    ,uint16_t		stream_type
	    ,void		*extension
	    ,unsigned short	extension_length );


#endif //__ZFONED_CTRL_H__
