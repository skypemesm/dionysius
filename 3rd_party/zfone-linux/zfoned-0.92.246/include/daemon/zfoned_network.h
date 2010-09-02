/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONED_NETWORK_H__
#define __ZFONED_NETWORK_H__

#include <zrtp.h>
#include "zfone_types.h"


int zfone_network_init(const char* firewall_path);
void zfone_network_down();

int zfone_network_add_rules(int is_adding, zrtp_voip_proto_t proto);
int zfone_network_send_ip(char* packet, unsigned int length, unsigned int to);
int zfone_network_get_packet(zfone_packet_t* packet);
int zfone_network_put_packet(zfone_packet_t* packet, int need_accept);

#endif //__ZFONED_NETWORK_H__

