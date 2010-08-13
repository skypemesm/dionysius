/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Vitaly Rozhkov <v.rozhkov@soft-industry.com> <vitaly.rozhkov@googlemail.com>
 */

#include "zrtp_poll.h"

#define _ZTU_ "zfone poll"

//-----------------------------------------------------------------------------
void init_mpoll(mpoll_t *head)
{
//	ZRTP_LOG(3, (_ZTU_, "init_mpoll() depth[%i]", MPOLL_MAX_DEPTH));
	
	head->_max_depth = MPOLL_MAX_DEPTH;
	head->_curr_depth = 0;
	head->_engaged = 1;
		
	init_mlist(&head->_mlist);
}

//-----------------------------------------------------------------------------
uint8_t _mpoll_check_available(mpoll_t *head)
{
	mlist_t *list_node = NULL;
	mpoll_t *poll_node = NULL;
	
	list_node = mlist_get_tail(&head->_mlist);
	if(NULL == list_node)
		return 0;
	
	poll_node = mlist_get_struct(mpoll_t, _mlist, list_node);
	return !poll_node->_engaged;	
}

//-----------------------------------------------------------------------------
uint8_t _mpoll_check_depth(mpoll_t *head)
{
	return !(head->_curr_depth+1 > head->_max_depth);
}

//-----------------------------------------------------------------------------
mpoll_t *_mpoll_add_new(mpoll_t* head, void* node, uint32_t offset)
{
	mpoll_t *mpoll_node = NULL;

	if(NULL == node)
		return NULL;
//	ZRTP_LOG(3, (_ZTU_, "add new node[%p]", node));

	mpoll_node = (mpoll_t*)((uint8_t*)node + offset);
	head->_curr_depth++;
	mpoll_node->_engaged = 1;
	mlist_add(&head->_mlist, &mpoll_node->_mlist);
		
	return mpoll_node;
}

//-----------------------------------------------------------------------------
mpoll_t *_mpoll_get_available(mpoll_t *head)
{
	mpoll_t *poll_node;
	mlist_t *list_node;

	list_node = mlist_get_tail(&head->_mlist);
	if(NULL == list_node)
		return NULL;
	
	poll_node = mlist_get_struct(mpoll_t, _mlist, list_node);
	if(!poll_node->_engaged)
	{
		poll_node->_engaged = 1;
		head->_curr_depth++;
		
		//put node to the head of list
		mlist_del(&poll_node->_mlist);
		mlist_add(&head->_mlist, &poll_node->_mlist);
		
//		ZRTP_LOG(3, (_ZTU_, "found available element. Curr depth[%i]", head->_curr_depth));
		return poll_node;

	}
	else
	{
//		ZRTP_LOG(3, (_ZTU_, "there are no available elements found"));
		return NULL;
	}
}

//-----------------------------------------------------------------------------

void mpoll_release(mpoll_t *head, mpoll_t *elem)
{
	//ZRTP_LOG(3, (_ZTU_, "release element. head=%p elem=%p eng=%d depth=%i", head, elem, elem->_engaged, head->_curr_depth));
	if(head && elem && elem->_engaged && head->_curr_depth)
	{
		head->_curr_depth--;
		elem->_engaged = 0;
		
		//put node to the tail of list
		mlist_del(&elem->_mlist);
		mlist_add_tail(&head->_mlist, &elem->_mlist);		
		//ZRTP_LOG(3, (_ZTU_, "release element. Curr depth[%i]", head->_curr_depth));
	}
}

