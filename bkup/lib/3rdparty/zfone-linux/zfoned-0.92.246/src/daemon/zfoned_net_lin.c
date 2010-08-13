/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */


#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>
#include <libipq.h>

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_	"zfone net"


char 	zfone_firewall_path[256];
    
uint8_t	zfone_is_tcp_rules_present;
uint8_t	zfone_is_udp_rules_present;

struct ipq_handle*	zfone_ipqh;
int 				zfone_raw_fd;


//=============================================================================
//    Zfone networker main part
//=============================================================================


//------------------------------------------------------------------------------
int zfone_network_init(const char* firewall_path)
{
	char one = 1;
    if (!firewall_path)
    {
		zrtp_print_log_console(1, "zfoned_network create: wrong"
												" params firewall path is NULL.\n" );
		return -1;
    }
    
    strncpy(zfone_firewall_path, firewall_path, sizeof(zfone_firewall_path)-1);
    
    zfone_is_tcp_rules_present = 0;
    zfone_is_udp_rules_present = 0;
    
    // Init netlink interface
    zfone_ipqh = ipq_create_handle(0, PF_INET);
    if (!zfone_ipqh || (ipq_set_mode(zfone_ipqh, IPQ_COPY_PACKET, 0) < 0))
    {
        zrtp_print_log_console(1, "zfoned network create: Error while"
			" init ipq lib. Is ip_queue module loaded?\n" );
		return -2;
    }
    ZRTP_LOG(3, (_ZTU_, "zfoned network create: Netlink was created successfully.\n"));
    
    if ((zfone_raw_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
    {
		zrtp_print_log_console(1, "zfoned network create: Can't open raw socket."
							    " zrtp IP filter must be run under root.\n" );
		return -3;
    }
    ZRTP_LOG(3, (_ZTU_, "zfoned network create: Raw socket for send with fd=%d created.\n", zfone_raw_fd));
    
    if ( setsockopt(zfone_raw_fd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1)
    {			
		zrtp_print_log_console(1, "zfoned network create: Can't set IP_HDRINCL option to Raw Socket.\n" );
		return -4;
    };    
    
    return 0;
}

//------------------------------------------------------------------------------
void zfone_network_down()
{
    if (zfone_raw_fd > 0)
    {
		ZRTP_LOG(3, (_ZTU_, "zfoned network destroy: Closing raw socket:%d.\n", zfone_raw_fd));
		if (0 != close(zfone_raw_fd))
			ZRTP_LOG(1, (_ZTU_, "zfoned network destroy: Network environment: can't close raw socket=%d.\n", zfone_raw_fd));
    }
    
    if (zfone_ipqh)
    {
		if (-1 == ipq_destroy_handle(zfone_ipqh))	
			ZRTP_LOG(1, (_ZTU_, "zfoned network destroy: Network environment: can't down netlink.\n"));
    }
}


//------------------------------------------------------------------------------
static int add_firewall_rule(const char* rule, const char* firewall_path)
{
    char* nptr = NULL;
    int i = 0;
    int arg_pos = 0;
    int tok_size = 0;
    char* args[20];
    char trule[257];
    pid_t pid = 0;
	int length;
    
    int res = -1;

    memset(args, 0, sizeof(args));
    memset(&trule, 0, sizeof(trule));

	if (strlen(rule) > sizeof(trule))
	{
		ZRTP_LOG(1, (_ZTU_, "zfoned zfone_add_firewall_rule: rule string is too long.\n"));
		goto FIREWALL_EXIT;
	}		

	if ( !firewall_path )
	{
        ZRTP_LOG(1, (_ZTU_, "zfoned zfone_add_firewall_rule: firewall path is NULL.\n"));
        goto FIREWALL_EXIT;
	}
	if ( (length = strlen(firewall_path)) <= 0 )
	{
        ZRTP_LOG(1, (_ZTU_, "zfoned zfone_add_firewall_rule: firewall path is empty.\n"));
        goto FIREWALL_EXIT;
	}
	length += 1; // For NULL terminating

    // Parsing rule string
    
    // Copy firewall path
    if ( !(args[arg_pos] = (char*) malloc(length)) )
    {
        ZRTP_LOG(1, (_ZTU_, "zfoned zfone_add_firewall_rule: Can't allocate memory for firewall rule creation.\n"));
        goto FIREWALL_EXIT;
    }
    strcpy(args[arg_pos], firewall_path);
    
    // Set firewall command params
    strcpy(trule, rule);
    nptr = (char*) strtok(trule, " ");
    while ( nptr )
    {
		arg_pos++;
		tok_size = strlen(nptr) + 1;
		if ( !(args[arg_pos] = (char*)malloc(tok_size+5)) )
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned zfone_add_firewall_rule: Can't allocate"
										   " memory for firewall rule creation.\n"));
			return -1;
		}
		strcpy(args[arg_pos], nptr);
		nptr = strtok(NULL, " ");
		
		if (arg_pos == 100)
		{
			ZRTP_LOG(1, (_ZTU_, "zfoned zfone_add_firewall_rule: To many arguments"
										   "=%d, must be less then 100.\n", arg_pos));
			goto FIREWALL_EXIT;
		}
    }
    arg_pos++;
    args[arg_pos] = NULL;
	
    // Add  rule to firewall
    pid = fork();
    if(pid < 0)
    {
        ZRTP_LOG(1, (_ZTU_, "zfoned zfone_add_firewall_rule: Can't create"
									   " new process to execute firewall.\n"));
        goto FIREWALL_EXIT;
    }	
    else
    {
        if(0 == pid)
        {
    	    execv(firewall_path, args);
			ZRTP_LOG(1, (_ZTU_, "zfoned zfone_add_firewall_rule: Can't execute firewall.\n"));
			goto FIREWALL_EXIT;
		}
		else
		{
			int status;
			waitpid(pid, &status, 0);
			if(status != 0)
			{
				goto FIREWALL_EXIT;
			}
		}
    }
    
    res = 0;;

FIREWALL_EXIT:

    // free memory
    for (i=0; i<arg_pos; i++)    
		if (args[i])
			free(args[i]);

    return res;
}

