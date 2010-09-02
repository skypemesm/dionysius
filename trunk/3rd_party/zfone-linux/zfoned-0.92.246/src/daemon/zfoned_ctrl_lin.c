/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Max Yegorov <egm@soft-industry.com>, <m.yegorov@gmail.com>
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <zrtp.h>

#include "zfone.h"

#define _ZTU_	"zfone ctrl"

#define MAX_MESSAGE_SIZE	2048*4

#define PROCESS_ERROR	-1
#define	PROCESS_TRUNK	0
#define	PROCESS_READ	1

static int	tcpsock = 0;
static int  clientsock = 0;
static int is_running = 0;

/* Registered callbacks */
static voip_ctrl_callback ctrl_cb[ZRTP_NR_CMDS];


/*
 process must be supplied by the user of the server
 process must return:
 PROCESS_TRUNK if buffer was processed and @size is amount of data processed
 PROCESS_READ if buffer was not processed and @size is amount of data processed
 
 on enter to process size stores amount of data in buffer
 */
static int process(const char* buffer, int *size)
{
    zrtp_cmd_t *cmd		= NULL;
    int cmd_length		= 0;    
    int   parsed		= 0;
    int   processed		= 0;
    int status 			= PROCESS_READ;
	
    while (processed < *size) {
		// Look for delimiter		
		unsigned int i = *size - processed - 1;
		for (; i; i--) {	    
			// Check main command header size. If data is not enough - wait for more
			if (i+1 <sizeof(zrtp_cmd_t))			
				goto PROC_EXIT;			
			
			cmd = (zrtp_cmd_t*) (buffer+processed);			
			// Check for delimiter. If Magic found - it is command
			if (cmd->imagic[0] == ZRTP_CMD_MAGIC0 && cmd->imagic[1] == ZRTP_CMD_MAGIC1 ) {				
				break;
			}
			
			processed++;
		}
		
		if (!i) { // can't find delimiter - waiting for more data					
			goto PROC_EXIT;
		}
		
		// Get command length
		cmd_length = zrtp_ntoh32(cmd->length) + sizeof(zrtp_cmd_t);
		
		// Check for fragmentation
		if ((*size-processed) >= cmd_length) {
			// Convert zrtp command date to the host byteorder for cross-platf:
			// Linux enjine and Mac-OS GUI (as example)
			zfone_cmd_ntoh(cmd);
			
			{
				char buf[256];
				ZRTP_LOG(3, (_ZTU_, "zfoned control: Command with size=%d"
							   " and code %d have been received. ses:%s stream:%d.\n",
							   cmd_length, cmd->code,
							   hex2str((const char*)cmd->session_id,
									   ZFONE_SESSION_ID_SIZE,
									   buf, sizeof(buf)),
							   cmd->stream_type));
			}
			
			// Execute command
			process_cmd(cmd, cmd_length);
			processed += cmd_length;
		} else {		
			goto PROC_EXIT;
		}
		
		// Seek to the next command
		parsed += cmd_length;
    }
    
    // If buffer contains whole command/commands - returne PROCESS_READ
    status = PROCESS_TRUNK;
	
PROC_EXIT:
    *size = parsed;
    return status; 
}

#define PORT_NUM	5000
//------------------------------------------------------------------------------
int init_ctrl_server(void)
{
	is_running = 0;

	memset(ctrl_cb, 0, ZRTP_NR_CMDS * sizeof(voip_ctrl_callback*));
	init_ctrl_callbacks();    

	do
	{
  		struct sockaddr_in sin;	
		int	bind_ret 		= -1;
  		int on	     		= 1;
	    int bind_retries	= 10;
		// Create TCP socket and bind it to the addr specified in the @sd
		tcpsock = socket(PF_INET, SOCK_STREAM, 0);
		if (tcpsock < 0)
		{
			ZRTP_LOG(1, (_ZTU_, "ZFONED tcp_srv_create: Can't create TCP socket\n. %s", strerror(errno)));
			break;
		}

		if (0 > setsockopt(tcpsock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)))
		{
			ZRTP_LOG(1, (_ZTU_, "ZFONED tcp_srv_create: Can't configure TCP socket. %s\n", strerror(errno)));
			close(tcpsock);
			return -1;
		}

		memset(&sin, 0, sizeof(sin));    
		sin.sin_family	= PF_INET;
		sin.sin_port	= zrtp_hton16(PORT_NUM);
		sin.sin_addr.s_addr = zrtp_hton32(INADDR_LOOPBACK);
		
		while (--bind_retries)
		{
			if (0 == (bind_ret = bind(tcpsock, (struct sockaddr*)&sin, sizeof(sin))))
				break;
			else    
			{
				ZRTP_LOG(1, (_ZTU_, "bind retry to %x:%u failed %s. sleep for 1 sec\n", INADDR_LOOPBACK, PORT_NUM, strerror(errno)));
				sleep(1);
			}
		}		

		if (0 != bind_ret)
			break;		

		if (0 > listen(tcpsock, 1))
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned (tcp_srv_create): Can't listen TCP socket. %s\n", strerror(errno)));
			break;
		}

		return 0;
	} while (0);
     
	if (tcpsock)
	{
		shutdown(tcpsock, 2);
		close(tcpsock);
	}

	return -1;
}

