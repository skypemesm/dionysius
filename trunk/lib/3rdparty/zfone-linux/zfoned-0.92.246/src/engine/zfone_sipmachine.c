/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>

#include "zfone.h"


#define _ZTU_ "zfone sipmachine"

//==============================================================================
//     SIP state-macine utility declarations
//==============================================================================


/*!
 * \brief return ready for use SIP session structure
 * SIP manager keep pool of the sip connections. You can get new one with
 * this function call and free connection with down_sip_connection() without
 * extra memory allocation. If there are no free connections in the pool - the new
 * one wil be created. 
 * \warning use  down_sip_connection() for releasing connections
 * \return
 *	- pointer to the ready for use connection
 *	- or NULL in case of error (not enoth memory etc.)
 */
static zfone_sip_session_t* get_sip_session(struct zfone_siprocessor* siproc);

/*!
 * \brief finds SIP session with specifed session-Id
 * \return
 *	- pointer to the found connection if success
 *	- NULL if connection with such session-Id doesn't exist
 */
static zfone_sip_session_t* find_sip_session( struct zfone_siprocessor* siproc,
											  const zfone_sip_id_t session_id );


/*!
 * \briegf release session and prepare to the next usage
 * \warning use this function every time on finish work with session
 */
static void down_sip_connection( struct zfone_siprocessor* siproc,
								 zfone_sip_session_t* sips,
								 uint8_t with_handler );

static void print_sessions_info(struct zfone_siprocessor* siproc);

static int copy_media_info(zfone_sip_session_t* sips, zfone_packet_t* packet);
static int print_media_info(zfone_sip_session_t* sips);

static void set_timer(zfone_sip_session_t* sips);
static void stop_timer(zfone_sip_session_t* sips);



//! type for SIP state handlers
typedef int sip_state_processor( struct zfone_siprocessor* siproc,
								 zfone_packet_t* packet,
								 zfone_sip_session_t* sips_info );
sip_state_processor* sip_state_processors[ZFONE_SIP_STATE_COUNT];



//==============================================================================
//     SIP state-macine logic
//==============================================================================


