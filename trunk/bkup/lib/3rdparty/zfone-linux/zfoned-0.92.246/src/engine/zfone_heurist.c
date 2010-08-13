/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 * Vitaly Rozhkov <v.rozhkov@soft-industry.com> <vitaly.rozhkov@googlemail.com>
 */

#include <zrtp.h>

#include "zfone.h"

#define _ZTU_ "zfone heurist"

extern void _zfone_init_prob_rtps(zfone_rtpcheck_info_t *info);

static zfone_heurist_status_t _update_call( struct zfone_heurist* self,
											zfone_prob_rtps_t *prtps,
											zfone_prob_rtps_t *pair_prtps );

zfone_heurist_status_t _create_call( struct zfone_heurist* self,
									 zfone_prob_rtps_t *prtps,
									 zfone_prob_rtps_t *pair_prtps,
									 zfone_rtpcheck_info_t *info );


//-----------------------------------------------------------------------------
zfone_heurist_status_t fast_analyse( struct zfone_heurist* self,
 									 zfone_packet_t* packet,
									 zrtp_voip_proto_t proto )
{
	int i = 0;
	zfone_rtpcheck_info_t *info = (zfone_rtpcheck_info_t *)packet->extra_data;
	zrtp_memset(info, 0, sizeof(zfone_rtpcheck_info_t) - sizeof(mpoll_t));
	info->packet = packet;

	for (i=0; i<ZFONE_SYMP_SCOUNT; i++)
	{
		if (ZFONE_SYMP_STATUS_ERROR == _rtp_static_sympt[i](self, info))
			break;
	}	
	
	return (info->score > ZFONED_RTP_PACKET_ACCEPT_BORDER) ?  zfone_hs_prtp : zfone_hs_nop;
}

//-----------------------------------------------------------------------------
#define RETURN_DEEP_ANALYZE(self, res) \
{ \
	zrtp_mutex_unlock(self->protector); \
	return res; \
}
           
//-----------------------------------------------------------------------------
zfone_heurist_status_t deep_analyse( struct zfone_heurist* self,
									 zfone_packet_t* packet,
									 zrtp_voip_proto_t proto )
{
	int i = 0;
	mpoll_t *poll_node = NULL, *poll_tmp = NULL;
	mlist_t *list_node = NULL;//, *list_tmp = NULL;
	zfone_rtpcheck_info_t *info = (zfone_rtpcheck_info_t*) packet->extra_data;
	zfone_prob_rtps_t* prtps = NULL;
	zfone_hs_node_t* head = NULL;
	zrtp_rtp_hdr_t *rtp_hdr = (zrtp_rtp_hdr_t*) (packet->packet + packet->offset);	
	uint64_t snow = zrtp_time_now();
#ifdef ZFONE_USE_HEURIST_DEBUG
    char ipbuff1[25]; char ipbuff2[25];	
#endif

	// WARNING: info may be NULL for proto == voip_proto_RTP. We don't want to make
	// it blow just to touch already recognized RTP sessions.
	zrtp_mutex_lock(self->protector);

	//
	// Looking for existing RTP stream and creating a new one if necessary
	//
	
	head = &self->rtps_map[zrtp_ntoh16(packet->local_port)];
	mlist_for_each(list_node, &head->head)
	{
		zfone_prob_rtps_t* elem = (zfone_prob_rtps_t*) mlist_get_struct(zfone_prob_rtps_t, mlist_map, list_node);
		if ( (rtp_hdr->ssrc == elem->media.ssrc) &&
			 (packet->remote_ip == elem->media.remote_ip) &&
			 (packet->remote_port == elem->media.remote_rtp) )
		{
			prtps = elem;
			break;
		}
	}

#ifdef ZFONE_USE_HEURIST_DEBUG
	if ( (voip_proto_RTP != proto) || (prtps && (prtps->state < ZFONE_PRTPS_STATE_PRE_ESTABL)) )
	{
		ZRTP_LOG(3, (_ZTU_, "-------------------------------------------------------\n"));
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: %s UDP:%d packet was received in state %s"
									   " with laddr=%s:%d raddr=%s:%d ssrc=%u seq=%d size=%u\n",
					   zfone_direction_names[info->packet->direction],
					   proto, prtps ? zfone_prtps_state_names[prtps->state] : "NULL",
					   zfone_ip2str(ipbuff1, 25, zrtp_ntoh32(info->packet->local_ip)),
					   zrtp_ntoh16(info->packet->local_port),
					   zfone_ip2str(ipbuff2, 25, zrtp_ntoh32(info->packet->remote_ip)),
					   zrtp_ntoh16(info->packet->remote_port),
					   zrtp_ntoh32(info->rtp_hdr->ssrc),
					   zrtp_ntoh16(info->rtp_hdr->seq),					   
					   info->packet->size ));
	}
