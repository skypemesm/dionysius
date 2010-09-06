/*
 * This header file contains all the major functions of the SQRKal app
 *
 * @author Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include <string>
#include "SRPP_functions.h"

using namespace std;

	//initialize stuff
	int init_SQRKal();

	//create SRPP session
	int start_SRPP(string address,int port);

	// initialize the GUI
	int init_GUI();
