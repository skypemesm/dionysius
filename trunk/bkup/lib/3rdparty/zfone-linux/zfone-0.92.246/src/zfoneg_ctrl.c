/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>

#include "zfoneg_config.h"
#include "zcmd_conv.h"
#include "tcp/zfoneg_tcpconn.h"
#include "zfoneg_ctrl.h"
#include "zfoneg_cb.h"

#define	PROCESS_TRUNK	0
#define	PROCESS_READ	1

struct connection
{
    char	buffer[ZRTP_MAX_CMD_LENGTH];
    int		size;	/*!< unprocessed data in buffer */
};


static guint	timer_id	= 0;
static voip_ctrl_callback *ctrl_cb[ZRTP_NR_CMDS];

static struct connection conn;

volatile	int	need_to_run;
pthread_t	thread_id;

/*!
    GUI TCP connection.
    \sa tcp_conn
*/
tcp_conn_t	tconn;

/*!
    Here we initiaize \link ControlClient#tconn tconn \endlink. Assign callbacks 
    \link ControlClient#ipf_ok ipf_ok \endlink and 
    \link ControlClient#ipf_failed ipf_failed \endlink. Resgister callbacks 
    \link ControlClient#register_callback register_callback \endlink
    
    \sa register_callback
    \sa tcp_conn_init
    \sa control_commands
*/
//------------------------------------------------------------------------------
void gui_init_ctrl_client(void)
{
    tcp_conn_init(&tconn);

    tconn.conn_ok_callback = ipf_ok;
    tconn.no_conn_callback = ipf_failed;

    tconn.create(&tconn);
    
    register_callback(UPDATE_STREAMS, cb_update_list);
    register_callback(SEND_VERSION, gui_cb_send_version);
    register_callback(STARTED, cb_started);
    register_callback(STOPPED, cb_stopped);
    register_callback(CRASHED, cb_crashed);
    register_callback(ZRTP_PACKET, cb_packet);
    register_callback(ERROR, cb_error);
    register_callback(SET_PREF, cb_set_preferences);
    register_callback(SET_DEFAULTS, cb_set_defaults);
    register_callback(SET_IPS, cb_set_ips);
	register_callback(SET_CACHE_INFO, cb_set_cache_info);
}

/*!
    \a gui_run_ctrl is called each time data is available. If connection is ok
    \link tcp_conn#is_ok \endlink then function reads data into local buffer.
    After read the function extracts command number and calls appropriate callback
    If read from the connection was failed then it destroys tconn \link tcp_conn#destroy
    tcp_conn::destroy \endlink
    
    \sa tcp_conn#read ControlClient#register_callback Commands::control_commands 
*/

static int z_process(const char* buffer, int *size)
{
    zrtp_cmd_t *cmd		= NULL;
    int cmd_length		= 0;    
    int   parsed		= 0;
    int   processed		= 0;
    int status 			= PROCESS_READ;

    while (processed < *size)
    {
	// looking for delimiter
	{
	    unsigned int i = *size - processed - 1;
	    for (; i; i--)
	    {	    
			// check main command header size
			if (i+1  < sizeof(zrtp_cmd_t) )
			{
			    //Can't read zrtp_cmd_t - waiting for more data.
			    goto PROC_EXIT;
			}	
		
			cmd = (zrtp_cmd_t*) (buffer+processed);
		
			// check delimiter
    		if ( cmd->imagic[0] == ZRTP_CMD_MAGIC0 && cmd->imagic[1] == ZRTP_CMD_MAGIC1 )
			{
			    // Magic found it is command
			    break;
			}
		
			processed++;
	    }
	    
	    if ( !i )
	    {
			// can't find delimiter - waiting for more data
			goto PROC_EXIT;
	    }
	}
	
	// get command length
	cmd_length = ntohl(cmd->length) + sizeof(zrtp_cmd_t);
	
    
	// check for fragmentation
	if ( (*size-processed) >= cmd_length )
	{
	    // convert zrtp command date to the host byteorder
	    zfone_cmd_ntoh(cmd);
	    	
	    // execute command
	    if ( (cmd->code < ZRTP_NR_CMDS) && ( ctrl_cb[cmd->code] ) )
	    {
	        ctrl_cb[cmd->code](cmd);
	    }
//	    printf("received command with code %d and length %d\n", cmd->code, cmd_length);
	    
	    processed += cmd_length;
	}
	else
	{
	    goto PROC_EXIT;
	}
	
	// seek to the next command
	parsed += cmd_length;
    }
    
    // if buffer contains whole command/commands - returne PROCESS_READ
    status = PROCESS_TRUNK;

PROC_EXIT:
    *size = parsed;
    return status; 
}

