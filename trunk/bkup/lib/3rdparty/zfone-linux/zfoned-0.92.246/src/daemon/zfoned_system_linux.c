/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>

#include "zfone.h"

// from /usr/src/linux/fs/proc/array.c.
static const char *proc_state_chars[] = 
{
	"R (running)",
	"S (sleeping)",
	"D (disk sleep)",
	"T (stopped)",
	"T (tracing stop)",
	"Z (zombie)",
	"X (dead)"
};

#define proc_states_count 7
#define STATE_STR	"State:"

//------------------------------------------------------------------------------
static zfoned_proc_state_t get_state(char* state_str)
{
    int pos = 0;
    
    for (pos=0; pos<proc_states_count; pos++)
    {
		if ( strstr(state_str, proc_state_chars[pos]) )    
	    	break;
    }
    
    if (proc_states_count == pos)
		pos = PROC_FAIL;
    
    return pos;
}

//------------------------------------------------------------------------------
zfoned_proc_state_t zfoned_get_proc_state(unsigned int pid)
{
    zfoned_proc_state_t status = PROC_NO;
    FILE* proc_stat_fd = NULL;
    char proc_path[20];
    memset(proc_path, 0, sizeof(proc_path));
    
    printf("zfoned: zfoned_check_env: start checking for pid=%d.\n", pid);
    
    //create proc path
    snprintf(proc_path, sizeof(proc_path), "%s%i/%s", "/proc/", pid, "status");
    
    //looking for process folder at /proc
    printf("zfoned: zfoned_check_env: try to open status file (%s) for our process.\n", proc_path);
    proc_stat_fd = fopen(proc_path, "r");
    if ( !proc_stat_fd )
  	{
		printf("zfoned: zfoned_check_env: can't open %s - process doesn't exist.\n", proc_path);
    }
    else
    {
		char proc_str[256];
		char proc_file[128];
		char* state_ptr = NULL;
		
		printf("zfoned: zfoned_check_env: process exist - get status.\n");
		
		memset(proc_str, 0, sizeof(proc_str));
		memset(proc_file, 0, sizeof(proc_file));
		
		//read pocess status
		if ( 0 >= fread(proc_str, sizeof(proc_str)-1, 1, proc_stat_fd))
		{
			printf("zfoned: zfoned_check_env: ERROR! can't read status line.\n");
			status = PROC_FAIL;
		}
		else
		{
			state_ptr = strstr(proc_str, STATE_STR);
			if ( !state_ptr )
			{
				printf("zfoned: zfoned_check_env: ERROR! can't find %s line"
					   " at status string at %s\n", STATE_STR, proc_str);
				status = PROC_FAIL;
			}
			else
			{
				status= get_state(state_ptr);
			}
		}
    }
    
    if ( proc_stat_fd )
		fclose(proc_stat_fd);
        
    return status;
}