#endif
	
	if ( prtps )
	{
		// Touch separate stream and entire call
		prtps->timestamp = snow;
		if(prtps->call)
		{
			if (ZFONE_IO_OUT == packet->direction)
				prtps->call->out_touch = snow;
			else
				prtps->call->in_touch = snow;
		}

		if (prtps->state >= ZFONE_PRTPS_STATE_PRE_ESTABL)
			RETURN_DEEP_ANALYZE(self, zfone_hs_touched);		

		// Ok, from this line we MUST to have info structure available
		info->prtps = prtps;

		// Skip first ZFONE_SKIP_BEFORE_ICE_COUNT packets if ICE is active. It will
		// prevent heurist from switching from one transport options to another.
#if 0
		if (prtps->psips && prtps->psips->sips && prtps->psips->sips->is_ice_enabled)
		{
			if (prtps->skip_before_ice < ZFONE_SKIP_BEFORE_ICE_COUNT)
			{
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: Skip RTP number %d"
								" before ICE session.\n", prtps->skip_before_ice));
				prtps->skip_before_ice++;
				RETURN_DEEP_ANALYZE(self, zfone_hs_skip);
			}
		}
#endif
	}	
	else
	{
		// Skip first ZFONE_SKIP_AFTER_BYE_COUNT packets after call closing by SIP
		// BYE message. It's possible to receive some ammount of incoming media
		// after hanging up.
		if ( zfone_is_rtp_after_bye(self, info) )
		{
			ZRTP_LOG(3, (_ZTU_, "zfoned heurist: Skip latecome RTP with ssrc=%u.\n",
											zrtp_ntoh32(info->rtp_hdr->ssrc)));
			RETURN_DEEP_ANALYZE(self, zfone_hs_skip);
		}

		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: there is no branch - create a new %s one with SSRC=%u.\n",
					    (info->packet->direction == ZFONE_IO_OUT)?"OUT":"IN", zrtp_ntoh32(info->rtp_hdr->ssrc)));

		poll_node = mpoll_get(&self->rtps_poll, zfone_prob_rtps_t, _mpoll);
		if ( !poll_node )
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned zfone_rtp_deep_analyse(): can't allocate new prtp session.\n"));
			RETURN_DEEP_ANALYZE(self, zfone_hs_error);
		}

		prtps = mpoll_get_struct(zfone_prob_rtps_t, _mpoll, poll_node);
		zrtp_memset(prtps, 0, sizeof(zfone_prob_rtps_t) - sizeof(mpoll_t));
		prtps->state	 		= ZFONE_PRTPS_STATE_ACTIVE;
		prtps->direction 		= info->packet->direction;

		prtps->media.ssrc 	 	= info->rtp_hdr->ssrc;
		prtps->media.local_ip	= info->packet->local_ip;
		prtps->media.local_rtp 	= info->packet->local_port;
		prtps->media.local_rtcp = zrtp_hton16(zrtp_ntoh16(prtps->media.local_rtp) + 1);
		prtps->media.remote_ip 	= info->packet->remote_ip;
		prtps->media.remote_rtp = info->packet->remote_port;
		prtps->media.remote_rtcp = zrtp_hton16(zrtp_ntoh16(prtps->media.remote_rtp) + 1);
		prtps->media.type		= info->type;

		prtps->timestamp 		= snow;
		prtps->codec	 		= (uint8_t)info->rtp_hdr->pt;

		info->prtps = prtps;
		mlist_add(&head->head, &prtps->mlist_map);
		_zfone_init_prob_rtps(info);
		
		// Try to associate new RTP stream with a SIP session
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: Try to associate RTP stream %p with some SIP session:\n", prtps));
		mpoll_for_each(poll_node, &self->sips_poll)
		{
			zfone_prob_sips_t *psips =  mpoll_get_struct(zfone_prob_sips_t, _mpoll, poll_node);
			if ( !_zfone_mark_prtps_by_sip(prtps, psips, 0) )
				break;
		}

		ZRTP_LOG(3, (_ZTU_, "zfoned_heurist: after new the session was created:\n"));
        self->show_state(self);
	}


	//
	// Dynamic testing
	//=========================================================================
	if  (info->score > ZFONED_RTP_STREAM_ACCEPT_BORDER)		  
	{
		// Skip dynamic tests if we have enough scores to recognize RTP stream without them		
		prtps->total_score = ZFONED_RTP_STREAM_ACCEPT_BORDER;
	}
	else
	{
		for (i=0; i<ZFONE_SYMP_DCOUNT; i++)		
			if (ZFONE_SYMP_STATUS_ERROR == _rtp_dynam_sympt[i](self, info))
				break;
	}
	ZRTP_LOG(3, (_ZTU_, "zfoned_heurist: %i dynamic test have been bassed"
								   " with score=%u.\n", i, info->prtps->total_score));
	
	// There is not enough score to activate stream but enough to make stream attachable
	//=========================================================================
	if(	(prtps->state < ZFONE_PRTPS_STATE_PRE_READY) &&
	 	(prtps->total_score >= ZFONED_RTP_STREAM_ACCEPT_BORDER/2) && 
		(prtps->total_score < ZFONED_RTP_STREAM_ACCEPT_BORDER) )
	{		
		ZRTP_LOG(3, (_ZTU_, "zfoned_heurist: stream with ssrc=%u switched to"
									   " PRE_READY state\n", zrtp_ntoh32(prtps->media.ssrc)));

		prtps->state = ZFONE_PRTPS_STATE_PRE_READY;
		prtps->pre_ready_timestamp = snow;		

		// If ICE is enabled - looking for changes in RTP streams. If remote
		// transport options was changed - we should update calls parameters.		
		self->show_state(self);		

		if (0 == zfone_heurist_check4changes(self, prtps, info))
		{
			ZRTP_LOG(3, (_ZTU_, "zfoned_heurist: after ICE changes:\n"));
			self->show_state(self);
		}
	}

	// There is enough score to activate RTP stream
	//=========================================================================
	else if (prtps->total_score >= ZFONED_RTP_STREAM_ACCEPT_BORDER)
	{
		zfone_prob_rtps_t *tmp_ps = NULL;
		prtps->state = ZFONE_PRTPS_STATE_READY;

		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: %s %s WAS ACCEPTED WITH TOTAL SCORE=%d\n",
		zfone_prtps_state_names[prtps->state], zfone_media_type_names[prtps->media.type], prtps->total_score));

		//
		// Looking for pair stream to combine a new call
		//
		mpoll_for_each_safe(poll_node, poll_tmp, &self->rtps_poll)
		{
			tmp_ps = mpoll_get_struct(zfone_prob_rtps_t, _mpoll, poll_node);

			// Common behavior: combining by local ports
			// Symmetric behaviour: combining by remote port
			if ( ((tmp_ps->media.local_ip == prtps->media.local_ip) &&
				  (tmp_ps->media.local_rtp == prtps->media.local_rtp))  ||
				 ((tmp_ps->media.remote_ip == prtps->media.remote_ip) &&
				  (tmp_ps->media.remote_rtp == prtps->media.remote_rtp)) )
			{

			ZRTP_LOG(3, (_ZTU_, "zfoned heurist: checking %s stream with ssrc=%u type=%s and state=%s...\n",
							zfone_direction_names[tmp_ps->direction],
							zrtp_ntoh32(tmp_ps->media.ssrc),
							zfone_media_type_names[tmp_ps->media.type],
							zfone_prtps_state_names[tmp_ps->state]));

			if (tmp_ps == prtps) // skip self
			{
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: continue <skip self>\n"));
				continue;
			}				
				
			// Collision: tmp_ps - Established connection. It's impossible to have few
			// streams on one port for desktop solutions.
			
			if ( (ZFONE_PRTPS_STATE_PRE_READY > tmp_ps->state) ||
				 (ZFONE_PRTPS_STATE_PRE_ESTABL < tmp_ps->state) ) // skip passive sessions and already established
			{
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: continue <combining by state>.\n"));
				continue;
			}

			// Combining by type
			// Warning!
			// Combinning with ZRTP-ready endpoint. We assume that ZRTp-ready
			// endpoint doesn't mix voice several channels within one UDP port.
			if (ZFONE_SDP_MEDIA_TYPE_ZRTP == prtps->media.type)
			{
				prtps->media.type = tmp_ps->media.type;
				prtps->codec	  = tmp_ps->codec;                
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: combining with ZRTP"
								" stream N1. Take %s and codec=%d.\n", zfone_media_type_names[prtps->media.type], prtps->codec));
			}			
			else if (ZFONE_SDP_MEDIA_TYPE_ZRTP == tmp_ps->media.type)
			{
				tmp_ps->media.type = prtps->media.type;
				tmp_ps->codec	   = prtps->codec;                
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: combining with ZRTP"
								" stream N2. Take %s and codec=%d.\n", zfone_media_type_names[prtps->media.type], prtps->codec));
			}			
			else if (tmp_ps->media.type != prtps->media.type)					  
			{
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: continue <combining by type>.\n"));
				continue;
			}
				
			if ((tmp_ps->direction != ZFONE_IO_UNKNOWN) && (tmp_ps->direction != prtps->direction))
			{
				if (ZFONE_PRTPS_STATE_PRE_ESTABL == tmp_ps->state)
                    _update_call(self, tmp_ps, prtps);					
				else
					_create_call(self, prtps, tmp_ps, info);

				tmp_ps->state = ZFONE_PRTPS_STATE_ESTABL;
				prtps->state  = ZFONE_PRTPS_STATE_ESTABL;

				self->show_state(self);
				RETURN_DEEP_ANALYZE(self, (ZFONE_PRTPS_STATE_PRE_ESTABL == tmp_ps->state) ? zfone_hs_full_updated : zfone_hs_full_created);
			}
			// COLLISION: active session in the same branch and with the same direction
			else
			{
				// Parallel session haven't been confirmed yet - destroy
				// both of them and start detection from the scratch
				if (tmp_ps->state < ZFONE_PRTPS_STATE_PRE_ESTABL)
				{
					ZRTP_LOG(3, (_ZTU_, "zfoned heurist: STREAM %p AND %p COLLISION N1"
									" in COMBINING state=%d.\n", prtps, tmp_ps, tmp_ps->state));
					_zfone_release_prtps(self, tmp_ps);
					_zfone_release_prtps(self, prtps);
				}
				else if (tmp_ps->state >= ZFONE_PRTPS_STATE_PRE_ESTABL)
				{
					uint64_t delta = snow - tmp_ps->timestamp;
					if (delta > ZFONE_SESSION_SLEEP_TIME)
					{
						ZRTP_LOG(3, (_ZTU_, "zfoned heurist: STREAM %p AND %p COLLISION"
									   " N2 in COMBINING (delta=%u).\n", prtps, tmp_ps, (uint32_t)delta));
						_zfone_release_prtps(self, tmp_ps);
					}
					else
					{
						ZRTP_LOG(3, (_ZTU_, "zfoned heurist: STREAM %p AND %p COLLISION"
									   " N3 in COMBINING (delta=%u).\n", prtps, tmp_ps, (uint32_t) delta));
						_zfone_release_prtps(self, prtps);
					}
				}

				RETURN_DEEP_ANALYZE(self, zfone_hs_error);				
			}
			} // combining transport parameters

		} // for each prtps (combining full streams)		


		// Don't create one-way calls by ZRTP packet because type of the stream
		// is unknown and may prevent to mistakes in detection logic. ZRTP prtps
		// will be linked with outgoing media prtps a little bit later.
		if (ZFONE_SDP_MEDIA_TYPE_ZRTP != prtps->media.type)
		{
			_create_call(self, prtps, NULL, info);
			prtps->state = ZFONE_PRTPS_STATE_PRE_ESTABL;
		}
		
