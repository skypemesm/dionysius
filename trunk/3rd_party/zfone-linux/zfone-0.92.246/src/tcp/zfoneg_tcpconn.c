/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
 */

#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>

#include "zfoneg_tcpconn.h"

//------------------------------------------------------------------------------
static int create_tcp_conn(tcp_conn_t* tc)
{
    if ( tc->is_ok )
    {
        return 1;
    }

    if ( tc->sock )
    {
        shutdown(tc->sock, 2);
        close(tc->sock);
        tc->sock = 0;
    }

    tc->sock = socket(PF_INET, SOCK_STREAM, 0);

    if ( tc->sock > 0 )
    {
        struct sockaddr_in sin;

        memset(&sin, 0, sizeof(sin));

        sin.sin_family          = AF_INET;
        sin.sin_addr.s_addr     = htonl(INADDR_LOOPBACK);
        sin.sin_port            = htons(5000);

        {
            if ( 0 == connect(tc->sock, (struct sockaddr*)&sin, sizeof(sin)) )
            {
                tc->is_ok = 1;

                if (tc->conn_ok_callback)
                    tc->conn_ok_callback();

		
				int fl = fcntl(tc->sock, F_GETFD);
				fl |= O_NONBLOCK;

				if ( 0 <= fcntl(tc->sock, F_SETFL, fl) )
				{
	          	    return 1;
				}
		
            }

            // Can't connect to the server zfone socket
            shutdown(tc->sock, 2);
            close(tc->sock);
            tc->sock = 0;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
static void destroy_tcp_conn(tcp_conn_t* tc)
{
    if ( tc->sock )
    {
        shutdown(tc->sock, 2);
        close(tc->sock);
        tc->sock  = 0;
        tc->is_ok = 0;
    }
}

//------------------------------------------------------------------------------
static ssize_t read_tcp_conn(tcp_conn_t* tc, char* buff, int size, int flags, int timeout)
{
    ssize_t res = -1;


    if ( tc->sock )
    {
	res = recv(tc->sock, buff, size, flags);

        if ( 0 > res && EAGAIN != errno )
        {
            tc->is_ok = 0;

            printf("!!!!!!!!!!! Zfone GUI read_tcp_conn: ERROR! read failed\n");

            if (tc->no_conn_callback)
                tc->no_conn_callback();
        }
    }

    return res;
}

//------------------------------------------------------------------------------
static ssize_t write_tcp_conn(tcp_conn_t* tc, char* buff, int size)
{
    ssize_t res = 0;

    if ( tc->sock )
    {
	res = send(tc->sock, buff, size, 0);
	
        if ( 0 > res && EAGAIN != errno )
        {
            tc->is_ok = 0;

            printf("Zfone GUI read_tcp_conn: ERROR! write failed possible problrm:%s\n", strerror(errno));

            if (tc->no_conn_callback)
                tc->no_conn_callback();
        }
    }

    return res;
}


/*!
    \brief init tcp_conn_t with proper pointers to functions
    
    Assign functions to operate on TCP connection, to the connection
*/
//------------------------------------------------------------------------------
void tcp_conn_init(tcp_conn_t* tc)
{
    memset(tc, 0, sizeof(*tc));

    tc->is_ok   = 0;
    tc->create  = create_tcp_conn;
    tc->destroy = destroy_tcp_conn;
    tc->read    = read_tcp_conn;
    tc->write   = write_tcp_conn;
}

void disable_tcp(tcp_conn_t* tc)
{
	tc->is_ok = 0;
	sleep(2);
}
