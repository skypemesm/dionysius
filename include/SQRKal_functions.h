/*
 * This header file contains all the major functions of the SQRKal app
 *
 * @author Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include <string>
#include "SRPP_functions.h"

using namespace std;

	//initialize stuff
	int init_SQRKal(int argcount, char ** args);

	//create SRPP session
	int start_SRPP();

	//Start the actual call
	int start_call();

	// initialize the GUI
	int init_GUI();

	//Create the Sender and Receiver Sockets
	int create_sockets();

	//destructor stuff
	int destroy_SQRKal();

	/*** Get a RTP Packet using JRTPLIB or libSRPP library ***/
	RTPMessage* get_rtp_packet(string library, string data);
