/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>

#include "zfone.h"

#define _ZTU_ "zfone hsip"


int _zfone_mark_prtps_by_sip( zfone_prob_rtps_t *prtps,
							 zfone_prob_sips_t *psips,
							 uint8_t use_new );

static void _zfone_handle_sip_closing( struct zfone_heurist *heurist,
									   zfone_sip_session_t  *sips );

static void _print_sips_rtps( struct zfone_heurist* self, 
							  zfone_prob_rtps_t *prtps, 
							  zfone_prob_rtps_t *pair_prtps,
							  uint64_t left_border,
							  uint64_t right_border );

//-----------------------------------------------------------------------------
int create_sip_session(struct zfone_heurist* self, zfone_sip_session_t* sips)
{
	zfone_prob_sips_t *psips = NULL;
	mpoll_t *poll_node = NULL;	

	zrtp_mutex_lock(self->protector);
	
#ifdef ZFONE_USE_HEURIST_DEBUG
	{
	char buff[SIP_SESSION_ID_SIZE*3];
	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: new SIP session id=%s was ADDED to the"
				   " poll.\n", hex2str((void*)sips->id, SIP_SESSION_ID_SIZE, buff, sizeof(buff))));
	self->show_state(self);
	}
#endif
	
	poll_node = mpoll_get(&self->sips_poll, zfone_prob_sips_t, _mpoll);
	if ( !poll_node )
	{
		ZRTP_LOG(1, (_ZTU_, "zfoned heurist: create_sip_session() can't allocate"
									   " memory for the new session.\n"));
		return voip_proto_ERROR;
	}
	
	psips = mpoll_get_struct(zfone_prob_sips_t, _mpoll, poll_node);	
	psips->created_at = zrtp_time_now();
	psips->sips = sips;
	psips->call = NULL;
	sips->psips = (void*) psips;

	// Mark RTP streams as confirmid by this SIP session
	mpoll_for_each(poll_node, &self->rtps_poll)
	{
		zfone_prob_rtps_t *prtps =  mpoll_get_struct(zfone_prob_rtps_t, _mpoll, poll_node);
		_zfone_mark_prtps_by_sip(prtps, psips, 1);
	}

	ZRTP_LOG(3, (_ZTU_, "heurist: after the SIP CREATE:\n"));
	self->show_state(self);

	zrtp_mutex_unlock(self->protector);
		
	return 0;
}

//-----------------------------------------------------------------------------
int destroy_sip_session(struct zfone_heurist* self, zfone_sip_session_t* sips)
{
	zfone_prob_sips_t *psips = (zfone_prob_sips_t*) sips->psips;
	if (!psips)
		return 0;

#ifdef ZFONE_USE_HEURIST_DEBUG
	{
	char buff[SIP_SESSION_ID_SIZE*3];
	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: handling SIP session %p closing ID\n"
				   ":%s.", sips, hex2str((void*)sips->id, SIP_SESSION_ID_SIZE, buff, sizeof(buff))));
	self->show_state(self);	
	}
#endif
	
	zrtp_mutex_lock(self->protector);

	// Mark confirmed call as BYECLOSED and use special tiemout to close them
	// further on RTP activity interuption. It's very dangerous to close RTP
	// stream immediately: BYE packet may be rejected by VoIP client and we
	// will lose encrypted channel.	
	if (psips->call)
	{
		uint8_t i = 0;
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: CONFIRMED call was FOUND"
									   " - makring all stream as BYE CLOSED.\n"));
		psips->call->conf_mode = ZFONE_CONF_MODE_CLOSED;

		// Add SIP closed call to the list
		for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
		{
			zfone_hstream_t *hstream = &psips->call->streams[i];

			if ((hstream->state != ZFONE_SDP_MEDIA_TYPE_UNKN) && hstream->in_stream)				 
			{				
				zfone_heurist_bye_t* bye = zrtp_sys_alloc(sizeof(zfone_heurist_bye_t));
				if (bye)
				{
					ZRTP_LOG(3, (_ZTU_, "zfoned heurist: REMEMBER SIP BYE for"
									" stream ssrc=%u\n", zrtp_ntoh32(hstream->in_stream->media.ssrc)));   

					zrtp_memset(bye, 0, sizeof(zfone_heurist_bye_t));
					bye->created_at = zrtp_time_now();
					zrtp_memcpy( &bye->media,
								 &hstream->in_stream->media,
								 sizeof(zfone_media_stream_t) );

					mlist_add(&self->bye_head, &bye->_mlist);
				}		
			}
		}
	}
	else // Than try to close unconfirmed streams	
	{
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: CONFIRMED call was NOT FOUND"
									   " - handle BYE in regular way.\n"));
		_zfone_handle_sip_closing(self, sips);
	}
	
	// Destroy SIP session
	mpoll_release(&self->sips_poll, &psips->_mpoll);
	
	zrtp_mutex_unlock(self->protector);

	return 0;
}