void run_ctrl_server(void)
{
	is_running = 1;
	
	while (is_running) 
	{
		int sock;
  		struct sockaddr_in  sin;
  
		socklen_t len = sizeof(sin);
		memset(&sin, 0, sizeof(sin));
		
		ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: Waiting for incomming connections...\n"));

		clientsock = accept(tcpsock, (struct sockaddr*)&sin, &len);
		if ( !clientsock )
			continue;

		ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: accepted client connection\n"));
	
		while (1)
		{
			char buffer[MAX_MESSAGE_SIZE*2];
			int size = 0;
			int read_cnt = 0;
		
			// conn->buffer: |[ unprocessed block of conn->size bytes ] empty space	|
			// We may have some command not fully read in, so lets read the rest of
			// command to the buffer, or simply to read new one
		
			read_cnt = recv(clientsock, buffer + size, MAX_MESSAGE_SIZE - size, 0);
			ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: received %d bytes from client\n", read_cnt));
			if (read_cnt <= 0) 
				break;

			// Fix size of data to process in the buffer
			size += read_cnt;
			int size_ = size;
			
			// Call user supplied routine. Typically it is up for sd->process
			// to detect bad command, garbage and so on
			//const int status = sd->process(fe->fd, conn->buffer, &size_);
			int status = process(buffer, &size_);
			switch (status) 
			{
				case PROCESS_TRUNK:
					// Command_process said that it processed qall data in the buffer.
					// So we can clear whole buffer for new commands storing
					size = 0;
					break;
				
				case PROCESS_READ:		
					// command_process said that we may discard first @size bytes in 
					// the buffer. And fix the size to read after that
					size -= size_;
					// Weird things, we have gone beyond the limits
					if (size < 0)
						size = 0;
					else
						memmove(buffer, buffer + size_, size);
					break;
				
				default: break;
			}
		}
		sock = clientsock;
		clientsock = 0;
		shutdown(sock, 2);
		close(sock);
	}
	
	ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: Sop main processing loop.\n"));
}

//------------------------------------------------------------------------------
void stop_ctrl_server(void)
{
	is_running = 0;
	if (tcpsock)	
	{
  		shutdown(tcpsock, 2);
  		close(tcpsock);
	}
	tcpsock = 0;
    unlink(ZFONE_PID_PATH);
}

//------------------------------------------------------------------------------
void register_ctrl_callback(unsigned int callback_type, voip_ctrl_callback cb)
{
    //  We do not need to register non-existent callback
    if (callback_type >= ZRTP_NR_CMDS)
		return;

    ctrl_cb[callback_type] = cb;
}

//------------------------------------------------------------------------------
int send_cmd( unsigned int	code,
	    	  zfone_session_id_t session_id,
	    	  uint16_t	stream_type,
	    	  void	*extension,
	    	  unsigned short extension_length )
{
    const int total_length = sizeof(zrtp_cmd_t) + extension_length;
    char *buffer = zrtp_sys_alloc(total_length);
    zrtp_cmd_t *cmd = (zrtp_cmd_t*)buffer;
    
	if ( !buffer )
		return -1;

    memset(buffer, 0, total_length);

    cmd->imagic[0] = ZRTP_CMD_MAGIC0;
    cmd->imagic[1] = ZRTP_CMD_MAGIC1;
    
    cmd->code	= code;
    cmd->length	= extension_length;
    if (session_id) memcpy(cmd->session_id, session_id, ZFONE_SESSION_ID_SIZE);
    cmd->stream_type = stream_type;
    memcpy(ZFONE_CMD_EXT(cmd), extension, extension_length);
    
    // convert command to the network byte-order
    zfone_cmd_hton(cmd);

    if (clientsock)
		send(clientsock, buffer, total_length, 0);

    if (ZRTP_PACKET != code)
    {
		char buf[256];
		ZRTP_LOG(3, (_ZTU_, "ZFONED send_cmd: Send command=%i with size=%i within ses :%s stream:%s:%x\n",
			code, total_length,
			session_id ? hex2str((const char*)cmd->session_id, ZFONE_SESSION_ID_SIZE, buf, sizeof(buf)) : "NULL",
			(stream_type == ZFONE_SDP_MEDIA_TYPE_AUDIO) ? "AUDIO" : "VIDEO", cmd->stream_type));
    }
    
	zrtp_sys_free(buffer);
    return total_length;
}

void process_cmd(zrtp_cmd_t *cmd, int in_size)
{
    if ( (cmd->code < ZRTP_NR_CMDS) && ( ctrl_cb[cmd->code] ) )
	{
        ctrl_cb[cmd->code](cmd, in_size, 0);
	}
}
