/***
 * This class implements all the methods required from a regular client socket
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "TCP_ClientSocket.h"

using namespace std;

	TCP_ClientSocket::TCP_ClientSocket(string address,int port)
	{
		cout << "client socket\n\n";

		strcpy(strHostName , address.c_str());
		nHostPort = port;

		printf("\nMaking a socket\n");
		 //make a socket
		hSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

		if(hSocket == SOCKET_ERROR)
		    {
		        printf("\nCould not make a socket\n");
		        return;
		    }

		//	  get IP address from name
		 pHostInfo=gethostbyname(strHostName);

		 // copy address into long
		 memcpy(&nHostAddress,pHostInfo->h_addr,pHostInfo->h_length);

		 // fill address struct
		 Address.sin_addr.s_addr=nHostAddress;
		 Address.sin_port=htons(nHostPort);
		 Address.sin_family=AF_INET;

		 printf("\nConnecting to %s on port %d",strHostName,nHostPort);

		//  connect to host
		 if(connect(hSocket,(struct sockaddr*)&Address,sizeof(Address))
		   == SOCKET_ERROR)
		 {
			printf("\nCould not connect to host\n");
			return;
		 }


	}


	TCP_ClientSocket::~TCP_ClientSocket()
	{

	    printf("\nClosing socket\n");
	    // close socket
	    if(close(hSocket) == SOCKET_ERROR)
	    {
	        printf("\nCould not close socket\n");
	        return ;
	    }
	}

	string TCP_ClientSocket::readData()
	{
		/* read from socket into buffer
		** number returned by read() and write() is the number of bytes
		** read or written, with -1 being that an error occurred */
		nReadAmount=read(hSocket,pBuffer,BUFFER_SIZE);
		printf("\nReceived \"%s\" from server\n",pBuffer);

		return pBuffer;
	}

	int TCP_ClientSocket::putData(string data)
	{
		/* write what we received back to the server */
		write(hSocket,data.c_str(),nReadAmount);
		printf("\nWriting \"%s\" to server",data.c_str());

	}
