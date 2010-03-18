/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */


#include <zrtp.h>

#include "zfone.h"

#define _ZTU_ "zfone detector"

// We use this function to extract SIP packet from UDP/TCP traffic
extern zfone_sip_type_t parse_start_line(const char *buff, uint32_t depth, zfone_start_line_t* start_line);


//=============================================================================
//  (CONST) Interfaces management part
//=============================================================================

//------------------------------------------------------------------------------
uint32_t 	interfaces[ZFONE_MAX_INTERFACES_COUNT];
uint8_t 	flags[ZFONE_MAX_INTERFACES_COUNT];
uint32_t 	interfaces_count;

int zfone_manager_initialize_ip_list()
{	
	interfaces_count = 0;
	zrtp_memset(&interfaces, 0, sizeof(interfaces));
	zrtp_memset(&flags, 0, sizeof(flags));

	return 0;
}

//------------------------------------------------------------------------------
int zfone_manager_add_ip(uint32_t ip, int is_static)
{
    uint32_t nip, i;

    if (interfaces_count == ZFONE_MAX_INTERFACES_COUNT)
    {		
		ZRTP_LOG(1, (_ZTU_, "manager ip_add: interfaces limit %d reached.\n", ZFONE_MAX_INTERFACES_COUNT));
		return -1;
    }
    
    nip = zrtp_hton32(ip);
	// May be interface is already in the list
    for (i = 0; i < interfaces_count; i++)
    {
		if ( interfaces[i] == nip )
		{
			flags[i] = is_static ? is_static : flags[i];	  			
			return 0;
		}
    }

	// If it's a new IP - adding to the list
    interfaces[interfaces_count] = nip;
    flags[interfaces_count++] = (uint8_t)is_static;	
    return 1;
}

//------------------------------------------------------------------------------
int zfone_manager_remove_ip(uint32_t ip)
{
    uint32_t i = 0;
    uint32_t nip = zrtp_hton32(ip);
    
	// Looking for interface by IP
    for (i=0; i< interfaces_count; i++)
    {
		if (interfaces[i] == nip)
		{
			// Move last IP to the place of deleting interface
			interfaces[i] = interfaces[interfaces_count-1];
			flags[i] = flags[interfaces_count-1];
		    
			interfaces[interfaces_count-1] = 0;
			flags[--interfaces_count] = 0;

			return 0;
		}
    }	

    return -1;
}