//------------------------------------------------------------------------------
int zfone_sip_process(struct zfone_siprocessor* siproc, zfone_packet_t* packet)
{
    int status = 0;
    zfone_sip_session_t* sips = NULL;
    zfone_sip_message_t* sipmsg = NULL;

    if (!siproc || !packet)
    {
		ZRTP_LOG(1, (_ZTU_, "zfone siprocessor process: wrong params!\n"));
		return -1;
    }
    
    sipmsg = (zfone_sip_message_t*) packet->extra_data;
    if (!sipmsg)
    {
		ZRTP_LOG(1, (_ZTU_, "zfone siprocessor process: SIP message structure is NULL!\n"));
		return -1;
    }
    
	// this is for test purpose only
	if ( sipmsg->body.is_zrtp_present && 0 )
	{
		zrtp_print_log_delim(3, LOG_LABEL, "ZRTP TAG WAS DETECTED");		
		return -3; // any other value, which indicates zrtp
	}

	// try to find SIP session which this packet belong to
    sips = find_sip_session(siproc, sipmsg->session_id);

	// Some voice mail machines doesn't sympathize to our HELLO packets.
	// So if SIParser has detected voicemail we don't start MZRTP engine.
	if ( sips && sipmsg->start_line.is_voicemail )
	{
		// todo: may be add some extra state...?
		zrtp_print_log_delim(3, LOG_LABEL, "VOICE MAIL WAS DETECTED");
		down_sip_connection(siproc, sips, 1);
		print_sessions_info(siproc);
		return status;
	}

    if (!sips)
    {
		// It's NEW INVITE packet with new session-ID - create one more session from any state
		if (ZFONE_SIP_METHOD_INVITE == sipmsg->start_line.method)
		{		
	  		sips = get_sip_session(siproc);
			if (!sips)
			{
				ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: Can't allocate memory for one more SIP session.\n"));
				return -1;
			}

	  		zrtp_memcpy(&sips->id, &sipmsg->session_id, SIP_SESSION_ID_SIZE);
			
	  		sips->direction = packet->direction;
	  		if (ZFONE_IO_IN == sips->direction)
	  		{
				zrtp_memcpy(&sips->local_name, &sipmsg->to, sizeof(zfone_sip_contact));
				zrtp_memcpy(&sips->remote_name, &sipmsg->from, sizeof(zfone_sip_contact));
	  		}
	  		else
	  		{
				zrtp_memcpy(&sips->local_name, &sipmsg->from, sizeof(zfone_sip_contact));
				zrtp_memcpy(&sips->remote_name, &sipmsg->to, sizeof(zfone_sip_contact));
	  		}
			
			sips->createdat = zrtp_time_now();
	    
			// debug_only
	  		{
			char id_buff[SIP_SESSION_ID_SIZE*2+5];	        
			ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: New %s session was detected ID:%s\n.",
						   zfone_direction_names[sips->direction], ZFONE_PRINT_SIPID(&sips->id, id_buff)));						   
			ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: Local name:<%s> <%s>\n",
						   &sips->local_name.name, &sips->local_name.uri));
			ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: Remote name:<%s> <%s>\n",
						   &sips->remote_name.name, &sips->remote_name.uri));
	  		}
		}
		else
		{
	  		// Skip all other packets outside of established sessions	    
    		return 0;
		}
    }

	// Extract local SIP User Agent type from outgoing packets
    if ( ZFONE_IO_OUT == packet->direction )  
    {
		if (ZFONE_AGENT_UNKN == sips->local_agent)
			sips->local_agent = sipmsg->user_agent;
		
		if (sipmsg->body.extra)
			sips->out_extra = sipmsg->body.extra;
    }
	else if (ZFONE_IO_IN == packet->direction)
    {
		if  (ZFONE_AGENT_UNKN == sips->remote_agent)
			sips->remote_agent = sipmsg->user_agent;

		if (sipmsg->body.extra)
			sips->in_extra = sipmsg->body.extra;
    }

    if (ZFONE_SIP_METHOD_INVITE == sipmsg->start_line.method)
    {
		uint32_t is_reinvite = 0;
    
		if (sips->direction != packet->direction)
		{
	  		// It must be reINVITE from different side - change direction and then remember new Cseq
	  		ZRTP_LOG(3, (_ZTU_, "It's reinvite from other side. Change"
							" direction from %s to %s. established = %d\n",
							zfone_direction_names[sips->direction],
							zfone_direction_names[packet->direction], sips->established));

	  		sips->direction = packet->direction;
		}
		else
		{
	  		if (sipmsg->cseq.seq <= sips->last_seq) is_reinvite = 1;
		}
    
		// Processing INVITE at every state using the same cheme:
		// update all possible media-streams which will be used as current after
		// confirmation by ACK
		if ( !is_reinvite ) // don't process lated INVITEs
		{
			if (0 > copy_media_info(sips, packet))
		    {
				ZRTP_LOG(1, (_ZTU_, "zfone sipmachine: can't copy media streams"
											   " from INVITE packet - session wil be closed.\n"));
			
				down_sip_connection(siproc, sips, 1);
				return -1;
		    }
		 
			sips->state 	= ZFONE_SIP_STATE_EARLY;
		    sips->last_seq 	= sipmsg->cseq.seq;
		    set_timer(sips); // set timer for INVITE commitment with 200 OK
  	    
		    ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: %s RE_INVITE Cseq= %d start\n"
						   " session handling from state EARLY.",
						   zfone_direction_names[packet->direction], sips->last_seq));
		    print_media_info(sips);
		}
		else
		{
		    ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: it's the previous"
										   " INVITE resending - skip processing.\n"));
		}
    }
    else if (ZFONE_SIP_METHOD_BYE == sipmsg->start_line.method)
    {
		char buff[SIP_SESSION_ID_SIZE*2+2];
		// close SIP session if we got BYE at every state.
		ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: %s BYE - CLOSE session with ID=%s.\n",
					   zfone_direction_names[packet->direction],
					   ZFONE_PRINT_SIPID(&sips->id, buff)));
	
		down_sip_connection(siproc, sips, 1);
		print_sessions_info(siproc);

    }
    else
    {
		// other packet handled according to session STATE   
		status = sip_state_processors[sips->state](siproc, packet, sips);
    }
    
    return status;
}

//------------------------------------------------------------------------------
int process_in_state_down( struct zfone_siprocessor* siproc,
						   zfone_packet_t* packet,
						   zfone_sip_session_t* sips_info )
{
    return 0;
}

