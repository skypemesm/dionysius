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

#include "SRPPMessage.hpp"

using namespace std;

class ServerSocket {

    int sock;         // handle to the socket
    int addr_len, bytes_read;
    char recv_data[1024];
    struct sockaddr_in server_addr , client_addr;

public:
	ServerSocket(int ServerPort);
	~ServerSocket();

	string getData();

	int putData(string );

};
