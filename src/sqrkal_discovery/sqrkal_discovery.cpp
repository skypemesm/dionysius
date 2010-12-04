/*
 * This header file implements all the major functions of the SQRKal app
 *  which deal with discovering SIP/RTP sessions running in the machine
 *
 * @author Saswat Mohanty <smohanty@cs.tamu.edu>
 */
#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <vector>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>

#include "sqrkal_proto_formats.hpp"
#include "sqrkal_discovery.h"
#include "SRPPSession.hpp"

#include <fstream>

#include <linux/netfilter.h>
extern "C" {
	#include <libipq.h>
}

using namespace std;

//namespace sqrkal_discovery {


 /**
  * Namespace variables
  */
	string firewall_path;
	int udp_added = 0;
	int tcp_added = 0;
	int rtp_added = 0;
	int is_discovering = 0;
	int is_sip = 0;
	int is_rtp = 0;
	int is_udp = 0;
	int is_srtp = 0;
	int is_srpp = 0;
	int apply_srpp = 0;
	int is_session_on = 0;
	int inport,outport;  //inport is my RTP port and output port is the other endpoint's RTP port

	int saw_invite_already = 0, saw_bye_already = 0;
	int sent_invite = 0,saw_ack = 0,saw_200ok = 0;

	int recv_count = 0, sent_count =0;

	struct ipq_handle*	sqrkal_discovery_ipqh;
	int 			sip_socket;

	int sip_port = 56789;

	vector<int> sip_ports (1,5060);
	vector<int> rtp_ports (1,5000);

	struct sockaddr_in out_addr, back_out_addr;
	unsigned int last_out_dest;
	unsigned int rtp_dest, rtp_src;
	char rtp_header[40];
	int rtp_hdset = 0;
	int frag_id = 0; // offset for fragmented packet id
	int srpp_ttl=65;
	char srpp_ttl_s[2];
	uint32_t ssrc_value = 0;

	RTPMessage rtp_msg;
	SRPPMessage srpp_msg;
	SRTPMessage srtp_msg;


	static sqrkal_discovery* thisinstance;

	vector<int> input_packet_sizes;
	vector<int> output_packet_sizes;

