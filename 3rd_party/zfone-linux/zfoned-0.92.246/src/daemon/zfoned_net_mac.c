/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip_fw.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone netmac"

#define zfone_first_rule 	1000
#define DIVERT_PORT			8888

int					zfone_divert_fd;
static int			divert_port_num;
static socklen_t 	zfone_from_length = sizeof(struct sockaddr_in);

//-----------------------------------------------------------------------------
int zfone_network_init(const char* firewall_path)
{
	struct sockaddr_in bind_port;
    int i = 0;
    
    ZRTP_LOG(3, (_ZTU_, "zfoned network create: starting network module\n"));
    
    // Create divert socket and binding
    bzero(&bind_port, sizeof(bind_port));
    zfone_divert_fd = socket(AF_INET, SOCK_RAW, IPPROTO_DIVERT);
    if (zfone_divert_fd < 0)
    {
		ZRTP_LOG(1, (_ZTU_, "zfoned network create: Can't create divert socket.\n"));
		return -1;
    }
    ZRTP_LOG(3, (_ZTU_, "zfoned network create: Divert socket with fd=%d was created.\n", zfone_divert_fd));
    
    bind_port.sin_family = PF_INET;
    bind_port.sin_addr.s_addr = INADDR_ANY;
    
    for (i=0; i<100; i++)
    {	
		bind_port.sin_port = zrtp_hton16(DIVERT_PORT+i);
		if ( 0 == bind(zfone_divert_fd, (struct sockaddr*) &bind_port, sizeof(bind_port)) )
		{
			ZRTP_LOG(3, (_ZTU_, "zfoned network create: Divert socket was created successfully.\n"));
			divert_port_num = DIVERT_PORT+i;
			return 0;
		}
    }
    
    ZRTP_LOG(1, (_ZTU_, "zfoned network create: Socket with fd=%d"
									" can't bind to the ports %d:%d.\n",
									zfone_divert_fd, DIVERT_PORT, DIVERT_PORT+100));

    return -1;
}

//-----------------------------------------------------------------------------
void zfone_network_down()
{
	if (zfone_divert_fd > 0 )
    {
		ZRTP_LOG(3, (_ZTU_, "zfoned network down: Close divert socket:%d.\n", zfone_divert_fd));
		if (0 != close(zfone_divert_fd))
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned networker down: Network environment"
										   " can't close divert socket=%d.\n", zfone_divert_fd));
		}
    }
}

//-----------------------------------------------------------------------------
void createBasicRule(struct ip_fw *a_fw, int a_rule_num, int a_is_udp, int a_is_in)
{
	bzero(a_fw, sizeof (struct ip_fw));
	a_fw->version = IP_FW_CURRENT_API_VERSION;
	a_fw->fw_number = a_rule_num;
	
	/* from any to any */
	//fw.fw_src.s_addr = *(in_addr_t*)(temp_hostent->h_addr);
	//fw.fw_smsk.s_addr = ~0;
	
	//fw.fw_dst.s_addr = *(in_addr_t*)(temp_hostent->h_addr); 
	//fw.fw_dmsk.s_addr = ~0;
	
	a_fw->fw_prot = a_is_udp ? IPPROTO_UDP : IPPROTO_TCP;
	a_fw->fw_flg = a_is_in ? IP_FW_F_DIVERT | IP_FW_F_IN | IP_FW_F_DME | IP_FW_F_DRNG  | IP_FW_F_SRNG 
						   : IP_FW_F_DIVERT | IP_FW_F_OUT | IP_FW_F_SME  | IP_FW_F_DRNG  | IP_FW_F_SRNG;	
	
	a_fw->fw_uar.fw_pts[0] = 24;
	a_fw->fw_uar.fw_pts[1] = 65535;
	a_fw->fw_uar.fw_pts[2] = 24;
	a_fw->fw_uar.fw_pts[3] = 65535;
	IP_FW_SETNSRCP(a_fw, 2);
	IP_FW_SETNDSTP(a_fw, 2);
//	a_fw->fw_nports = 2;
	
	a_fw->fw_un.fu_divert_port=divert_port_num;
}