#if (ZRTP_PLATFORM == ZP_WIN32_KERNEL)
		//if (0 == prtps->call)
		//	DbgBreakPoint();
#endif
	} // packet processing border

	RETURN_DEEP_ANALYZE(self, zfone_hs_nop);
}

//------------------------------------------------------------------------------
static void _zfone_remove_hstream(struct zfone_heurist *heurist, zfone_hstream_t* hstream)
{
	if (hstream->in_stream) _zfone_release_prtps(heurist, hstream->in_stream);
	if (hstream->out_stream) _zfone_release_prtps(heurist, hstream->out_stream);	
	
	if (hstream->type == ZFONE_SDP_MEDIA_TYPE_AUDIO)
		heurist->a_calls_count--;
	else if (hstream->type == ZFONE_SDP_MEDIA_TYPE_VIDEO)
		heurist->v_calls_count--;

	zrtp_memset(hstream, 0, sizeof(zfone_hstream_t));
	hstream->state = ZFONE_PRTPS_STATE_PASIVE;
}

void  clean_closed_calls(struct zfone_heurist *heurist)
{
	mlist_t *node = NULL, *tmp_node = NULL;
	uint8_t i = 0;
	uint64_t snow = 0;
	zfone_hresult_t res;
	
	zrtp_mutex_lock(heurist->protector);
		
	snow = zrtp_time_now();
	
	mlist_for_each_safe(node, tmp_node, &heurist->rtpc_head)
	{
		zfone_prob_rtp_call_t *call = mlist_get_struct(zfone_prob_rtp_call_t, mlist, node);
		uint8_t is_closed = 0;

		// Looking for closed sessions/calls
		switch (call->conf_mode)
		{
		case ZFONE_CONF_MODE_CLOSED:			
			is_closed = (((snow - call->out_touch) * ZFONE_CALL_REMOVE_BYE_COEF) >= ZFONE_CALL_REMOVE_TIME);
			break;

		case ZFONE_CONF_MODE_UNKN:		
			is_closed = ( ((snow - call->out_touch) >= ZFONE_CALL_REMOVE_TIME) &&
						  ((snow - call->in_touch) >= ZFONE_CALL_REMOVE_TIME) );
			break;

		case ZFONE_CONF_MODE_CONFIRM:
		case ZFONE_CONF_MODE_LINK:
			is_closed = ( (((snow - call->out_touch) / ZFONE_CALL_REMOVE_SIP_COEF) >= ZFONE_CALL_REMOVE_TIME) &&
						  (((snow - call->in_touch) / ZFONE_CALL_REMOVE_SIP_COEF) >= ZFONE_CALL_REMOVE_TIME) );
			break;

		default:
			break;
		};

		if (is_closed)		
		{			
#ifdef ZFONE_USE_HEURIST_DEBUG
			ZRTP_LOG(3, (_ZTU_, "zfoned heurist: Remove time exceeded idelta=%x%x"
											" odelta=%x%x. now=%x%x\n", (snow - call->in_touch),
											(snow - call->out_touch), snow));
			heurist->show_state(heurist);
#endif
			zrtp_memset(&res, 0, sizeof(res));
			res.zfone_session = call->zfone_session;
			res.hstream = NULL;
			zfone_heurist_action_handler(&res, ZFONE_HEURIST_ACTION_CLOSE);

			for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
			{
				zfone_hstream_t *hstream = &call->streams[i];
				if (ZFONE_PRTPS_STATE_PASIVE != hstream->state)
					_zfone_remove_hstream(heurist, hstream);
			}

			if (call->psips) call->psips->call = NULL; // This avoids double-free on SIP BYE after time-out
			mlist_del(&call->mlist);
			zrtp_sys_free(call);
	
			ZRTP_LOG(3, (_ZTU_, "zfoned heurist: After deleting a=%d v=%d calls"
						   " left.\n\n", heurist->a_calls_count, heurist->v_calls_count));
		}

		// looking for SIP removed RTP streams
		else
		{
			for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
			{
				zfone_hstream_t *hstream = &call->streams[i];
				uint64_t idelta = 0;
				uint64_t odelta = 0;
				is_closed = 0;

				if (ZFONE_PRTPS_STATE_PASIVE == hstream->state)
					continue;

				idelta = hstream->in_stream ? (snow - hstream->in_stream->timestamp) : ZFONE_SESSION_REMOVE_TIME+1;
				odelta = hstream->out_stream ? (snow - hstream->out_stream->timestamp) : ZFONE_SESSION_REMOVE_TIME+1;

				if (ZFONE_PRTPS_STATE_CLOSED == hstream->state)
				{
					if ((idelta > ZFONE_SESSION_REMOVE_TIME/5) && (odelta > ZFONE_SESSION_REMOVE_TIME/5))
					{
						is_closed = 1;
						ZRTP_LOG(3, (_ZTU_, "zfoned heurist: REMOVE RTP stream N%d by SIP EVENT.\n", i));
					}
				}
				else if (ZFONE_SDP_MEDIA_TYPE_VIDEO == hstream->type)
				{
					// There is some traffice interuption during SECURing process. Prevent stream
					// from closing in transition states.
					if ( hstream->zfone_stream && hstream->zfone_stream->zrtp_stream &&
						(hstream->zfone_stream->zrtp_stream->state != ZRTP_STATE_SECURE)&&
						(hstream->zfone_stream->zrtp_stream->state != ZRTP_STATE_CLEAR) &&
						(hstream->zfone_stream->zrtp_stream->state != ZRTP_STATE_NONE) )
					{
						idelta = idelta / 6;
						odelta = odelta / 6;
					}

					if ( (idelta > ZFONE_SESSION_REMOVE_TIME) &&
						 (odelta > ZFONE_SESSION_REMOVE_TIME) )
					{
						is_closed = 1;
						ZRTP_LOG(3, (_ZTU_, "zfoned heurist: REMOVE VIDEO RTP stream N%d by TIMEOUT.\n", i));
					}
				}

				if ( is_closed )
				{
					heurist->show_state(heurist);

					zrtp_memset(&res, 0, sizeof(res));
					res.zfone_session = call->zfone_session;
					res.hstream = hstream;
					zfone_heurist_action_handler(&res, ZFONE_HEURIST_ACTION_REMOVE);
					_zfone_remove_hstream(heurist, hstream);
				}				
			}
		} // removing RTP streams closed by SIP REINVITE
	} // for all calls
	
	zrtp_mutex_unlock(heurist->protector);
}

