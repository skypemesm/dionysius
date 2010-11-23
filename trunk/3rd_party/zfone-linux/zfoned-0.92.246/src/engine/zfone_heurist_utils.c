/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 * Vitaly Rozhkov <v.rozhkov@soft-industry.com> <vitaly.rozhkov@googlemail.com>
 */

#include <zrtp.h>

#include "zfone.h"

#define _ZTU_ "zfone hutils"

//=============================================================================
//  TESTS
//=============================================================================


// Transport layer checks:
// - UDP/TCP only	
// - appropriate RTP packet lengths;
// - UDP port from "User" ports space.
//-----------------------------------------------------------------------------
static zfone_symptom_status_t _zsym_transport_check( struct zfone_heurist* heurist,
													 zfone_rtpcheck_info_t* info )
{
	zfone_symptom_status_t s = ZFONE_SYMP_STATUS_ERROR;
	do
	{
		if ( ((info->packet->proto != voip_proto_UDP) && (voip_proto_TCP != info->packet->proto)) ||
			 (!(zfone_cfg.params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_TCP) && (voip_proto_TCP == info->packet->proto)) )
		{
			//ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zsym_transport_check()\n"
			//  			 " break 0. proto=%d\n", info->packet->proto));
			break;
		}
		
		if ( ((info->packet->size - info->packet->offset) < ZRTP_SYMP_MIN_RTP_LENGTH) ||
			 ((info->packet->size - info->packet->offset) > ZRTP_SYMP_MAX_RTP_LENGTH) )
		{
			//ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zsym_transport_check()\n"
			// 				 " break 1. size=%d\n", (info->packet->size - info->packet->offset)));
			break;
		}
		
		if (zrtp_ntoh16(info->packet->local_port) < ZRTP_SYMP_MIN_RTP_PORT)
		{
			//ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zsym_transport_check()\n"
			// 				 " break 2. port=%d\n", zrtp_ntoh16(info->packet->local_port)));
			break;
		}
		
		info->rtp_hdr = (zrtp_rtp_hdr_t*) (info->packet->packet + info->packet->offset);
		info->score += ZFONE_SYMP_TRANSP_COST;
		s = ZFONE_SYMP_STATUS_MATCH;
	} while(0);
	
	return s;
}

// returns ZFONE_SYMP_STATUS_MATCH if info->packet is NOT an alert media
// generated by libzrtp on linux
//-----------------------------------------------------------------------------
static zfone_symptom_status_t _zsym_skipalert_check( struct zfone_heurist* heurist,
												     zfone_rtpcheck_info_t* info )
{
	zfone_symptom_status_t s = ZFONE_SYMP_STATUS_MATCH;
	
	mlist_t *list_node = NULL;	
	if (info->packet->direction == ZFONE_IO_IN)	
	{
		int i = 0;
    	mlist_for_each(list_node, &manager.ctx_head)
    	{
			zfone_ctx_t* ctx = mlist_get_struct(zfone_ctx_t, mlist, list_node);
			for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
			{
				zfone_stream_t* stream = &ctx->streams[i];
				if (stream->type != ZFONE_SDP_MEDIA_TYPE_UNKN)				
				{					
					if ((stream->alert_code != ZRTP_ALERT_PLAY_NO) && (stream->alert_ssrc == info->rtp_hdr->ssrc))
					{
//						ZRTP_LOG(3, (_ZTU_, "zfone: (_zsym_skipalert_check) skip alert packet"
//							" ssrc=%u seq=%d.\n", zrtp_ntoh32(info->rtp_hdr->ssrc), zrtp_ntoh16(info->rtp_hdr->seq)));
						s = ZFONE_SYMP_STATUS_DIST;
						break;
					}
				}					 
			}
    	}
	}

	if (ZFONE_SYMP_STATUS_MATCH == s)
		info->score += ZFONE_SYMP_RTP_NONALERT_COST;

	return s;
}

//-----------------------------------------------------------------------------
#define ZFONE_PAYLOAD_UNSUP				1
#define ZFONE_PAYLOAD_UNDEF				0
#define ZFONE_PAYLOAD_ARRAY_SIZE		128

typedef struct zfone_payload
{
	uint8_t				def_size;	// 0 - undefined; - 1- unsupported payload type.
	zfone_media_type_t	type;
} zfone_payload_t;

static  zfone_payload_t _zfone_rtp_payload_types[ZFONE_PAYLOAD_ARRAY_SIZE];

