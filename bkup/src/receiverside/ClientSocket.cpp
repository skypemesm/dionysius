/*
 * ClientSocket.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: saswat
 */

#include "ClientSocket.h"
#include "SocketException.h"
#include <iostream>
#include <string>



int initSocket( std::string ServerHost,int clientPort )
{
  try
    {

      ClientSocket client_socket ( serverHost, clientPort );

      std::string reply;
      try
	{
	  client_socket << "Test message.";
	  client_socket >> reply;
	}
      catch ( SocketException& ) {}

      std::cout << "We received this response from the server:\n\"" << reply << "\"\n";;

    }
  catch ( SocketException& e )
    {
      std::cout << "Exception was caught:" << e.description() << "\n";
    }

  return 0;
}

