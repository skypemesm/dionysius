/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
 */

#ifndef __ZFONEG_TCP_CONN_H__
#define __ZFONEG_TCP_CONN_H__

#include <sys/types.h>

/*!
    \defgroup ControlClient Control Client
    \{
*/

typedef struct tcp_conn tcp_conn_t;

/*!	
    \brief tcp connection

    Control client connection.    
*/
struct tcp_conn
{
    int sock;	/*!< socket */
    int	is_ok;  /*!< flag signalling that connection is ok */

    int     (*create)(tcp_conn_t* tc);   /*!< creates connection. Makes socket and
					  connects to the control server */
    void    (*destroy)(tcp_conn_t* tc);  /*!< destroys connection */
    ssize_t (*read)(tcp_conn_t* tc, char* buff, int size, int flags, int timeout);
	    /*!< read data from the connection
		\param tc connection  
		\param buff buffer to store read data
		\param size size of the buffer
		\param flags specific flags to pass to read function
		\param timeout timeout in msec to expire read from the connection \a tc 
		
		\return amount of bytes read
	     */
    ssize_t (*write)(tcp_conn_t* tc, char* buff, int size);
	    /*!< write data to the connection
		\param tc connection  
		\param buff buffer storing data to send
		\param size size of the buffer
		
		\return amount of bytes send
	    */
    
    void    (*no_conn_callback)(void); /*!< Callback to call when connection is broken */
    void    (*conn_ok_callback)(void); /*!< Callback when connection became ok */
};

extern void tcp_conn_init(tcp_conn_t* tc);

void disable_tcp(tcp_conn_t* tc);

/*!
    \}
*/

#endif //__ZFONEG_TCP_CONN_H__