//-----------------------------------------------------------------------------
int is_sip_session_busy( struct zfone_heurist* self,
						 zfone_sip_session_t* sips )
{
	zfone_prob_sips_t *psips = (zfone_prob_sips_t*) sips->psips;
	if (psips && psips->call)
	{
		zfone_prob_rtp_call_t *call = psips->call;
		uint8_t i = 0;
		for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
		{
			if (ZFONE_PRTPS_STATE_PASIVE != call->streams[i].state)
				return 0;
		}
	}
	
	return -1;
}

//-----------------------------------------------------------------------------
#define _ZFONE_RE_MARK_SIP()\
{\
	mpoll_t *poll_node = NULL;\
	mpoll_for_each(poll_node, &self->rtps_poll)\
	{\
		zfone_prob_rtps_t *prtps =  mpoll_get_struct(zfone_prob_rtps_t, _mpoll, poll_node);\
		_zfone_mark_prtps_by_sip(prtps, psips, 1);\
	}\
}

int update_sip_session(struct zfone_heurist* self, zfone_sip_session_t* sips)
{
	uint32_t i = 0;
	zfone_media_stream_t *new_stream = NULL, *conf_stream = NULL;
	zfone_prob_sips_t *psips = (zfone_prob_sips_t*) sips->psips;

	if (!psips)
		return 0;
	
	if (!psips->call)
		return 0;

	zrtp_mutex_lock(self->protector);

#ifdef ZFONE_USE_HEURIST_DEBUG
	{
	char buff[SIP_SESSION_ID_SIZE*3];
	ZRTP_LOG(3, (_ZTU_, "heurist: SIP session %s was UPDATED. Looking for changes\n"
				   ":n", hex2str((void*)sips->id, SIP_SESSION_ID_SIZE, buff, sizeof(buff))));
	}
#endif	

	// Looking for changes in media streams at first: stream may be closed,
	// updated or resumed
	for (i=0; i<sips->new_count; i++)
	{
		new_stream = &sips->new_streams[i];
		conf_stream = &sips->conf_streams[i];

		// Loocking for closed streams: port should be set to 0.
		if ( (!new_stream->local_rtp && conf_stream->local_rtp) ||
			 (!new_stream->remote_rtp && conf_stream->remote_rtp) )
		{
			ZRTP_LOG(3, (_ZTU_, "heurist: stream with ports %d:%d was REMOVED from"
						   " the SIP session.\n", conf_stream->local_rtp, conf_stream->remote_rtp));

			if (conf_stream->hstream)
			{
				conf_stream->hstream->state = ZFONE_PRTPS_STATE_CLOSED;
				ZRTP_LOG(3, (_ZTU_, "heurist: removing stream was linked with zfone"
								" stream%p - makring it as closed.\n", conf_stream->hstream));
			}
			else
			{
				ZRTP_LOG(3, (_ZTU_, "heurist: there are no streams linked with this"
											   " SIP media - use \"time window\" algorithm.\n"));
				// TODO: use time windows between the event and RTP media interception
			}			
		}

		// Looking for resumed streams: pervious part was 0 but now nopt 0.
		else if ( (new_stream->local_rtp && !conf_stream->local_rtp) ||
				  (new_stream->remote_rtp && !conf_stream->remote_rtp) )
		{
			ZRTP_LOG(3, (_ZTU_, "heurist: stream with ports %d:%d was RESUMED from"
						   " the SIP session.\n", new_stream->local_rtp, new_stream->remote_rtp));
			_ZFONE_RE_MARK_SIP();

		}

		// Looking for updated streams: any changes in transport options
		else if ( (new_stream->local_rtp  != conf_stream->local_rtp) ||
				  (new_stream->remote_rtp != conf_stream->remote_rtp) )
		{
			// TODO: work on stream updates later. Putting on hole - the most important option.
		}
	}

	// Looking for added streams: number of streams have to be encreased
	if (sips->new_count > sips->conf_count)
	{
		ZRTP_LOG(3, (_ZTU_, "heurist: number of SIP media streams was increased from"
					" %d to %d. Adding them to the heurist engine.\n", sips->conf_count, sips->new_count));

		// Refresh creation time to allow it to be linked with new calls by time window
		psips->created_at = zrtp_time_now();

		for (i=sips->conf_count; i<sips->new_count; i++)
			_ZFONE_RE_MARK_SIP();
	}

	ZRTP_LOG(3, (_ZTU_, "heurist: after the SIP UPDATE:\n"));
	self->show_state(self);

	zrtp_mutex_unlock(self->protector);
	
	return 0;
}