//------------------------------------------------------------------------------
static int enter_established_state( struct zfone_siprocessor* siproc,
								  zfone_sip_session_t* sips_info );

int process_in_state_early( struct zfone_siprocessor* siproc,
						    zfone_packet_t* packet,
							zfone_sip_session_t* sips_info )
{
    zfone_sip_message_t* sip_message = (zfone_sip_message_t*) packet->extra_data;

    // processing RESPONSEs
    if (ZFONE_SIP_RESPONSE == sip_message->start_line.type)
    {
		if (200 < sip_message->start_line.status_code) // cancel session establishment
		{
			// close session on 3,4,5,6 responses to the last INVITE
			if ( (sips_info->last_seq == sip_message->cseq.seq) &&
				 (sip_message->cseq.name == ZFONE_SIP_METHOD_INVITE))
			{	
				// debug_only
				{
				static char buff[SIP_SESSION_ID_SIZE*2+2];
				ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: %s ERROR response -"
								" terminate session with ID=%s.\n",
							    zfone_direction_names[packet->direction],
							    ZFONE_PRINT_SIPID(sips_info->id, buff)));
				}
				down_sip_connection(siproc, sips_info, 1);				
			}
		}
		else if (200 == sip_message->start_line.status_code)
		{
			// cswitching to CONFIRMED state on 200 OK to last INVITE
			if ( (sips_info->last_seq == sip_message->cseq.seq) &&
				 (sip_message->cseq.name == ZFONE_SIP_METHOD_INVITE) )
			{
				if (0 > copy_media_info(sips_info, packet))
				{
					ZRTP_LOG(1, (_ZTU_, "zfone sipmachine: can't copy media"
												   " stream information from OK message.\n"));
					down_sip_connection(siproc, sips_info, 1);
					return -1;
				}

				ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: 200 OK %s entering ESTABLISHED"
							    " state.\n", zfone_direction_names[sip_message->packet->direction]));
				print_media_info(sips_info);

				enter_established_state(siproc, sips_info);
			}
		}
	}
	// processing REQUESTs
	else
	{
		// terminate unestablished session
		if ( (ZFONE_SIP_METHOD_CANCEL == sip_message->start_line.method) &&
			 (sips_info->last_seq == sip_message->cseq.seq))
		{
			// debug_only
			char buf[SIP_SESSION_ID_SIZE*2+2];
			ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: %s CANCELE packet"
							" - terminate session with ID=%s.\n",
							zfone_direction_names[packet->direction],
							ZFONE_PRINT_SIPID(sips_info->id, buf)));
			
			// notice engine about changes and then release session
			down_sip_connection(siproc, sips_info, 1);
		}
    }

    return 0;
}