int zfone_network_add_rules(int is_adding, zrtp_voip_proto_t proto)
{
	char rule[256];
    uint8_t is_udp = (proto == voip_proto_UDP);
    
    if ((voip_proto_TCP != proto) && (voip_proto_UDP != proto))
    {
		zrtp_print_log_console(1, "zfoned network: usupported"
								" protocol %d in firewall rules.", proto);
		return -1;
    }
    
    // Add protection from rules readding. Removing can be done several timres.
    if (is_adding && ((zfone_is_udp_rules_present && is_udp) || (zfone_is_tcp_rules_present && !is_udp)) )
    {
		ZRTP_LOG(3, (_ZTU_, "zfoned networker: %s firewall rules is"
									   " already there.\n", is_udp ? "UDP" : "TCP"));
		return 0;
    }

    ZRTP_LOG(3, (_ZTU_, "zfoned add_rules: %s ipfilter proto=%s...\n",
				    is_adding ? "INSERTING rules to" : "REMOVING rules from",
					is_udp ? "UDP" : "TCP" ));

    if (is_adding)
		snprintf(rule, sizeof(rule), "-A INPUT -p %s --destination-port 1024: -j QUEUE", is_udp ? "udp" : "tcp");
    else
		snprintf(rule, sizeof(rule),  "-D INPUT -p %s --destination-port 1024: -j QUEUE", is_udp ? "udp" : "tcp");
    if ( (0 !=  add_firewall_rule(rule, zfone_firewall_path)) && is_adding )
    {
		return -1;
    }
	
    if (is_adding)
		snprintf(rule, sizeof(rule), "-A OUTPUT -p %s --source-port 1024: -j QUEUE", is_udp ? "udp" : "tcp");
    else
		snprintf(rule, sizeof(rule), "-D OUTPUT -p %s --source-port 1024: -j QUEUE", is_udp ? "udp" : "tcp");
	
    if ( (0 !=  add_firewall_rule(rule, zfone_firewall_path)) && is_adding )
    {
		// Try to remove previous one
		snprintf(rule, sizeof(rule), "-D INPUT -p %s --destination-port 1024: -j QUEUE", is_udp ? "udp" : "tcp");
		add_firewall_rule(rule, zfone_firewall_path);
    	return -1;
    }

    if (is_adding)
		snprintf(rule, sizeof(rule), "-A FORWARD -p %s --destination-port 1024: -j QUEUE", is_udp ? "udp" : "tcp");
    else
		snprintf(rule, sizeof(rule), "-D FORWARD -p %s --destination-port 1024: -j QUEUE", is_udp ? "udp" : "tcp");
    if ( (0 !=  add_firewall_rule(rule, zfone_firewall_path)) && is_adding )
    {
		return -1;
    }

    // All rules added/deleted successfully - set flags
    if (is_udp)
        zfone_is_udp_rules_present = is_adding ? 1 : 0;
    else
        zfone_is_tcp_rules_present = is_adding ? 1 : 0;
    
    return 0;
}

