/*
 * This header file implements all
 * the methods required from a regular server socket
 *
 * @author Saswat Mohanty <smohanty@cs.tamu.edu>
 */


	#include "TCP_ServerSocket.h"

	using namespace std;

	/** Constructor **/
    TCP_ServerSocket::TCP_ServerSocket(int ServerPort)
    {
    	nHostPort = ServerPort;
    	nAddressSize=sizeof(struct sockaddr_in);


    	/* make a socket */
    	 hServerSocket=socket(AF_INET,SOCK_STREAM,0);

		if(hServerSocket == SOCKET_ERROR)
		{
			cout << "\nCould not make a server socket\n";
			return ;
		}

		/* fill address struct */
		Address.sin_addr.s_addr=INADDR_ANY;
		Address.sin_port=htons(nHostPort);
		Address.sin_family=AF_INET;

		cout << "\nBinding to port " << nHostPort;

		/* bind to a port */
		if(bind(hServerSocket,(struct sockaddr*)&Address,sizeof(Address))
							== SOCKET_ERROR)
		{
			cout << "\nCould not connect to host\n";
			return ;
		}

		/*  get port number */
		getsockname( hServerSocket, (struct sockaddr *) &Address,(socklen_t *)&nAddressSize);
		printf("opened socket as fd (%d) on port (%d) for stream i/o\n",hServerSocket, ntohs(Address.sin_port) );

			printf("Server\n\
				  sin_family        = %d\n\
				  sin_addr.s_addr   = %d\n\
				  sin_port          = %d\n"
				  , Address.sin_family
				  , Address.sin_addr.s_addr
				  , ntohs(Address.sin_port)
				);


		printf("\nMaking a listen queue of %d elements",QUEUE_SIZE);

		/* establish listen queue */
		if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR)
		{
			cout << "\nCould not listen\n";
			return ;
		}

		printf("\nWaiting for a connection\n");
		/* get the connected socket */
		hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);

		printf("\nGot a connection");


  }



    /** Destructor **/
    TCP_ServerSocket::~TCP_ServerSocket()
    {
		/* close socket */
		if(close(hSocket) == SOCKET_ERROR)
		{
			 cout << "\nCould not close socket\n";
			 return ;
		}


    }


	string TCP_ServerSocket::getData()
	{
		read(hSocket,pBuffer,BUFFER_SIZE);

		if(strcmp(pBuffer,MESSAGE) == 0)
			printf("\nThe messages match");
		else
			printf("\nSomething was changed in the message");

		return pBuffer;
	}

	int TCP_ServerSocket::putData(string data)
	{

		strcpy(pBuffer,MESSAGE);
		printf("\nSending \"%s\" to client",pBuffer);
		/* number returned by read() and write() is the number of bytes
		** read or written, with -1 being that an error occured
		** write what we received back to the server */
		write(hSocket,pBuffer,strlen(pBuffer)+1);
		/* read from socket into buffer */
	}