// 0	- PCMU
// 3	- GSM
// 4	- G723
// 5	- DVI4
// 6	- DVI4
// 7	- LPC
// 8	- PCMA
// 9	- G722
// 10	- L16
// 11	- L16
// 12	- QCELP
// 13	- CN
// 14	- MPA
// 15	- G728
// 16	- DVI4
// 17	- DVI4
// 18	- G729
// 97	- iLBC 800
// 98	- iLBC 800			(for SJphone)
// 100	- eg711U 800		(for Gizmo) / SPEEX/16000			(4 X-Lite)
// 101	- eg711A 800		(for Gizmo) / telephone-event/8000	(4 X-Lite)
// 102	- iLBC 800			(for Gizmo)
// 103	- ISAC 1600			(for Gizmo)
// 105 -  SPEEX-FEC/8000	(4 X-Lite)
// 106	- telephone 800		(for Gizmo) / SPEEX-FEC/16000		(4 X-Lite)
// 107  - BV32/16000		(4 X-Lite)
// 110	- SPEEX 800
// 111	- SPEEX 1600 
// 114	- SPEEX 1600
// 117  - red/8000			(4 Gizmo)
// 119  - BV32-FEC/16000	(4 X-Lite)

// 31	- H261    			(4 XMeeting)
// 32	- MPV
// 33	- H263  			(4 Gizmo and Counterpath)
// 34	- H264				(for iChat)   / H263/90000 (4 X-Lite)
// 115	- H263  			(for sjphone)
// 126	 -264				(for iChat)
void _zfone_init_payloads_list()
{
  uint8_t i = 0;
  
  for (i=0; i<ZFONE_PAYLOAD_ARRAY_SIZE; i++)
  {
	  _zfone_rtp_payload_types[i].def_size	= ZFONE_PAYLOAD_UNSUP;
	  _zfone_rtp_payload_types[i].type		= ZFONE_SDP_MEDIA_TYPE_UNKN;
  }

  // Set AUDIO payloads
  _zfone_rtp_payload_types[0].def_size		= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[0].type			= ZFONE_SDP_MEDIA_TYPE_AUDIO;
  for (i=2; i<19; i++)
  {
	  _zfone_rtp_payload_types[i].def_size	= ZFONE_PAYLOAD_UNDEF;		
	  _zfone_rtp_payload_types[i].type		= ZFONE_SDP_MEDIA_TYPE_AUDIO;
  }
  
  _zfone_rtp_payload_types[97].def_size		= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[97].type			= ZFONE_SDP_MEDIA_TYPE_AUDIO;
  _zfone_rtp_payload_types[98].def_size		= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[98].type			= ZFONE_SDP_MEDIA_TYPE_AUDIO;  
  for (i=9; i<104; i++)
  {
	  _zfone_rtp_payload_types[i].def_size	= ZFONE_PAYLOAD_UNDEF;		
	  _zfone_rtp_payload_types[i].type		= ZFONE_SDP_MEDIA_TYPE_AUDIO;
  }
  for (i=105; i<=107; i++)
  {
	  _zfone_rtp_payload_types[i].def_size	= ZFONE_PAYLOAD_UNDEF;		
	  _zfone_rtp_payload_types[i].type		= ZFONE_SDP_MEDIA_TYPE_AUDIO;
  }

  _zfone_rtp_payload_types[110].def_size	= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[110].type		= ZFONE_SDP_MEDIA_TYPE_AUDIO;
  _zfone_rtp_payload_types[111].def_size	= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[111].type		= ZFONE_SDP_MEDIA_TYPE_AUDIO;  
  _zfone_rtp_payload_types[114].def_size	= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[114].type		= ZFONE_SDP_MEDIA_TYPE_AUDIO;  
  _zfone_rtp_payload_types[117].def_size	= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[117].type		= ZFONE_SDP_MEDIA_TYPE_AUDIO;  
  _zfone_rtp_payload_types[119].def_size	= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[119].type		= ZFONE_SDP_MEDIA_TYPE_AUDIO;

  // Set VIDEO payloads
  for (i=31; i<35; i++)
  {
	  _zfone_rtp_payload_types[i].def_size	= ZFONE_PAYLOAD_UNDEF;		
	  _zfone_rtp_payload_types[i].type		= ZFONE_SDP_MEDIA_TYPE_VIDEO;
  }  
  _zfone_rtp_payload_types[126].def_size 	= ZFONE_PAYLOAD_UNDEF;
  _zfone_rtp_payload_types[126].type		= ZFONE_SDP_MEDIA_TYPE_VIDEO;
  _zfone_rtp_payload_types[115].def_size	= ZFONE_PAYLOAD_UNDEF;		
  _zfone_rtp_payload_types[115].type		= ZFONE_SDP_MEDIA_TYPE_VIDEO;
}


//  RTP Data Header Validity Checks according to RFC 3550 (sec A.1):
// 1. version = 2;
// 2. know payload type (and in particular it must not be equal to SR or RR);

