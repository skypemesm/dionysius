/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
 */

#ifndef __ZFONEG_CTRL_H__
#define __ZFONEG_CTRL_H__

#include <glib/giochannel.h>

#include "zfoneg_commands.h"
#include "zfoneg_tcpconn.h"

int state2pic[ZRTP_NR_CMDS+4];

extern tcp_conn_t  tconn;

extern void	   gui_init_ctrl_client(void);
extern gboolean    gui_stop_ctrl_client(GIOChannel *channel, GIOCondition condition, gpointer data);
extern void        gui_run_ctrl_client(GIOChannel *channel);
extern gboolean    gui_run_ctrl(void);

extern int send_cmd(unsigned int	code
    		    ,zfone_session_id_t	session
	    	    ,stream_id_t	stream
	    	    ,void		*extension
	    	    ,unsigned short	extension_length );

#endif //__ZFONEG_CTRL_H__


