/*
 * This source file implements all the major functions of the SQRKal app
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include "SQRKal_functions.h"
//#include "SRPPSession.h"
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
	int create_SRPPSession(string address)
	{
		cout << " Trying to contact SQRKal endpoint at " << address << endl;

	}


	// initialize the GUI
	int init_GUI()
	{

	}