// 3. if the P bit is set, then the last octet == (total length - header size);
// 4. the X bit must be zero if the profile does not support extensions.
//	  Otherwise:  extension length < packet size - (header size + padding);
// 5. the length of the packet must be consistent with CC and payload type
//	  (if payloads have a known length).
static zfone_symptom_status_t _zsym_rtp_static_check( struct zfone_heurist* heurist,
											  		  zfone_rtpcheck_info_t* info )
{
	zfone_symptom_status_t s = ZFONE_SYMP_STATUS_ERROR;

	do
	{
		zfone_payload_t* pt = NULL;

		if (info->rtp_hdr->version != 2)
			break;

		if (info->rtp_hdr->ssrc == 0)
			break;

		// TODO: analyze RTCP later
		// skip all RTCP packets
		{
			zrtp_rtcp_hdr_t* rtcp_hdr = (zrtp_rtcp_hdr_t*) info->rtp_hdr;
			if ( (rtcp_hdr->pt == 192) || (rtcp_hdr->pt == 193) ||
				 ((rtcp_hdr->pt > 199) && (rtcp_hdr->pt < 208)) )
			{
				uint16_t rtcp_len = zrtp_ntoh16(rtcp_hdr->len);
				if (0 == ((info->packet->size - 8) % 4))
				{
					if (rtcp_len == ((info->packet->size - 8) / 4) - 1)
					{					
						//ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zsym_rtp_check() break 1. skip RTCP\n"));
						break;
					}
				}

#if (ZRTP_PLATFORM == ZP_DARWIN)
				// There is one more trick in our RTP detection logic: the last version of iChat
				// sends some strange utility packets masked as RTP/RTCP and they break our detection
				// logic. We have to reject such packets (may be lated we will fid out what these
				// packets mean).
				
				// This string should distinguish RTP timestamp and RTCP length field
				if (zrtp_ntoh16(rtcp_hdr->len) < 100)
				{
					// This is not a RTP but not a RTCP as well. Check lenks to make sure
					if (rtcp_len != ((info->packet->size - 8)/4 - 1))
					{
						ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zsym_rtp_check() break 2. skip unknown packet (iChat hack)\n"));
						break;
					}
				}
#endif
			}
		}
		
		if (info->rtp_hdr->pt >= ZFONE_PAYLOAD_ARRAY_SIZE)
		{
			//ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zsym_rtp_check() break\n"
			// 				 " 2. unsupported payload type=%d\n", info->rtp_hdr->pt));
			break;
		}
		pt = &_zfone_rtp_payload_types[info->rtp_hdr->pt];
		if (pt->type ==  ZFONE_SDP_MEDIA_TYPE_UNKN)
		{
			//ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zsym_rtp_check() break\n"
			// 				 " 3. unsupported payload type=%d\n", info->rtp_hdr->pt));
			break;
		}
		else
		{
		  	info->type = pt->type;
			if (pt->def_size != ZFONE_PAYLOAD_UNDEF)
			{
				// TODO: check payload size according to its' specification
				// TODO: packet size == payload size + SSRC
			}
		}
		
		if ( info->rtp_hdr->p )
		{
			uint8_t payload_length = (uint8_t)*(info->packet->packet + info->packet->size - 1);
			if (payload_length > (info->packet->size - info->packet->offset - RTP_V2_HDR_SIZE))
			{				
				//ZRTP_LOG(3, (_ZTU_, "zfoned heurist: _zsym_rtp_check() break\n"
				// 				" 4. P=1 and length=%d size=%d.\n", payload_length, info->packet->size));
				break;
			}
		}
		
		if ( info->rtp_hdr->x )
		{
			// TODO: check extension lengths
		}
		info->score += ZFONE_SYMP_RTP_STATIC_COST;
			
		s = ZFONE_SYMP_STATUS_MATCH;
	} while (0);

	return s;
}

