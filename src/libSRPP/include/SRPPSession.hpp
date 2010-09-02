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
	int receiverPort,myPort, lastSeqNo;
	time_t start_time;
	double currentBurstTime;
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
		struct tm * timeinfo;
		time ( &start_time );
		timeinfo = localtime ( &start_time );

		startTime = asctime(timeinfo);

		encryption_key = 1655;

		lastSeqNo = 0;
		currentBurstTime = 0;

	}

	int increment_seq()
	{
		return ++lastSeqNo;
	}

	int clear_seq()
	{
		return (lastSeqNo = 0);
	}

	void increment_bursttime()
	{
		time_t timenow;
		time(&timenow);
		currentBurstTime += difftime(timenow,start_time);
	}

	int clear_currentBurstTime()
	{
		return (currentBurstTime = 0);
	}
};