//-----------------------------------------------------------------------------
zfone_prob_sips_t* _find_appropriate_sips( struct zfone_heurist* self,
										   zfone_prob_rtps_t *prtps,
										   zfone_prob_rtps_t *pair_prtps )
{
	uint64_t snow = zrtp_time_now();
	mpoll_t* poll_node = NULL;	
	uint64_t left_sip_border = 0, right_sip_border = 0;
	uint64_t first_rtps_time = prtps->pre_ready_timestamp;
	uint64_t second_rtps_time = first_rtps_time;

	zfone_prob_sips_t *tmp_sips = NULL, *prev_sips = NULL;	
	zfone_prob_sips_t *left_sips = NULL, *right_sips = NULL;
	zfone_prob_rtps_t *tmp_rtps = NULL;

	int found_concurent = 0;
	if (pair_prtps)
	{
		if(prtps->pre_ready_timestamp >= pair_prtps->pre_ready_timestamp)
		{
			first_rtps_time = pair_prtps->pre_ready_timestamp;
			second_rtps_time = prtps->pre_ready_timestamp;
		}
		else
		{
			first_rtps_time = prtps->pre_ready_timestamp;
			second_rtps_time = pair_prtps->pre_ready_timestamp;
		}
	}

	// Find nearest, left SIP (skip already linked sips and younger ones)
	mpoll_for_each(poll_node, &self->sips_poll)
	{
		tmp_sips = mpoll_get_struct(zfone_prob_sips_t, _mpoll, poll_node);		
		if(tmp_sips->call || (tmp_sips->created_at > first_rtps_time))
			continue;
		
		if(!left_sips || (first_rtps_time - left_sips->created_at > first_rtps_time - tmp_sips->created_at))
			left_sips = tmp_sips;
	}
		
	// No sip sessions found to link - nothing TODO, exit
	if(!left_sips)
	{
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: can't find left SIP session\n"));
		return NULL;
	}	

	// Find previous SIPs;
	mpoll_for_each(poll_node, &self->sips_poll)
	{
		tmp_sips = mpoll_get_struct(zfone_prob_sips_t, _mpoll, poll_node);		
		if(tmp_sips == left_sips) //skip left_sips
			continue;
		
		if(tmp_sips->created_at > left_sips->created_at) //skip younger sips
			continue;

		if(	!prev_sips || (left_sips->created_at - prev_sips->created_at > left_sips->created_at - tmp_sips->created_at))
			prev_sips = tmp_sips;
	}
	// Can't link when previous SIPs isn't linked. There is a unresolved sitation when
	// ZFone can't shouse one of the two unjlinked neighboring SIP sessions.
	if(prev_sips && !prev_sips->call)
	{
		if(left_sips->created_at - prev_sips->created_at < ZFONE_CONCURENT_SIP_TIMEOUT)
		{
			ZRTP_LOG(3, (_ZTU_, "zfoned heurist: found UNLINKED previous SIP session\n"));
			return NULL;
		}
	}

	// Find nearest, Right SIP session (skip already linked sips and older ones)
	mpoll_for_each(poll_node, &self->sips_poll)
	{
		tmp_sips = mpoll_get_struct(zfone_prob_sips_t, _mpoll, poll_node);			
		if(tmp_sips->call || (second_rtps_time > tmp_sips->created_at))
			continue;
		
		if(!right_sips || (right_sips->created_at - second_rtps_time > tmp_sips->created_at - second_rtps_time))
			right_sips = tmp_sips;
	}
	
	left_sip_border = left_sips->created_at;
	if (!right_sips)
	{
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: can't find right SIP session - use NOW timestamp\n"));
		right_sip_border = snow;
	}
	else
	{		
		right_sip_border = right_sips->created_at;
	}

#ifdef ZFONE_USE_HEURIST_DEBUG
	_print_sips_rtps(self, prtps, pair_prtps, left_sip_border, right_sip_border);
#endif

	// Find concurent RTPs on the interval
	// Skip all linked and confirmed rtps and <= PRE_READY state
	mpoll_for_each(poll_node, &self->rtps_poll)
	{
		tmp_rtps = mpoll_get_struct(zfone_prob_rtps_t, _mpoll, poll_node);		

		if((tmp_rtps == prtps) || (pair_prtps && (tmp_rtps == pair_prtps)) ) //skip self rtps
			continue;
				
		if(	tmp_rtps->state < ZFONE_PRTPS_STATE_PRE_READY || 
			( tmp_rtps->call && 
			  ( (ZFONE_CONF_MODE_CONFIRM == tmp_rtps->call->conf_mode) || 
			    (ZFONE_CONF_MODE_LINK == tmp_rtps->call->conf_mode) ) ) )
			continue;
		
		if(	(tmp_rtps->pre_ready_timestamp >= left_sip_border) &&
			(tmp_rtps->pre_ready_timestamp <= right_sip_border))
		{
			found_concurent = 1;
			break;
		}
	}

	if(found_concurent)
	{
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: found CONCURENT RTP session\n"));
		return NULL;
	}
	
	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: no concurents found. link SIP session\n"));

	prtps->psips = left_sips;
	prtps->conf_mode = ZFONE_CONF_MODE_LINK;
	zrtp_memcpy(prtps->sip_id, left_sips->sips->id, sizeof(zfone_sip_id_t));
	if (prtps->call) prtps->call->conf_mode = ZFONE_CONF_MODE_LINK;

	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: LINKING SIP session with RTP call by TIME WINDOW.\n"));

	return left_sips;
}



