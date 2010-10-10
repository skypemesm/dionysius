/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * For licensing and other legal details, see the file zrtp_legal.c.
 * 
 * Vitaly Rozhkov <v.rozhkov@soft-industry.com> <vitaly.rozhkov@googlemail.com>
 */

#ifndef __ZRTP_POLL_H__
#define __ZRTP_POLL_H__

#include <zrtp.h>

#define MPOLL_MAX_DEPTH 100

typedef struct {
	uint32_t _max_depth;
	uint32_t _curr_depth;
	uint32_t _engaged;
	mlist_t _mlist;
} mpoll_t;


#if defined(__cplusplus)
extern "C"
{
#endif

void init_mpoll(mpoll_t *head);

uint8_t _mpoll_check_available(mpoll_t *head);
mpoll_t *_mpoll_get_available(mpoll_t *head);
uint8_t _mpoll_check_depth(mpoll_t *head);
mpoll_t *_mpoll_add_new(mpoll_t* head, void* node, uint32_t offset);


#define mpoll_get(head, poll_type, poll_name)\
_mpoll_check_available(head) ? _mpoll_get_available(head):\
	_mpoll_check_depth(head)?\
		_mpoll_add_new( head, zrtp_sys_alloc(sizeof(poll_type)), ((uint32_t)&((poll_type*)0)->poll_name) ) : NULL

void mpoll_release(mpoll_t *head, mpoll_t *elem);

#define done_mpoll(head, poll_type, poll_name){ \
poll_type *poll_data; \
mpoll_t *poll_node; \
mlist_t *pos, *n; \
mlist_for_each_safe(pos, n, &( (head)->_mlist) ){ \
	poll_node = mlist_get_struct(mpoll_t, _mlist, pos); \
	poll_data = mlist_get_struct(poll_type, poll_name, poll_node); \
	\
	zrtp_sys_free(poll_data); \
}\
(head)->_curr_depth = 0;\
(head)->_engaged = 1; \
init_mlist( &( (head)->_mlist) ); \
}\

#define mpoll_for_each(pos, head)\
	for( 	pos = mlist_get_struct(mpoll_t, _mlist, (head)->_mlist.next); \
			&(pos)->_mlist != &(head)->_mlist && 1 == (pos)->_engaged; \
			pos = mlist_get_struct(mpoll_t, _mlist, (pos)->_mlist.next))

#define mpoll_for_each_safe(pos, n, head)\
	for( 	pos = mlist_get_struct(mpoll_t, _mlist, (head)->_mlist.next), \
			n = mlist_get_struct(mpoll_t, _mlist, (pos)->_mlist.next); \
			&(pos)->_mlist != &(head)->_mlist && 1 == (pos)->_engaged; \
			pos = n, n = mlist_get_struct(mpoll_t, _mlist, (pos)->_mlist.next))


#define mpoll_get_struct(type, poll_name, poll_ptr)\
	mlist_get_struct(type, poll_name, poll_ptr)

#if defined(__cplusplus)
}
#endif

#endif //__ZRTP_POLL_H__
