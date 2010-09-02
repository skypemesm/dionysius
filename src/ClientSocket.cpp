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

		//get a rtp packet
		RTPMessage * rtp_msg = get_rtp_packet("srpplib",data);

		//Pad it and make a SRPP message
		SRPPMessage srpp_msg_this = srpp::rtp_to_srpp(rtp_msg);

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




	/*** Get a RTP Packet using JRTPLIB library ***/

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
