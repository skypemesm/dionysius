/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */ 

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone updater"

#if (ZRTP_PLATFORM == ZP_LINUX)
#	define PLATFORM_STR "zfone/update-versions/linux-proxy-version"
#elif  (ZRTP_PLATFORM == ZP_DARWIN)
#	define PLATFORM_STR "zfone/update-versions/mac-proxy-version"
#else
#	error "No platform defined for update request generation!"
#endif // Platform detection

static const unsigned short http_server_port = 	80;

#define UPDATER_SERVER_PATH_MAX_PATH			128

struct zfone_updater_params_t
{
    uint32_t	server_ip;
    char		server_name[UPDATER_SERVER_PATH_MAX_PATH];
    char		server_path[UPDATER_SERVER_PATH_MAX_PATH];    
    int 		check_force;
};

//==============================================================================
//    Updater private part
//==============================================================================


//------------------------------------------------------------------------------
static int create_request(const char *server_name, char* buff, int size)
{
	if ( !buff || size <= 0 )
		return -1;
		
    snprintf(buff, size, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", PLATFORM_STR, server_name);
    
    zrtp_print_log_delim(3, LOG_START_SELECT, "Zfone reqest");
    ZRTP_LOG(3, (_ZTU_, "zfoned updater: %s.\n", buff));
    
    return strlen(buff);
}

#define real_OK_str "HTTP/1.1 200 OK"
#define real_OK_str_s strlen(real_OK_str)

//------------------------------------------------------------------------------
static int parse_response(const char* buff, uint32_t size, char* res,  char* url, uint32_t url_length)
{
    uint32_t bytes_left = size;
    uint32_t offset = 0;
    char* nptr = (char*) buff;
    
    // check response result
    if (size <= real_OK_str_s)
    {
		ZRTP_LOG(1, (_ZTU_, "zfoned updater: To small response string size %d.\n", size));
		return -1;
    }
    if (strncmp(nptr, real_OK_str,  real_OK_str_s))
    {
		ZRTP_LOG(1, (_ZTU_,"zfoned updater: wrong OK string in HTTP responses:\n %s.\n", buff));
		return -1;
    }
    
    // Searching where is payload begins
    nptr = zfone_find_str(nptr, bytes_left, "\r\n\r\n", &offset);
    if (!nptr)
    {
		ZRTP_LOG(1, (_ZTU_,"zfoned updater: can't find HTTP payload.\n"));
		return -1;
    }
    bytes_left -= offset+4;
    nptr += 4;
    if (bytes_left < ZFONE_VERSION_SIZE)
    {
		ZRTP_LOG(1, (_ZTU_,"zfoned updater: wrong payload size %d in HTTP response.\n", bytes_left));
		return -1;
    }
    
    // copy version string
    strncpy(res, nptr, ZFONE_VERSION_SIZE);    
    res[ZFONE_VERSION_SIZE] = 0;
    
    //  next string must be a URI for updates downloading
    nptr = zfone_find_char(nptr, bytes_left, "\n", &offset);
    if (!nptr)
    {
		ZRTP_LOG(1, (_ZTU_,"zfoned updater: URL for updates was missed at server response message.\n"));
		return -1;
    }
    bytes_left -= offset;
    nptr += 1; 
    
    nptr = zfone_get_next_token(nptr, bytes_left, "\n \t", 0, NULL, &offset);
    if (!nptr)    
    {
		ZRTP_LOG(1, (_ZTU_,"zfoned updater: wrong server response - wrong URL string.\n"));
		return -1;
    }
    if (offset > url_length-1)
    {
		ZRTP_LOG(1, (_ZTU_,"zfoned updater: can't handle response -  URL is to long:%d.\n", offset));
		return -1;
    }
    bytes_left -= offset;
    
    strncpy(url, nptr, offset);
    url[offset] = 0;
    
    return size - bytes_left;
}

//------------------------------------------------------------------------------
static void* process(void* param)
{
    zfone_version_t version;
    struct sockaddr_in srv_addr;
    int32_t tcp_sock = 0;
    struct zfone_updater_params_t* updater = (struct zfone_updater_params_t*) param;
    char packet_buff[1024];
    char ip_str[25];
    int size = 0;
    int res = -1;

    ZRTP_LOG(3, (_ZTU_, "zfoned updater: Start checking for opdates:"
								   " creating reqest, establishing connection...\n"));
    
    // Create a request string
    size = create_request(updater->server_name, packet_buff, sizeof(packet_buff));
    if (size <= 0)
		goto UPDATE_EXIT;
    
    // Create TCP socket
    tcp_sock = socket(PF_INET,SOCK_STREAM,0);
    if (tcp_sock == -1)
    {
		ZRTP_LOG(1, (_ZTU_, "zfoned updater: Can't create TCP socket.\n"));
		goto UPDATE_EXIT;
    }
    
    // Connecting to the server
	zrtp_memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port   = zrtp_hton16(http_server_port);
    srv_addr.sin_addr   = *((struct in_addr*)(&updater->server_ip));
    
    ZRTP_LOG(3, (_ZTU_, "zfoned updater: connecting to:%s.\n",
				   zfone_ip2str(ip_str, 25, zrtp_ntoh32(updater->server_ip))));
    if ( 0 != connect(tcp_sock, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) )
    {
		ZRTP_LOG(1, (_ZTU_, "zfoned updater: Can't connect to %s:%d.\n",
					    zfone_ip2str(ip_str, 25, zrtp_ntoh32(updater->server_ip)), http_server_port ));
		goto UPDATE_EXIT;
    }
    
    // send reqest to server    
    ZRTP_LOG(3, (_ZTU_, "zfoned updater: sending request...\n"));
    if (0 < send(tcp_sock, packet_buff, size, 0))
    {
		int attempts_count = 0;
		// traying get response till GET_RESPONSE_ATTEMPT_COUNT limit reqched
		while (attempts_count < GET_RESPONSE_ATTEMPT_COUNT)
		{
			usleep(GET_RESPONSE_TIMEOUT*1000);
			memset(packet_buff, 0, sizeof(packet_buff));
		
			ZRTP_LOG(3, (_ZTU_, "zfoned updater: trying to get response (%d attempt).\n", attempts_count));
			size = recv(tcp_sock, packet_buff, sizeof(packet_buff), MSG_PEEK);
			if (size > 0)
			{		
				char version_str[128];
				char url[128];
				
				// Ok it's Zfone server response - get packet and pars it
				size = recv(tcp_sock, packet_buff, sizeof(packet_buff), 0);
				
				ZRTP_LOG(3, (_ZTU_, "zfoned updater: rerver response received - start parsing.\n"));
				zrtp_print_log_delim(3, LOG_START_SELECT, "Zfone server response");
				ZRTP_LOG(3, (_ZTU_, "zfoned updater: %s.\n", packet_buff)); 	
						
				size = parse_response(packet_buff, size, version_str, url, sizeof(url));
				if (size <= 0)
				{
					ZRTP_LOG(1, (_ZTU_, "zfoned updater: Can't parse server response."
												   "Checking for updates failed.\n"));
					goto UPDATE_EXIT;
				}
						
				// prepare version structure for using
				memset(&version, 0, sizeof(zfone_version_t));
				ZSTR_SET_EMPTY(version.version_str);
				ZSTR_SET_EMPTY(version.url);
				
				// parse version string
				if (3 != sscanf(version_str, "%d.%d.%d",&version.maj_version, &version.sub_version, &version.build))
				{
					ZRTP_LOG(1, (_ZTU_, "zfoned updater: Wrong version string format,"
												   " must be v.ss.bbb. Missed '.' after v or after ss.\n"));
					goto UPDATE_EXIT;
				}
		
				if ((version.maj_version < 0) || (version.sub_version < 0) || (version.build < 0))
				{
					ZRTP_LOG(1, (_ZTU_, "zfoned updater: Wrong version string format."
												   " One of the version components is wrong.\n"));
					goto UPDATE_EXIT;
				}
												
				// fill version structure			
				zrtp_zstrcpyc((zrtp_stringn_t*)&version.version_str, version_str);
				zrtp_zstrcpyc((zrtp_stringn_t*)&version.url, url);
				version.version_int = (version.maj_version * 1000L) + version.sub_version;    		    
				
				ZRTP_LOG(3, (_ZTU_, "zfoned updater: Done. Updates response succesfully"
											   " parsed at sent to daemon core.\n"));
				
				res = 0;
				break;
			}
			attempts_count++;
		}
		if (attempts_count >= GET_RESPONSE_ATTEMPT_COUNT)
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned updater: Can't get server response"
										   " for %d attempts.\n", attempts_count));
		}
    }
    else
    {
		ZRTP_LOG(1, (_ZTU_, "zfoned updater: Can't send request to server.\n"));
		res = -1;
    }
    
