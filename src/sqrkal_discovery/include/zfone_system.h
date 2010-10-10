/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Victor Krikun <v.krikun@soft-industry.com>
 */

#ifndef __ZFONE_SYSTEM_H__
#define __ZFONE_SYSTEM_H__

typedef enum zfoned_proc_state_t
{
    PROC_RUNNING	= 0,
    PROC_SLEEPING	= 1,
    PROC_DSLEEP		= 2,
    PROC_STOPPED	= 3,
    PROC_TSTOP		= 4,
    PROC_ZOMBIE		= 5,
    PROC_DEAD		= 6,
    PROC_FORK		= 7,
    PROC_FAIL		= 8,
    PROC_NO		= 9
} zfoned_proc_state_t;

zfoned_proc_state_t zfoned_get_proc_state(unsigned int pid);

#endif //__ZFONE_SYSTEM_H__