//------------------------------------------------------------------------------
static int enter_established_state( struct zfone_siprocessor* siproc,								  
								    zfone_sip_session_t* sips_info )
{
	unsigned int i = 0;	
    zfone_media_stream_t* stream = NULL;
	char buf[SIP_SESSION_ID_SIZE*2+2];

	sips_info->state = ZFONE_SIP_STATE_ESTABL;
	set_timer(sips_info); // start timer for ACK receiving

	// check media session. IP's matching laready checked during media streams comping.
	// Now we will check streams coherenity:
	//		- "closed" streams must be closed at both sides;
	//		- streams numbers at response and request must be the same
	// 		- re-INVITE can't decrease streams number
	//
	// (RFC 3264 par 8)
	// " If an SDP is offered, which is different from the previous SDP, the
	// new SDP MUST have a matching media stream for each media stream in
	// the previous SDP.  In other words, if the previous SDP had N "m="
	// lines, the new SDP MUST have at least N "m=" lines.  The i-th media
	// stream in the previous SDP, counting from the top, matches the i-th
	// media stream in the new SDP, counting from the top.  This matching is
	// necessary in order for the answerer to determine which stream in the
	// new SDP corresponds to a stream in the previous SDP.  Because of
	// these requirements, the number of "m=" lines in a stream never
	// decreases, but either stays the same or increases.  Deleted media
	// streams from a previous SDP MUST NOT be removed in a new SDP;
	// however, attributes for these streams need not be present."
	for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
	{
		stream = &sips_info->new_streams[i];
		if  (stream->type != ZFONE_SDP_MEDIA_TYPE_UNKN)
		{
			if ( (!stream->local_rtp || !stream->remote_rtp) &&
				 (stream->local_rtp != stream->remote_rtp) )
			{
				stream->local_rtp = 0;
				stream->remote_rtp = 0;					
			}
			else
			{
				sips_info->new_count++;
			}

			if ( sips_info->established && (!stream->remote_ip || !stream->local_ip) )
			{
				stream->dir_attr = ZFONE_SDP_DIR_ATTR_INACTIVE;				
				ZRTP_LOG(3, (_ZTU_, "Mark %s stream as inactive because"
								" of zero ip (remote=%x local=%x)\n",
							    zfone_media_type_names[stream->type],
							    stream->remote_ip, stream->local_ip));
			}
		}
	}
		
	if (sips_info->new_count < sips_info->conf_count) // see comments (RFC 3264 par 8)
	{
		// debug_only
		ZRTP_LOG(1, (_ZTU_, "zfone sipmacine: re-INVITE can't decrease streams number.\n"));
		ZRTP_LOG(1, (_ZTU_, "zfone sipmacine: Such session %s. will be closed.\n",
					   i, hex2str((const char*)&sips_info->id, SIP_SESSION_ID_SIZE, buf, sizeof(buf))));
		
		down_sip_connection(siproc, sips_info, 1);
	}
   
	// debug_only
	{
		char date[32];
		char start_msg[128];
		memset(date, 0, sizeof(date));    
		zrtp_print_log_delim(3, LOG_SPACE, "");
		zrtp_print_log_delim(3, LOG_SPACE, "");
#if ZRTP_PLATFORM == ZP_WIN32_KERNEL			
		RtlStringCchPrintfA( start_msg, sizeof(start_msg), " ZFONED DETECTED NEW CALL at %s",
							 zrtph_get_time_str(date, (int)sizeof(date)));
#else 
		snprintf( start_msg, sizeof(start_msg), " ZFONED DETECTED NEW CALL at %s",
				 zrtph_get_time_str(date, (int)sizeof(date)) );
#endif
		zrtp_print_log_delim(3, LOG_LABEL, start_msg);
		print_sessions_info(siproc);
	}
	
	zfone_sip_action_handler( sips_info,
							 !sips_info->established ? ZFONED_SIPS_CREATE : ZFONED_SIPS_UPDATE );

	// save media-streams information as confirmed
	sips_info->established = 1;				
	sips_info->conf_count = sips_info->new_count;
	zrtp_memcpy( sips_info->conf_streams,
				 sips_info->new_streams,
				 MAX_SDP_RTP_CHANNELS*sizeof(zfone_media_stream_t));
	sips_info->new_count = 0;
	zrtp_memset( sips_info->new_streams,
				 0,
				 MAX_SDP_RTP_CHANNELS*sizeof(zfone_media_stream_t) );

	return 0;
}

//------------------------------------------------------------------------------
int process_in_state_establ( struct zfone_siprocessor* siproc,
							 zfone_packet_t* packet,
							 zfone_sip_session_t* sips_info )
{
	zfone_sip_message_t* sip_message = (zfone_sip_message_t*) packet->extra_data;    

    if ( (ZFONE_SIP_REQUEST == sip_message->start_line.type) && 
		 (ZFONE_SIP_METHOD_ACK == sip_message->start_line.method) )
    {		
		if (sips_info->last_seq == sip_message->cseq.seq)
		{
			stop_timer(sips_info); // Full SIP negotiation have finished - stop all timers
			sips_info->state = ZFONE_SIP_STATE_CONFIRM;

			ZRTP_LOG(3, (_ZTU_, "zfone sipmacine: ACK %s entering CONFIRMED state.\n",
							zfone_direction_names[sip_message->packet->direction]));				
		}
    }

    return 0;
}

//------------------------------------------------------------------------------
int process_in_state_confirm( struct zfone_siprocessor* siproc,
							  zfone_packet_t* packet,
							  zfone_sip_session_t* sips_info )
{
    //zfone_sip_message_t* sip_message = (zfone_sip_message_t*) packet->extra_data;
    return 0;
}


