/*
 * This source file implements all the major functions of the SQRKal app
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "SQRKal_functions.h"
#include "ServerSocket.h"
#include "ClientSocket.h"
#include "SRPPSession.hpp"
#include "CryptoProfile.hpp"

//Using JRTPLIB
#include "rtppacketbuilder.h"
#include "rtppacket.h"
#include "rtprawpacket.h"
#include "rtpaddress.h"
#include "rtpipv4address.h"


#include <iostream>
#include <string>
#include <cstdlib>

//include Qt framework
//#include <QTGui>

using namespace std;

		int initiator, sender_port, receiver_port, endpoint_receiver_port;
		string endpoint_ip_address;

	    int sender_sock, receiver_sock;         // handle to the socket
	    int addr_len, bytes_read;
	    char recv_data[1024];
	    struct sockaddr_in receiver_addr , sender_addr;
	    SRPPSession * newsession;

	//initialize stuff
	int init_SQRKal(int argcount, char * args[])
	{
		//<sender-port><receiver-port>(<endpoint-ip-address><endpoint-receiver-port>)
		initiator = atoi(args[1]);
		sender_port = atoi(args[2]);
		receiver_port = atoi(args[3]);
		if (argcount > 4)
		{
			endpoint_ip_address = args[4];
			endpoint_receiver_port = atoi(args[5]);
		}

		cout << initiator << "::" << sender_port << "::" << receiver_port << "::" << endpoint_ip_address << ":: "
				<< endpoint_receiver_port << endl;

		//setup the basic parameters
		cout << "Setting up the basic parameters in SQRKal......." << endl;

		//setup the basic media channels
		cout << "Setting up the basic media channels in SQRKal ......." << endl;
		if (start_SRPP() < 0)
			return -1;


		//wait for SRPP connections
		cout << "SQRKal waiting now for any SRPP connections......\n\n" << endl;

		return 0;
	}

	//create SRPP session
	int start_SRPP()
	{

		//initialize SRPP
		srpp::init_SRPP();

		//Create a SRPP Session with a particular CryptoProfile
		CryptoProfile * crypto = new CryptoProfile("Simple XOR");
		newsession = srpp::create_session(endpoint_ip_address, endpoint_receiver_port,*crypto);


		cout << "Session started at " << newsession->startTime << endl;
		cout << "FOR receiver with IP " << newsession->receiverIP << endl << endl;

		//Create the socket threads for sending and receiving channels
		create_sockets();

		cout << "Created the sockets..\n\n";

		return 0;
	}


	// initialize the GUI
	int init_GUI()
	{
		//As you already know we are using the QT Framework.

		//General window .
		// QApplication app(argc, argv);
		/* QLabel label("Hello, world!");
		 label.show();

		 return app.exec();
*/
		//return 0;
	}


	//Create the sender and receiver sockets ...
	int create_sockets()
	{
		//Create the Receiver (Server ) Socket
		cout << "Creating the server socket..\n\n";
		if ((receiver_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
			cerr << "Cannot create server socket. Exiting... \n\n";
					exit(1);
				}

				receiver_addr.sin_family = AF_INET;
				receiver_addr.sin_port = htons(receiver_port);
				receiver_addr.sin_addr.s_addr = INADDR_ANY;
				bzero(&(receiver_addr.sin_zero),8);


				if (bind(receiver_sock,(struct sockaddr *)&receiver_addr,
					sizeof(struct sockaddr)) == -1)
				{
					cerr << "Cannot bind server socket to " << receiver_port
							<< ". Exiting... \n\n";
					exit(1);
				}


			cout << "\nServer Waiting for client on port " << receiver_port << endl;
			fflush(stdout);

		if (initiator == 1)
		{
			//Create the Sender (Client Socket)
			cout << "Creating a client socket..\n";

			if ((sender_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
			{
				cerr << "Cannot create client socket. Exiting... \n\n";
				exit(1);
			}

			sender_addr.sin_family = AF_INET;
			sender_addr.sin_port = htons(endpoint_receiver_port);
			struct hostent * host = (struct hostent *) gethostbyname(endpoint_ip_address.c_str());
			sender_addr.sin_addr = *((struct in_addr *)host->h_addr);
			bzero(&(sender_addr.sin_zero),8);



			//set the sockets etc in the session
			newsession->set_sockets(sender_sock,receiver_sock,sender_addr,receiver_addr);

		}
		else  /// we will create the sending socket once we receive a message from an initiator endpoint
		{
			newsession->set_sockets(receiver_sock,receiver_addr);
		}

	}

	// Start the work now :)
	int start_call()
	{

		//In general there has to be two threads running - one for the server and other for the client.
		if (initiator == 1)
		{
			SRPPMessage srpp_msg = srpp::create_srpp_message ("SENDING FROM INITIATOR");
			int bytes = srpp::send_message(&srpp_msg);

			// Send the message using the sender socket
			//srpp::start_session(); // Right now we are not testing signaling

		}
		else
		{
			//wait till you receive the first message from an initiator, create the sender socket, and then
			//{send a message, wait for a message}*  _ THIS IS TILL WE ADD THREADING _
			SRPPMessage srpp_msg = srpp::receive_message();
			while (true)
			{
				cout << "Payload received right now: " << srpp_msg.encrypted_part.original_payload << endl;
				srpp_msg = srpp::receive_message();
			}

		}



	}


	// ------------------------------------------------------------------------------------------------//

	/*** Get a RTP Packet using JRTPLIB or SRPPLIB library ***/

	RTPMessage* ClientSocket::get_rtp_packet(string library, string data)
	{

		if (library == "srpplib")
		{
			return &(srpp::create_rtp_message(data));
		}
	else if (library == "jrtplib")
		{
			/// Get a RTPPacket Builder object and get a RTP packet and return it
			int status;
			//string data = "abc";

			//create and initialise a rtppacketbuilder object
			RTPPacketBuilder* pkt_builder = new RTPPacketBuilder();

			status = pkt_builder->Init(MAXPAYLOADSIZE);
			if (status < 0)
				cout << "ERROR INITIALISING RTPPACKETBUILDER: "<< status << endl;

			//set default params
			status = pkt_builder->SetDefaultPayloadType(27);
			if (status < 0)
				cout << "ERROR SETTING DEFAULT PAYLOADTYPE: "<< status << endl;

			status = pkt_builder->SetDefaultMark(false);
			if (status < 0)
				cout << "ERROR SETTING DEFAULT MARK: "<< status << endl;

			status = pkt_builder->SetDefaultTimestampIncrement(1);
					if (status < 0)
						cout << "ERROR SETTING DEFAULT TIMESTAMP: "<< status << endl;


			//build a packet
			status = pkt_builder->BuildPacket(&data,data.length());

			if (status < 0)
				cout << "ERROR BUILDING A PACKET: "<< status << endl;

			cout << "PacketLength:" << pkt_builder->GetPacketLength() << endl;

			//set it to rtppacket and return the pointer
			RTPIPv4Address address(ntohl(inet_addr("127.0.0.1")),35000);
			RTPTime thistime = RTPTime::CurrentTime();

			void * nothing;

			RTPRawPacket* rtp_rawpacket = new RTPRawPacket(
										pkt_builder->GetPacket(),
										pkt_builder->GetPacketLength(),
										&address,
										thistime,
										true
										);

			RTPPacket * rtp_packet = new RTPPacket(*rtp_rawpacket);


			cout << "payload type:"<< (int)(rtp_packet->GetPayloadType()) << endl;
			cout << "sequence number:" << (rtp_packet->GetSequenceNumber()) << endl;

			int len = (rtp_packet->GetPayloadLength());
			uint8_t* str = rtp_packet->GetPayloadData();

			for(int i = 0; i <len; i++)
				printf("%c ",str[i]);

			str[len] = '\0';
			//printf("Nope:%s\n",itoa(str));
			cout <<"This is it: " << str << endl;

			return (RTPMessage *)rtp_packet;

		}
	}

	//destructor stuff
	int destroy_SQRKal()
	{

	}
