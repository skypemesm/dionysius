/*
 * Entry point of the SQRKal application
 *
 * This app will secure your VoiP calls from traffic analysis attacks by padding
 * the RTP/SRTP traffic appropriately.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */


#include "SQRKal_functions.h"
#include <iostream>
#include <cstdlib>
#include <string>

using namespace std;

/***
 * Usage: (As of now) sqrkal <1-if sender,0-if receiver>(<receiver-ip-address><receiver-port>)
 *
 */

// Main entry point
int main(int argc, char * argv[])
{

	if (argc < 2)
	{
		cout << " Usage: (As of now)" << argv[0] <<
				" <1-if sender,0-if receiver>(<receiver-ip-address><receiver-port>)\n\n";

		return -1;
	}

	cout << "SQRKal starting up ........\n\n";

	//initialize stuff
	init_SQRKal();

	//create SRPP session
	if (*argv[1] == '0')      // This endpoint wants to start as a receiver
	{
		create_SRPPSession("receiver",0);
	}
	else if (*argv[1] == '1')  // This endpoint wants to start as a sender
	{
		create_SRPPSession(argv[2],atoi(argv[3]));
	}

	// initialize the GUI
	init_GUI();

	cout << "Arighty then ..We are done. \n";

}
