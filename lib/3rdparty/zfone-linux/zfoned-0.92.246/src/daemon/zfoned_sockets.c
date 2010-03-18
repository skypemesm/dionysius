/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov <egm@soft.cn.ua>, <m.yegorov@gmail.com>
 */

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>

#include <zrtp.h>
#include "zfone.h"

//#include "zfoned_sockets.h"

//------------------------------------------------------------------------------
ssize_t sig_aware_write(int fd, const void* buf, size_t count)
{
    ssize_t rv = -1;
    do
    {
#if ZRTP_PLATFORM == ZP_LINUX
	rv = send(fd, buf, count, MSG_NOSIGNAL);
#else
	rv = send(fd, buf, count, 0);
#endif
    } while ( rv == -1 && errno == EINTR );

    return rv;
}

//------------------------------------------------------------------------------
ssize_t mtimed_recv(int s, void *buf, const size_t len, const int flags, const int read_timeout)
{
    sigset_t origmask;
    sigset_t sigmask;

    static const short poll_mask = POLLIN;
    int		       msg_size  = 0;

    struct pollfd pfd = {s, poll_mask, 0};	

    sigfillset(&sigmask);
    pthread_sigmask(SIG_SETMASK, &sigmask, &origmask);

    if ( 0 < poll(&pfd, 1, read_timeout) )
    {
	//printf("mtimed_recv %i revents 0x%x, events 0x%x\n", pfd.fd, pfd.revents, pfd.events);

	if ( pfd.revents & POLLIN )
	{
	    msg_size = recv(s, buf, len, 0);

	    if ( !msg_size )
		msg_size = -1;
	}
	else if ( pfd.revents & ( POLLHUP | POLLERR | POLLNVAL ) )
	{
	    //printf("mtimed_recv: some shit happened!\n");
	    msg_size = -1;
	}
    }
    //else
	//printf("timed_recv receive timed out (s=%i: %s)\n", s, strerror(errno));

    pthread_sigmask(SIG_SETMASK, &origmask, NULL);

    //printf("mtimed_recv receive (s=%i: %i)\n", s, msg_size);
    return msg_size;
}
