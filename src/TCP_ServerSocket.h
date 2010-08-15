/***
 * This class includes all the methods required from a regular server socket
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstdio>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         100
#define MESSAGE             "This is the message I'm sending back and forth"
#define QUEUE_SIZE          5

using namespace std;

class TCP_ServerSocket {

    int port;
    int hSocket,hServerSocket;  /* handle to socket */
	struct hostent* pHostInfo;   /* holds info about a machine */
	struct sockaddr_in Address; /* Internet socket address stuct */
	int nAddressSize;
	char pBuffer[BUFFER_SIZE];
	int nHostPort;

public:
	TCP_ServerSocket(int ServerPort);
	~TCP_ServerSocket();

	string getData();

	int putData(string );

};