/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ALL UTILITY FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ **/


	/*************** ADD FIREWALL RULES ********************************/
	int sqrkal_discovery::add_firewall_rule(string rule)
	{
		 rule = "iptables " + rule;

		 return system(rule.c_str());
	}

	int sqrkal_discovery::add_all_rules(int is_adding, int is_udp)
	{

	 // cout << "PID: " << getpid() << endl;
		string rule;

	    if (is_adding && ((udp_added && is_udp) || (tcp_added && !is_udp)) )
	    {
			cout << "Rules already added\n";
			return 0;
	    }

	    string proto = is_udp ? "udp" : "tcp";
	    if (is_adding) {
		if (add_firewall_rule(" -F") < 0)
			cerr << " ERROR IN ADDING RULE" << endl ;

	     }


	    if (is_adding)
			rule = "-A INPUT -p "+proto;
	    else
	    	rule = "-D INPUT -p "+proto;

	    rule = rule + " --destination-port 5060 -m ttl ! --ttl-eq " + srpp_ttl_s + " -j QUEUE";

	    if ( (0 !=  add_firewall_rule(rule)) && is_adding )
	    {
			return -1;
	    }

	    rule = "";

	    if (is_adding)
	    	rule = "-A OUTPUT -p "+proto;
	    else
	    	rule = "-D OUTPUT -p "+proto;

	    rule  = rule + " --destination-port 5060 -m ttl ! --ttl-eq " + srpp_ttl_s + " -j QUEUE";

	    if ( (0 !=  add_firewall_rule(rule)) && is_adding )
	    {
			// Try to remove previous one
	    	rule = "-D INPUT -p "+ proto + " --destination-port 5060 -m ttl ! --ttl-eq " + srpp_ttl_s + " -j QUEUE";
			add_firewall_rule(rule);
	    	return -1;
	    }

	  /*
	    if (is_adding)
			rule = "-t nat -A POSTROUTING -p "+ proto + " --source-port 56789 -j SNAT --to-source 192.168.1.107:5060";
	    else
	    	rule = "-t nat -D POSTROUTING -p "+ proto + " --source-port 56789 -j SNAT --to-source 192.168.1.107:5060";


	    if ( (0 !=  add_firewall_rule(rule)) && is_adding )
	    {
			return -1;
	    }
	*/

	    // All rules added/deleted successfully - set flags
	    if (is_udp)
		udp_added = is_adding ? 1 : 0;
	    else
		tcp_added = is_adding ? 1 : 0;

	   if (is_adding)
		    cout << "FIREWALL rules added\n";
	   else
		    cout << "FIREWALL rules removed\n";

	    return 0;
	}


	/** THis function adds all the firewall rules for incoming and outgoing RTP/SRTP traffic **/
	int sqrkal_discovery::add_all_rtp_rules(int is_adding, int is_udp)
	{
	    if (is_adding && rtp_added )
	    {
			cout << "RTP firewall rules already added\n";
			return 0;
	    }
	    string rule;
	    char rtpport[7];
	    sprintf(rtpport,"%d",inport);
	    sprintf(srpp_ttl_s,"%d",srpp_ttl);

	    string proto = is_udp ? "udp" : "tcp";
	    ///////// REMOVE THIS WHEN SIP PROCESSING RE-USED
	    if (is_adding)
	    {
	   		//if (add_firewall_rule(" -F") < 0)
	   		//	cerr << " ERROR IN ADDING RULE" <<endl ;

	   	}

	    if (is_adding)
			rule = "-A INPUT -p "+proto;
	    else
	    	rule = "-D INPUT -p "+proto;

	    rule += " --destination-port "+string(rtpport)+" -m ttl ! --ttl-eq " + srpp_ttl_s + " -j QUEUE";

	    if ( (0 !=  add_firewall_rule(rule)) && is_adding )
	    {
			return -1;
	    }

	    rule = "";

	    if (is_adding)
	    	rule = "-A OUTPUT -p "+proto;
	    else
	    	rule = "-D OUTPUT -p "+proto;

	    rule += " --source-port "+string(rtpport)+" -m ttl ! --ttl-eq " + srpp_ttl_s + " -j QUEUE";

	    if ( (0 !=  add_firewall_rule(rule)) && is_adding )
	    {
			// Try to remove previous one
	    	rule = "-D INPUT -p "+ proto + " --destination-port "+string(rtpport)+" -m ttl ! --ttl-eq " + srpp_ttl_s + " -j QUEUE";
			add_firewall_rule(rule);
	    	return -1;
	    }


	    // All rules added/deleted successfully - set flags
	   if (is_adding)
		   { rtp_added =1; cout << "FIREWALL rules added for RTP traffic \n";}
	   else
		    { rtp_added =0; cout << "FIREWALL rules removed for RTP traffic \n";}

	    return 0;
	}
	/************************************************************************************/



	/**
	 * Create sockets for our discovery process to forward/send/receive SIP/SDP messages
	 */
	int sqrkal_discovery::create_sockets()
	{
		// We need to create RAW sockets since we want to operate in a MITM mode,
		// and we will need to form the source and destination addresses of the packets
		char one = 1;
		sqrkal_discovery_ipqh = ipq_create_handle(0, PF_INET);
		if (!sqrkal_discovery_ipqh || (ipq_set_mode(sqrkal_discovery_ipqh, IPQ_COPY_PACKET, BUFSIZE) < 0))
		{
			cerr << " IP QUEUE library not loaded.\n" ;
			return -2;
		}

		if ((sip_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)       //CHANGE THIS TO IPPROTO_RAW LATER
		    {
			 cout << "Unable to create the SIP sockets."<< sip_socket<<" \n";
			 return -3;
		    }

		 if ( setsockopt(sip_socket, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1)
		    {
			cerr << "Unable to set option to Raw Socket.\n";
			return -4;
		    };

		 return 0;

	}



    /** RETURNS 1 if this IP is one of the localhosts. 0 otherwise **/
     int sqrkal_discovery::isLocalIP(string IP)
     {

    	//uint32_t thisip = ((struct sockaddr_in )(inet_addr(IP.c_str())) )->sin_addr.s_addr;
    	 uint32_t thisip = inet_addr(IP.c_str());

    	 /** GET MY ADDRESS **/
        	 struct ifreq   buffer[32];
    		 struct ifconf  intfc;
    		 struct ifreq  *pIntfc;
    		 int  num_intfc;

    		 intfc.ifc_len = sizeof(buffer);
    		 intfc.ifc_buf = (char*) buffer;

    		 if (ioctl(sip_socket, SIOCGIFCONF, &intfc) < 0)
    			  {
    				 cerr <<"ioctl SIOCGIFCONF failed\n";
    			  }

    		  pIntfc    = intfc.ifc_req;
    		  num_intfc = intfc.ifc_len / sizeof(struct ifreq);

    		  for (int i = 0; i < num_intfc; i++)
    		  {
    			  struct ifreq *item = &(pIntfc[i]);
    			  uint32_t addr = ((struct sockaddr_in *)&item->ifr_addr)->sin_addr.s_addr;

       			  if ( thisip == addr)
    					return 1;
    		  }

        	return 0;
         }




     /**
      * Get the direction of the packet
      *
      * RETURN ZERO IF PACKETS COMING IN
      *         ONE IF PACKETS GOING OUT
      *         -1 OTHERWISE
      */
     int sqrkal_discovery::get_direction(IP_Header& ipHeader)
     {

  	 /** GET MY ADDRESS **/
	 struct ifreq   buffer[32];
	 struct ifconf  intfc;
	 struct ifreq  *pIntfc;
	 int  num_intfc;

	 intfc.ifc_len = sizeof(buffer);
	 intfc.ifc_buf = (char*) buffer;

	 if (ioctl(sip_socket, SIOCGIFCONF, &intfc) < 0)
		  {
			 cerr <<"ioctl SIOCGIFCONF failed\n";
		  }

	  pIntfc    = intfc.ifc_req;
	  num_intfc = intfc.ifc_len / sizeof(struct ifreq);

	  for (int i = 0; i < num_intfc; i++)
	  {
		  struct ifreq *item = &(pIntfc[i]);
		  uint32_t addr = ((struct sockaddr_in *)&item->ifr_addr)->sin_addr.s_addr;

		  if ( ipHeader.saddr == addr)
				return 1;
	  }

    	return 0;
     }


	/********* CALCULATE IP and UDP checksums and return the buff  ***************/
	/** @cite http://www.enderunix.org/docs/en/rawipspoof/ **/


	struct psd_udp {
		uint32_t src;
		uint32_t dst;
		unsigned char pad;
		unsigned char proto;
		unsigned short udp_len;
		unsigned char data[BUFSIZE];
	};

	unsigned short sqrkal_discovery::in_cksum(unsigned short *addr, int len)
	{
		int nleft = len;
		int sum = 0;
		unsigned short *w = addr;
		unsigned short answer = 0;

		while (nleft > 1) {
			sum += *w++;
			//if (sum & 0x80000000)
                         //sum = (sum & 0xFFFF) + (sum >> 16);
			nleft -= 2;
		}

		if (nleft == 1) {
			*(unsigned char *) (&answer) = *(unsigned char *) w;
			sum += answer;
		}

		sum = (sum >> 16) + (sum & 0xFFFF);
		sum += (sum >> 16);
		answer = ~sum;
		return (answer);
	}

	unsigned short sqrkal_discovery::in_cksum_udp(int src, int dst, unsigned short *addr, int len)
	{
		struct psd_udp* buf = new struct psd_udp;

		buf->src = src;
		buf->dst = dst;
		buf->pad = 0;
		buf->proto = IPPROTO_UDP;
		buf->udp_len = htons(len);

		memcpy(&(buf->data), addr, len); //copy data
		unsigned short res = in_cksum((unsigned short *)buf, len+12);
		delete buf;
		return res;
	}



	int sqrkal_discovery::form_checksums(char * buff)
	{
 	  // Get IP and UDP headers
	  IP_Header* ipHdr  = (IP_Header*)(buff);
	  struct UDP_Header* udpHdr = (struct UDP_Header*) (buff + 4*ipHdr->ihl);


	  //---- Form and fill IP checksum now--------------------------------------
	  ipHdr->check = 0;
	  ipHdr->check = in_cksum((unsigned short *)ipHdr, sizeof(*ipHdr));


	  //---- calculate and fill udp checksum now ---
	  udpHdr->checksum = 0;

	 // udpHdr->checksum = in_cksum_udp(ipHdr->saddr, ipHdr->daddr, (unsigned short *)(buff + 4*ipHdr->ihl+8), ntohs(udpHdr->length));
	  return 0;

	}



	/**
	 * SEND SIP/SDP Message
	 */
     int sqrkal_discovery::send_raw_message(char *buff, int length )
     {

	if (out_addr.sin_port == 0 )
	{
		cout << "SORRY dont know where to send this raw packet\n";
		return -1;
	}

	if (length <= 0)
		return -1;

	IP_Header* ipHdr  = (IP_Header*)(buff);

	int byytes = sendto(sip_socket, buff, length, 0,
					 (struct sockaddr *)&(out_addr), sizeof(struct sockaddr));
	if (byytes < 0)
	{
		cout << "ERROR IN SENDING DATA: " << strerror(errno) << "\nLENGTH:" << length << endl << endl;
		for (int i = 0; i<length;i++)
		 printf("%c",buff[i]);
	}

	cout << "\nWriting " << byytes << " bytes \"" << inet_ntoa(out_addr.sin_addr) << ":" << ntohs(out_addr.sin_port) << " LEN:" << length << endl << endl;

	/*
	in_addr abc;
	abc.s_addr = ipHdr->saddr;

	//cout << "SENT FROM " << inet_ntoa(abc);

	abc.s_addr = ipHdr->daddr;
	//cout << " TO " <<inet_ntoa(abc) <<endl;
	*/

	return byytes;
     }


     /** Fragments the IP message and sends it accordingly **/
     int sqrkal_discovery::send_fragmented_message(char* buff,int bytes_read)
     {
/*			printf("###---------------------------------------\n");
				for(int i = 0;i< 20;i++)
					printf("%x",buff[i]);
				printf ("\n");

				for(int i = 20;i< bytes_read;i++)
					printf("%c",buff[i]);
				printf("\n---------------------------------------\n");*/

				IP_Header* ipHdr  = (IP_Header*)(buff);

				ipHdr->id = htons(15032+frag_id++);
				ipHdr->frag_off = htons(8192);
				ipHdr->tot_len=IP_MTU;

				form_checksums((char * )buff);
				send_raw_message((char *)buff,IP_MTU);

				int bytes_left = bytes_read-IP_MTU;
				char buff1[bytes_left+20];
				memcpy(buff1,buff,20);//copy header
				memcpy(buff1+20,buff+IP_MTU,bytes_left);

				ipHdr  = (IP_Header*)(buff1);
				ipHdr->frag_off = htons(185);
				ipHdr->tot_len = bytes_left + 20;

				form_checksums(buff1);
				send_raw_message(buff1,bytes_left+20);


				//-----------------------------------------------------
	/*			for(int i = 0;i< 20;i++)
					printf("%x",buff[i]);
				printf ("\n");

				for(int i = 20;i< IP_MTU;i++)
					printf("%c",buff[i]);
				printf("\n----------------\n");

				for(int i = 0;i< 20;i++)
					printf("%x",buff[i]);
				printf( "\n");

				for(int i = 20;i< bytes_left+20;i++)
					printf("%c",buff1[i]);
				printf("\n-------------------------###\n");*/
     }


 	/**
 	 * SEND RTP Message (for dummy messages)
 	 */
      int sqrkal_discovery::send_rtp_message(char *buff, int length )
      {

		if (length <= 0)
			return -1;

		char * point = new char[BUFSIZE];

		//Add headers
		if (rtp_hdset == 0)
			{cout << "NO RTP HEADER AVAILABLE?? THAT's BIZZARRE. BIG BUG.\n"; exit(-1);}

		memcpy(point,rtp_header,28);

		IP_Header* ipHdr  = (IP_Header*) point;
		struct UDP_Header* udpHdr = (struct UDP_Header*)(point + 20);
		udpHdr->source = htons(inport);
		udpHdr->destination = htons(outport);
		ipHdr->daddr = rtp_dest;
		ipHdr->saddr = rtp_src;
		udpHdr->length = htons(length+8);
		ipHdr->tot_len = htons(length+28);

		ipHdr->ttl=srpp_ttl;

		memcpy(point+28,buff,length);
		RTP_Header * rtpp = (RTP_Header *)(rtp_header+28);

		if(rtp_hdset == 2 && rtpp->version != 0)  // if we have rtp_header stored, and its not an invalid one.
		{
			memcpy(point+28,rtp_header+28,12);

			if (buff[1] == 0x7c)
			{
				// This is a signaling message and has to have a payload type 124
				*(point+29) = buff[1];
			}

		}
		else
		{
			if(rtp_hdset != 2)
				cout << "Havent seen a RTP Header yet? " << endl;
			else
				cout << "Version 0 RTP packet received and sent forward." << endl;
		}

		rtpp->seq = htons(lastSequenceNo);
		rtpp->ts = htonl(ntohl(rtpp->ts)+20);

		if (ssrc_value != 0)
			rtpp->ssrc = htonl(ssrc_value);

		cout << "Sequence: " << ntohs(rtpp->seq) << endl;
		//cout << "SSRC: " << ntohl(rtpp->ssrc) << endl;
		//cout << "PAYLOAD TYPE: " << rtpp->pt << endl;

		// RECALCULATE THE CHECKSUM
		thisinstance->form_checksums((char * )point);


		//set out_addr.
		out_addr.sin_addr.s_addr = rtp_dest;
		out_addr.sin_port = htons(outport);

		//send data
		int byytes = sendto(sip_socket, point, length+28, 0,
						 (struct sockaddr *)&(out_addr), sizeof(struct sockaddr));
		if (byytes < 0)
		{
			cout << "ERROR IN SENDING DATA: " << strerror(errno) << "\nLENGTH:" << length << endl << endl;
			for (int i = 0; i<length;i++)
			 printf("%c",buff[i]);
		}

		cout << "\nWriting SRPP " << byytes << " bytes \"" << inet_ntoa(out_addr.sin_addr) << ":" << ntohs(out_addr.sin_port) << " LEN:" << length << endl << endl;
		/*for (int i = 0; i<length;i++)
			 printf("%c",buff[i]);*/

		return byytes;
   	  }



      /**
       * RECEIVE RTP Message (for dummy messages)
       */
      SRPPMessage sqrkal_discovery::receive_rtp_message()
		{

     	    SRPPMessage dummy;
			unsigned char* buf = new unsigned char[BUFSIZE];

			// Read a packet from the queue
			int timeout = 300000000; // 5 minute
			int status = ipq_read(sqrkal_discovery_ipqh, buf, BUFSIZE, timeout);
			if (status < 0)
			{
				cerr << "ERROR while receiving RTP from ipqueue:" << ipq_errstr() << "\n";
				return dummy;
			}

			ipq_packet_msg_t * m = NULL;

			switch (ipq_message_type(buf))
			{

			 case NLMSG_ERROR:
				printf("Received error message %d\n",
				ipq_get_msgerr(buf));
				break;

			 case IPQM_PACKET:
				m = ipq_get_packet(buf);

				if (((int)m->data_len) <= 0)
				{
					cout << "Empty packet \n";
					break;
				}

				// DROP THIS PACKET, if not marked
				status = ipq_set_verdict(sqrkal_discovery_ipqh, m->packet_id, NF_DROP, m->data_len, buf);
				if (status < 0) {
					cerr << " PASSER while setting verdict for message \n ";
					break;
				}

				/*for (int i = 0; i < m->data_len; i++)
				{printf("%c",m->payload[i]);}*/

				// process the received packet
				if (srpp::isSignalingMessage ((char*)m->payload+28) == 1)
				{
					/*cout << "Signaling Message:\n";*/ return srpp::processReceivedData((char*)m->payload + 28, m->data_len-28);
				}
				else
					{/*cout <<"Not Signaling\n";*/ thisinstance->process_packet(m->payload, m->data_len);break;}


			}

			delete[] buf;

			return dummy;

	}



	/** Verify if this string is an IP ADDRESS */
	bool sqrkal_discovery::isValidIpAddress(const char *ipAddress)
	{
	    struct sockaddr_in sa;
	    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
	    return result != 0;
	}



	bool sqrkal_discovery::resolve_and_set(string hostname)
	{
		 struct hostent     *he;
		   if ((he = gethostbyname(hostname.c_str())) == NULL) {
			cerr << "Error resolving hostname:"<< hostname << " Error No.:" << h_errno << "\n";
			herror(hstrerror(h_errno));
			return false;
		   }

		   memcpy(&out_addr.sin_addr, he->h_addr_list[0], he->h_length);
	}


     /*****************             SRPP RELATED METHODS             ***************************/

	int sqrkal_discovery::initialize_srpp()
	{
		//initialize SRPP
 		srpp::init_SRPP();

 		//Create a SRPP Session with a particular CryptoProfile
 		CryptoProfile * crypto = new CryptoProfile("Simple XOR");
	    SRPPSession * newsession = srpp::create_session("127.0.0.1", inport,*crypto);//this is a dummy
 		cout << "SRPP started at time " << newsession->startTime << ".\n";

 		//cerr << "Press Enter to Start." << endl;
 		//getchar();
 		cerr << "SQRKal Started now.\n";

		srpp::setSendFunctor(send_rtp_message);
 		srpp::setReceiveFunctor(receive_rtp_message);
 		cout << "SET the required functors in SRPP.\n\n";


	}

 	//create SRPP session
 	int sqrkal_discovery::start_SRPP()
 	{
 		time_t start_time;
 		//get present date time
		struct tm * timeinfo;
		time ( &start_time );
		timeinfo = localtime ( &start_time );
		cout << "Signaling Started at " << asctime(timeinfo) << endl;

 		int status = srpp::start_session();
 		cout << "SIGNALING STATUS " << status << endl;

		time ( &start_time );
		timeinfo = localtime ( &start_time );
		cout << "Signaling Ended at " << asctime(timeinfo) << endl;

        if(srpp::isSignalingComplete() == 0)
        {
        	//cout << " SRPP NOT SUPPORTED AT OTHER ENDPOINT \n." << endl;
        	srpp::disable_srpp();
        	srpp::stop_session();

        	//stop_discovering
        	return -1;

        }
        else
        {
        	srpp::enable_srpp();
        	RTP_Header *rtpp = (RTP_Header *)(rtp_header+28);
			cout << "SETTING SEQUENCE NUMBER TO:" << ntohs(rtpp->seq) << endl;
			srpp::set_starting_sequenceno(ntohs(rtpp->seq));
        }

 		/*char data[40];
 		for (int i = 0; i < 120 ; i++)
		{
			sprintf(data,"Test message no. %d", i);

			//Pad it and make a SRPP message
			SRPPMessage srpp_msg = srpp::create_srpp_message(data);
			//send it through
			srpp::send_message(&srpp_msg);

			cout << "..Sending packet with data " << data << "..." << endl;

		}
 		 */
		return 0;

 	}


/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ END OF UTILITY FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~**/




/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ALL PROCESSING FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ **/


    /**
     * Parse the received/sent SIP message and its SDP payload
     */
	int sqrkal_discovery::parse_sip(string message, int direction){

		/**
		 * In order to reduce the overhead due to this processing, I am performing greedy parsing inside of applying the whole header.
		 * and applying SIP state machine. Instead I am searching for string relevant to the requirements - a] RTP port
		 */

		/******************************************************************************************/
		//    CHECK FOR TYPE OF SIP MESSAGE (worry only about INVITE, (200 OK OR ACK) and BYE)
		/******************************************************************************************/
		/** check for BYE first **/
		if (message.find("BYE ") !=string::npos)
		{
			//stop our session
			//cout << "\nGOT A BYE.. stop our session\n";

			if (saw_bye_already == 1)
				return -1;
			else
				saw_bye_already = 1;

			ofstream outfile;
			outfile.open ("sizes.txt");

			//Write the input and output packet lengths to file.
			for (int i = 0; i< input_packet_sizes.size(); i++)
			{
				outfile << input_packet_sizes[i] << "\t" << output_packet_sizes[i] << endl;
			}
			input_packet_sizes.clear();
			output_packet_sizes.clear();
			outfile.close();


			is_session_on = 0;

			if(apply_srpp == 1 )
				srpp::stop_session();


			add_all_rtp_rules(0,1); //remove rtp rules
			sent_count = 0;
			recv_count = 0;
			saw_invite_already = 0;
			saw_bye_already = 0;
			
			out_addr.sin_addr.s_addr = last_out_dest;



			return 0;

		}
		else if ( message.find("SIP/2.0 4") !=string::npos)   ///////TODO:handle 400 level errors correctly
		{ /** CHECK for 400 error message **/

			//stop the session
			//cout << "GOT A 400 error message. Stopping the session.\n";
			//srpp::stop_session();
			//is_session_on = 0;
			//saw_invite_already = 0;
			out_addr.sin_addr.s_addr = last_out_dest;
			return 0;

		}
		else if (message.find("200 OK") != string::npos || message.find("ACK ") != string::npos )
		{     		/** check if I received a 200 OK or a ACK message **/

			if (message.find("200 OK") != string::npos)
			{
				//cout << "\nGOT A 200 OK message\n";
				if ( sent_invite == 1 && direction == 0 )
				{
					saw_200ok = 1;
				}
			}
			else
			{
				//cout << "GOT AN ACK\n";
				if(saw_invite_already == 1 && message.find("Route:") != string::npos)
				{
					saw_ack =1;
					//cout << "Setting saw ack \n";
				}

			}

			//cout << message << endl;

			//check for any SDP content and parse it
			if(message.find("Content-Type: application/sdp") != string::npos)
			{
				unsigned int l,m;
				if ((l = message.find("m=audio ")) != string::npos)
				{
					m = message.find_first_of(' ',l+8);
					//cout << "Found audio at:" << l << "::" << m<< endl;
					//cout << message.substr(l+8,m-l-8) << endl;

					if (direction == 0) // INwards
					{
						outport = atoi((message.substr(l+8,m-l-8)).c_str());
						//cout << "SET OUTPORT TO " << outport << endl;
					}
					else
					{
						inport = atoi((message.substr(l+8,m-l-8)).c_str());
						//cout << "SET INPORT TO " << inport << endl;
					}
				}

				//check for srtp i.e RTP/SAVP
				if ((l = message.find("RTP/SAVP ",m) != string::npos) || (l = message.find("zrtp",m) != string::npos))
				{
					cout << "SRTP";
					is_srtp = 1;
				}
				else if (l = message.find("RTP/AVP ",m) != string::npos)
				{
					cout << "NOT SRTP\n";
					is_srtp = 0;
				}
				else if (l = message.find("SRPP:",m) != string::npos)
				{
					if (direction == 1) // OUTWARD
					{
						cout << "Your voip app uses SRPP. There is no need for SQRKAL.\n Exiting...."<< endl;
						stop_discovering(0);
						return 0;
					}
					cout << "SRPP\n";
					is_srpp = 1;
				}

			} // end of has-sdp check


			//start rtp session
			if ((!inport || !outport) )
			{
				cout << "Do not have both ports info.. IN:" << inport << " OUT :" << outport << "\n";
				return 0;
			}

			//cout << "BOTH ports info present.. IN:" << inport << " OUT :" << outport << "\n";

			if (is_session_on == 0  && saw_invite_already != 0)
			{
				// Get the receiver address from c=IN IP4...
				unsigned int l,m;
				if ((l = message.find("c=IN IP4 ")) != string::npos && saw_ack == 0)
				{
					m = message.find_first_of(" \n",l+9);

					if (direction == 0) // INwards
					{
						rtp_dest = inet_addr((message.substr(l+9,m-l-9)).c_str());
						//cout << "SET rtp_dest:" << (message.substr(l+9,m-l-9)) << " to " << rtp_dest << endl;
					}
					else
					{
						rtp_src = inet_addr((message.substr(l+9,m-l-9)).c_str());
					}
				}

				rtp_ports.push_back(inport);
				rtp_ports.push_back(outport);

				/* I should start the media session only if
				        a] I sent invite, received 200 OK and sent ack
				     or b] I received an invite, sent 200 OK and received ack

				*/
				if ((sent_invite == 0 && saw_ack == 1)
						|| (sent_invite == 1 && saw_200ok == 1 && saw_ack == 1))
				{
					add_all_rtp_rules(1,1);
					sent_invite = 0;
					saw_ack = 0;
					saw_200ok = 0;

				    saw_invite_already = 0;
				    is_session_on = 1;

				    //cout << "GOING TO START SRPP SESSION NOW >>>>\n\n";
				}

			}

		     out_addr.sin_addr.s_addr = last_out_dest;

			return 0;

		}
		else if (message.find("INVITE sip:") != string::npos && message.find("Content-Type: application/sdp") != string::npos)
		{    /** check for invite and SDP message **/

			cout << "\n THIS IS AN INVITE MESSAGE WITH SDP \n";
			//cout << message << endl;

			saw_invite_already = 1;

			// CHECK for m=audio %d RTP/AVP
			unsigned int l,m;
			if ((l = message.find("m=audio ")) != string::npos)
			{
				m = message.find_first_of(' ',l+8);

				if (direction == 0) // INwards
				{
					outport = atoi((message.substr(l+8,m-l-8)).c_str());
					cout << "SET OUTPORT TO " << outport << endl;
				}
				else
				{
					inport = atoi((message.substr(l+8,m-l-8)).c_str());
					cout << "SET INPORT TO " << inport << endl;
				}
			}

			//check for srtp i.e RTP/SAVP
			if (l = message.find("RTP/SAVP ",m) != string::npos || (l = message.find("zrtp",m) != string::npos))
			{
				cout << "SRTP";
				is_srtp = 1;
			}
			else if (l = message.find("RTP/AVP ",m) != string::npos)
			{
				cout << "NOT SRTP\n";
				is_srtp = 0;
			}
			else if (l = message.find("SRPP:",m) != string::npos)
			{
				if (direction == 1) // OUTWARD
				{
					cout << "Your voip app uses SRPP. There is no need for SQRKAL.\n Exiting...."<< endl;
					stop_discovering(0);
					return 0;
				}
				cout << "SRPP\n";
				is_srpp = 1;
			}


			// Get the receiver address from c=IN IP4...
			if ((l = message.find("c=IN IP4 ")) != string::npos)
			{
				m = message.find_first_of(" \n",l+9);

				if (direction == 0) // INwards
				{
					rtp_dest = inet_addr((message.substr(l+9,m-l-9)).c_str());
					//cout << "SINET rtp_dest:" << (message.substr(l+9,m-l-9)) << "to" << rtp_dest << endl;
				}
				else
				{
					rtp_src = inet_addr((message.substr(l+9,m-l-9)).c_str());
				}
			}




		  if (direction == 0) // INWARDS
		   {
			return 0;   // since we have already set the out_addr earlier
		   }

		   sent_invite = 1;

			// SET the other endpoint address by performing a DNS lookup on a hostname
			l = message.find("INVITE sip:");
			m = message.find("@",l+11);
			l = message.find(" ",m);
			string hostname = "sip."+message.substr(m+1,l-m-1);
			//cout << "SETTING DESTINATION ADDRESS TO SERVER:" << hostname << endl;


			/* resolve hostname to an IP  */
			if (isValidIpAddress(hostname.c_str()))
			{
			  out_addr.sin_addr.s_addr = inet_addr(hostname.c_str());
			}
			else
			{
				resolve_and_set(hostname);
			}
			out_addr.sin_family = AF_INET;
			out_addr.sin_port = htons(5060);
			last_out_dest = out_addr.sin_addr.s_addr;


		}
		else if (message.find("REGISTER sip:") != string::npos)
		{    /** check for REGISTER message **/

			//cout << " THIS IS A REGISTER MESSAGE \n";

			unsigned int l,m;
			l = message.find("REGISTER sip:");
			m = message.find(" ",l+13);
			string hostname = "sip."+ message.substr(l+13,message.find(" ",l+13)-l-13);
			//cout << "SETTING DESTINATION ADDRESS TO SERVER:" << hostname << endl;


			/* resolve hostname to an IP  */
			if (isValidIpAddress(hostname.c_str()))
			{
			  out_addr.sin_addr.s_addr = inet_addr(hostname.c_str());
			}
			else
			{
				resolve_and_set(hostname);
			}
			   out_addr.sin_family = AF_INET;
			   out_addr.sin_port = htons(5060);

			if (direction == 1) // Outgoing message
				last_out_dest = out_addr.sin_addr.s_addr;


		}
		else if (message.find("SUBSCRIBE sip:") != string::npos || message.find("PUBLISH sip:") != string::npos )
		{    /** check for SUBSCRIBE message **/


			unsigned int l,m;
		    if (message.find("SUBSCRIBE sip:") != string::npos)
		    {
			//cout << " THIS IS A SUBSCRIBE MESSAGE \n";
			l = message.find("SUBSCRIBE sip:");
		    }else{
			//cout << " THIS IS A PUBLISH MESSAGE \n";
			l = message.find("PUBLISH sip:");
		    }

			m = message.find("@",l+14);
			l = message.find_first_of(" :",m);
			string hostname = message.substr(m+1,l-m-1);

			//cout << "SETTING DESTINATION ADDRESS TO SERVER:" << hostname << endl;
			if (direction == 1 && isLocalIP(hostname) == 1) /** if we get an outgoing message which has localhost IP as subscribe **/
			{

			}
			/* resolve hostname to an IP  */
			if (isValidIpAddress(hostname.c_str()))
			{
			  out_addr.sin_addr.s_addr = inet_addr(hostname.c_str());
			}
			else
			{
				resolve_and_set(hostname);

			}
			   out_addr.sin_family = AF_INET;
			   out_addr.sin_port = htons(5060);

			if (direction == 1)
				last_out_dest = out_addr.sin_addr.s_addr;

		}
		else
		{
			out_addr.sin_addr.s_addr = last_out_dest;
		}


		return 0;

	}








	/** --------------- PROCESS A RECEIVED PACKET ------------------------**/
	int sqrkal_discovery::process_packet( unsigned char *buff, int bytes_read)
	{

		if (bytes_read < 28)
			return -1;

		int offset;
		uint16_t saddr		= 0;
		uint16_t daddr		= 0;
		uint16_t local_ip, remote_ip, local_port, remote_port;
		int testing = 1;

		 in_addr abc,abc1;

		 // APPLY IP HEADER
		 IP_Header* ipHdr  = (IP_Header*)(buff);

		// Parsing TCP/UDP header
		 offset = 4*ipHdr->ihl;
		if (ipHdr->protocol == UDP_PROTOCOL )
		{
			struct UDP_Header* udpHdr = (struct UDP_Header*) (buff + offset);
			saddr = ntohs(udpHdr->source);
			daddr = ntohs(udpHdr->destination);
			is_udp = 1;

		}
		else if (ipHdr->protocol == TCP_PROTOCOL)
		{
			struct TCP_Header* tcpHdr = (struct TCP_Header*) (buff + offset);
			saddr = ntohs(tcpHdr->source);
			daddr = ntohs(tcpHdr->destination);
			is_udp = 0;
		}
		else
		{
			return 0;
		}


		//--------------------CHECK IF ITS A SIP OR RTP PACKET---------------------------

		// Parse IP header
		int direction = get_direction(*ipHdr);
		if ( direction == 0) //inwards
		{
			local_ip = ipHdr->daddr;
			remote_ip = ipHdr->saddr;
			local_port = daddr;
			remote_port = saddr;
		}
		else if (direction == 1) // outwards
		{
			local_ip = ipHdr->saddr;
			remote_ip = ipHdr->daddr;
			remote_port = daddr;
			local_port = saddr;
		}
		else if (direction < 0)
		{
			//cerr << "WRONG PACKET:: NOT MY PACKET" << endl;
			return 0;
		}


		// parse this message to check if its a SIP message
		//ASSUMPTION: we receive SIP Messages before any RTP Messages are sent through... might need to change this later
		is_sip = 0; is_rtp = 0;

		//check if message is from or to a sip port
		for(int i = 0;i < sip_ports.size(); i++)
		{

			if (local_port == sip_ports[i] ||remote_port == sip_ports[i])
				 {
					is_sip = 1;
					break;
				 }
				 i++;
		}

		if(is_sip == 0)
		{
			 //check if message is from or to a rtp port
			 for (int j=0;j < rtp_ports.size();j++)
			 {
				 if (local_port == rtp_ports[j] ||remote_port == rtp_ports[j])
				 {
					is_rtp = 1;
					break;
				 }
				 j++;
			 }

		 }


		if (is_sip == 0 && is_rtp == 0)
		{
			//cerr << "NOT SIP OR RTP PACKET\n";
			return 0;
		}


		/** PRINT DETAILS ABOUT THE PACKET NOW **/
/*
		 abc.s_addr = ipHdr->saddr;
		 abc1.s_addr = ipHdr->daddr;

		 cout << "\n---------------------------------------------------------------\n";
		 cout << "TOS:"<< ntohs(ipHdr->tos) << "|" << bytes_read << " bytes FROM " << inet_ntoa(abc) << ":" << saddr
				 << " TO " << inet_ntoa(abc1) << ":" << daddr << endl;

		 if (direction == 1)
				cout << "OUTWARDS    -------------->>>>>>>>>>>\n";
		 else
				cout << "INWARDS     <<<<<<<<<<<<-------------\n";

*/

		if (is_sip == 1)
		{

			//HANDLE SIP
			offset += sizeof(struct UDP_Header);
			string str ((char *)buff+offset, bytes_read - offset);

			// parse SIP packet
			int parseresult = parse_sip(str,direction);

			//Forward the same packet now.
			if (out_addr.sin_port != 0 && parseresult >= 0)
			{

				if (direction == 0) // inwards
					out_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

				//cout << " WILL SEND this forward to " << inet_ntoa(out_addr.sin_addr) << ":" << out_addr.sin_port << endl;


				//SET the IP and UDP header appropriately
				offset -= sizeof(struct UDP_Header);
				struct UDP_Header* udpHdr = (struct UDP_Header*) (buff + offset);
				udpHdr->source = htons(5060);
				udpHdr->destination = out_addr.sin_port;
				ipHdr->daddr = out_addr.sin_addr.s_addr;

				//// Hack to get working in case zrtp present
				/*if (str.find("zrtp-hash") != string::npos && direction == 0)
				{
					unsigned int l = 0,m =0;
					m=str.find("zrtp-hash");

					while(m != -1)
					{
						l=m;
						//buff[l+28]='m';buff[l+32]='o';
						m=str.find("zrtp-hash",l+1);
					}
				}*/


				// MANGLE THE PACKET
				//ipHdr->tos= 32;
				ipHdr->ttl = srpp_ttl;

				// Handle fragmentation if its greater than the MTU
				if (bytes_read > IP_MTU)
				{
					send_fragmented_message((char*)buff,bytes_read);
				}
				else
				{
					// RECALCULATE THE CHECKSUM
					form_checksums((char * )buff);

					// send raw message
					send_raw_message((char *)buff,bytes_read);
				}

				//set out_addr.
				if(direction == 0) //inside
				{
					out_addr.sin_addr.s_addr = ipHdr->saddr;
				}


			}


			//set rtp_header
			if (rtp_hdset == 0)
			{
				memcpy(rtp_header,buff,28);rtp_hdset=1;
				/*for (int i = 0; i < 40; i++)
							printf("%x ",rtp_header[i]);*/
			}

		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//                                                       HANDLE RTP/SRTP
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		 if (is_rtp == 1)
		 {
			 /*RTP_Header * rtpp = (RTP_Header*) buff;

			 if(rtpp->version == 0)
			 {
				 cout << "Unknown version 0" << endl;
				 return 0;
			 }*/


		  /** PRINT DETAILS ABOUT THE PACKET NOW **/
		  abc.s_addr = ipHdr->saddr;
		  abc1.s_addr = ipHdr->daddr;

  		  cout << "\n---------------------------------------------------------------\n";
		 /* cout << "TOS:"<< ntohs(ipHdr->tos) << "|" << bytes_read << " bytes FROM " << inet_ntoa(abc) << ":" << saddr
				 << " TO " << inet_ntoa(abc1) << ":" << daddr << endl;*/

			 if (direction == 1)
					cout << "OUTWARDS    -------------->>>>>>>>>>>\n";
			 else
					cout << "INWARDS     <<<<<<<<<<<<-------------\n";

			if (direction == 0) // COMING IN
			{

				rtp_dest = ipHdr->saddr;
				recv_count ++;


				// check if its a signaling message
				if (srpp::isSignalingMessage ((char *)buff+28) == 1)
				{
					/*cout <<"Signaling\n";*/ srpp::processReceivedData((char*)buff + 28, bytes_read-28);

				}

				//set rtp_header
				if (rtp_hdset == 0 || rtp_hdset == 1 || rtp_header[29] & 0x80 != 0x80 )
				{memcpy(rtp_header,buff,40);rtp_hdset=2; /*cout << "SET\n";*/}

				/*for (int i = 0; i < 40; i++)
							printf("%x ",rtp_header[i]);*/

				//if we see a different port, then we start a new session
				if((outport != saddr && apply_srpp == 0) || (recv_count == 1 && sent_count < 5))
				{
					if (start_SRPP() >= 0)
					{
						cout << "Will APPLY SRPP FROM NOW ON...RECEIVE\n\n";
						apply_srpp = 1;

					}
				}

				if (apply_srpp == 1)
				{
					// RECEIVING A SRPP PACKET PROCESS ACCORDINGLY
					if (is_srtp == 0)
					{
						cout << " WE RECEIVED A RTP PACKET\n";

						//---------------- CONVERT THE RECEIVED SRPP Message to RTP Message -----------------
						int new_size = bytes_read - 28;

						if (srpp_msg.network_to_srpp((char *)buff+28,new_size,srpp::getKey()) >= 0)
						{

							//cout << "bytes read:" << bytes_read << endl;
							//srpp_msg.print();

							rtp_msg = srpp::srpp_to_rtp(&srpp_msg);
							//rtp_msg.print();


							new_size = sizeof(rtp_msg.rtp_header)-4*(15-ntohs(rtp_msg.rtp_header.cc)) + srpp_msg.encrypted_part.original_payload.size();
							//cout << "New Size:" << new_size << "\n";

							unsigned char * tmp_buff = buff; // no time.. stupid hack

							char rtp_buff[new_size+10];
							rtp_msg.rtp_to_network(rtp_buff,new_size);

							buff = tmp_buff;

							/*for (int i = 0; i < new_size; i++)
								printf("%x ", rtp_buff[i] );

							printf("\n***************************\n");*/


							memcpy(buff+28,rtp_buff,new_size);
							bytes_read = new_size+28;

							/*for (int i = 28; i < BUFSIZE; i++)
								printf("%x ", rtp_msg.payload[i] );

							printf("\n--------\n");*/

						}
					}
					else
					{
						cout << " WE RECEIVED A SRTP PACKET\n";
						cout << "KEY:" << srpp::getKey() << endl;

						//----------------CONVERT THE RECEIVED SRPP Message to SRTP Message-----------------
						int new_size = bytes_read - 28;

						if (srpp_msg.network_to_srpp((char *)buff+28,new_size,srpp::getKey()) >= 0)
						{

							//srpp_msg.print();
							new_size = srpp_msg.encrypted_part.original_payload.size();
							//cout << "New Size:" << new_size << "\n";

							char srtp_buff[new_size];
							if (srpp::srpp_to_srtp(&srpp_msg,srtp_buff,new_size) >= 0)
							{

								memcpy(buff+28,srtp_buff,new_size);
								bytes_read = new_size+28;

									/*for (int i = 28; i < new_size+28; i++)
										printf("%x ", buff[i] );

									printf("\n--------\n");*/
							}

						}

					}
				}

				//----------------------------send it forward---------------------------------------------
				//SET the IP and UDP header appropriately
				struct UDP_Header* udpHdr = (struct UDP_Header*) (buff + offset);
				udpHdr->destination = htons(inport);

				ipHdr->daddr = rtp_src;

				if (!rtp_src)
				{	cout << " RTP SOURCE not set yet " << rtp_src << endl; ipHdr->daddr = inet_addr("127.0.0.1");}

				abc.s_addr = ipHdr->saddr;
				//cout << "SRC: " << inet_ntoa(abc) << endl;
				abc1.s_addr = ipHdr->daddr;
				//cout << "DST: " << inet_ntoa(abc1) << endl;

				udpHdr->length = htons(bytes_read-20);
				ipHdr->tot_len = htons(bytes_read);

				// MANGLE THE PACKET
				ipHdr->ttl = srpp_ttl;

				//set out_addr.
				out_addr.sin_addr.s_addr = ipHdr->daddr;
				out_addr.sin_port = htons(inport);

				// Handle fragmentation if its greater than the MTU
				if (bytes_read > IP_MTU)
				{
					send_fragmented_message((char*)buff,bytes_read);
				}
				else
				{
					// RECALCULATE THE CHECKSUM
					form_checksums((char * )buff);

					// send raw message
					send_raw_message((char *)buff,bytes_read);
				}


			}
			else
			{

				 rtp_src = ipHdr->saddr;
				 if (!outport || !rtp_dest)
						 return 0;

				 sent_count ++;

				//if we see a different port, then we start a new session
				if((apply_srpp == 0) && sent_count == 1 && rtp_hdset > 0)
				{
					if (start_SRPP() >= 0)
					{
						cout << "Will APPLY SRPP FROM NOW ON...SEND\n\n";
						apply_srpp = 1;

					}
				}

				if (apply_srpp == 1)
				{
					input_packet_sizes.push_back(bytes_read);

					// PAD IT AND SEND SRPP PACKET ACCORDINGLY
					if (is_srtp == 0)
					{
						/*for (int i = 28; i < bytes_read; i++)
							printf("%x ", buff[i] );

						printf("\n******************************\n");*/


						cout << " WE SENT A RTP PACKET\n";
						RTP_Header* rtp_hdr = (RTP_Header *)(buff+28);

						if (ssrc_value == 0)
							ssrc_value = rtp_hdr->ssrc;

						//rtp_msg_p->print();

						srpp_msg = srpp::rtp_to_srpp(*rtp_hdr,(char*) buff+40 ,bytes_read-40);
						//srpp_msg.print();
						cout << "Sequence Number:  " << srpp_msg.srpp_header.seq << endl;


						int new_size = sizeof(srpp_msg.srpp_header) + srpp_msg.encrypted_part.original_payload.size()  +
								srpp_msg.encrypted_part.srpp_padding.size() + 3* sizeof(uint32_t);

						//cout << "bytes read:" << bytes_read << " Size::" << new_size << endl;

						char srpp_buff[new_size+10];
						srpp_msg.srpp_to_network(srpp_buff, srpp::getKey());

						memcpy(buff+28,srpp_buff,new_size);
						bytes_read = new_size+28;

						/*for (int i = 0; i < srpp_msg.encrypted_part.original_payload.size(); i++)
								printf("%x ", srpp_msg.encrypted_part.original_payload[i] );

						printf("\n--------\n");

						for (int i = 0; i < new_size; i++)
							printf("%x ", srpp_buff[i] );*/
						//printf("\n--------\n");

					}
					else
					{
						/*for (int i = 28; i < bytes_read; i++)
								printf("%x ", buff[i] );
						printf("\n--------\n");*/

						cout << " WE SENT A SRTP PACKET\n";
						RTP_Header* srtp_hdr = (RTP_Header *)(buff+28);

						if (ssrc_value == 0)
							ssrc_value = srtp_hdr->ssrc;

						srpp_msg = srpp::rtp_to_srpp(*srtp_hdr,(char*) buff+28 ,bytes_read-28); // we will keep the whole packet in payload
						//srpp_msg.print();

						int new_size = sizeof(srpp_msg.srpp_header) + srpp_msg.encrypted_part.original_payload.size()  +
								srpp_msg.encrypted_part.srpp_padding.size() + 3* sizeof(uint32_t);

						//cout << "bytes read:" << bytes_read << " Size::" << new_size << endl;

						char srpp_buff[new_size+10];
						srpp_msg.srpp_to_network(srpp_buff, srpp::getKey());


						memcpy(buff+28,srpp_buff,new_size);
						bytes_read = new_size+28;

						/*for (int i = 0; i < srpp_msg.encrypted_part.original_payload.size(); i++)
								printf("%x ", srpp_msg.encrypted_part.original_payload[i] );

						printf("\n--------\n");

						for (int i = 0; i < new_size; i++)
							printf("%x ", srpp_buff[i] );*/

						//printf("\n--------\n");


					}
				}

				output_packet_sizes.push_back(bytes_read);

				//---------------------------------send forward------------------------------------------
				//SET the IP and UDP header appropriately
				struct UDP_Header* udpHdr = (struct UDP_Header*) (buff + offset);
				udpHdr->destination = htons(outport);

				 abc.s_addr = ipHdr->saddr;
				//cout << "SRC: " << inet_ntoa(abc) << endl;
				ipHdr->saddr = ipHdr->saddr;
				ipHdr->daddr = rtp_dest;
				udpHdr->length = htons(bytes_read-20);
				ipHdr->tot_len = htons(bytes_read);

				// MANGLE THE PACKET
				ipHdr->ttl = srpp_ttl;

				//set rtp_header
				if (rtp_hdset == 0 || rtp_hdset == 1 || rtp_header[29] & 0x80 != 0x80 )
				{memcpy(rtp_header,buff,40);rtp_hdset=2;}
				/*for (int i = 0; i < 40; i++)
							printf("%x ",rtp_header[i]);*/

				//set out_addr.
				out_addr.sin_addr.s_addr = rtp_dest;
				out_addr.sin_port = htons(outport);

				// Handle fragmentation if its greater than the MTU
				if (bytes_read > IP_MTU)
				{
					send_fragmented_message((char*)buff,bytes_read);
				}
				else
				{
					// RECALCULATE THE CHECKSUM
					form_checksums((char * )buff);

					// send raw message
					send_raw_message((char *)buff,bytes_read);
				}


			}


		}
	}




	/** MAIN LOOP **/
	int sqrkal_discovery::discover_sessions()
	{
		srpp_ttl = 65 + rand()%25 + rand()%5;
		sprintf(srpp_ttl_s,"%d",srpp_ttl);

		//start socket to listen on our inward sip port 56789 and outward sip port 56790
		int addr_len, bytes_read;
		struct sockaddr_in receiver_addr , sender_addr;

		//SET THESE SOCKETS IN SRPP

		//Create Sockets
		create_sockets();

		//Add the firewall rules
		add_all_rules(1,1);

		//Initialize SRPP
		initialize_srpp();

		int status;
		unsigned char* buf = new unsigned char[BUFSIZE];

		unsigned long long int start_time = 0;
		unsigned long long int curr_time = 0;

		in_addr abc,abc1;

		/*-------------------------------------------- DISCOVER PART ------------------------------------- */




		is_discovering = 1;

		//See if we receive any packets
		while(is_discovering == 1)
		{
			cout << ".";
			signal(SIGINT,sqrkal_discovery::stop_discovering);

			// Read a packet from the queue
			status = ipq_read(sqrkal_discovery_ipqh, buf, BUFSIZE, 0);
			if (status < 0)
				{
				cerr << "ERROR while receiving packet from ipqueue::" << ipq_errstr() << "\n";
				printf("Received error message %d\n",ipq_get_msgerr(buf));
				//return -1;
				}


			switch (ipq_message_type(buf)) {
		 	 case NLMSG_ERROR:
				printf("Received error message %d\n",
				ipq_get_msgerr(buf));
				break;


			 case IPQM_PACKET: {
				ipq_packet_msg_t *m = ipq_get_packet(buf);

				if (((int)m->data_len) <= 0)
					continue;

				// DROP THIS PACKET, if not marked
				status = ipq_set_verdict(sqrkal_discovery_ipqh, m->packet_id, NF_DROP, m->data_len, buf);
				if (status < 0) {
				cerr << " PASSER while setting verdict for message \n ";
				break;
				} else
				{
				 //cout << "\nDROPPED QUEUED PACKET with id " << m->packet_id << endl;
				}

				// process the received packet
				process_packet(m->payload, m->data_len);


				break;

			}


			default:
					printf("Unknown message type!\n");
					break;

			} // end of switch



		}


		//delete buf;





	}


	/** STOP DISCOVERY **/
	void sqrkal_discovery::stop_discovering(int i)
	{
		if (apply_srpp == 1)
			{srpp::stop_session(); }

 		is_discovering = 0;
		close(sip_socket);

		 // Remove the rules
		thisinstance->add_all_rules(0,1);
		//thisinstance->add_all_rtp_rules(0,1);

		cout << "SQRKAL STOPPED DISCOVERING NOW \n" ;
		signal(SIGINT,SIG_DFL);

	}
	//CONSTRUCTOR and DESTRUCTOR
	sqrkal_discovery::sqrkal_discovery()
	{
		thisinstance = this;
		discover_sessions();

	}

	sqrkal_discovery::~sqrkal_discovery()
	{
		/****************** CLEANUP STUFF ********************************/
				stop_discovering(0);
	}


/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ END OF PROCESSING FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~**/


//} // End of namespace

