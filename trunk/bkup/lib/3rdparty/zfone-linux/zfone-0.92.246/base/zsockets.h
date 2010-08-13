/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov <egm@soft.cn.ua>, <m.yegorov@gmail.com>
 */

#ifndef __ZSOCKETS_H__
#define __ZSOCKETS_H__

#include <sys/types.h>

extern ssize_t sig_aware_write(int fd, const void* buf, size_t count);

// sock - socket
// buf  - dest buffer
// len  - buffer length
// flags - falgs ( see man recv)
// read_timeout - timeout in msecs
extern ssize_t mtimed_recv(int sock
			  ,void *buf
			  ,const size_t len
			  ,const int flags
			  ,const int read_timeout);

#endif //__ZSOCKETS_H__
