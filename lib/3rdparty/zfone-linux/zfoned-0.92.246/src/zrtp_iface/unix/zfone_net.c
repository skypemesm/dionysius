/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>
#include "zfone.h"


//------------------------------------------------------------------------------
int zrtp_send_rtp_callback( const zrtp_stream_t* ctx,
				   char* rtp_packet,
				   unsigned int rtp_packet_length )
{
    char* ip_packet;	
    uint32_t ip_packet_len = rtp_packet_length + 28;
    struct zrtp_iphdr* ipHdr = NULL;
    struct zrtp_udphdr* udpHdr = NULL;    

    zfone_stream_t* stream = (zfone_stream_t*) ctx->usr_data;	
	zfone_media_stream_t *media = (stream->out_media.type != ZFONE_SDP_MEDIA_TYPE_UNKN) ?
								   &stream->out_media : &stream->in_media;    
    
	ip_packet = zrtp_sys_alloc(1500);
	
    zrtp_memset(ip_packet, 0, ip_packet_len);
    zrtp_memcpy(ip_packet+28, rtp_packet, rtp_packet_length);
    
    // Fill IP header
    ipHdr 			= (struct zrtp_iphdr*) ip_packet;
    ipHdr->ihl 		= 5;
    ipHdr->version 	= 4;
    ipHdr->ttl 		= 64;
    ipHdr->protocol = 17;
    ipHdr->saddr	= media->local_ip;
    ipHdr->daddr 	= media->remote_ip;
    ipHdr->tot_len 	= zrtp_hton16(ip_packet_len);
    
    // Fill UDP header
    udpHdr 			= (struct zrtp_udphdr*) (ip_packet + 20);
    udpHdr->source 	= media->local_rtp;
    udpHdr->dest 	= media->remote_rtp;
    udpHdr->len 	= zrtp_hton16(ip_packet_len-20);
    
    // Recalculate checksumm after manipulations
    zfone_insert_cs(ip_packet, ip_packet_len);
    
    // Send packet
	zfone_network_send_ip(ip_packet, ip_packet_len, media->remote_ip);
    
	zrtp_sys_free(ip_packet);
    return 0;
}

