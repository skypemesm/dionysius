/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov <egm@soft.cn.ua>, <m.yegorov@gmail.com>
 */

#ifndef __MLIST_H__
#define __MLIST_H__

#include <stdlib.h>

typedef struct mlist mlist_t;
struct mlist
{
    mlist_t  *next;
    mlist_t  *prev;
};

#define mlist_list_offset(type, list_name) ((int)&((type*)0)->list_name)

#define mlist_get_struct(type, list_name, list_ptr) \
	    ((type*)(((char*)(list_ptr)) - mlist_list_offset(type,list_name)))

//------------------------------------------------------------------------------
static inline void init_mlist(mlist_t* head)
{
    head->next = head;
    head->prev = head;
}

//------------------------------------------------------------------------------
static inline void mlist_insert_node(mlist_t* node, mlist_t* prev, mlist_t* next)
{
    next->prev	= node;
    node->next	= next;
    node->prev	= prev;
    prev->next	= node;    
}

//------------------------------------------------------------------------------
static inline void mlist_add(mlist_t* head, mlist_t* node)
{
    mlist_insert_node(node, head, head->next);
}

//------------------------------------------------------------------------------
static inline void mlist_add_tail(mlist_t *head, mlist_t *node)
{
    mlist_insert_node(node, head->prev, head);
}

//------------------------------------------------------------------------------
static inline void mlist_remove(mlist_t* prev, mlist_t* next)
{
    next->prev = prev;
    prev->next = next;
}

//------------------------------------------------------------------------------
static inline void mlist_del(mlist_t *node)
{
    mlist_remove(node->prev, node->next);
    node->next = node->prev = NULL;
}

//------------------------------------------------------------------------------
static inline void mlist_del_tail(mlist_t *node)
{
    mlist_remove(node->prev, node->next);
    node->next = node->prev = NULL;
}

//------------------------------------------------------------------------------

typedef void (*mlist_callback_t)(mlist_t* node, void* data);
static inline void mlist_for_each(mlist_t* head, mlist_callback_t mcb, void* data)
{
    mlist_t* cur = head->next;
    
    for (; cur!=head; cur=cur->next)
    {
	 mcb(cur, data);
    }
    
}


#define mlist_for_each_mac(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define mlist_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

//------------------------------------------------------------------------------
typedef void (*mlist_destroy_callback_t)(mlist_t* node);
static inline void mlist_destroy(mlist_t* head, mlist_destroy_callback_t mcb)
{
    mlist_t* cur = head->next;
    mlist_t* next = NULL;
    
    if (cur != head)
    {
	cur = cur->next;
	while (cur != head)
	{
	    next = cur->next;
	    mcb(cur->prev);
	    cur = next;
	}
	mcb(cur->prev);
    }
}

//------------------------------------------------------------------------------
typedef int (*mlist_find_callback_t)(mlist_t* node, void* data);

static inline mlist_t* mlist_find(mlist_t* head, mlist_find_callback_t mcb, void* data)
{
    mlist_t* cur = head->next;

    for ( ; cur!=head; cur=cur->next)
    {
	if ( mcb(cur, data) )
	    return cur;
    }
    
    return NULL;
}

#endif //__MLIST_H__
