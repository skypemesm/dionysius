/***
 * This class includes all the methods required from a regular server socket
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstdio>

#ifndef RTPPACKETBUILDER_H
	#define RTPPACKETBILDER_H
	//Using JRTPLIB
	#include "rtppacketbuilder.h"
	#include "rtppacket.h"
#endif


#ifndef SRPP_FUNCTIONS_H
	#define  SRPP_FUNCTIONS_H
	#include "SRPP_functions.h"
#endif

using namespace std;

class ServerSocket {

    int sock;         // handle to the socket
    int addr_len, bytes_read;
    char recv_data[1024];
    struct sockaddr_in server_addr , client_addr;
    SRPPSession * thissession;

public:
	ServerSocket(SRPPSession * mysession, int ServerPort);
	~ServerSocket();

	string getData();

	int putData(string );

};
