
/*
 * ServerSocket.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: saswat
 */

#include "ServerSocket.h"
#include "SocketException.h"
#include <string>

int initSocket ( int serverPort )
{
  try
    {
      // Create the socket
      ServerSocket server ( serverPort );

      while ( true )
	{

	  ServerSocket new_sock;
	  server.accept ( new_sock );

	  try
	    {
	      while ( true )
		{
		  std::string data;
		  new_sock >> data;
		  new_sock << data;
		}
	    }
	  catch ( SocketException& ) {}

	}
    }
  catch ( SocketException& e )
    {
      std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
    }

  return 0;
}

