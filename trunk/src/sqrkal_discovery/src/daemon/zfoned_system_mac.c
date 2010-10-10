/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "zfone.h"


//------------------------------------------------------------------------------
zfoned_proc_state_t zfoned_get_proc_state(unsigned int pid)
{
	struct kinfo_proc *kp;
    int mib[4], nentries;
    size_t bufSize = 0;
    int status = 0;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = pid;
    
    if (sysctl(mib, 4, NULL, &bufSize, NULL, 0) < 0) 	
        return PROC_FAIL;
    
    kp= (struct kinfo_proc *)zrtp_sys_alloc( bufSize );
    if (kp == NULL)
		return PROC_FAIL;
    
	if ( sysctl(mib, 4, kp, &bufSize, NULL, 0) < 0 ) 
	{
		zrtp_sys_free(kp);
		return PROC_FAIL;
    }

    nentries = bufSize / sizeof(struct kinfo_proc);

    if ( nentries == 0 ) 
	{
		free(kp);
		return PROC_NO;
    }

	// process pooc state
	// mapping to zfone zfoned_proc_state_t for linux compatibility
	switch (kp->kp_proc.p_stat)
	{	
	case SIDL:
		status = PROC_FORK;
		break;

	case SRUN:		
		status = PROC_RUNNING;
		break;

	case SSLEEP:
		status = PROC_SLEEPING;		
		break;

	case SSTOP:
		status = PROC_STOPPED;
		break;

	case SZOMB:
		status = PROC_ZOMBIE;
		break;
	}

	free(kp);
	return status;
}