//------------------------------------------------------------------------------
void clean_dead_streams(struct zfone_heurist *heurist)
{	
	mpoll_t *node = NULL, *n = NULL;
	zfone_prob_rtps_t *session = NULL;	
	
	zrtp_mutex_lock(heurist->protector);

	mpoll_for_each_safe(node, n, &heurist->rtps_poll)
	{		
		session = mpoll_get_struct(zfone_prob_rtps_t, _mpoll, node);
		// look over all non-established sessions. Established calls may be closed
		// by clean_closed_calls() only.
		if (session->state < ZFONE_PRTPS_STATE_PRE_ESTABL)
		{
			int64_t delta = zrtp_time_now() - session->timestamp;
			
			if (session->psips)
				delta = delta / ZFONE_SESSION_REMOVE_SIP_COEF;
			
			if( ((session->state < ZFONE_PRTPS_STATE_PRE_READY) && (delta >= ZFONE_SESSION_SLEEP_TIME)) ||
				((session->state >= ZFONE_PRTPS_STATE_PRE_READY) && (delta >= ZFONE_SESSION_REMOVE_TIME)) )
			{
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: REMOVE DEAD session"
											   " [%u]\n", zrtp_ntoh32(session->media.ssrc)));
				_zfone_release_prtps(heurist, session);
			}			
		}
	}

	zrtp_mutex_unlock(heurist->protector);
}

