/*
 * This source file implements all the major functions of the SQRKal app
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "SQRKal_functions.h"
#include "ServerSocket.h"
#include "ClientSocket.h"
#include "SRPPSession.hpp"

#include <iostream>
#include <string>

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

	}

	//create SRPP session
	int create_SRPPSession(string address, int port)
	{

		//Create a SRPP Session
		SRPPSession* newsession = new SRPPSession(
									address,port,port);


		cout << "Session started at " << newsession->startTime << endl;
		cout << "FOR receiver with IP " << newsession->receiverIP << endl << endl;

		//Create the socket threads

		if (address == "receiver")
		{
			ServerSocket* serversock = new ServerSocket(3530);
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
			ClientSocket* clientsock = new ClientSocket(address,3530);

			string data = "";
			while (data.empty()){
				clientsock->putData("sending from client");
				//data = clientsock->getData();
			}

		}

		cout << "Created the sockets..\n\n";

		cout << "Session ending now...\n\n";

	}


	// initialize the GUI
	int init_GUI()
	{

	}
