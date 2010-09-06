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


//Using JRTPLIB
#include "rtppacketbuilder.h"
#include "rtppacket.h"
#include "rtprawpacket.h"
#include "rtpaddress.h"
#include "rtpipv4address.h"

#include "SRPP_functions.h"

using namespace std;

class ClientSocket
{
	int sock;   					/** handle to socket */
	struct sockaddr_in server_addr;
	struct hostent *host;
	int bytes_read, addr_len;
	char recv_data[1024];
	SRPPSession * thissession;

public:
	ClientSocket(SRPPSession* mysession,string , int);
	~ClientSocket();

	string getData();

	int putData(string data);

	RTPMessage* get_rtp_packet(string library, string data);

};