//------------------------------------------------------------------------------
static zfone_symptom_status_t _zsym_zrtp_hello_check( struct zfone_heurist* heurist,
												      zfone_rtpcheck_info_t* info )
{
	if (ZFONE_IO_IN == info->packet->direction) // look at incoming packets only
	{
		uint8_t length = info->packet->size - info->packet->offset - 8;
		if ( (length  > ZRTP_HELLO_STATIC_SIZE) &&
			 (0 == info->rtp_hdr->version) &&
			 (ZRTP_HELLO == _zrtp_packet_get_type(info->rtp_hdr, length)) )
		{
			// We should react on Hellos from non-ZFone, embeded into real VoIP
			// clients endpoints. ZFone may make assumptions about outgoing
			// ssrc (when there is no traffic) and it will breack our protection
			// against voice/video mixing.
			zrtp_packet_Hello_t *hello = (zrtp_packet_Hello_t*) ((char*)info->rtp_hdr + RTP_HDR_SIZE);
			if ( (0 != zrtp_memcmp(hello->cliend_id, ZFONE_WIN_CLIENT_ID, 12)) &&
				 (0 != zrtp_memcmp(hello->cliend_id, ZFONE_LIN_CLIENT_ID, 12)) &&
				 (0 != zrtp_memcmp(hello->cliend_id, ZFONE_MAC_CLIENT_ID, 12)) )
			{
#ifdef ZFONE_USE_HEURIST_DEBUG
			ZRTP_LOG(3, (_ZTU_, "zfoned heurist: ZRTP HELLO packet"
						   " received from other side %.12s ssrc:lp:rp %u:%d:%d!\n",						    
						   hello->cliend_id,						   
						    zrtp_ntoh32(info->rtp_hdr->ssrc),
							zrtp_ntoh16(info->packet->local_port),
							zrtp_ntoh16(info->packet->remote_port)));
#endif
				// Made heurist to allow this packet without the dynamic tests
				info->score += ZFONED_RTP_STREAM_ACCEPT_BORDER;
				info->type = ZFONE_SDP_MEDIA_TYPE_ZRTP;
				return ZFONE_SYMP_STATUS_MATCH;
			}
		}
	}

	return ZFONE_SYMP_STATUS_DIST;
}


//------------------------------------------------------------------------------
#define RTP_SEQ_MOD (uint16_t) (1<<16)
#define MAX_DROPOUT (uint16_t) 3000;
#define MAX_MISORDER (uint16_t) 100;

static void _zfone_init_seq(zfone_rtpcheck_info_t *info)
{

	if(!info || !info->rtp_hdr || !info->prtps)	
		return;

	info->prtps->seq_data.max_seq = zrtp_ntoh16(info->rtp_hdr->seq);
	info->prtps->seq_data.bad_seq = RTP_SEQ_MOD + 1;
	info->prtps->seq_data.cycles = 0;
    info->prtps->seq_data.received = 0;
    info->prtps->seq_data.received_prior = 0;
    info->prtps->seq_data.expected_prior = 0;
	
	info->prtps->seq_data.max_ts = zrtp_ntoh32(info->rtp_hdr->ts);
}

// Call this function when the new SSRC is registered
void _zfone_init_prob_rtps(zfone_rtpcheck_info_t *info)
{
	_zfone_init_seq(info);
	info->prtps->seq_data.max_seq = zrtp_ntoh16(info->rtp_hdr->seq) - 1;
	info->prtps->seq_data.probation = ZFONE_HEURIST_MIN_SEQUENTIAL;
	info->prtps->seq_data.tmp_score = 0;
}


//------------------------------------------------------------------------------
static zfone_symptom_status_t _zsym_rtp_seq_check( struct zfone_heurist* heurist, 
												   zfone_rtpcheck_info_t* info )
{
	zrtp_rtp_hdr_t*		rtp_hdr = info->rtp_hdr;
	zfone_prob_rtps_t*	prtps = info->prtps;
	uint16_t seq = zrtp_ntoh16(rtp_hdr->seq);
	uint32_t ts = zrtp_ntoh32(rtp_hdr->ts);	
	
	//
	// Source is not valid until MIN_SEQUENTIAL packets with
	// sequential sequence numbers have been received.
	//
	if (prtps->seq_data.probation > 0)
	{
		// packet is in sequence	
		if ((seq == prtps->seq_data.max_seq + 1) && (ts >= prtps->seq_data.max_ts))
		{
			// Make dynamic test to run twice faster for SIP confirmed streams.
			prtps->seq_data.probation -= (prtps->psips) ? 2 : 1;
			prtps->seq_data.max_seq = seq;
			prtps->seq_data.max_ts = ts;
			
			if (prtps->seq_data.probation <= 0)
			{
				_zfone_init_seq(info);
				prtps->seq_data.received++;
				prtps->total_score += ZFONE_SYMP_SEQ_COST;
				prtps->total_score -= prtps->seq_data.tmp_score;
				return ZFONE_SYMP_STATUS_MATCH;				
			}
			else if((prtps->seq_data.probation <= ZFONE_HEURIST_MIN_SEQUENTIAL/2) && 
					(0 == prtps->seq_data.tmp_score))
			{
				prtps->seq_data.tmp_score = ZFONE_SYMP_SEQ_COST/2;
				prtps->total_score += prtps->seq_data.tmp_score;
			}
		}
		else
		{
			prtps->seq_data.probation = ZFONE_HEURIST_MIN_SEQUENTIAL - 1;
			prtps->seq_data.max_seq = seq;
			prtps->seq_data.max_ts = ts;
//			ZRTP_LOG(3, (_ZTU_, "zfoned heurist:  _zsym_rtp_seq_check(): RESET to sec=%d ts=%u!\n", seq, ts));

		}
		
		return ZFONE_SYMP_STATUS_DIST;
	}
	else
	{
		// Source is valid. Do nothing
		return ZFONE_SYMP_STATUS_MATCH;
	}
#if 0  // TODO: why do we need this?
	//
	// Source is valid now. Check each packet
	//
	else if (udelta < MAX_DROPOUT)
	{
		// In order, with permissible gap
		if (seq < prtps->max_seq)
		{
            // Sequence number wrapped - count another 64K cycle.
			prtps->cycles += RTP_SEQ_MOD;
		}
		prtps->max_seq = seq;
	}
	else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) 
	{
		// The sequence number made a very large jump
		if (seq == prtps->bad_seq)
		{
			// Two sequential packets -- assume that the other side restarted
			// without telling us so just re-sync (i.e., pretend this was the
			//  first packet
			_zrtp_init_seq(info);
		}
		else
		{
			prtps->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
			return ZFONE_SYMP_STATUS_ERROR;
		}
	}
	else
	{
		// duplicate or reordered packet
	}
	prtps->received++;
