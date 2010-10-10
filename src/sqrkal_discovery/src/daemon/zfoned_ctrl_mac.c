/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <unistd.h>
#include <pthread.h>

#include <launch.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone ctrlmac"

#ifdef ZFONE_ENABLE_DEBUG
#	include <sys/types.h>
#	include <netinet/in.h>
#	include <unistd.h>
#	include <errno.h>

//#	define ZFONE_DEBUG_PORT_NUM 17003
#	define ZFONE_DEBUG_PORT_NUM 65501
static int the_server_socket = 0;
#endif

static uint8_t is_running = 0;
static int kq = 0;

// Registered callbacks
static voip_ctrl_callback ctrl_cb[ZRTP_NR_CMDS];

//------------------------------------------------------------------------------
int init_ctrl_server(void)
{
#ifdef ZFONE_ENABLE_DEBUG
	zrtp_status_t s = zrtp_status_fail;
	do
	{
  		struct sockaddr_in sin;
  		int on	     		= 1;
		
		the_server_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (the_server_socket < 0) {
			ZRTP_LOG(1, (_ZTU_, "ERROR! Can't create TCP socket. %s\n", strerror(errno)));
			break;
		}
		
		if (0 > setsockopt(the_server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
			ZRTP_LOG(1, (_ZTU_, "ERROR! Can't configure TCP socket. %s\n", strerror(errno)));
			break;
		}
		
		memset(&sin, 0, sizeof(sin));    
		sin.sin_family	= PF_INET;
		sin.sin_port	= zrtp_hton16(ZFONE_DEBUG_PORT_NUM);
		sin.sin_addr.s_addr = zrtp_hton32(INADDR_LOOPBACK);
		
		if (0 != bind(the_server_socket, (struct sockaddr*)&sin, sizeof(sin))) {
			ZRTP_LOG(1, (_ZTU_, "ERROR! Can't bind to TCP socket. %s\n", strerror(errno)));
			break;
		}
		
		if (0 > listen(the_server_socket, 1)) {
			ZRTP_LOG(1, (_ZTU_, "ERROR! Can't listen TCP socket. %s\n", strerror(errno)));
			break;
		}
		
		s = zrtp_status_ok;
	} while (0);
	
	if (zrtp_status_ok  != s) {
		if (the_server_socket) {
			close(the_server_socket);
		}
	}
#else
	size_t i;
	static struct kevent kev_init;
	launch_data_t listening_fd_array;
	launch_data_t sockets_dict;
	launch_data_t checkin_response;
	launch_data_t checkin_request;	
	
	ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: Start initalization....\n"));
	
	
	// Register GUI commands handlers
	init_ctrl_callbacks();
	
	if (-1 == (kq = kqueue()))
	{
		ZRTP_LOG(1, (_ZTU_, "ZFONED ctrl server: Can't inialize kernel messages queue.\n"));
		return -1;
	}
	
	//
	// Register ourselves with launchd.
	//
	if ((checkin_request = launch_data_new_string(LAUNCH_KEY_CHECKIN)) == NULL)
	{
		 ZRTP_LOG(1, (_ZTU_, "ZFONED ctrl server: launch_data_new_string(\"" LAUNCH_KEY_CHECKIN "\") Unable to create string.\n"));
		return -2;		
	}
	
	if ((checkin_response = launch_msg(checkin_request)) == NULL)
	{
		ZRTP_LOG(1, (_ZTU_, "ZFONED ctrl server: launch_msg(\"" LAUNCH_KEY_CHECKIN "\") IPC failure.\n"));
		return -3;
	}
	
	if (LAUNCH_DATA_ERRNO == launch_data_get_type(checkin_response))
	{
		ZRTP_LOG(1, (_ZTU_, "ZFONED ctrl server: Check-in failed:\n"));
		return -4;
	}
	
	launch_data_t the_label = launch_data_dict_lookup(checkin_response, LAUNCH_JOBKEY_LABEL);
	if (NULL == the_label)
	{
		ZRTP_LOG(1, (_ZTU_, "ZFONED ctrl server: No label found.\n"));
		return -5;
	}
	
	
	//
	// Retrieve the dictionary of Socket entries in the config file
	//
	sockets_dict = launch_data_dict_lookup(checkin_response, LAUNCH_JOBKEY_SOCKETS);
	if (NULL == sockets_dict)
	{
		ZRTP_LOG(1, (_ZTU_, "ZFONED ctrl server: No sockets found to answer requests on!\n"));
		return -6;
	}
	
	
	//
	//Get the dictionary value from the key "ZFoneSocket", as defined in .plist file.
	//
	listening_fd_array = launch_data_dict_lookup(sockets_dict, "ZFoneSocket");
	if (NULL == listening_fd_array)
	{
		ZRTP_LOG(1, (_ZTU_, "ZFONED ctrl server: No known sockets found to answer requests on!\n"));
		return -7;
	}
	
	//
	// Initialize a new kernel event.  This will trigger when a connection occurs
	// on our listener socket
	//
	for (i = 0; i < launch_data_array_get_count(listening_fd_array); i++)
	{
		launch_data_t this_listening_fd = launch_data_array_get_index(listening_fd_array, i);		
		EV_SET(&kev_init, launch_data_get_fd(this_listening_fd), EVFILT_READ, EV_ADD, 0, 0, NULL);
		if (kevent(kq, &kev_init, 1, NULL, 0, NULL) == -1)
		{
			ZRTP_LOG(1, (_ZTU_, "ZFONED ctrl server: kevent() failed.\n"));
			return -8;
		}
	}
	
	launch_data_free(checkin_response);
	
	ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: Initalization Done.\n"));
#endif
	
    return 0;
}


//------------------------------------------------------------------------------
#define PROCESS_ERROR	-1
#define	PROCESS_TRUNK	0
#define	PROCESS_READ	1

static const int MAX_MESSAGE_SIZE = 2048*4;
static int the_socket = 0;


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
	
    while (processed < *size)
	{
		// Look for delimiter		
		unsigned int i = *size - processed - 1;
		for (; i; i--)
		{	    
			// Check main command header size. If data is not enough - wait for more
			if (i+1 <sizeof(zrtp_cmd_t))			
				goto PROC_EXIT;			
			
			cmd = (zrtp_cmd_t*) (buffer+processed);			
			// Check for delimiter. If Magic found - it is command
			if (cmd->imagic[0] == ZRTP_CMD_MAGIC0 && cmd->imagic[1] == ZRTP_CMD_MAGIC1 )
			{
				break;
			}
			
			processed++;
		}
		
		// can't find delimiter - waiting for more data					
		if (!i)
		{
			goto PROC_EXIT;
		}
		
		// Get command length
		cmd_length = zrtp_ntoh32(cmd->length) + sizeof(zrtp_cmd_t);
		
		// Check for fragmentation
		if ((*size-processed) >= cmd_length)
		{
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
		}
		else
		{		
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

void run_ctrl_server(void)
{		
	struct sockaddr_storage ss;
	socklen_t slen = sizeof(ss);
	static struct kevent kev_listener;		
	
	is_running = 1;
	
	ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: Waiting for incomming connections...\n"));

#ifdef ZFONE_ENABLE_DEBUG
	while (is_running)
	{
		struct sockaddr_in  sin;
		
		socklen_t len = sizeof(sin);
		memset(&sin, 0, sizeof(sin));
		
		the_socket = accept(the_server_socket, (struct sockaddr*)&sin, &len);
		if (!the_socket) {
			continue;
		}
		
		ZRTP_LOG(1, (_ZTU_, "Connected.\n"));
		break;
	}
#else

	while (is_running)
	{
		// Get the next event from the kernel event queue.
		the_socket = kevent(kq, NULL, 0, &kev_listener, 1, NULL);
		if (-1 == the_socket)
		{
			is_running = 0;
			break;
		}
		else if (0  == the_socket)
		{
			is_running = 0;
			break;
		}
		
		ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: Got one connection, accepring.\n"));
		// Accept an incoming connection.
		the_socket = accept(kev_listener.ident, (struct sockaddr *)&ss, &slen);
		if (-1 == the_socket)
		{
			ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: try one more time...\n"));
			continue; // this isn't fatal
		}
		ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: Connected.\n"));
		
		break;
	}
#endif
	
	while (is_running && (0 < the_socket))
	{
		char buffer[MAX_MESSAGE_SIZE*2];
		int size = 0;
		int read_cnt = 0;
		
		// conn->buffer: |[ unprocessed block of conn->size bytes ] empty space	|
		// We may have some command not fully read in, so lets read the rest of
		// command to the buffer, or simply to read new one
		
		read_cnt = recv(the_socket, buffer + size, MAX_MESSAGE_SIZE - size, 0);
		if (read_cnt > 0)
		{
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
		else
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned ctrl server: GUI have been closed and we have to show down.\n"));
			is_running = 0;
			zfone_stop();
			zfone_down();
		}
	}
	
	ZRTP_LOG(3, (_ZTU_, "ZFONED ctrl server: Sop main processing loop.\n"));
}

//------------------------------------------------------------------------------
void stop_ctrl_server(void)
{
	is_running = 0;
#ifdef ZFONE_ENABLE_DEBUG
	if (the_server_socket) {
		shutdown(the_server_socket, 2);
		close(the_server_socket);
		the_server_socket = 0;
	}
#endif
}

//------------------------------------------------------------------------------
void register_ctrl_callback(unsigned int callback_type, voip_ctrl_callback cb)
{
    //  We do not need to register non-existent callback
    if (callback_type < ZRTP_NR_CMDS)
	{
		 ctrl_cb[callback_type] = cb;
	}
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
	
	if (!buffer)
	{
		return -1;
	}	
    memset(buffer, 0, total_length);

    cmd->imagic[0] = ZRTP_CMD_MAGIC0;
    cmd->imagic[1] = ZRTP_CMD_MAGIC1;
    
    cmd->code	= code;
    cmd->length	= extension_length;
    if (session_id)
	{
		memcpy(cmd->session_id, session_id, ZFONE_SESSION_ID_SIZE);
	}
    cmd->stream_type = stream_type;
    memcpy(ZFONE_CMD_EXT(cmd), extension, extension_length);
    
    // Convert command to the network byte-order
    zfone_cmd_hton(cmd);
	
	if (the_socket)
	{
		send(the_socket, buffer, total_length, 0);
	}

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
    if ((cmd->code < ZRTP_NR_CMDS) && ( ctrl_cb[cmd->code] ))
	{
        ctrl_cb[cmd->code](cmd, in_size, 0);
	}
}
