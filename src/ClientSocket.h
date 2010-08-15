/***
 * This class includes all the methods required from a regular client socket
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

using namespace std;

class ClientSocket
{
	int sock;   					/** handle to socket */
	struct sockaddr_in server_addr;
	struct hostent *host;
	int bytes_read, addr_len;
	char recv_data[1024];

public:
	ClientSocket(string , int);
	~ClientSocket();

	string getData();

	int putData(string data);

};