//-----------------------------------------------------------------------------
void _zfone_release_prtps( struct zfone_heurist* heurist,
 						   zfone_prob_rtps_t *prtps )
{
	if (prtps->call)
	{
		if (prtps->direction == ZFONE_IO_OUT)
			prtps->call->streams[prtps->pos].out_stream = NULL;
		else
			prtps->call->streams[prtps->pos].in_stream = NULL;		
	}

	prtps->state = ZFONE_PRTPS_STATE_PASIVE;
	mlist_del(&prtps->mlist_map);
	mpoll_release(&heurist->rtps_poll, &prtps->_mpoll);
}

//-----------------------------------------------------------------------------
static zfone_heurist_status_t _update_call( struct zfone_heurist *self,
											zfone_prob_rtps_t *prtps,
											zfone_prob_rtps_t *pair_prtps )
{
	zfone_hresult_t res;
	zfone_hstream_t *hstream = NULL;

	// TODO: FOR TESTING ONLE - REMOVE THIS CODE BLOCK
	{
		if (prtps->pos >=2)
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned heurist: AAAAAAAAAAAAAAAAAAAAAAAA heurist -1 pos=%d\n", prtps->pos));
			return zfone_hs_error;
		}		

		if (pair_prtps->direction > ZFONE_IO_OUT)
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned heurist: AAAAAAAAAAAAAAAAAAAAAAAA heurist -2 dir=%d\n", pair_prtps->direction));
			return zfone_hs_error;
		}

		if (prtps->direction > ZFONE_IO_OUT)
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned heurist: AAAAAAAAAAAAAAAAAAAAAAAA heurist -3 dir=%d\n", prtps->direction));
			return zfone_hs_error;
		}
	}

	hstream = &prtps->call->streams[prtps->pos];

	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: UPDATED ONE_WAY call %p:%s with %p:%s to FULL call\n",
				    prtps, zfone_prtps_state_names[prtps->state], pair_prtps, zfone_prtps_state_names[prtps->state]));		

    // We used outgoing = ~incoming ssrc in case if we have "one-way" incoming
	// call only. Now it's time to replace it with original values.
	if (ZFONE_IO_IN == prtps->direction)
		hstream->out_stream = pair_prtps;
	else
		hstream->in_stream 	= pair_prtps;	

	// Copy call pointer and heurist stream position from upgreating stream
	pair_prtps->pos  = prtps->pos;
	pair_prtps->media.hstream = prtps->media.hstream;
	pair_prtps->call = prtps->call;

	// Return action to the main engine
	zrtp_memset(&res, 0, sizeof(res));
	res.hstream			= hstream;
	res.zfone_session	= prtps->call->zfone_session;
	zfone_heurist_action_handler(&res, ZFONE_HEURIST_ACTION_CHANGE);

	ZRTP_LOG(3, (_ZTU_, "zfoned_heurist: after _call_update():\n"));
    self->show_state(self);

	return zfone_hs_full_updated;
}

