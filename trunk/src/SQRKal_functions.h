/*
 * This header file contains all the majore functions of the SQRKal app
 *
 * @author Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include <string>
using namespace std;

	//initialize stuff
	int init_SQRKal();

	//create SRPP session
	int create_SRPPSession(string address);

	// initialize the GUI
	int init_GUI();