int zfone_network_add_rules(int is_adding, zrtp_voip_proto_t proto)
{

	struct ip_fw fw;
	int fw_sock;
	
	uint8_t is_udp = (proto == voip_proto_UDP);
    
    if ((voip_proto_TCP != proto) && (voip_proto_UDP != proto))
    {
		zrtp_print_log_console(1, "zfoned networker: usupported protocol"
							   " %d in firewall rules.\n", proto);
		return -1;
    }
	
	/* open a socket */
	if ((fw_sock=socket(AF_INET, SOCK_RAW, IPPROTO_RAW))==-1) 
	{
		ZRTP_LOG(3, (_ZTU_, "zfoned networker: could not create a raw socket: %s\n", strerror(errno)));
		return -1;
	}
	
	do {
		/* add rule for incoming brunch */
		createBasicRule(&fw, is_udp ? 10000 : 10002, is_udp, 1);
		if (setsockopt(fw_sock, IPPROTO_IP, is_adding ? IP_FW_ADD : IP_FW_DEL, &fw, sizeof(fw))==-1)
		{
			ZRTP_LOG(3, (_ZTU_, "zfoned networker: could not set rule: %s\n", strerror(errno)));
			break;
		}
		
		/* add rule for autgoing brunch */
		createBasicRule(&fw, is_udp ? 10001 : 10004, is_udp, 0);
		if (setsockopt(fw_sock, IPPROTO_IP, is_adding ? IP_FW_ADD : IP_FW_DEL, &fw, sizeof(fw))==-1)
		{
			ZRTP_LOG(3, (_ZTU_, "zfoned networker: could not set rule: %s\n", strerror(errno)));
			break;
		}
	} while (0);
	
	close(fw_sock);
	
	return 0;
}

//-----------------------------------------------------------------------------
int zfone_network_send_ip(char* packet, unsigned int length, unsigned int to)
{
	int s = 0;
    struct sockaddr_in sout;
    int soutlen = sizeof(struct sockaddr_in);
    
    memset(&sout, 0, soutlen);
    sout.sin_family = AF_INET;
    sout.sin_addr.s_addr = 0;
	
    {
        char ip_str_1[24]; char ip_str_2[24];
		struct zrtp_iphdr* ipHdr        = (struct zrtp_iphdr*)packet;
		struct zrtp_udphdr* udpHdr      = (struct zrtp_udphdr*)(packet + 20);
			
		ZRTP_LOG(3, (_ZTU_, " ZFONED zrtp_send_ip: Send %d bytes packet FROM=%s:%u TO=%s:%u.\n", length,
					zfone_ip2str(ip_str_1, 24, zrtp_ntoh32(ipHdr->saddr)), (unsigned int) zrtp_ntoh16(udpHdr->source),
					zfone_ip2str(ip_str_2, 24, zrtp_ntoh32(ipHdr->daddr)), (unsigned int) zrtp_ntoh16(udpHdr->dest)) );
    }
										
    
    sendto(zfone_divert_fd, packet, length, 0, (struct sockaddr*) &sout, soutlen);    

    return s;
}

//-----------------------------------------------------------------------------
int zfone_network_get_packet(zfone_packet_t* packet)
{
	memset(&packet->from, 0, zfone_from_length);
    packet->from_length = zfone_from_length;
	int size = recvfrom( zfone_divert_fd,
						 packet->buffer,
						 sizeof(packet->buffer),
						 0,
						 (struct sockaddr*) &packet->from,
						 &packet->from_length );
    
	packet->size = size;
    packet->packet = packet->buffer;
    
    return size;
}

// -----------------------------------------------------------------------------
int zfone_network_put_packet(zfone_packet_t* packet, int need_accept)
{
	if (need_accept > 0)
    {
		sendto( zfone_divert_fd,
				packet->packet,
				packet->size,
				0,
				(struct sockaddr*) &packet->from,
				packet->from_length );
    }
    
    return 0;
}