//=============================================================================
//  Utils
//=============================================================================


//------------------------------------------------------------------------------
int zfone_is_rtp_after_bye( struct zfone_heurist *heurist,
							zfone_rtpcheck_info_t* info )
{
	mlist_t *list_node = NULL, *tmp_list_node = NULL;
	uint64_t snow = zrtp_time_now();

	mlist_for_each_safe(list_node, tmp_list_node, &heurist->bye_head)
	{
		zfone_heurist_bye_t* bye = mlist_get_struct(zfone_heurist_bye_t, _mlist, list_node);
		if ((snow - bye->created_at) > ZFONE_SKIP_AFTER_BYE_TIMEOUT)
		{
			mlist_del(list_node);
			zrtp_sys_free(bye);
		}
		else if (bye->media.ssrc == info->rtp_hdr->ssrc)
		{
			return 1;
		}
	}

	return 0;
}


//------------------------------------------------------------------------------
static void _zfone_handle_sip_closing( struct zfone_heurist *heurist,
									   zfone_sip_session_t  *sips )
{
	mlist_t *node = NULL;
	zfone_prob_rtp_call_t *a_call=NULL, *v_call=NULL, *curr_call=NULL;
	
#ifdef ZFONE_USE_HEURIST_DEBUG	
	char buff[SIP_SESSION_ID_SIZE*3];
	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zfone_handle_sip_closing() SIP"
				   " BYE signal was received with ID=%s.\n",
				   hex2str((void*)sips->id, SIP_SESSION_ID_SIZE, buff, sizeof(buff))));
	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: a_calls[%i], v_calls[%i]\n",
				   heurist->a_calls_count, heurist->v_calls_count));