#endif

	return ZFONE_SYMP_STATUS_MATCH;
}


//------------------------------------------------------------------------------
zfone_rtp_symptom_handler* _rtp_static_sympt[ZFONE_SYMP_SCOUNT] =
{
	_zsym_transport_check,
	_zsym_skipalert_check,
	//_zsym_zrtp_hello_check,
	_zsym_rtp_static_check
};

zfone_rtp_symptom_handler* _rtp_dynam_sympt[ZFONE_SYMP_DCOUNT] =
{
	_zsym_rtp_seq_check
};



//=============================================================================
//  Heurist Utils
//=============================================================================


extern zfone_heurist_status_t _create_call( struct zfone_heurist* self,
											zfone_prob_rtps_t *prtps,
											zfone_prob_rtps_t *pair_prtps,
											zfone_rtpcheck_info_t *info );

//-----------------------------------------------------------------------------
uint8_t zfone_heurist_check4changes( struct zfone_heurist *heurist,
									 zfone_prob_rtps_t* prtps,
									 zfone_rtpcheck_info_t *info )
{	
#ifdef ZFONE_USE_HEURIST_DEBUG
    char ipbuff1[25]; char ipbuff2[25];	
	char ipbuff3[25]; char ipbuff4[25];	
#endif
	uint8_t was_changed = 0;
	mpoll_t *poll_node = NULL, *poll_tmp = NULL;

	ZRTP_LOG(3, (_ZTU_, "zfoned zfoned_heurist: looking for duplicated streams:\n"));

	mpoll_for_each_safe(poll_node, poll_tmp, &heurist->rtps_poll)
	{
		zfone_prob_rtps_t* elem = (zfone_prob_rtps_t*) mpoll_get_struct(zfone_prob_rtps_t, _mpoll, poll_node);

		if (elem == prtps)
			continue; // skip self

		if (prtps->media.ssrc == elem->media.ssrc)
		{
			ZRTP_LOG(3, (_ZTU_, "zfoned zfoned_heurist: Duplicated RTP streams was"
										   " discovered - checking ports and IPs:\n"));
			if ( (prtps->media.remote_rtp != elem->media.remote_rtp) ||
				 (prtps->media.remote_ip != elem->media.remote_ip)   ||
				 (prtps->media.local_rtp != elem->media.local_rtp)   ||
				 (prtps->media.local_ip != elem->media.local_ip)  )
			{
#ifdef ZFONE_USE_HEURIST_DEBUG
				ZRTP_LOG(3, (_ZTU_, "zfoned_heurist: transport options were"
				" CHANGED from %s:%d<-->%s:%d to %s:%d<-->%s:%d state=%s.\n",
				zfone_ip2str(ipbuff1, 25, zrtp_ntoh32(elem->media.local_ip)), zrtp_ntoh16(elem->media.local_rtp),
				zfone_ip2str(ipbuff2, 25, zrtp_ntoh32(elem->media.remote_ip)), zrtp_ntoh16(elem->media.remote_rtp),
				zfone_ip2str(ipbuff3, 25, zrtp_ntoh32(prtps->media.local_ip)), zrtp_ntoh16(prtps->media.local_rtp),
				zfone_ip2str(ipbuff4, 25, zrtp_ntoh32(prtps->media.remote_ip)), zrtp_ntoh16(prtps->media.remote_rtp),								   
				zfone_prtps_state_names[elem->state] ));
#endif
				// Update RTP stream transport options in all active states
				if ( (elem->state >= ZFONE_PRTPS_STATE_ACTIVE) &&
					 (elem->state < ZFONE_PRTPS_STATE_CLOSED) )
				{
					// If local port have been changed - put updated stream to the apropriate branch
					if (prtps->media.local_rtp != elem->media.local_rtp)
					{
						zfone_hs_node_t *new_head = &heurist->rtps_map[zrtp_ntoh16(prtps->media.local_rtp)];
						mlist_del(&elem->mlist_map);
						mlist_add(&new_head->head, &elem->mlist_map);
					}

					prtps->media.hstream = elem->media.hstream;
					zrtp_memcpy(&elem->media, &prtps->media, sizeof(zfone_media_stream_t));						
				}
				
				ZRTP_LOG(3, (_ZTU_, "zfoned heurist: looking for changes in calls map.. state=%d\n", elem->conf_mode));

				// Some transport options have been changed - it brings entire calls map to changes.
				// We should review all existing stream for possible changes in calls structure.				

				// TODO: restore this when have time
				/*
				switch (elem->conf_mode)
				{
				case ZFONE_CONF_MODE_LINK:
					// Do nothing with the "Linked" streams
					break;

				case ZFONE_CONF_MODE_CONFIRM:
					// If the stream is not confirmed by SIP any longer - clear confirmation
					// and refresh the states of other "Unknown" streams
					if (0 != _zfone_mark_prtps_by_sip(elem, elem->psips, 0))
					{
						mpoll_t *node = NULL;

						ZRTP_LOG(3, (_ZTU_, "!!! zfoned heurist: call was UN-CONFIRMED - clear SIP marks\n"));
						elem->psips->call = NULL;
						elem->conf_mode = ZFONE_CONF_MODE_UNKN;						
						elem->psips = NULL;
						zrtp_memset(elem->sip_id, 0, sizeof(zfone_sip_id_t));
						if (elem->call) elem->call->conf_mode = ZFONE_CONF_MODE_UNKN;
						
						ZRTP_LOG(3, (_ZTU_, "!!! zfoned heurist: look for changes in other streams...\n"));
						mpoll_for_each(node, &heurist->rtps_poll)						
						{
							zfone_prob_rtps_t *tmp = mpoll_get_struct(zfone_prob_rtps_t, _mpoll, node);
							if ((tmp->conf_mode == ZFONE_CONF_MODE_UNKN) && (tmp != elem))
								_find_appropriate_sips(heurist, tmp, NULL);							
						}

						was_changed = 1;						
					}
					break;

				case ZFONE_CONF_MODE_UNKN:
					if (0 == _zfone_mark_prtps_by_sip(elem, elem->psips, 0))					
					{
						ZRTP_LOG(3, (_ZTU_, "!!! zfoned heurist: call was CONFIRMED\n"));
						was_changed = 1;
					}	
					else
					{
						if ( _find_appropriate_sips(heurist, elem, NULL) )
						{
							ZRTP_LOG(3, (_ZTU_, "!!! zfoned heurist: call was LINKED\n"));
							was_changed = 1;
						}
					}

				default:
					break;
				}


				*/
				if ( (elem->state == ZFONE_PRTPS_STATE_PRE_ESTABL) ||
					 (elem->state == ZFONE_PRTPS_STATE_ESTABL) )
				{
					zfone_hresult_t res;
					zrtp_memset(&res, 0, sizeof(res));
					res.zfone_session = elem->call->zfone_session;
					res.hstream = &elem->call->streams[elem->pos];

					/*
					if (was_changed)
					{						
						ZRTP_LOG(3, (_ZTU_, "!!! zfoned heurist: call was  changed so"
										" remove the old Zfone stream and try to recreate it\n"));

						zfone_heurist_action_handler(&res, ZFONE_HEURIST_ACTION_REMOVE);
						_create_call(heurist, elem, NULL, info);
					}
					else
					{	
					*/				
						zfone_heurist_action_handler(&res, ZFONE_HEURIST_ACTION_CHANGE);
					/*
                    }	
					*/				
				}				

				// Remove duplicated session (we will use the updated one)
				_zfone_release_prtps(heurist, prtps);

				was_changed = 1;
				break;
			}
			else
			{
				ZRTP_LOG(1, (_ZTU_, "zfoned_heurist: any changes in transport"
								" options - STRANGE BEHAVIOR. Leave this session.\n"));
			}
		} // changes in transport oprions
	} // through all RTPSes

	return !was_changed;
}


