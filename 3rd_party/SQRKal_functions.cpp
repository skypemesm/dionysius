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

#include <iostream>
#include <string>

//include Qt franework
//#include <QTGui>

using namespace std;


	//initialize stuff
	int init_SQRKal()
	{
		//setup the basic parameters
		cout << "Setting up the basic parameters in SQRKal......." << endl;

		//setup the basic signaling channels
		cout << "Setting up the basic signaling channels in SQRKal ......." << endl;

		//wait for SRPP connections
		cout << "SQRKal waiting now for any SRPP connections......\n\n" << endl;

		return 0;
	}

	//create SRPP session
	int start_SRPP(string address, int port)
	{

		//initialize SRPP
		srpp::init_SRPP();

		//Create a SRPP Session
		CryptoProfile * crypto = new CryptoProfile("Simple XOR");
		SRPPSession* newsession = srpp::create_session(address,port,*crypto);


		cout << "Session started at " << newsession->startTime << endl;
		cout << "FOR receiver with IP " << newsession->receiverIP << endl << endl;

		//Create the socket threads

		if (address == "receiver")
		{
			ServerSocket* serversock = new ServerSocket(newsession,3530);
			srpp::start_session();
			while(true)
			{
				string data = serversock->getData();
				//serversock->putData("sending from server");

				cout << "-----------------------------------------\n";
			}
		}
		else
		{
			cout << " Trying to contact SQRKal endpoint at " << address << endl;

			ClientSocket* clientsock = new ClientSocket(newsession,address,3530);
			srpp::start_session();

			//clientsock->get_rtp_packet("abc");

			string data = "";
			//while (data.empty()){
				clientsock->putData("sending from client");
				//data = clientsock->getData();
			//}

		}

		cout << "Created the sockets..\n\n";

		cout << "Session ending now...\n\n";

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