//==============================================================================
//     SIP state-macine utility part
//==============================================================================


//! prepare sip session structure for use
static void init_clear_sips(zfone_sip_session_t* sips)
{        
    memset(sips, 0, sizeof(zfone_sip_session_t));
    sips->local_agent = ZFONE_AGENT_UNKN;        
}

//------------------------------------------------------------------------------
static zfone_sip_session_t* get_sip_session(struct zfone_siprocessor* siproc)
{
    zfone_sip_session_t* new_session = NULL;
    mlist_t* node = NULL;
    mlist_for_each(node, &siproc->sip_head)
    {
		zfone_sip_session_t* tmp_sips = mlist_get_struct(zfone_sip_session_t, mlist, node);    
		if ( !tmp_sips->is_active )
		{
			new_session = tmp_sips;
			break;
		}
    }
        
    if ( !new_session )
    {
		// there are no free connectuion - allocate new one and add to the pool
		new_session = zrtp_sys_alloc(sizeof(zfone_sip_session_t));
		if (!new_session)
		{
			ZRTP_LOG(1, (_ZTU_, "zfone sipmachine: Can't allocate memory"
										   " for new sip connection.\n"));
			return NULL;
		}
		memset(new_session, 0, sizeof(zfone_sip_session_t));
		mlist_add(&siproc->sip_head, &new_session->mlist);
    }
    
    // prepare session structure to use
    {
		mlist_t tmp_mlist;
		zrtp_memcpy(&tmp_mlist, &new_session->mlist, sizeof(mlist_t) );
		init_clear_sips(new_session);
		zrtp_memcpy(&new_session->mlist, &tmp_mlist, sizeof(mlist_t) );
		new_session->is_active = 1;
    }
    
    return new_session;
}

//------------------------------------------------------------------------------
static zfone_sip_session_t* find_sip_session( struct zfone_siprocessor* siproc,
											  const zfone_sip_id_t session_id )
{
    zfone_sip_session_t* curr_sips = NULL;
    mlist_t *node = NULL, *tmp = NULL;
    
    mlist_for_each_safe(node, tmp, &siproc->sip_head)
	{
		zfone_sip_session_t* tmp_sips = mlist_get_struct(zfone_sip_session_t, mlist, node);

		// Removing unused SIP sessions by timeout
		if (tmp_sips->is_active && (tmp_sips->timestamp != SIP_TIMESTAMP_ESTABL))
		{		
			uint8_t is_expired = 0;
			uint32_t delta = 0;
			delta = (zrtp_time_now()*1000 - tmp_sips->timestamp);			

			switch (tmp_sips->state)
			{
			case ZFONE_SIP_STATE_EARLY:
				is_expired = (delta > SIP_TIMEOUT_PROOT_T1); 
				break;

			case ZFONE_SIP_STATE_ESTABL:
				is_expired = (delta > SIP_TIMEOUT_CLIENT_T2);
				if (is_expired)
					is_expired = (0 != heurist.is_sip_session_busy(&heurist, tmp_sips));
				break;

			case ZFONE_SIP_STATE_CONFIRM:
				is_expired = (delta > SIP_TIMEOUT_UNUSED_T3);
				if (is_expired)
					is_expired = (0 != heurist.is_sip_session_busy(&heurist, tmp_sips));
				break;

			default:
				break;
			}

			if (is_expired)
			{
				char buf[SIP_SESSION_ID_SIZE*2+2];
				ZRTP_LOG(3, (_ZTU_, "zfone sipmachine: closing session %s by"
												" timeout in state %d.\n",
							    ZFONE_PRINT_SIPID(&tmp_sips->id, buf), tmp_sips->state));
				
				down_sip_connection(siproc, tmp_sips, 1); // close session by timeout
			}
		}
		
		if (tmp_sips->is_active && !zfone_sipidcmp(tmp_sips->id, session_id))
		{
			curr_sips = tmp_sips;
			// we don't break there because we want to go over all sessions
		}
    }
        
    return curr_sips;
}

//------------------------------------------------------------------------------
static void down_sip_connection( struct zfone_siprocessor* siproc,
								 zfone_sip_session_t* sips,
								 uint8_t with_handler )
{
	if (with_handler)
		zfone_sip_action_handler(sips, ZFONED_SIPS_CLOSE);										

	sips->state = ZFONE_SIP_STATE_DOWN;
    sips->is_active = 0;
    sips->established = 0;
    stop_timer(sips);
}

