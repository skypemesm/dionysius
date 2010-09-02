/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <pthread.h>

#include <zrtp.h>

#include "zfone.h"

#define _ZTU_ "zfone engine"

extern zrtp_zid_t				zid;			//!< local side zid, not for router versions
extern zrtp_global_t 		*zrtp_global;	//!< zrtp context for global data storing
extern zfone_cfg_error_t 		config_state;	//!< define configuration process status

//! main processing loop
static void* processing(void* param);
static int is_runing = 0;


//------------------------------------------------------------------------------    
int zfoned_start_engine()
{
    pthread_t thread;
    
    is_runing = 1;
    if (0 != pthread_create(&thread, NULL, processing, NULL))
    {
		ZRTP_LOG(1, (_ZTU_, "ZFONED engine start: can't create processing thread.\n"));
		return -1;
    }
    
	return 0;
}

void zfoned_stop_engine()
{
	is_runing = 0;
}

//------------------------------------------------------------------------------
static void _zfone_prepare_packet_for_sending(zfone_packet_t* packet)
{
	struct zrtp_iphdr *iph	= ((struct zrtp_iphdr*) packet->packet);
	int iphdr_length 		= iph->ihl * 4;
	iph->tot_len = zrtp_hton16(packet->size);

	switch (packet->proto)
	{
	case voip_proto_TCP:
		{		
		//struct zrtp_tcphdr *th = (struct zrtp_tcphdr*)(packet->packet + iphdr_length);
		} break;		
	case voip_proto_UDP:
		{
		struct zrtp_udphdr *uh = (struct zrtp_udphdr*)(packet->packet + iphdr_length);
		uh->len = zrtp_hton16(packet->size - iphdr_length);
		} break;
	default:
		break;
	}			
	
	zfone_insert_cs((char *)packet->packet, packet->size);
}

#define ZFONE_NET_TEST 0
#define ZFONE_FAST_ICHAT 1

