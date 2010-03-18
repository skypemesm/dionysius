/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok mailto: <chaser@soft-industry.com>
 */

#ifndef	__ZFONEG_UTILS_H__
#define __ZFONEG_UTILS_H__

char* zfone_ip2str(char* buff, unsigned int ip);
unsigned int zfone_str2ip(char* str);
int is_active(list_streams_info_t *stream);
int find_active_stream(list_box_item_t *item);
void put_to_offset(GtkWidget *parent, GtkWidget *child, int offset);
int create_report();

#endif