//------------------------------------------------------------------------------
static int copy_media_info(zfone_sip_session_t* sips, zfone_packet_t* packet)
{
    uint32_t i,j,k = 0;
    zfone_sip_message_t* sipmsg = (zfone_sip_message_t*) packet->extra_data;
    
    if (!sips || !sipmsg)
		return -1; // wrong params
    
    if (ZFONE_SIP_BODY_SDP != sipmsg->ctype)
		return  0; // just skip packets without SDP inside
    
    if (0 != sipmsg->body.contact.range)
    {
		ZRTP_LOG(2, (_ZTU_,"zfone sipmacine: MULTICAST STREAMS DIDN'T"
									  " SUPPORTED IN CURRENT VERSION\n"));
		// TODO: add multycast streams support
		return -1;
    }
    
    // copy all media streams to the session structure
    j = i = k = 0;
    while (i<MAX_SDP_RTP_CHANNELS)
    {
		if (ZFONE_SDP_MEDIA_TYPE_UNKN != sipmsg->body.streams[k].type ) // skip all unsupported types
		{
			if (sipmsg->body.streams[k].rtp_ports.range+i > MAX_SDP_RTP_CHANNELS)
			{
				ZRTP_LOG(1, (_ZTU_, "zfone sipmachine: too many media streams"
							    " %d but only %d supported by current version.\n",
								sipmsg->body.streams[k].rtp_ports.range+i, MAX_SDP_RTP_CHANNELS));
				return -1;
			}	    
		    			
			// todo: simplify this. Now we don't care about direction. Heurist is looking for RTP on both ports
			if (ZFONE_IO_OUT == sipmsg->packet->direction)
			{
				j = 0;
				// if multycast ports format - create several streams    
				while (j < sipmsg->body.streams[i].rtp_ports.range || !j)
				{
					sips->new_streams[i+j].type = sipmsg->body.streams[k].type;					
					sips->new_streams[i+j].local_rtp = sipmsg->body.streams[k].rtp_ports.base + (j?j+1:j);
					sips->new_streams[i+j].local_rtcp = sips->new_streams[i+j].local_rtp+1;
					sips->new_streams[i+j].dir_attr = sipmsg->body.streams[k].dir_attr;

					if (!sips->is_ice_enabled && sipmsg->body.streams[k].ice_port)
						sips->is_ice_enabled = 1;

					if ( sipmsg->body.streams[k].extra ) 
					{						
						//sips->new_streams[i+j].o_x_ssrc = sipmsg->body.streams[k].extra;
						// todo: temporary measure, local_agent has to be parsed from sip
						if ( sips->local_agent == ZFONE_AGENT_UNKN )
						{
							sips->local_agent = ZFONE_AGENT_TIVI;
						}
					}
					j++;
				}
			}
			else
			{
				j = 0;
				// if multicast ports format - create several streams    
				while (j < sipmsg->body.streams[i].rtp_ports.range || !j)
				{
					sips->new_streams[i+j].type = sipmsg->body.streams[k].type;
					sips->new_streams[i+j].remote_ip = sipmsg->body.contact.base ?
													   sipmsg->body.contact.base :
													   sipmsg->body.streams[k].contact.base;
					sips->new_streams[i+j].remote_rtp = sipmsg->body.streams[k].rtp_ports.base + (j?j+1:j);
					sips->new_streams[i+j].remote_rtcp = sips->new_streams[i+j].remote_rtp+1;
					sips->new_streams[i+j].dir_attr = sipmsg->body.streams[k].dir_attr;

					if (!sips->is_ice_enabled && sipmsg->body.streams[k].ice_port)
						sips->is_ice_enabled = 1;					
				
					if ( sipmsg->body.streams[k].extra ) 
					{		
						// todo: restore this for tivi support
						//sips->new_streams[i+j].i_x_ssrc = sipmsg->body.streams[k].extra;
						// todo: temporary measure, remote_agent has to be parsed from sip
						if ( sips->remote_agent == ZFONE_AGENT_UNKN )
						{
							sips->remote_agent = ZFONE_AGENT_TIVI;
						}
					}
					j++;

				}
#if 0
				if ( sipmsg->body.streams[k].ice_ip && sipmsg->body.streams[k].ice_port )
				{
					sips->new_streams[i+j].type = sipmsg->body.streams[k].type;
					sips->new_streams[i+j].remote_ip = sipmsg->body.streams[k].ice_ip;
					sips->new_streams[i+j].remote_rtp = sipmsg->body.streams[k].ice_port;
					sips->new_streams[i+j].remote_rtcp = sips->new_streams[i+j].remote_rtp+1;
					// extra is used for tivi, which doesnt has alt attribute in sdp, so for now we just skip tivi check here in order to simplify testing
					ZRTP_LOG(3, (_ZTU_, "Additional media stream was created with ip %x and port %d\n", 
								  sips->new_streams[i+j].remote_ip, sips->new_streams[i+j].remote_rtp));
					j++;
				}
#endif
			}	    
			i+=j;
		}
		else
		{
			sips->new_streams[i].type = ZFONE_SDP_MEDIA_TYPE_UNKN;
			i++;
		}
		k++;
    }
    
	// todo: may be we can just eliminate this and use single IP for all streams?

    // Check IP addresses in media-streams fields. IP's must be the same.
    // Current version can't process multicast messages.
    {
		// TODO: add multicast streams support		
		uint32_t last_remote_ip = 0;
		
		for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
		{
			if (ZFONE_SDP_MEDIA_TYPE_UNKN != sips->new_streams[i].type)
			{
				if ( last_remote_ip && (last_remote_ip != sips->new_streams[i].remote_ip) )
				{
					ZRTP_LOG(1, (_ZTU_, "zfone sipmachine: Unexpected media streams"
												   " configuration was detected.\n"));
					ZRTP_LOG(1, (_ZTU_, "zfone sipmachine: multicast streaming not"
												   " supported by current versuion. Session wil be closed.\n"));
					return -1;
				}				
				last_remote_ip = sips->new_streams[i].remote_ip;
			}
		}
    }
    
    return 0;
}