//------------------------------------------------------------------------------
int zfone_network_send_ip(char* packet, unsigned int length, unsigned int to)
{
    int s = 0;
    struct sockaddr_in sout;
    int soutlen = sizeof(struct sockaddr_in);
    
    memset(&sout, 0, soutlen);
    sout.sin_family = PF_INET;
    sout.sin_addr.s_addr = to;
    sout.sin_port = 0;

    {
	char ip_str_1[24]; char ip_str_2[24];
	struct zrtp_iphdr* ipHdr	= (struct zrtp_iphdr*)packet;
	struct zrtp_udphdr* udpHdr	= (struct zrtp_udphdr*)(packet + 20);

	ZRTP_LOG(3, (_ZTU_, " VOIPD zrtp_send_ip: stream Send %d:%d bytes packet FROM=%s:%u TO=%s:%u.\n",
					length, zrtp_ntoh16(ipHdr->tot_len),
					zfone_ip2str(ip_str_1, 24, zrtp_ntoh32(ipHdr->saddr)), (unsigned int) zrtp_ntoh16(udpHdr->source),
					zfone_ip2str(ip_str_2, 24, zrtp_ntoh32(ipHdr->daddr)), (unsigned int) zrtp_ntoh16(udpHdr->dest)));		    
    }
    
    if (0 >=  (s = sendto(zfone_raw_fd, packet, length, 0, (struct sockaddr*) &sout, soutlen)))
    {				
		ZRTP_LOG(1, (_ZTU_, "zfoned  zrtp_send_ip: Can't write packet to raw socket\n"));
		return -1;
    }
            
    return s;
}

//------------------------------------------------------------------------------
int zfone_network_get_packet(zfone_packet_t* packet)
{
	int readResult = 0;

    // Get message from the Linux kernel
    readResult = ipq_read(zfone_ipqh, packet->buffer, MAX_PACKET_SIZE, 0);
        
    if ( readResult < 0 || (ipq_message_type(packet->buffer) == NLMSG_ERROR) )
    {
		ZRTP_LOG(1, (_ZTU_, "zfoned network get_packet: ERROR! error during packet reading form netlink.\n"));
        return -1;
    }
    
    // Get packet body
    packet->message = ipq_get_packet((u_int8_t*)packet->buffer);
    if ( (!packet->message) || (packet->message->data_len < 20) )
    {
        ZRTP_LOG(3, (_ZTU_, "zfoned network get_packet: read wrong packet %d\n",
						packet->message ? (int)packet->message->data_len : -1));
        return -1;
    }
    
    packet->packet = packet->message->payload;
    packet->size = packet->message->data_len;
	    
    return packet->size;
}

//------------------------------------------------------------------------------
int zfone_network_put_packet(zfone_packet_t* packet, int need_accept)
{
	if (need_accept > 0)
    {
		if (0 > ipq_set_verdict( zfone_ipqh,
								 packet->message->packet_id,
								 NF_ACCEPT,
								 packet->size,
								 packet->packet ))
		{
		    ZRTP_LOG(1, (_ZTU_, "zfoned network put_packet: Can't accept packet with ID=%lu.\n",
						    packet->message->packet_id));
		    return -1;
		}
    }
    else
    {
		if (0 > ipq_set_verdict( zfone_ipqh,
								 packet->message->packet_id,
								 NF_DROP,
								 0,
								 NULL ))
		{
		    ZRTP_LOG(1, (_ZTU_, "zfoned network put_packet: Can't drop packet with ID=%lu.\n",
							packet->message->packet_id));
		    return -1;
		}
    }
    
    return 0;
}