#endif

	// Let's find calls to process
	if( (heurist->a_calls_count + heurist->v_calls_count > 0) &&
		((heurist->a_calls_count <= 1) && (heurist->v_calls_count <=1)))
	{
		mlist_for_each(node, &heurist->rtpc_head)
		{
			curr_call = mlist_get_struct(zfone_prob_rtp_call_t, mlist, node);
			if (ZFONE_SDP_MEDIA_TYPE_AUDIO == curr_call->streams[0].type)			
				a_call = curr_call;				
			else if (ZFONE_SDP_MEDIA_TYPE_VIDEO == curr_call->streams[0].type)
				v_call = curr_call;
		}
	}
				
	if(a_call || v_call)
	{
		// Mark all confirmed stream as BYECLOSED and use special tiemout to close
		// them further on RTP activity interuption. It's very dangerous to close
		// RTP stream immediately: BYE packet may be rejected by VoIP client and
		// we will lose encrypted channel.
		if(a_call)
			a_call->conf_mode = ZFONE_CONF_MODE_CLOSED;				
		if(v_call)
			v_call->conf_mode = ZFONE_CONF_MODE_CLOSED;				
	}
}

//-----------------------------------------------------------------------------
int _zfone_mark_prtps_by_sip( zfone_prob_rtps_t *prtps,
							  zfone_prob_sips_t *psips,
							  uint8_t use_new )
{	
	uint8_t i = 0;
	
	if ( prtps->psips ) // Skip already linked streams
		return -1;

	if (prtps->call && (prtps->call->conf_mode == ZFONE_CONF_MODE_CLOSED)) // skip closed calls
		return -1;
	
	
	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: zfone_mark_prtps_by_sip() count"
					" n:c %d:%d use_new=%d.\n",
					psips->sips->new_count, psips->sips->conf_count, use_new));

	for (i=0; i<(use_new ? psips->sips->new_count : psips->sips->conf_count); i++)
	{
		zfone_media_stream_t *medias = use_new ? &psips->sips->new_streams[i] : &psips->sips->conf_streams[i];
#ifdef ZFONE_USE_HEURIST_DEBUG
		char ipbuff1[25]; char ipbuff2[25];		
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zfone_mark_prtps_by_sip()"
									   " MEDIA: local=%d:%s remote=%d:%s,.\n",
						medias->local_rtp, zfone_ip2str(ipbuff1, sizeof(ipbuff1), medias->local_ip),
						medias->remote_rtp, zfone_ip2str(ipbuff2, sizeof(ipbuff2), medias->remote_ip)));
#endif
		if ( (medias->remote_ip && (zrtp_hton32(medias->remote_ip) == prtps->media.remote_ip)) ||
			  psips->sips->is_ice_enabled  // this may give an extra chance to link with ICE by local port
		    )
		{
			if ( (medias->local_rtp == zrtp_ntoh16(prtps->media.remote_rtp)) ||
				 (medias->remote_rtp == zrtp_ntoh16(prtps->media.remote_rtp)) ||
				 (medias->local_rtp == zrtp_ntoh16(prtps->media.local_rtp)) )
			{
#ifdef ZFONE_USE_HEURIST_DEBUG
				char buff[SIP_SESSION_ID_SIZE*3];
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zfone_mark_prtps_by_sip()"
							   " RTP stream %p:%d was CONFIRMED by SIP session %s\n",
							   prtps, zrtp_ntoh16(prtps->media.remote_rtp),
							   ZFONE_PRINT_SIPID(psips->sips->id, buff)));
