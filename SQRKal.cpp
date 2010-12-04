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
#include <cstdio>
#include <string>
#include "md5.h"
#include "sqrkal_discovery.h"

using namespace std;

/***
 * Usage: (As of now) sqrkal <1-if initiator,0-if not initiator><sender-port><receiver-port>(<endpoint-ip-address><endpoint-receiver-port>)
 *
 */

// Main entry point
int main(int argc, char * argv[])
{
	freopen ("output_log.txt","w",stdout);

	if (argc == 2 )
	{
		string arg1 = argv[1];

		if (arg1 == "--full-bandwidth" || arg1 == "-fb")
			srpp::set_full_bandwidth();

		if (arg1 == "--burst-padding" || arg1 == "-bp")
					srpp::set_burst_padding();

		if (arg1 == "--gradual-ascent" || arg1 == "-ga")
					srpp::set_gradual_ascent();

		if (arg1 == "--min-prob-bin" || arg1 == "-mpb")
					srpp::set_min_prob_bin();

		if (arg1 == "--small-perturbation" || arg1 == "-sp")
					srpp::set_small_perturbation();

		if (arg1 == "--usage" )
		{	cerr << "./sqrkal [--full-bandwidth | -fb]|[--burst-padding | -bp]|[--gradual-ascent | -ga]|[--min-prob-bin | -mpb][--small-perturbation | -sp] "
				"	" << endl; return 1;}

	}

	sqrkal_discovery sqrkald;
	fclose(stdout);

	return 0;

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

