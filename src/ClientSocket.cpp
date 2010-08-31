/***
 * This class implements all the methods required from a regular client socket
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "ClientSocket.h"



using namespace std;

	ClientSocket::ClientSocket(SRPPSession* mysession,string address,int port)
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

		thissession = mysession;

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
		SRPPMessage srpp_msg = srpp::create_srpp_message("");

		bytes_read = recvfrom(sock,&srpp_msg,sizeof(srpp_msg),0,
				(struct sockaddr *)&server_addr,
								(socklen_t *)&addr_len);

		//Decrypt
		CryptoProfile * crypto = new CryptoProfile("Simple XOR");

		srpp_msg = srpp::decrypt_srpp(&srpp_msg,crypto,thissession);


		srpp_msg.encrypted_part.original_payload[bytes_read] = '\0';


		if (bytes_read > 0)
		{
			printf("\n(%s , %d) said : ",inet_ntoa(server_addr.sin_addr),
							ntohs(server_addr.sin_port));
			printf("%s\n", (srpp_msg.encrypted_part.original_payload));
		}

		fflush(stdout);
		return string(srpp_msg.encrypted_part.original_payload);
	}

	/** Send data to the server socket  **/
	int ClientSocket::putData(string data)
	{
		//Create a SRPPMessage with data in its payload
		SRPPMessage srpp_msg = srpp::create_srpp_message(data);

		//encrypt
		CryptoProfile * crypto = new CryptoProfile("Simple XOR");

		srpp_msg = srpp::encrypt_srpp(&srpp_msg,crypto,thissession);

/*
		if (!(data == srpp_msg.encrypted_part.original_payload))
		{
			cout << "ERROR" << srpp_msg.encrypted_part.original_payload << endl;
			exit (-1);
		}
*/

		sendto(sock, &srpp_msg, sizeof(srpp_msg), 0,
			              (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

		cout << "\nWriting " << sizeof(srpp_msg) << " bytes \""
				<< srpp_msg.encrypted_part.original_payload << "\" to server" << endl;


	}