#endif
				zrtp_memcpy(prtps->sip_id, psips->sips->id, sizeof(zfone_sip_id_t));
				prtps->psips = psips;
				prtps->conf_mode = ZFONE_CONF_MODE_CONFIRM;

				// This is a already established but non confirmed call - update SIP link
				if (prtps->call && (prtps->call->conf_mode == ZFONE_CONF_MODE_UNKN))
				{
					zrtp_memcpy( prtps->call->sip_id,
								 psips->sips->id,
								 sizeof(zfone_sip_id_t) );
					prtps->call->psips = prtps->psips;
					prtps->call->conf_mode = ZFONE_CONF_MODE_CONFIRM;
				}

				return 0;
			}
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
#ifdef ZFONE_USE_HEURIST_DEBUG
typedef enum
{
	SIPS_T = 1,
	RTPS_T = 2,
	BORDER_T = 3
} node_type;

typedef struct
{
	uint64_t 	time;
	node_type	type;
	uint8_t		is_current;
	uint8_t		is_linked;
	mlist_t		_mlist;
} node_t;

static void _print_sips_rtps( struct zfone_heurist* self,
							  zfone_prob_rtps_t *prtps,
							  zfone_prob_rtps_t *pair_prtps,
							  uint64_t left_border,
							  uint64_t right_border )
{
	mlist_t head;
	mpoll_t *poll_node = NULL;
	mlist_t	*list_node = NULL, *tmp_list_node = NULL;

	zfone_prob_sips_t *sips = NULL;
	zfone_prob_rtps_t *rtps = NULL;

	uint8_t inserted = 0;
	char *p_buffer 	 = NULL;
	
	const int buffer_size = 1024;
	char* buffer = zrtp_sys_alloc(buffer_size);
	if (!buffer)
		return;
	p_buffer = buffer;
	zrtp_memset(p_buffer, 0, buffer_size);

	init_mlist(&head);
	
	// Create sorted list from sips poll
	mpoll_for_each(poll_node, &self->sips_poll)
	{
		node_t *new_node = zrtp_sys_alloc(sizeof(node_t));
		if (!new_node)		
			break;

		sips = mpoll_get_struct(zfone_prob_sips_t, _mpoll, poll_node);
	
		new_node->time = sips->created_at;
		new_node->type = SIPS_T;
		new_node->is_current = 0;
		new_node->is_linked = (NULL != sips->call);
		
		inserted = 0;
		mlist_for_each_safe(list_node, tmp_list_node, &head)
		{
			node_t *node = mlist_get_struct(node_t, _mlist, list_node);
			if(node->time >= new_node->time)
			{
				mlist_insert(list_node, &new_node->_mlist);
				inserted = 1;
				break;
			}
		}
		if (!inserted)
			mlist_add_tail(&head, &new_node->_mlist);
	}
	
	// Add rtps nodes into sorted list
	mpoll_for_each(poll_node, &self->rtps_poll)
	{
		node_t *new_node = zrtp_sys_alloc(sizeof(node_t));
		if (!new_node)
			break;

		rtps = mpoll_get_struct(zfone_prob_rtps_t, _mpoll, poll_node);
	
		new_node->time = rtps->pre_ready_timestamp;
		new_node->type = RTPS_T;
		new_node->is_current = ((prtps == rtps) || (pair_prtps && (pair_prtps == rtps)));
		new_node->is_linked = ( rtps->call && 
								( (ZFONE_CONF_MODE_CONFIRM == rtps->call->conf_mode) || 
								  (ZFONE_CONF_MODE_LINK == rtps->call->conf_mode) ) );

		inserted = 0;
		mlist_for_each_safe(list_node, tmp_list_node, &head)
		{
			node_t *node = mlist_get_struct(node_t, _mlist, list_node);
			if(node->time >= rtps->pre_ready_timestamp)
			{
				mlist_insert(list_node, &new_node->_mlist);
				inserted = 1;
				break;
			}
		}
		if (!inserted)
			mlist_add_tail(&head, &new_node->_mlist);
	}
	
	// Add left_border into the sorted list
	if (left_border)
	{
		node_t *new_node = zrtp_sys_alloc(sizeof(node_t));
		if (new_node)
		{
			new_node->time = left_border;
			new_node->type = BORDER_T;
			new_node->is_current = 0;
	
			inserted = 0;
			mlist_for_each_safe(list_node, tmp_list_node, &head)
			{
				node_t *node = mlist_get_struct(node_t, _mlist, list_node);
				if(node->time >= left_border)
				{
					mlist_insert(list_node, &new_node->_mlist);
					inserted = 1;
					break;
				}
			}
			if (!inserted)
				mlist_add_tail(&head, &new_node->_mlist);
		}
	}
	
	// Ddd right_border into the sorted list
	if (right_border)
	{
		node_t *new_node = zrtp_sys_alloc(sizeof(node_t));
		if (new_node)
		{
			new_node->time = right_border;
			new_node->type = BORDER_T;
			new_node->is_current = 0;
	
			inserted = 0;
			mlist_for_each_safe(list_node, tmp_list_node, &head)
			{
				node_t *node = mlist_get_struct(node_t, _mlist, list_node);
				if(node->time >= right_border)
				{
					mlist_insert(list_node, &new_node->_mlist);
					inserted = 1;
					break;
				}
			}
			if (!inserted)
				mlist_add_tail(&head, &new_node->_mlist);		
		}		
	}
	
	// Print out the list
	mlist_for_each(list_node, &head)
	{
		const char delim[] = "---";
		const uint8_t sign_size = 3;

		node_t *node = mlist_get_struct(node_t, _mlist, list_node);

		if (buffer_size <= (p_buffer - buffer + 10))
			break;

		zrtp_memcpy(p_buffer, delim, sign_size);
		p_buffer += sign_size;		

		switch(node->type)
		{
		case SIPS_T:
			zrtp_memcpy( p_buffer,
						 ((node->is_linked) ? "[o]" : " o "),
						 sign_size );
			break;
		case RTPS_T:
			zrtp_memcpy( p_buffer,
						 (node->is_current) ? "(v)" : ((node->is_linked) ? "[v]" : " v "),
						 sign_size );
			break;
		case BORDER_T:
			zrtp_memcpy(p_buffer, " | ", sign_size);
			break;		
		default:
			zrtp_memcpy(p_buffer, "ERR", sign_size);
			break;
		}
		p_buffer += sign_size;		
	}
	p_buffer = 0;
	
	ZRTP_LOG(3, (_ZTU_, "SIPS and RTPS state:\n"));
	ZRTP_LOG(3, (_ZTU_, " o  - SIP session\n"));
	ZRTP_LOG(3, (_ZTU_, "[o] - linked SIP session\n"));
	ZRTP_LOG(3, (_ZTU_, " v  - RTP session\n"));
	ZRTP_LOG(3, (_ZTU_, "[v] - linked RTP session\n"));
	ZRTP_LOG(3, (_ZTU_, "(v) - currently considered RTP session\n"));
	ZRTP_LOG(3, (_ZTU_, " |  - consider time border\n"));	
	ZRTP_LOG(3, (_ZTU_, "%s\n", buffer));
	
	// release created list
	mlist_for_each_safe(list_node, tmp_list_node, &head)
	{
		node_t *node = mlist_get_struct(node_t, _mlist, list_node);
		mlist_del(list_node);
		zrtp_sys_free(node);
	}
	
	zrtp_sys_free(buffer);
}
#endif