//------------------------------------------------------------------------------
static int print_media_info(zfone_sip_session_t* sips)
{
    int i,j = 0;
    char ip_buff1[25]; char ip_buff2[25];
        
    ZRTP_LOG(3, (_ZTU_, "New streams:\n"));
    j = 0;
    for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
    {
		if (sips->new_streams[i].type != ZFONE_SDP_MEDIA_TYPE_UNKN)
		{
			ZRTP_LOG(3, (_ZTU_, "\t%s stream No %d: Local %s:%d:%d Remote %s:%d:%d\n",
							zfone_media_type_names[sips->new_streams[i].type], i,
							zfone_ip2str(ip_buff1, 25, sips->new_streams[i].local_ip),
							sips->new_streams[i].local_rtp, sips->new_streams[i].local_rtcp,
							zfone_ip2str(ip_buff2, 25, sips->new_streams[i].remote_ip),
							sips->new_streams[i].remote_rtp, sips->new_streams[i].remote_rtcp));
			j++;
		}
    }
    if (0 == j)    
		ZRTP_LOG(3, (_ZTU_, "\tThis section have no media streams yet.\n"));    
    
    j=0;
    ZRTP_LOG(3, (_ZTU_, "Confirmed streams:\n"));
    for (i=0; i<MAX_SDP_RTP_CHANNELS; i++)
    {
		if (sips->conf_streams[i].type != ZFONE_SDP_MEDIA_TYPE_UNKN)
		{
			ZRTP_LOG(3, (_ZTU_, "\t%s stream No %d: Local %s:%d:%d Remote %s:%d:%d\n",
				(sips->conf_streams[i].type == ZFONE_SDP_MEDIA_TYPE_AUDIO) ? "Audio": "Video" , i,
				zfone_ip2str(ip_buff1, 25, sips->conf_streams[i].local_ip),
				sips->conf_streams[i].local_rtp,
				sips->conf_streams[i].local_rtcp,
				zfone_ip2str(ip_buff2, 25, sips->conf_streams[i].remote_ip),
				sips->conf_streams[i].remote_rtp,
				sips->conf_streams[i].remote_rtcp));
    			j++;	
		}
    }
    if (0 == j)
		ZRTP_LOG(3, (_ZTU_, "\tThis section have no media streams yet.\n"));    
    
    return 0;
}

