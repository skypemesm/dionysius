/*
 * This header file implements all
 * the methods required from a regular server socket
 *
 * @author Saswat Mohanty <smohanty@cs.tamu.edu>
 */


#include "ServerSocket.h"
#include "SRPP_functions.h"



using namespace std;

	/** Constructor **/
    ServerSocket::ServerSocket(SRPPSession * mysession,int ServerPort)
    {
    	cout << "Creating the server socket..\n\n";
    	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    		cerr << "Cannot create server socket. Exiting... \n\n";
    	            exit(1);
    	        }

    	        server_addr.sin_family = AF_INET;
    	        server_addr.sin_port = htons(ServerPort);
    	        server_addr.sin_addr.s_addr = INADDR_ANY;
    	        bzero(&(server_addr.sin_zero),8);


    	        if (bind(sock,(struct sockaddr *)&server_addr,
    	            sizeof(struct sockaddr)) == -1)
    	        {
    	        	cerr << "Cannot bind server socket to " << ServerPort
    	        			<< ". Exiting... \n\n";
    	            exit(1);
    	        }

    	        addr_len = sizeof(struct sockaddr);

    	        thissession = mysession;

    		cout << "\nServer Waiting for client on port " << ServerPort << endl;
    	    fflush(stdout);

  }



    /** Destructor **/
    ServerSocket::~ServerSocket()
    {
		/* close socket */
		if(close(sock) == -1)
		{
			 cout << "\nCould not close socket\n";
			 return ;
		}


    }

    /**Receive data from the socket **/
	string ServerSocket::getData()
	{
		SRPPMessage srpp_msg = srpp::create_srpp_message("");

		bytes_read = recvfrom(sock,&srpp_msg,sizeof(srpp_msg),0,
				(struct sockaddr *)&server_addr,
								(socklen_t *)&addr_len);
		cout << bytes_read << endl;

		//decrypt the packet
		srpp_msg = srpp::decrypt_srpp(&srpp_msg);


		srpp_msg.encrypted_part.original_payload[bytes_read] = '\0';

		if (bytes_read > 0)
		{
			printf("\n(%s , %d) said : ",inet_ntoa(client_addr.sin_addr),
														   ntohs(client_addr.sin_port));
			printf("%s\n", srpp_msg.encrypted_part.original_payload);
		}

		fflush(stdout);
		return string(srpp_msg.encrypted_part.original_payload);
	}

	/**Send data to the socket **/
	int ServerSocket::putData(string data)
	{
		SRPPMessage srpp_msg = srpp::create_srpp_message(data);

		//encrypt
		srpp_msg = srpp::encrypt_srpp(&srpp_msg);


		data.copy((srpp_msg.encrypted_part.original_payload),data.length(),0);

		sendto(sock, &srpp_msg, sizeof(srpp_msg), 0,
	              (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

	     cout << "\nWriting " << sizeof(srpp_msg) << " bytes \""<< srpp_msg.encrypted_part.original_payload /*data.c_str()*/ << "\" to client" << endl;

		return 1;
	}