//------------------------------------------------------------------------------
gboolean gui_run_ctrl(void)
{
    int read_cnt;
//    char cmd_buff[ZRTP_MAX_CMD_LENGTH];
//    char *cmd_buff;
    

//    cmd_buff = malloc(ZRTP_MAX_CMD_LENGTH);

    read_cnt = tconn.read(&tconn, conn.buffer + conn.size, ZRTP_MAX_CMD_LENGTH - conn.size, MSG_WAITALL, 20);

    if ( read_cnt > 0 )
    {
        // Fix size of data to process in the buffer
        conn.size += read_cnt;
        int size    = conn.size;
	    
        // Call user supplied routine. Typically it is up for sd->process
        // to detect bad command, garbage and so on
        const int status = z_process(conn.buffer, &size);
	    
        switch ( status )
        {
	    	case PROCESS_TRUNK:
	    	{
	    	    // command_process said that it processed qall data in the buffer.
			    // So we can clear whole buffer for new commands storing
			    conn.size = 0;
			    break;
			}
			case PROCESS_READ:
			{
	  		    // command_process said that we may discard first @size bytes in 
			    // the buffer. And fix the size to read after that
			    conn.size -= size;
			    // Weird things, we have gone beyond the limits
			    if ( conn.size < 0 )
				{
					conn.size = 0;
				}
				else
			    memmove(conn.buffer, conn.buffer + size, conn.size - size);
/*
			    memmove(conn.buffer, conn.buffer + size, conn.size - size);
			    conn.size -= size;
			    // Weird things, we have gone beyond the limits
			    if ( conn.size < 0 )
					conn.size = 0;
*/			
			    break;
		  	}
			default: break;
		}
    }
    else
    {
        // mark entry as broken
    }


    return TRUE;
}

/*!
    stops and removes timer source in GTK's GMainLoop. Destroys TCP connection.
*/
//------------------------------------------------------------------------------
gboolean gui_stop_ctrl_client(GIOChannel *channel, GIOCondition condition, gpointer data)
{
    if ( timer_id )
    {
	g_source_remove(timer_id);
	timer_id = 0;
    }
    
    tconn.destroy(&tconn);
    
    return FALSE;
}

/*!
    \brief starts control client
    
    Adds timer source to GTK's main event loop. Each time timer is expired 
    \a gui_run_ctrl is run.
    
    \sa gui_run_ctrl
*/
//------------------------------------------------------------------------------
void gui_run_ctrl_client(GIOChannel *channel)
{
    conn.size = 0;
    if ( timer_id )
    {
	g_source_remove(timer_id);
	timer_id = 0;
    }	
    timer_id = g_timeout_add(100, (GSourceFunc)gui_run_ctrl, NULL);
}

/*!
    \brief stores callback for specified state
    
    \param callback_type defines state to which callback \a cb must respond
    \param cb pointer to function
    
    \sa voip_ctrl_callback
*/
//------------------------------------------------------------------------------
void register_callback(unsigned int callback_type, voip_ctrl_callback* cb)
{
    /* We do not need to register non-existent callback */
    if ( callback_type >= ZRTP_NR_CMDS )
	return;

    ctrl_cb[callback_type] = cb;
}

/*!
    \brief sends command to the control server

    send_cmd uses tcp_conn_t server to send data. send_cmd prepares command and copies
    extension to the end of the command \link Commands#zrtp_cmd_t zrtp_cmd_t \endlink

    \param code command code

    \sa tcp_conn_t zrtp_cmd_t
*/
//------------------------------------------------------------------------------
int send_cmd(unsigned int	code
	    ,zfone_session_id_t	session_id
	    ,stream_id_t	stream_id
	    ,void		*extension
	    ,unsigned short	extension_length )
{
    int ret = 0;

    const int	total_length	= sizeof(zrtp_cmd_t) + extension_length;
    char	buffer[total_length];
    zrtp_cmd_t	*cmd		= (zrtp_cmd_t*)buffer;
    
    memset(buffer, 0, total_length);

    cmd->imagic[0] = ZRTP_CMD_MAGIC0;
    cmd->imagic[1] = ZRTP_CMD_MAGIC1;
    
    cmd->code	    = code;


    if ( session_id )
	memcpy(cmd->session_id, session_id, ZFONE_SESSION_ID_SIZE);
    else
	memset(cmd->session_id, 0,  ZFONE_SESSION_ID_SIZE);

    cmd->stream_id  = stream_id;
    cmd->length	    = extension_length;

    memcpy(&(cmd->data), extension, extension_length);
    
    // convert zrtp command date to network mode
    zfone_cmd_hton(cmd);
    
    if ( tconn.sock )
    {
        if ( 0 > tconn.write(&tconn, buffer, total_length) )
	{
	    perror("Send failed\n");
	}
	else
	{
	    ret = 1;
	}
    }

    return ret;
}