//-----------------------------------------------------------------------------
static void show_state(struct zfone_heurist *heurist)
{
	mlist_t *list_node = NULL;
	mpoll_t *poll_node = NULL;
	int j = 0;
	int branches_count = 0;
	char ipbuff[25];
	
	uint64_t snow = zrtp_time_now();

	zrtp_print_log_delim(3, LOG_START_SUBSECTION, "heurist state");
	
	// Show branches
	ZRTP_LOG(3, (_ZTU_, "branches list:\n"));
	for(j=0; j<ZFONE_PORTS_SPACE_SIZE; j++)
	{
		// Skip empty branches
		if(&heurist->rtps_map[j].head != heurist->rtps_map[j].head.next)
		{
			branches_count++;
			ZRTP_LOG(3, (_ZTU_, "port %u:\n", j));
			
			mlist_for_each(list_node, &heurist->rtps_map[j].head)
			{
				zfone_prob_rtps_t *p_session = mlist_get_struct(zfone_prob_rtps_t, mlist_map, list_node);
				// [ssrc, IP:port, direction, type, state, codec, timestamp, delta]
				ZRTP_LOG(3, (_ZTU_, "\t[%u, %s:%d, %s, %s, %s, codec:%d ts:%u, delta:%i]\n",
							    zrtp_ntoh32(p_session->media.ssrc),
								zfone_ip2str(ipbuff, sizeof(ipbuff), zrtp_ntoh32(p_session->media.remote_ip)),
								zrtp_ntoh16(p_session->media.remote_rtp),
							    zfone_direction_names[p_session->direction],
							    zfone_media_type_names[p_session->media.type],
							    zfone_prtps_state_names[p_session->state],
								p_session->codec,
							    (uint32_t)(p_session->timestamp & 0x00000000FFFFFFFF),
								(int32_t)((int64_t)(snow - p_session->timestamp) & 0x00000000FFFFFFFF)));

#if (ZRTP_PLATFORM == ZP_WIN32_KERNEL)
				//if (0 == p_session->call)
				//		DbgBreakPoint();
#endif
			}
		}
	}
	
	if(!branches_count)
		ZRTP_LOG(3, (_ZTU_, "[ empty ]\n"));
	
	// Show calls list
	ZRTP_LOG(3, (_ZTU_, "calls list a_count=%d v_count=%d:\n",
				   heurist->a_calls_count,  heurist->v_calls_count));

	if(0 == (heurist->a_calls_count + heurist->v_calls_count))
	{
		ZRTP_LOG(3, (_ZTU_, "[ empty ]\n"));
	}
	else
	{
		mlist_for_each(list_node, &heurist->rtpc_head)
		{
			zfone_prob_rtp_call_t *p_call = mlist_get_struct(zfone_prob_rtp_call_t, mlist, list_node);			
			for(j=0; j<MAX_SDP_RTP_CHANNELS; j++)
			{
				zfone_hstream_t *hstream = &p_call->streams[j];
				if (ZFONE_PRTPS_STATE_PASIVE != hstream->state)
				{
					ZRTP_LOG(3, (_ZTU_, "[%s %s %u<->%u, its:%u, ots:%u, idelta:%i oidelta:%i]\n",
								zfone_media_type_names[hstream->type],
								zfone_confirm_mode_names[p_call->conf_mode],
								hstream->out_stream ? zrtp_ntoh32(hstream->out_stream->media.ssrc) : 0,
								hstream->in_stream ? zrtp_ntoh32(hstream->in_stream->media.ssrc) : 0,								
								(uint32_t)(p_call->in_touch & 0x00000000FFFFFFFF),
								(uint32_t)(p_call->out_touch & 0x00000000FFFFFFFF),
								(int32_t)((int64_t)(snow - p_call->in_touch) & 0x00000000FFFFFFFF ),
								(int32_t)((int64_t)(snow - p_call->out_touch) & 0x00000000FFFFFFFF )));
				}
			}
		}
	}
	
	// Show SIP sessions list
	branches_count = 0;
	ZRTP_LOG(3, (_ZTU_, "SIP list:\n"));
	mpoll_for_each(poll_node, &heurist->sips_poll)
	{
		zfone_prob_sips_t *psips =  mpoll_get_struct(zfone_prob_sips_t, _mpoll, poll_node);
		char buff[SIP_SESSION_ID_SIZE*3];
		ZRTP_LOG(3, (_ZTU_, "[SIP session %p:%p ID:%s ts=%d]\n",
						psips, psips->sips,
						psips->sips ? hex2str((void*)psips->sips->id, SIP_SESSION_ID_SIZE, buff, sizeof(buff)): "NULL!",
						(psips->created_at & 0x00000000FFFFFFFF)));
		branches_count++;
	}
	
	if(!branches_count)
		ZRTP_LOG(3, (_ZTU_, "[ empty ]\n"));
	
	zrtp_print_log_delim(3, LOG_END_SUBSECTION, "end");	
}