UPDATE_EXIT:
	// Shutdown TCP connection and close socket
	if (tcp_sock > 0)
	{
		if (0 != shutdown(tcp_sock, SHUT_RDWR))
			ZRTP_LOG(1, (_ZTU_, "ZFONED updater destroy: can't shutdown connection to server.\n"));
    
		if (-1 == close(tcp_sock))	
			ZRTP_LOG(1, (_ZTU_, "ZFONED updater destroy: can't close TCP socket.\n"));
	}
    
    // Notify daemon about checkking complite
    zfone_check4updates_done(&version, res, updater->check_force);
    
    return NULL;
}


//==============================================================================
//     Updater public part
//==============================================================================


//-----------------------------------------------------------------------------
void zfone_check4updates( uint32_t ip,
						  const char *name,
						  const char *path,
						  int forces )
{	
	pthread_t thread;
	zrtp_status_t s = zrtp_status_fail;
	struct zfone_updater_params_t zfone_updater_params;	

	// Initalization and parsing input parameters
	do
	{
		if (!name || !path)
			break;

		zrtp_memset(&zfone_updater_params, 0, sizeof(zfone_updater_params));

		zfone_updater_params.check_force = forces;
		zfone_updater_params.server_ip = zrtp_hton32(ip);

		if (strlen(name) > UPDATER_SERVER_PATH_MAX_PATH)
		{
			ZRTP_LOG(1, (_ZTU_, "updater create: can't create updater - server name to long.\n"));
			break;
		}
		strncpy(zfone_updater_params.server_name, name, strlen(name));

		if (strlen(path) > UPDATER_SERVER_PATH_MAX_PATH)
		{
			ZRTP_LOG(1, (_ZTU_, "updater create: can't create updater server path to long.\n"));
			break;
		}
		strncpy(zfone_updater_params.server_path, path, strlen(name));

		s = zrtp_status_ok;
	} while (0);

	if (zrtp_status_ok == s)
	{
		// Start checkking for updates in separate thread to release maijn processing
		// loop. Result will be returned by zfone_check4updates_done() callback
		ZRTP_LOG(3, (_ZTU_, "zfoned updater: create thread for UPDATE request processing... \n"));
		pthread_create(&thread, NULL, process, &zfone_updater_params);	
	}	
}