//------------------------------------------------------------------------------
static void set_timer(zfone_sip_session_t* sips)
{
    sips->timestamp = zrtp_time_now()*1000;
}

static void stop_timer(zfone_sip_session_t* sips)
{
    sips->timestamp = SIP_TIMESTAMP_ESTABL;
}

//------------------------------------------------------------------------------
static void print_sessions_info(struct zfone_siprocessor* siproc)
{
    mlist_t* node = NULL;
    zfone_sip_session_t* session = NULL;
    
    zrtp_print_log_delim(3, LOG_START_SECTION, "SESSIONS LIST");
    mlist_for_each(node, &siproc->sip_head)
    {
		session = mlist_get_struct(zfone_sip_session_t, mlist, node);
		if (session->is_active)
		{
			zrtp_print_log_delim(3, LOG_START_SUBSECTION, "");
			{		
				char buf[SIP_SESSION_ID_SIZE*2+2];
				ZRTP_LOG(3, (_ZTU_, "    Session id:          %s\n",
							   hex2str((const char*)session->id, SIP_SESSION_ID_SIZE, buf, sizeof(buf))));
			}
			ZRTP_LOG(3, (_ZTU_, "    Session local name:  %s\n", session->local_name.name));
			ZRTP_LOG(3, (_ZTU_, "    Session remote name: %s\n", session->remote_name.name));
			ZRTP_LOG(3, (_ZTU_, "    Session direction:   %s\n", session->direction ? "INPUT" : "OUTPUT"));
			ZRTP_LOG(3, (_ZTU_, "    Session sequence:    %d\n", session->last_seq));
			zrtp_print_log_delim(3, LOG_END_SELECT, "");
			print_media_info(session);
			zrtp_print_log_delim(3, LOG_END_SELECT, "");
		}
    }
    zrtp_print_log_delim(3, LOG_END_SECTION, "SESSIONS LIST");
}

//-----------------------------------------------------------------------------
void  zfone_sipmacine_reset(struct zfone_siprocessor* self)
{
	mlist_t* node = NULL;
    mlist_t* tmp = NULL;
    zfone_sip_session_t* session = NULL;
    
    mlist_for_each_safe(node, tmp, &self->sip_head)
    {
		session = mlist_get_struct(zfone_sip_session_t, mlist, node);
		down_sip_connection(self, session, 0);
    }
}


//==============================================================================
//     SIP state-machine initialization/deinitialization  routine
//==============================================================================


//------------------------------------------------------------------------------
static int create(struct zfone_siprocessor* siproc, zrtp_zid_t *zid)
{    
    // Initialize SIP state-mamchine

	if (!siproc->inited)
    {
		init_mlist(&siproc->sip_head);
	  		
		sip_state_processors[ZFONE_SIP_STATE_DOWN] =  process_in_state_down;
		sip_state_processors[ZFONE_SIP_STATE_EARLY] =  process_in_state_early;
		sip_state_processors[ZFONE_SIP_STATE_ESTABL] =  process_in_state_establ;
		sip_state_processors[ZFONE_SIP_STATE_CONFIRM] =  process_in_state_confirm;		

		zrtp_memcpy(&siproc->zid, zid, sizeof(zrtp_zid_t));
		
		siproc->inited = 1;
    }

    return 0;
}

//------------------------------------------------------------------------------
static void destroy(struct zfone_siprocessor* siproc)
{
	if (siproc->inited)
    {	
		// destroy all SIP session structures
		mlist_t *node = NULL, *tmp_node = NULL;
		siproc->inited = 0;
		mlist_for_each_safe(node, tmp_node, &siproc->sip_head)		
			zrtp_sys_free(mlist_get_struct(zfone_sip_session_t, mlist, node));
		init_mlist(&siproc->sip_head);
    }
}

//------------------------------------------------------------------------------
int zfone_siproc_ctor(struct zfone_siprocessor* siproc)
{       
    memset(siproc, 0, sizeof(struct zfone_siprocessor));

    // activate methods
    siproc->create 	= create;
    siproc->destroy = destroy;
    siproc->process	= zfone_sip_process;
    siproc->refresh	= zfone_sipmacine_reset;
    
    return 0;
}