//-----------------------------------------------------------------------------
zfone_heurist_status_t _create_call( struct zfone_heurist* self,
									 zfone_prob_rtps_t *prtps,
									 zfone_prob_rtps_t *pair_prtps,
									 zfone_rtpcheck_info_t *info )
{
	uint8_t i = 0;
	zfone_hstream_t *hstream = NULL;
	zfone_hresult_t res;
	zfone_prob_rtp_call_t *call = NULL;
	zfone_heurist_action_t ret_action = ZFONE_HEURIST_ACTION_CREATE; // Use create event by default	
	uint64_t snow = zrtp_time_now();	

	// WARNING! pay attantion: pair_prtps is NULL for "One-Way" calls

	ZRTP_LOG(3, (_ZTU_, "zfoned heurist: CREATING a new %s call from %p:%s and %p:%s\n",
				   pair_prtps ? "REGULAR" : "ONE_WAY", prtps, zfone_prtps_state_names[prtps->state],
				   pair_prtps, pair_prtps ? zfone_prtps_state_names[pair_prtps->state] : "NULL" ));
		
	// If RTP streams were not confirmed by SIP session - try to link by time		
	if (!prtps->psips || (pair_prtps ? !pair_prtps->psips : 0))
	{
		zfone_prob_sips_t *psips = _find_appropriate_sips(self, prtps, pair_prtps);
		if (psips && pair_prtps)
		{							
			pair_prtps->psips = psips;
			pair_prtps->conf_mode = ZFONE_CONF_MODE_LINK;
			zrtp_memcpy(pair_prtps->sip_id, psips->sips->id, sizeof(zfone_sip_id_t));			
		}
	}

	// If both streams within new session are confirmed - look for existing
	// call to attach session for. (Use SIP session ID as a primary key).
	// If such stream can't be found - create a new one. (For CREATE* actions only)
	if (prtps->psips && (pair_prtps ? (pair_prtps->psips ? 1 : 0) : 1))
	{
		mlist_t *list_node = NULL;
	
		char buff[ SIP_SESSION_ID_SIZE*3];
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: Looking for call to attach for sip_id:%s.\n",
					   hex2str((void*)prtps->sip_id, SIP_SESSION_ID_SIZE, buff, sizeof(buff))));

		mlist_for_each(list_node, &self->rtpc_head)
		{
			zfone_prob_rtp_call_t *curr_call = mlist_get_struct(zfone_prob_rtp_call_t, mlist, list_node);	

			ZRTP_LOG(3, (_ZTU_, "zfoned heurist: checking for Call with sips=%p and SIP ID:%s.\n",
						   curr_call->psips, ZH_PRINT_SIPS(curr_call, buff)));

			if (curr_call->psips && !zfone_sipidcmp(curr_call->sip_id, prtps->sip_id))
			{	
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: Call was FOUND - ATTACHING to it.\n"));	
				// TODO: look for collisions by stream type, IP, port
				call = curr_call;
				ret_action = ZFONE_HEURIST_ACTION_ADD;
				break;
			}
		}
	}

	if (!call)
	{
		ZRTP_LOG(3, (_ZTU_, "zfoned heurist: Call wasn't FOUND - creating a NEW one.\n"));

		call = zrtp_sys_alloc(sizeof(zfone_prob_rtp_call_t));
		if ( !call )
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned zrtp_deep_analyze(): can't allocate memory for the new call.\n"));
			return zfone_hs_error;
		}
		zrtp_memset(call, 0, sizeof(zfone_prob_rtp_call_t));
		mlist_add(&self->rtpc_head, &call->mlist);

		if (prtps->psips && (pair_prtps ? (pair_prtps->psips ? 1 : 0) : 1))
		{
			zrtp_memcpy(call->sip_id, prtps->psips->sips->id, sizeof(zfone_sip_id_t));
			call->psips = prtps->psips;
			call->conf_mode = prtps->conf_mode;
			call->psips->call = call;
		}
		
		call->in_touch = snow;
		call->out_touch = snow;

		// TODO: may be we should remove mistakally marked streams there.
	}

	prtps->call = call;
	if (pair_prtps) pair_prtps->call = call;

	// Looking for free slot for the new stream
	for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
	{
		if (ZFONE_PRTPS_STATE_PASIVE == call->streams[i].state)
		{
			hstream = &call->streams[i];
			hstream->state = ZFONE_PRTPS_STATE_ACTIVE;
			break;
		}
	}

	if (!hstream)
	{
		ZRTP_LOG(1, (_ZTU_, "zfoned heurist: Can't attach one more stream to %p."
					    " Limit %d was reached.\n", call, MAX_SDP_RTP_CHANNELS));
		return zfone_hs_error;
	}	

	hstream->type = prtps->media.type;
	if (ZFONE_IO_IN == prtps->direction)
	{		
		hstream->in_stream 			= prtps;
		hstream->out_stream 	 	= pair_prtps;
	}
	else
	{		
		hstream->in_stream 		 	= pair_prtps;
		hstream->out_stream		 	= prtps;
	}	

	prtps->pos = i;	
	prtps->media.hstream = hstream;

	if (pair_prtps)
	{
		pair_prtps->pos = i;
		pair_prtps->media.hstream = hstream;
	}

	// Increase streams counter for "Smart closing" of non confirmed streams	
	if (hstream->type ==  ZFONE_SDP_MEDIA_TYPE_AUDIO)
		self->a_calls_count++;
	else if (hstream->type ==  ZFONE_SDP_MEDIA_TYPE_VIDEO)
		self->v_calls_count++;
	
	// Link SIP media stream with the current heurist stream. We will use this
	// link to update heurist's states on any updates in SIP session states.
	if ( ((hstream->in_stream) ? hstream->in_stream->psips : 0) &&
		 ((hstream->out_stream) ? hstream->out_stream->psips : 0) )
	{
		for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
		{
			zfone_media_stream_t *sipmedia = &call->psips->sips->conf_streams[i];
			if ( (prtps->media.remote_ip == sipmedia->remote_rtp) ||				  
				 (prtps->media.remote_rtp == sipmedia->local_rtp) )
			{
				sipmedia->hstream = hstream;
				break;
			}
		}
	}

	// Return action to the main engine		
	zrtp_memset(&res, 0, sizeof(res));
	res.hstream			= hstream;
	res.zfone_session	= call->zfone_session;
	zfone_heurist_action_handler(&res, ret_action);
	call->zfone_session = res.zfone_session;

	ZRTP_LOG(3, (_ZTU_, "zfoned_heurist: after _call_create():\n"));
    self->show_state(self);

	return zfone_hs_full_created;
}

