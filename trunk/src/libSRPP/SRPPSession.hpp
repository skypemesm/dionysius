/**
 *
 * This class contains all the information related to a SRPP Session
 * like the addresses of the endpoints, level of padding, and other parameters.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#include <iostream>
#include <string>
#include <ctime>

using namespace std;

class SRPPSession
{

public:
	string receiverIP, startTime;
	int receiverPort,myPort;
	unsigned int encryption_key;

	/**
	 * @param receiverIP: IP of the receiver
	 * @param receiverPort: Port of the client
	 * @param myPort: port at which my server is running
	 * @start time
	 */
	SRPPSession(
			string thisreceiverIP,
			int thisreceiverPort,
			int thismyPort
			)
	{
		receiverIP = thisreceiverIP;
		receiverPort = thisreceiverPort;
		myPort = thismyPort;

		//get present date time
		time_t rawtime;
		struct tm * timeinfo;
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );

		startTime = asctime(timeinfo);

		encryption_key = 1655;

	}


};
