/*
 * This header file contains all the major functions of the SQRKal app
 *  which deal with discovering SIP/RTP sessions running in the machine
 *
 * @author Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#ifndef SQRKAL_DISCOVERY_H
#define SQRKAL_DISCOVERY_H

#include <iostream>
#include <string>
#include "sqrkal_proto_formats.hpp"

using namespace std;

namespace sqrkal_discovery {

#define BUFSIZE 2048
#define TCP_PROTOCOL 6
#define UDP_PROTOCOL 17
#define SIP_PROTOCOL 20
#define RTP_PROTOCOL 31


/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ALL UTILITY FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ **/


	/*************** ADD FIREWALL RULES ********************************/
	int add_firewall_rule(string rule);
	int add_all_rules(int is_adding, int is_udp);

	/** THis function adds all the firewall rules for incoming and outgoing RTP/SRTP traffic **/
	int add_all_rtp_rules(int is_adding, int is_udp);

	/************************************************************************************/



	/**
	 * Create sockets for our discovery process to forward/send/receive SIP/SDP messages
	 */
	int create_sockets();

	/** STOP DISCOVERY **/
	int stop_discovering();

    /** RETURNS 1 if this IP is one of the localhosts. 0 otherwise **/
     int isLocalIP(string IP);



     /**
      * Get the direction of the packet
      *
      * RETURN ZERO IF PACKETS COMING IN
      *         ONE IF PACKETS GOING OUT
      *         -1 OTHERWISE
      */
     int get_direction(IP_Header& ipHeader);

	/********* CALCULATE IP and UDP checksums and return the buff  ***************/
	/** @cite http://www.enderunix.org/docs/en/rawipspoof/ **/
/*	struct psd_udp {
		uint32_t src;
		uint32_t dst;
		unsigned char pad;
		unsigned char proto;
		unsigned short udp_len;
		unsigned char data[BUFSIZE];
	};*/

	unsigned short in_cksum(unsigned short *addr, int len);
	unsigned short in_cksum_udp(int src, int dst, unsigned short *addr, int len);
	int form_checksums(char * buff);

	/**
	 * SEND SIP/SDP Message
	 */
     int send_raw_message(char *buff, int length );

	/** Verify if this string is an IP ADDRESS */
	bool isValidIpAddress(const char *ipAddress);
	bool resolve_and_set(string hostname);

     /*****************             SRPP RELATED METHODS             ***************************/


/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ END OF UTILITY FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~**/




/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ALL PROCESSING FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ **/

    /**
     * Parse the received/sent SIP message and its SDP payload
     */
	int parse_sip(string message, int direction);
	/** --------------- PROCESS A RECEIVED PACKET ------------------------**/
	int process_packet( unsigned char *buff, int bytes_read);

	/** MAIN LOOP **/
	int discover_sessions();
}

#endif
