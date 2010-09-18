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
#include <string>
#include "md5.h"

using namespace std;

/***
 * Usage: (As of now) sqrkal <1-if initiator,0-if not initiator><sender-port><receiver-port>(<endpoint-ip-address><endpoint-receiver-port>)
 *
 */

// Main entry point
int main(int argc, char * argv[])
{
	if (argc < 6 )
	{
		if (argc < 4 || *argv[1] == '1')
		{
		cout << " Usage: (As of now)" << argv[0] <<
				" <1-if initiator,0-if not initiator><sender-port><receiver-port>(<endpoint-ip-address><endpoint-receiver-port>)\n\n"
				" You must specify the endpoint-ip-address and endpoint-receiver-port, if you are the initiator. \n\n";

		return -1;
		}
	}

	cout << "SQRKal starting up ........\n\n";

	//initialize stuff
	init_SQRKal(argc,argv);

	// initialize the GUI
	init_GUI();

	//start the actual call + signaling NOW
	start_call();

	cout << "Arighty then ..We are done. \n";

	destroy_SQRKal();

}