//------------------------------------------------------------------------------
int zfone_manager_refresh_ip_list()
{
	uint32_t	changed = 0;

#if ZRTP_PLATFORM != ZP_WIN32_KERNEL 
	// ( we use system events to refresh list of network interfaces on Windows )

    uint32_t	i, ifaces_count = 0;
    struct 	ifaddrs* ifaddr = NULL;
    struct 	ifaddrs* ifaddr_head = NULL;
    uint32_t	count_bak = 0;
	uint32_t	ips_bak[ZFONE_MAX_INTERFACES_COUNT];

    // remove all IPs except user-defined
    count_bak = interfaces_count;
	memcpy(ips_bak, interfaces, sizeof(ips_bak));

    for (ifaces_count=0; ifaces_count<interfaces_count; ifaces_count++)
    {
		if (0 == flags[ifaces_count])
		{
	  		while ( interfaces_count )
	  		{
	  			if (0 != flags[interfaces_count-1])	  			
	  				break;
	  			interfaces_count--;
	  		}
	  		
	  		if ( !interfaces_count )	  		
	  			break;	  		
	  		
	  		interfaces[ifaces_count] = interfaces[interfaces_count-1];
	  		flags[ifaces_count] = flags[interfaces_count-1];
	    
	  		interfaces[interfaces_count-1] = 0;
	  		flags[--interfaces_count] = 0;
		}
    }
  
    // try to get interfaces list
    if ( -1 == getifaddrs(&ifaddr))
    {
        PRINT_START_ERROR(1, "ZFONE refresh_ip_list: Can't get interfaces list - try late.\n");
        return -1;
    }

    ifaces_count = 0;
    ifaddr_head = ifaddr;
    
    // going through all interfaces and  excluding locals and loopbacks
    while (ifaddr)
    {
		if (ifaddr->ifa_addr)
		{
	        uint32_t ip = ((struct sockaddr_in*) (ifaddr->ifa_addr))->sin_addr.s_addr;
    
  	      if ( (ip > 0) && (ip != 2130706433) && strncmp(ifaddr->ifa_name, "lo", 2) && 
    		   (ifaddr->ifa_addr->sa_family == AF_INET) )
			{
    		    zfone_manager_add_ip(zrtp_ntoh32(ip), 0); // register probably dynamic interface at detection unite
	  			++ifaces_count;
			}
		}
	    
		ifaddr = ifaddr->ifa_next;
    }
	
    freeifaddrs(ifaddr_head);

	if (!interfaces_count)
	{
		PRINT_START_ERROR(1, "+++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		PRINT_START_ERROR(3, "++++++++++++++ CANT DETECT ANY INTERFACES++++++++++++\n");
		PRINT_START_ERROR(3, "+++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	}
    
    if (count_bak != interfaces_count)
    {		
  		changed = 1;
    }
    else
    {
  		for (ifaces_count=0; ifaces_count<count_bak; ifaces_count++)
  		{
  			for (i=0; i<interfaces_count; i++)
  			{
  				if (ips_bak[ifaces_count] == interfaces[i])
  				{
  					break;
  				}
  			}
  			if (i == interfaces_count)
  			{
  				changed = 1;
  				break;
  			}
  		}
    }

	// print out the IPs list
	if (changed)
	{
		char	buffer[25];
		ZRTP_LOG(3, (_ZTU_, "ZFONE refresh_ip_list: there are changes in IP stack:\n"));
		for (ifaces_count=0; ifaces_count < interfaces_count; ifaces_count++)
		{
	  		zfone_ip2str(buffer, sizeof(buffer), zrtp_ntoh32(interfaces[ifaces_count]));
	  		ZRTP_LOG(3, (_ZTU_, "	%s:%s\n", buffer, flags[ifaces_count] ? "USER" : "AUTO"));
		}
	}
#endif
    return changed;
}

//------------------------------------------------------------------------------
int zfone_manager_is_local_ip(uint32_t ip)
{
    unsigned int i = 0;
    for (i=0; i < interfaces_count; i++)
    {
		if (interfaces[i] == ip)		
			return 1;		
    }

    return 0;
}

//-----------------------------------------------------------------------------
int zfone_manager_prepare_packet(zfone_packet_t* raw_packet)
{
	struct zrtp_iphdr* ipHdr = NULL;
    uint16_t saddr		= 0;
    uint16_t daddr		= 0;

    // Check params: pointers, size, protocol etc. All unsupported packets will be skipped
    if ( !raw_packet )
		return -1;
    
    if (raw_packet->size > MAX_PACKET_SIZE)
		return -2;

    if ( (voip_proto_ETHER != raw_packet->external_proto) && (voip_proto_IP != raw_packet->external_proto) )
		return -3;
        
    if ( ((voip_proto_ETHER == raw_packet->external_proto) && ( raw_packet->size < 42 )) ||
		 ((voip_proto_IP == raw_packet->external_proto) && ( raw_packet->size < 28 )) )
		return -4;
    
    // Skip ethernet header if needed
    raw_packet->offset = (voip_proto_ETHER == raw_packet->external_proto) ? 14 : 0;

    // Parsing IP header
    ipHdr  = (struct zrtp_iphdr*)(raw_packet->packet + raw_packet->offset);
    
    // Skip TCP before processing if SIP TCP scaning disabled
    raw_packet->proto = ipHdr->protocol;
    if ( (voip_proto_TCP == raw_packet->proto) &&
		 !(zfone_cfg.params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_TCP) )
		return -5;    
        
    // Check IP address and determinate direction
    if (zfone_manager_is_local_ip(ipHdr->saddr))
		raw_packet->direction = ZFONE_IO_OUT;
    else if (zfone_manager_is_local_ip(ipHdr->daddr))
		raw_packet->direction = ZFONE_IO_IN;
    else
        return -6; // packet not for our hosts space - skip it
    
#if ZRTP_PLATFORM != ZP_WIN32_KERNEL // Windows Driver may handle fragmented packets
    // Check IP header
    if (raw_packet->size < zrtp_ntoh16(ipHdr->tot_len))
    {
        ZRTP_LOG(1, (_ZTU_, "manager prepare packet: Wrong packet size:%u,"
					    " in IP header:%u.\n", raw_packet->size, zrtp_ntoh16(ipHdr->tot_len)));
        return -7;
    }
#endif
    
    // Clear packet structure
    raw_packet->voip_proto = voip_proto_UNKN;
    raw_packet->extra_data = NULL;
    
    // Set IPs
    if (ZFONE_IO_IN == raw_packet->direction)
    {
		raw_packet->local_ip = ipHdr->daddr;
		raw_packet->remote_ip = ipHdr->saddr;
    }
    else
    {
		raw_packet->local_ip = ipHdr->saddr;
		raw_packet->remote_ip = ipHdr->daddr;
    }
        
    // Skip IP header
    raw_packet->offset += ipHdr->ihl*4;
    
    // Parsing TCP/UDP header
    if (voip_proto_UDP == raw_packet->proto)
    {
		struct zrtp_udphdr* udpHdr = (struct zrtp_udphdr*) (raw_packet->packet + raw_packet->offset);
		raw_packet->offset += sizeof(struct zrtp_udphdr);
		saddr = udpHdr->source;
		daddr = udpHdr->dest;
    }
    else if (voip_proto_TCP == raw_packet->proto)
    {
		struct zrtp_tcphdr* tcpHdr = (struct zrtp_tcphdr*) (raw_packet->packet + raw_packet->offset);
		raw_packet->offset += tcpHdr->doff * 4;
		saddr = tcpHdr->source;
		daddr = tcpHdr->dest;
    }
    else
    {
		ZRTP_LOG(1, (_ZTU_, "manager prepare packet: IP Packet includs"
						" packet of unsupported protocol:%d\n", ipHdr->protocol));
		return -8;
    }
    
    // Set ports
    if (ZFONE_IO_IN == raw_packet->direction)
    {
		raw_packet->local_port = daddr;
		raw_packet->remote_port = saddr;
    }
    else
    {
		raw_packet->local_port = saddr;
		raw_packet->remote_port = daddr;
    }
    
    return 0;
}


//=============================================================================
//  VoIP protocol detection routine
//=============================================================================


//------------------------------------------------------------------------------
void config_sip(struct zfone_manager* manager, struct zfone_configurator* config)
{
    unsigned int  i = 0;
    unsigned short port = 0;

    // clear all previous values
	zrtp_memset(manager->local_ports, 0, sizeof(manager->local_ports));
	for (i=0; i<ZFONE_PORTS_SPACE_SIZE; i++)	
		manager->local_ports[i].proto = voip_proto_UNKN;
    
    zrtp_print_log_delim(3, LOG_START_SELECT, "configuring SIP	sniffer");
	ZRTP_LOG(3, (_ZTU_, "zfoned detector: listening for specifed ports:\t %s\n", (config->params.sniff.sip_scan_mode &ZRTP_BIT_SIP_SCAN_PORTS) ? "ENABLED" : "DISABLED"));
    ZRTP_LOG(3, (_ZTU_, "zfoned detector: scaning all UDP ports:\t\t	%s\n", (config->params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_UDP) ?	"ENABLED" : "DISABLED"));
	ZRTP_LOG(3, (_ZTU_, "zfoned detector: scaning all TCP ports:\t\t %s\n", (config->params.sniff.sip_scan_mode &	ZRTP_BIT_SIP_SCAN_TCP) ? "ENABLED" : "DISABLED"));
	ZRTP_LOG(3, (_ZTU_, "zfoned detector: RTP detection based on	SIP:\t %s\n", (config->params.sniff.rtp_detection_mode &	ZRTP_BIT_RTP_DETECT_SIP) ? "ENABLED" : "DISABLED"));
	ZRTP_LOG(3, (_ZTU_, "zfoned detector: RTP detection based on	RTP:\t %s\n", (config->params.sniff.rtp_detection_mode &	ZRTP_BIT_RTP_DETECT_RTP) ? "ENABLED" : "DISABLED"));
    
    if (config->params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_PORTS)
    {
		// setup new ports if needed
		for (i=0; i<ZRTP_MAX_SIP_PORTS_FOR_SCAN; i++)
		{
			port = config->params.sniff.sip_ports[i].port;
			if (0 < port)
			{
				zfone_port_wrap_t* wrap = &manager->local_ports[port];	
				wrap->proto = voip_proto_SIP;				
				wrap->transport |= (config->params.sniff.sip_ports[i].proto == voip_proto_UDP) ? ZRTP_BIT_SIP_UDP : ZRTP_BIT_SIP_TCP;
	
				ZRTP_LOG(3, (_ZTU_, "zfoned manager: %s port %d marked as SIP.\n",
								config->params.sniff.sip_ports[i].proto == voip_proto_UDP ? "UDP" : "TCP", port));				
			}
		}
    }
    
    zrtp_print_log_delim(3, LOG_END_SELECT, "configuring SIP sniffer");
}

//------------------------------------------------------------------------------
zrtp_voip_proto_t detect_proto(struct zfone_manager* manager, zfone_packet_t* packet)
{
    zrtp_voip_proto_t proto = voip_proto_UNKN;
	
	// It happens when VoIP client uses the same UDP port for SIP and RTP.
	// Every single SIP packet can be extracted from the UDP stream with
	// very high probability by means of dynamic tests. But for RTP we apply
	// heuristics at the beginning of the call only. So check for RTP before
	// the SIP tests.	
 
    // By default looking for RTP just on UDP ports space. But if user may force
	// ZFone to listen on TCP
 	if ( (voip_proto_UDP == packet->proto) ||
		 ((zfone_cfg.params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_TCP) && (voip_proto_TCP == packet->proto)) )
    {
		mlist_t *node = NULL;
		mlist_for_each(node, &manager->streams_head)
		{
			zfone_stream_t *stream = (zfone_stream_t*) mlist_get_struct(zfone_stream_t, _mlist, node);
			uint8_t is_in = (stream->in_media.type != ZFONE_SDP_MEDIA_TYPE_UNKN);
			uint8_t is_out = (stream->out_media.type != ZFONE_SDP_MEDIA_TYPE_UNKN);
			
			if ( (is_in && (stream->in_media.remote_ip == packet->remote_ip)) ||
				 (is_out && (stream->out_media.remote_ip == packet->remote_ip)) )
			{
				if ( (is_in && (stream->in_media.remote_rtp == packet->remote_port)) ||
				     (is_out && (stream->out_media.remote_rtp == packet->remote_port)) )
				{
					proto = voip_proto_RTP;
				}
				else if ( (is_in && (stream->in_media.remote_rtcp == packet->remote_port)) ||
						  (is_out && (stream->out_media.remote_rtcp == packet->remote_port)) )
				{
					proto = voip_proto_RTCP;					
				}

				packet->stream = stream;
				packet->ctx    = stream->zfone_ctx;
								
				// We use this trick to resolve audio and video mixing in
				// single UDP channel. Some VoIP clients triy to simplify NAT
				// traversing and use the same UDP port for Video and Voice
				// (some times even fort SIP, like iChat 4.0)
				if (proto != voip_proto_UNKN)
				{
					uint8_t reject = 0;					
					
					if ( ((packet->size - packet->offset) > ZRTP_SYMP_MIN_RTP_LENGTH) &&
						 (packet->ctx && (packet->ctx->luac == ZFONE_AGENT_ICHAT || packet->ctx->luac == ZFONE_AGENT_TIVI)) )
					{	
						uint8_t is_control = 0;
						uint32_t ssrc = 0;						
						if (proto == voip_proto_RTP)
						{							
							zrtp_rtp_hdr_t* rtphdr = (zrtp_rtp_hdr_t*)(packet->packet + packet->offset);
							ssrc = rtphdr->ssrc;
							is_control = (_zrtp_packet_get_type(rtphdr, packet->size) != ZRTP_NONE);
						}
						else if (proto == voip_proto_RTCP)
						{
							ssrc = ((zrtp_rtcp_hdr_t*)(packet->packet + packet->offset))->ssrc;
						}

						do{
						if ((ZFONE_IO_OUT == packet->direction) && is_out && (stream->out_media.ssrc == ssrc))
							break;

						if (ZFONE_IO_IN == packet->direction)
						{
							if (is_in && (stream->in_media.ssrc == ssrc))
								break;
							if (is_control && !is_in && is_out && (~stream->out_media.ssrc == ssrc))
								break;								
						}
						
						reject = 1;
						} while(0);
					} // iChat hack

					if (reject)					
					{
						proto = voip_proto_UNKN;
						packet->stream = NULL;
						packet->ctx    = NULL;
					}					
				} // processing "proto" result

				if (proto != voip_proto_UNKN)
					break; // apropriate stream was found
			} // comparing by IP
		} // for each RTP stream
	} // skan all UDP with apropriate length

    // looking for SIP. Skip system ports space.
    if ((voip_proto_UNKN == proto) && (zrtp_ntoh16(packet->local_port) > 999))
    {
		zfone_port_wrap_t* wrap = NULL;	
		uint16_t port = zrtp_ntoh16(packet->local_port);

		// Try to detect SIP using predefined static ports at firts		
		if (zfone_cfg.params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_PORTS)
		{
			wrap = &manager->local_ports[port];
		    
			if ( ((wrap->transport & ZRTP_BIT_SIP_UDP) && (voip_proto_UDP == packet->proto)) ||
				((wrap->transport & ZRTP_BIT_SIP_TCP) && (voip_proto_TCP == packet->proto)) )
			{
				proto = manager->local_ports[port].proto;
			}
		} // manual detection

		// Then if protocol still unknown and user wants to scan UDP or TCP - do it.
		if ( (voip_proto_UNKN == proto) &&
			( ((zfone_cfg.params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_UDP) && (packet->proto == voip_proto_UDP)) ||
			((zfone_cfg.params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_TCP) && (packet->proto == voip_proto_TCP))) &&
			((packet->size - packet->offset) > MIN_SIP_MESSAGE_SIZE)
			)
		{
			zfone_sip_type_t status = ZFONE_SIP_UNKNOWN;
			char* stok = (char *)(packet->packet + packet->offset);

			int32_t line_size = zfone_find_line_end( stok,
													ZRTP_MIN(packet->size, MIN_SIP_MESSAGE_SIZE+MAX_SIP_URI_SIZE),
													NULL );
			if (line_size > MIN_SIP_START_LINE_SIZE)
			{				
				status = parse_start_line(stok, line_size, NULL);
				if ((ZFONE_SIP_RESPONSE == status) || (ZFONE_SIP_REQUEST == status))
				{
					// This packet looks like SIP - try to parse such packets. If we will
					// detect errors during full parsing - this port will be cleared.
					proto = voip_proto_SIP;
				    
					// If detection by pecifed ports enabled - mark such port as SIP for future deetection.
					// It will be cleared if error during next parsing.			
					if (zfone_cfg.params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_PORTS)
					{
						zfone_port_wrap_t* wrap = &manager->local_ports[zrtp_ntoh16(packet->local_port)];	
						wrap->proto = voip_proto_SIP;				
						wrap->transport |= (packet->proto == voip_proto_UDP) ? ZRTP_BIT_SIP_UDP : ZRTP_BIT_SIP_TCP;						
					}			
				}
			}
		} //automatic detection
    } // SIP detection
    
    return proto;
}