//=============================================================================
//  Heurist Interface
//=============================================================================



//-----------------------------------------------------------------------------
static int create(struct zfone_heurist* self)
{
    if (!self->_is_initiated)
    {
		int i = 0;
		_zfone_init_payloads_list();

		for (i=0; i<ZFONE_PORTS_SPACE_SIZE; i++)
			init_mlist(&self->rtps_map[i].head);

		init_mlist(&self->bye_head);
		init_mlist(&self->rtpc_head);
		init_mpoll(&self->rtps_poll);
        init_mpoll(&self->sips_poll);
		init_mpoll(&self->info_poll);

		zrtp_mutex_init(&self->protector);
        self->_is_initiated = 1;		
	}
    
    return 0;
}

//-----------------------------------------------------------------------------
static void clear(struct zfone_heurist* self, uint8_t with_notif)
{
	int i = 0;
	mlist_t *node = NULL, *tmp = NULL;

	zrtp_mutex_lock(self->protector);

	mlist_for_each_safe(node, tmp, &self->rtpc_head)
	{
		zfone_prob_rtp_call_t* rc = mlist_get_struct(zfone_prob_rtp_call_t, mlist, node);
		if (with_notif)
		{
			zfone_hresult_t res;
			zrtp_memset(&res, 0, sizeof(res));
			res.zfone_session = rc->zfone_session;
			res.hstream = NULL;
			zfone_heurist_action_handler(&res, ZFONE_HEURIST_ACTION_CLOSE);
		}
		mlist_del(node);
		zrtp_sys_free(rc);
	}
	init_mlist(&self->rtpc_head);

	mlist_for_each_safe(node, tmp, &self->bye_head)
	{
		zfone_heurist_bye_t* bye = mlist_get_struct(zfone_heurist_bye_t, _mlist, node);
		mlist_del(node);
		zrtp_sys_free(bye);
	}
	init_mlist(&self->bye_head);

	zrtp_memset(self->rtps_map, 0, sizeof(self->rtps_map));
	for (i=0; i<ZFONE_PORTS_SPACE_SIZE; i++)	
		init_mlist(&self->rtps_map[i].head);

	done_mpoll(&self->rtps_poll, zfone_prob_rtps_t, _mpoll);
	done_mpoll(&self->sips_poll, zfone_prob_sips_t, _mpoll);
	done_mpoll(&self->info_poll, zfone_rtpcheck_info_t, _mpoll);

	zrtp_mutex_unlock(self->protector);
}