static void* processing(void* param)
{
	// Allocate memory from the heap for the Kernel mode
	zfone_packet_t *packet = zrtp_sys_alloc(sizeof(zfone_packet_t));
    
	while (is_runing)
    {
		zrtp_voip_proto_t proto = voip_proto_UNKN;
		zrtp_status_t proc_res = zrtp_status_fail;
		int need_accept = 1;
	    int status = 0;
		uint8_t is_control = 0;

		packet->stream = NULL;
		packet->ctx = NULL;
	
		// get packet
		if ( 0 > zfone_network_get_packet(packet) )		
		    continue; // can't read packet - try one more time
	
		// prepare packet for processing: resolve direction, IPs, parse protocols etc.
		packet->external_proto = voip_proto_IP;
		if (0 == (status = zfone_manager_prepare_packet(packet)))
		{
			zfone_rtpcheck_info_t info;
			zrtp_memset(&info, 0, sizeof(info));
			packet->extra_data = (void*)&info;

		    // Define packet type: rtp, sip etc. If this is a RTP or RTCP packet - zfone session field
		    // zfone stream value will be setted
		    proto = manager.detect_proto(&manager, packet);	

		    switch (proto)
		    {
			case voip_proto_RTP:
			{
				is_control = _zrtp_packet_get_type((zrtp_rtp_hdr_t*)(packet->packet + packet->offset), packet->size) != ZRTP_NONE;				
			
				// allows RTP heuristics to close channel by timeout
				if (zfone_hs_prtp == heurist.fast_analyse(&heurist, packet, voip_proto_RTP)) {
					if (zfone_hs_forwarded == heurist.deep_analyse(&heurist, packet, voip_proto_RTP)) {
						heurist.deep_analyse(&heurist, packet, voip_proto_PRTP);
					}
				}
				
				packet->size -= packet->offset;
												
		  	    if (packet->direction == ZFONE_IO_OUT)
			    {
					proc_res = zrtp_process_rtp( packet->stream->zrtp_stream,
												 (char *)packet->packet + packet->offset,
												 &packet->size);
			    }
			    else if (!packet->stream->hear_ctx || is_control)
				{					
					/* Skip linux Zfone alert packets. */
					if (packet->stream->alert_ssrc == info.rtp_hdr->ssrc)
					{
						proc_res = zrtp_status_ok;
					}
					else
					{										
						proc_res = zrtp_process_srtp( packet->stream->zrtp_stream,
													  (char *)packet->packet + packet->offset,
													  &packet->size);
					}
			    }
				else
				{
					proc_res = zrtp_status_ok;
				}
				
			    packet->size += packet->offset;

				//WARNING!
				// Be careful not to pass plaintext packets to the network in
				// cases when rtp_process_xx return status is not the expected
				// value of zrtp_status_ok, or zrtp_status_forward. If we get a
				// bad status return, this indicates no encryption was performed. 
				// It would be disasterous to pass unencrypted packets out to the
				//  network. Read developers guide for additional information.
			    if (proc_res == zrtp_status_ok) {
					_zfone_prepare_packet_for_sending(packet);
				} else {
					need_accept = 0;
				}
				
				break;
			}

			case voip_proto_RTCP:
	  		    packet->size -= packet->offset;
			    if (packet->direction == ZFONE_IO_OUT)
				{
					proc_res = zrtp_process_rtcp( packet->stream->zrtp_stream,
												 (char *)packet->packet + packet->offset,
												 &packet->size );
					//ZRTP_LOG(3, (_ZTU_, "zfoned engine: RTCP was encrypted with status =%d.", proc_res));
				}
	  		    else
				{
					proc_res = zrtp_process_srtcp( packet->stream->zrtp_stream,
												   (char *)packet->packet + packet->offset,
												   &packet->size );
					//ZRTP_LOG(3, (_ZTU_, "zfoned engine: RTCP was decrypted with status =%d.", proc_res));
				}
			    packet->size += packet->offset;
		    
				//WARNING!
				// Be careful don't pass plaintext packets to the network in
				// cases when rtp_process_xx return status is not the expected
				// value of zrtp_status_ok, or zrtp_status_forward. If we get a
				// bad status return, this indicates no encryption was performed. 
				// It would be disasterous to pass unencrypted packets out to the
				//  network. Read developers guide for additional information.
				if (proc_res == zrtp_status_ok) {
					_zfone_prepare_packet_for_sending(packet);
				} else {
					need_accept = 0;
				}
				break;
				
			case voip_proto_SIP:
			{
			    zfone_sip_message_t sip_message;
			    memset(&sip_message, 0, sizeof(sip_message));

			    // adding SIP message to zfone-packet structure
			    packet->extra_data = (void*)&sip_message;
			    sip_message.packet = packet;
		    
				// parse SIP packet				
				status = zfone_sip_parse(packet);
				if (0 == status)
				{
					// parsing was finished successfully - put packet to SIP state-machine
					status = siproc.process(&siproc, packet);
				}
				// -1 - small packet, -2 no status-line and -3 - not SIP. Some times happens :)
				else if (status < -3)
	  	    	{
					ZRTP_LOG(1, (_ZTU_, "ZFONED main loop: error %d during SIP parsing - skip it.", status));
				}
				
				
				proto = voip_proto_UNKN;
			} //break; It happens when VoIP client uses the same UDP port for SIP and RTP.
			  //       We allow ZFone to apply heuristics to SIP packets as well. ZFone is
			  //       smart enough to distinguish SIP and RTP but at other hand it should
			  //       prevent from losing the call in case of SIP ports reusing.
			
			case voip_proto_UNKN:
			{
			    if (zfone_hs_prtp == heurist.fast_analyse(&heurist, packet, voip_proto_UNKN))
					heurist.deep_analyse(&heurist, packet, voip_proto_PRTP);
				break;
			}
			
			case voip_proto_ERROR:
				break;
			
			default:
				break;
		    } // switch(packet type)
		} // accepted by detector

		
#if ZFONE_FAST_ICHAT
		// It's unsafe to allow sending of the plain media in transition states
		// But it should prevent iChat from closing the call during switching to
		// SECURE (when there is no media going).
		
		if ( (zrtp_status_drop == proc_res) &&
			 ( ((voip_proto_RTP == proto) && !is_control) || (voip_proto_RTCP == proto) ) )
		{
			zfone_media_stream_t *media = (packet->stream->in_media.type != ZFONE_SDP_MEDIA_TYPE_UNKN) ?
										  &packet->stream->in_media : &packet->stream->out_media;			
			
			if  ( media->hstream &&
				 ((
					media->hstream->in_stream &&
					media->hstream->in_stream->psips &&
					(media->hstream->in_stream->psips->sips->local_agent == ZFONE_AGENT_ICHAT || 
					 media->hstream->in_stream->psips->sips->remote_agent == ZFONE_AGENT_ICHAT)
				  ) ||
				  (
					media->hstream->out_stream &&
					media->hstream->out_stream->psips &&
					(media->hstream->out_stream->psips->sips->local_agent == ZFONE_AGENT_ICHAT ||
					 media->hstream->out_stream->psips->sips->remote_agent == ZFONE_AGENT_ICHAT )
				  )) )
			{
				switch (packet->stream->zrtp_stream->state)
				{
				case ZRTP_STATE_START_INITIATINGSECURE:
				case ZRTP_STATE_INITIATINGSECURE:
				case ZRTP_STATE_PENDINGSECURE:
				case ZRTP_STATE_WAIT_CONFIRM2:
				//case ZRTP_STATE_INITIATINGCLEAR: // this may be a securiti issue. let's better crash iChat rather than send plain media
				//case ZRTP_STATE_PENDINGCLEAR:
					need_accept = 1;					
					break;
				default:
					break;
				}				
			}
		} // skip dropped by libzrtp iChat media
#endif

		// register RTP activity
		if (voip_proto_RTP == proto)		
			zfone_checker_reg_rtp(packet, packet->direction, proc_res != zrtp_status_drop);
			
		// Feed libzrtp pseudo RNG with dropping outgoing RTP media. It is good
		// entropy sources unaccessable for MiTM because we drop them

		if (!need_accept && !is_control && (voip_proto_RTP == proto) && (packet->direction == ZFONE_IO_OUT))
		{
			zrtp_entropy_add( zrtp_global, 
							  (const unsigned char*)packet->packet + packet->offset,
							  packet->size - packet->offset );
			//ZRTP_LOG(3, (_ZTU_, "ZFONED engine: %d pytes of droipped RTP media was feeded to RNG.", packet->size - packet->offset));
		}

		// apply verdict and return packet back to network or just drop it		
	  	zfone_network_put_packet(packet, need_accept);
    }// main while() processing loop

	zrtp_sys_free(packet);
    return NULL;
}

