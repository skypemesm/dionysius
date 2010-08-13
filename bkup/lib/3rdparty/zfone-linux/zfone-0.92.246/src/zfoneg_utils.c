/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok mailto: <chaser@soft-industry.com>
 */

#include <stdio.h>

#include "zrtp_types.h"
#include "zfoneg_listbox.h"
#include "zfoneg_utils.h"

char* zfone_ip2str(char* buff, unsigned int ip)
{
    sprintf(buff, "%i.%i.%i.%i", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
    return buff;
}

unsigned int zfone_str2ip(char* str)
{
    int i = 0;
    unsigned int ip = 0;
    
    for(i=0; i<4; i++)
    {
	int part = 0;

	while(*str>='0' && *str<='9')
	{
    	    part *= 10;
	    part += (*str - '0');
	    str++;
	}
	if(*str == '.')
	{
	    str++;
	}
	ip <<= 8;
	ip += part;
    }

    return ip;
}

int is_active(list_streams_info_t *stream)
{
    if ( stream->state != ZRTP_STATE_ACTIVE && stream->state != ZRTP_STATE_NONE )
    {
		return 1;
    }
    return 0;
}

int find_active_stream(list_box_item_t *item)
{
    int i;
    for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
    {
		if (is_active(&item->conn_info.streams[i]))
		{
	  		return i;
		}
    }
    return -1;
}

void put_to_offset(GtkWidget *parent, GtkWidget *child, int offset)
{
	GtkWidget *align = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(parent), align);
    gtk_widget_show(align);
	gtk_fixed_put(GTK_FIXED(align), child, offset, 0);
}