static void destroy(struct zfone_heurist* self)
{    
	if (self->_is_initiated)
	{
		clear(self, 0);
		zrtp_mutex_destroy(self->protector);
		self->_is_initiated = 0;
	}
}

//-----------------------------------------------------------------------------
void reset(struct zfone_heurist *self)
{
	clear(self, 1);
}

//------------------------------------------------------------------------------
extern int	update_sip_session(struct zfone_heurist* self, zfone_sip_session_t* sips);
extern int  create_sip_session(struct zfone_heurist* self, zfone_sip_session_t* sips);
extern int	destroy_sip_session(struct zfone_heurist* self, zfone_sip_session_t* sips);
extern int is_sip_session_busy(struct zfone_heurist* self, zfone_sip_session_t* sips);

extern void clean_dead_streams(struct zfone_heurist *heurist);
extern void  clean_closed_calls(struct zfone_heurist *heurist);
extern zfone_heurist_status_t deep_analyse( struct zfone_heurist* self,
											zfone_packet_t* packet,
											zrtp_voip_proto_t proto );
extern zfone_heurist_status_t fast_analyse( struct zfone_heurist* self,
											zfone_packet_t* packet,
											zrtp_voip_proto_t proto);

int zfone_heurist_ctor(struct zfone_heurist* heurist)
{
	zrtp_memset(heurist, 0, sizeof(struct zfone_heurist));
	
    heurist->create 				= create;
    heurist->destroy 				= destroy;	
	heurist->show_state				= show_state;
	heurist->fast_analyse			= fast_analyse;
	heurist->deep_analyse			= deep_analyse;
	heurist->clean_closed_calls		= clean_closed_calls;
	heurist->clean_dead_streams		= clean_dead_streams;
    heurist->create_sip_session		= create_sip_session;
    heurist->destroy_sip_session	= destroy_sip_session;
	heurist->update_sip_session		= update_sip_session;
	heurist->is_sip_session_busy	= is_sip_session_busy;
	heurist->reset					= reset;
    
    return 0;
}
