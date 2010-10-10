/*
 * This source file implements all the major functions of the SQRKal app
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "SQRKal_functions.h"
#include "SRPPSession.hpp"
#include "CryptoProfile.hpp"

//Using JRTPLIB
#include "rtppacketbuilder.h"
#include "rtppacket.h"
#include "rtprawpacket.h"
#include "rtpaddress.h"
#include "rtpipv4address.h"


#include <iostream>
#include <cstring>
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
	int init_SQRKal(int argcount, char **args)
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

		/*cout << initiator << "::" << sender_port << "::" << receiver_port << "::" << endpoint_ip_address << ":: "
				<< endpoint_receiver_port << endl;
*/
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
			fflush(stdout);

			//Create the Sender (Client Socket)
			if ((sender_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
			{
				cerr << "Cannot create client socket. Exiting... \n\n";
				exit(1);
			}

			sender_addr.sin_family = AF_INET;
			sender_addr.sin_port = htons(sender_port);
			sender_addr.sin_addr.s_addr = INADDR_ANY;
			bzero(&(sender_addr.sin_zero),8);

			if (bind(sender_sock,(struct sockaddr *)&sender_addr,
								sizeof(struct sockaddr)) == -1)
			{
				cerr << "Cannot bind client socket to " << sender_port
						<< ". Exiting... \n\n";
				exit(1);
			}


		// If we are the initiator, we will need to contact the other endpoint and hence we set the appropriate sender_addr structure
		if (initiator == 1)
		{
			//SET SENDER_ADDR TO POINT TO THE ENDPOINT RECEIVER
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
			newsession->sendersocket = sender_sock;
		}

	}

	// Start the work now :)
	int start_call()
	{

		//In general there has to be two threads running - one for the server and other for the client.

		//If we are the initiator, we will start the session
		if (initiator == 1)
		{
/*			SRPPMessage sr = srpp::create_srpp_message("TEST MESSAGE");
			sr.srpp_header.version = 3;
			sr.authentication_tag = 1755;
			sr.encrypted_part.original_seq_number = 1012;
			sr.encrypted_part.dummy_flag = 1;

			string str = "DUMMY HERE";
			sr.encrypted_part.srpp_padding = vector<char>(str.begin(),str.end());
			sr.encrypted_part.pad_count = sr.encrypted_part.srpp_padding.size();

			srpp::send_message(&sr);
			return -1;*/
/*
char data[40];
			sprintf(data,"Test message no. 3");

			//get a rtp packet
			RTPMessage * rtp_msg = get_rtp_packet("srpplib",data);

			//Pad it and make a SRPP message
			SRPPMessage srpp_msg = srpp::rtp_to_srpp(rtp_msg);

			srpp_msg.print();
			//encrypt it
			srpp_msg = srpp::encrypt_srpp(&srpp_msg);

			srpp_msg.print();

			//send it through
			srpp::send_message(&srpp_msg);

return -1;
*/

			// Initiate the session..
			if (srpp::start_session() >= 0)
			{
				newsession->srpp_timer->pauseTimer();

				char data[40];

				//The signaling etc has been complete
				//Now start sending and receiving messages
				for (int i = 0; i < 300 ; i++)
				{
					sprintf(data,"Test message no. %d", i);

					//get a rtp packet
					RTPMessage * rtp_msg = get_rtp_packet("srpplib",string(data));

					//Pad it and make a SRPP message
					SRPPMessage srpp_msg = srpp::rtp_to_srpp(rtp_msg);

					//encrypt it
					//srpp_msg = srpp::encrypt_srpp(&srpp_msg);

					//send it through
					srpp::send_message(&srpp_msg);

					cout << "..Sending packet with data " << data << "..." << endl;

				}

			}
			else
			{
				cout << "\n\n SIGNALING FAILED.. SQRKAL CANNOT WORK SINCE SRPP IS NOT SUPPORTED AT OTHER END.. EXITING.."<< endl;
				return -1;
			}

			// Stop the session now.
			//srpp::stop_session();

			while (true);
		}
		else     // If we are NOT the initiator, we need to wait for an incoming session
		{
			//wait till you receive the first message from an initiator, create the sender socket, and then
			//{send a message, wait for a message}*  _ THIS IS TILL WE ADD THREADING _
			SRPPMessage srpp_msg = srpp::receive_message();
//return -1;
			RTPMessage rtp_msg;

			//Block for other signaling messages.
			while (srpp::isSignalingComplete() != 1)
			{
				//cout << "Payload received right now (SIGNALING): " << srpp_msg.encrypted_part.original_payload << endl;
				srpp_msg = srpp::receive_message();
			}

			//Block to receive and send media messages
			while(srpp::isMediaSessionComplete() != 1)
			{
				srpp_msg = srpp::receive_message();

				if (srpp::isSignalingMessage(&srpp_msg) != 1)
				{
					cout << ".. Received SRPP Packet has Sequence Number:" << srpp_msg.srpp_header.seq << endl;
					//srpp_msg.print();

					//decrypt the message
					//srpp_msg = srpp::decrypt_srpp(&srpp_msg);

					// Convert to RTP message
					rtp_msg = srpp::srpp_to_rtp(&srpp_msg);

					if (!string(rtp_msg.payload).empty())
					{
						cout << ".. Received Packet has Media Payload:" << rtp_msg.payload << "..." << endl;
						cout << ".. Received Packet has Sequence Number:" << rtp_msg.rtp_header.seq << "..." << endl;
					}
					else
					{
						cout << " -- Received a dummy packet --- \n\n";
					}
				}

			}



		}



	}


	// ------------------------------------------------------------------------------------------------//

	/*** Get a RTP Packet using JRTPLIB or SRPPLIB library ***/

	RTPMessage* get_rtp_packet(string library, string data)
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
