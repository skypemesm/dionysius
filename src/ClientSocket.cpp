/***
 * This class implements all the methods required from a regular client socket
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "ClientSocket.h"

using namespace std;

	ClientSocket::ClientSocket(string address,int port)
	{
		cout << "Creating a client socket..\n";

		host = (struct hostent *) gethostbyname(address.c_str());


		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			cerr << "Cannot create client socket. Exiting... \n\n";
			exit(1);
		}

		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		server_addr.sin_addr = *((struct in_addr *)host->h_addr);
		bzero(&(server_addr.sin_zero),8);


		addr_len = sizeof(struct sockaddr);

	}


	ClientSocket::~ClientSocket()
	{

	    printf("\nClosing socket\n");
	    // close socket
	    if(close(sock) == -1)
	    {
	        printf("\nCould not close socket\n");
	        return ;
	    }
	}

	/** Get data from the server socket  **/
	string ClientSocket::getData()
	{

		bytes_read = recvfrom(sock,recv_data,1024,0,(struct sockaddr *)&server_addr,
						(socklen_t *)&addr_len);


		recv_data[bytes_read] = '\0';

		if (bytes_read > 0)
		{
			printf("\n(%s , %d) said : ",inet_ntoa(server_addr.sin_addr),
														   ntohs(server_addr.sin_port));
			printf("%s\n", recv_data);
		}

		fflush(stdout);

		return string(recv_data);
	}

	/** Send data to the server socket  **/
	int ClientSocket::putData(string data)
	{
	     sendto(sock, data.c_str(), data.length(), 0,
	              (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

	     cout << "\nWriting \""<< data.c_str() << "\" to server" << endl;


	}
