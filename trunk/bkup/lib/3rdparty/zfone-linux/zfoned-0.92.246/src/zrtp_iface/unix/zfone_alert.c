/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Viktor Krikun <v.krikun@soft-industry.com>
 */

#include <unistd.h>
#include <pthread.h>

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone alert"

#if (ZRTP_PLATFORM == ZP_LINUX)	 // Linux implementation throught the RTP stream

// default RTP packet payload size for pcmu.
#define ZRTP_SND_PAYLOAD_SIZE		160

#define IP_UDP_RTP_HDRS_SIZE		40
#define UDP_RTP_HDRS_SIZE			20

#define ZRTP_MAX_ALERT_NAME_LENGTH	16
static const char clear_alert_file[]  = "clear.pcmu";
static const char secure_alert_file[] = "secure.pcmu";
static const char error_alert_file[] = "amiss.pcmu";
static const char inactive_alert_file[] = "inactive.pcmu";

static char	alert_dir_path[256];
static uint32_t	alert_dir_len = 0;


//------------------------------------------------------------------------------
int zrtph_init_alert(const char* alert_dir_name)
{
    int test_path_length = sizeof(alert_dir_path)+ZRTP_MAX_ALERT_NAME_LENGTH;
	char *test_path = zrtp_sys_alloc(test_path_length);
    FILE* file	= NULL;

    zrtp_print_log_delim(3, LOG_START_SUBSECTION, "alert player initialization");

	if (!test_path)
	{
		ZRTP_LOG(1, (_ZTU_, "zrtp_init_alert: failed to allocate buffer\n"));
		return zrtp_print_log_delim_and_exit(3, LOG_START_SUBSECTION, NULL, -5);
	}
    
    ZRTP_LOG(3, (_ZTU_, "zrtp_init_alert: looking for alert sounds files at %s...\n", alert_dir_name));
    
    alert_dir_len = strlen(alert_dir_name);
    if (alert_dir_len > sizeof(alert_dir_path)-1 )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtp_init_alert: alert_dir_name is too long."
					" Must be shorter than %d.\n", sizeof(alert_dir_path)-1));
		zrtp_sys_free(test_path);
		return zrtp_print_log_delim_and_exit(3, LOG_START_SUBSECTION, NULL, -1);
    }
    
    memset(alert_dir_path, 0, sizeof(alert_dir_path));
    strncpy(alert_dir_path, alert_dir_name, alert_dir_len);
    
    // check alert sound files
    snprintf(test_path, test_path_length, "%s%s", alert_dir_path, clear_alert_file);
    ZRTP_LOG(3, (_ZTU_, "zrtp_init_alert: looking for the clear sound at:%s...\n", test_path));
    if ( NULL == (file = fopen(test_path, "r")) )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtp_init_alert: can't open clear alert file: %s.\n", test_path));
		zrtp_sys_free(test_path);
		return zrtp_print_log_delim_and_exit(3, LOG_START_SUBSECTION, NULL, -2);
    }
    fclose(file);
    
    sprintf(test_path, "%s%s", alert_dir_path, secure_alert_file);
    ZRTP_LOG(3, (_ZTU_, "zrtp_init_alert: looking for the secure sound at:%s...\n", test_path));
    if ( NULL == (file = fopen(test_path, "r")) )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtp_init_alert: can't open secure alert file: %s.\n", test_path));
		zrtp_sys_free(test_path);
		return zrtp_print_log_delim_and_exit(3, LOG_START_SUBSECTION, NULL, -3);
    }
    fclose(file);
    
    sprintf(test_path, "%s%s", alert_dir_path, error_alert_file);
    ZRTP_LOG(3, (_ZTU_, "zrtp_init_alert: looking for the error sound at:%s...\n", test_path));
    if ( NULL == (file = fopen(test_path, "r")) )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtp_init_alert: can't open error alert file: %s.\n", test_path));
		zrtp_sys_free(test_path);
		return zrtp_print_log_delim_and_exit(3, LOG_START_SUBSECTION, NULL, -4);
    }
    fclose(file);
    
    zrtp_print_log_delim(3, LOG_END_SUBSECTION, "alert player initialization");
        
	zrtp_sys_free(test_path);
    return 0;
}
 
//------------------------------------------------------------------------------
static void* play(void* param)
{
	zfone_stream_t *zfone_stream = (zfone_stream_t*) param;
	zfone_media_stream_t *media = NULL;
    
    uint8_t packet[ZRTP_SND_PAYLOAD_SIZE+100];
    uint16_t packet_length	= 0;
        
    FILE* file 					= NULL;
    uint8_t* alert_buffer		= NULL;
    uint8_t* alert_buffer_del	= NULL;
    int	 alert_buffer_length	= 0;
    
	uint32_t timestamp			= 0;
    uint32_t rtp_seq			= 0;
    
    char *test_path;
	int test_path_length = sizeof(alert_dir_path)+ZRTP_MAX_ALERT_NAME_LENGTH;
    
    struct zrtp_iphdr* ipHdr	= (struct zrtp_iphdr*)packet;
    struct zrtp_udphdr* udpHdr	= (struct zrtp_udphdr*)(packet + 20); // addr + IP_hdr_size
    zrtp_rtp_hdr_t* rtpHdr 		= (zrtp_rtp_hdr_t*)(packet + 20 + 8); // addr + IP + UDP hdrs
	
	if ( !(test_path = zrtp_sys_alloc(test_path_length)) )
		goto ZRTP_PLAY_EXIT;	  
	
	// We don't need sound laert for the Video channel.	
	if (zfone_stream->type == ZFONE_SDP_MEDIA_TYPE_VIDEO)
	{
		ZRTP_LOG(3, (_ZTU_, "zrtph_play_alert: Skip alert playing for Video stream %p.\n", zfone_stream));
		goto ZRTP_PLAY_EXIT;		
	}
    
	media = (zfone_stream->out_media.type != ZFONE_SDP_MEDIA_TYPE_UNKN) ?
			&zfone_stream->out_media : &zfone_stream->in_media;
        
    zrtp_print_log_delim(3, LOG_START_SUBSECTION, "Alert playing");
    ZRTP_LOG(3, (_ZTU_, "zrtph_play_alert: start playing alert sound alert_type=%u ssrc=%u.\n",
				 zfone_stream->alert_code, zrtp_ntoh32(zfone_stream->alert_ssrc)));
    
    // open necesasry alert sound file
    memset(test_path, 0, test_path_length);
    strncpy(test_path, alert_dir_path, alert_dir_len);
    switch (zfone_stream->alert_code)
    {
	case ZRTP_ALERT_PLAY_SECURE:
	    strncpy(test_path+alert_dir_len, secure_alert_file, sizeof(secure_alert_file));
		break;
	case ZRTP_ALERT_PLAY_CLEAR:
	    strncpy(test_path+alert_dir_len, clear_alert_file, sizeof(secure_alert_file));
		break;
	case ZRTP_ALERT_PLAY_ERROR:
	    strncpy(test_path+alert_dir_len, error_alert_file, sizeof(secure_alert_file));
		break;
	case ZRTP_ALERT_PLAY_NOACTIVE:
	    strncpy(test_path+alert_dir_len, inactive_alert_file, sizeof(secure_alert_file));
		break;
	default:
	    ZRTP_LOG(1, (_ZTU_, "zrtph_play_alert: Unsupported alert tupe:%d.\n", zfone_stream->alert_code));
	    goto ZRTP_PLAY_EXIT;
    }
    
    if ( NULL == (file = fopen(test_path, "r")) )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtph_play_alert: Can't open alert sound file:%s.\n", test_path));
		goto ZRTP_PLAY_EXIT;
    }
    
    // read pcmu data from raw file
    if ( 0 != fseek(file, 0, SEEK_END) )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtph_play_alert: Can't calculate %s file size. (first fseek).\n", test_path));
		goto ZRTP_PLAY_EXIT;
    }
    alert_buffer_length = ftell(file);
    if (0 == alert_buffer_length)
    {
		ZRTP_LOG(1, (_ZTU_, "zrtph_play_alert: Can't calculate %s file size."
						"(ftell return alert_buffer_length = 0).\n", test_path));
		goto ZRTP_PLAY_EXIT;
    }
    if ( 0 != fseek(file, 0, SEEK_SET) )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtph_play_alert: Can't calculate %s file size. (second fseek).\n", test_path));
		goto ZRTP_PLAY_EXIT;
    }
    
    if ( NULL == (alert_buffer = zrtp_sys_alloc(alert_buffer_length)) )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtph_play_alert: Can't allocate memory for alert sound playing.\n"));
		goto ZRTP_PLAY_EXIT;
    }
    alert_buffer_del = alert_buffer;
    
    if ( 0 == fread(alert_buffer, alert_buffer_length, 1, file) )
    {
		ZRTP_LOG(1, (_ZTU_, "zrtph_play_alert: ERROR! Can't read alert sound from file %s.\n", test_path));
    }
            
    memset(packet, 0, sizeof(packet));
    
    // fill IP header
    ipHdr->ihl		= 5;
    ipHdr->version 	= 4;
    ipHdr->frag_off = 0x40;
    ipHdr->ttl 		= 64;
    ipHdr->protocol = 17;
    ipHdr->saddr	= media->remote_ip;
    ipHdr->daddr 	= media->local_ip;
    ipHdr->tot_len  = zrtp_hton16(ZRTP_SND_PAYLOAD_SIZE + IP_UDP_RTP_HDRS_SIZE);	

    udpHdr->source 	= media->remote_rtp;
    udpHdr->dest 	= media->local_rtp;
    udpHdr->len 	= zrtp_hton16(ZRTP_SND_PAYLOAD_SIZE + UDP_RTP_HDRS_SIZE);

    rtpHdr->version  	= 2;
    rtpHdr->pt       	= 0; // PCMU
    rtpHdr->ssrc		= zfone_stream->alert_ssrc;

    // sending sound
    while (alert_buffer_length > 0)
    {
		zrtp_memcpy(packet + IP_UDP_RTP_HDRS_SIZE, alert_buffer,
				 ((alert_buffer_length > ZRTP_SND_PAYLOAD_SIZE) ? ZRTP_SND_PAYLOAD_SIZE : alert_buffer_length) );
		alert_buffer_length -= ZRTP_SND_PAYLOAD_SIZE;
		alert_buffer += ZRTP_SND_PAYLOAD_SIZE;
		
		rtpHdr->seq = zrtp_hton16(rtp_seq++);
		rtpHdr->ts  = zrtp_hton32(timestamp);
		
		// calculate checksum and send packet
		packet_length = ZRTP_SND_PAYLOAD_SIZE + IP_UDP_RTP_HDRS_SIZE;
		
		zfone_insert_cs((char*)packet, packet_length);
		
		zfone_network_send_ip((char*)packet, packet_length, ipHdr->daddr);
		
		// add delay after packet sending and change timestamp
		usleep(ZRTP_SND_PAYLOAD_SIZE*100);
		timestamp   += (uint32_t) ZRTP_SND_PAYLOAD_SIZE;
    }

ZRTP_PLAY_EXIT:
    if (file)
		fclose(file);
    if (alert_buffer_del)
		zrtp_sys_free(alert_buffer_del);
	if (test_path)
  		zrtp_sys_free(test_path);
	
    // unblock outgoing RTP stream
    ZRTP_LOG(3, (_ZTU_, "zrtph_play_alert: unblock outgoing RTP stream.\n"));
	zfone_stream->alert_code = ZRTP_ALERT_PLAY_NO;
    
    zrtp_print_log_delim(3, LOG_START_SUBSECTION, "Alert playing");
    
    // stop thread
    pthread_exit(NULL);
    
    return NULL;
}

#else	// Mac OS implementation

//------------------------------------------------------------------------------
int zrtph_init_alert(const char* alert_dir_name)
{
    return 0;
}

//------------------------------------------------------------------------------
extern void zrtph_alert_play(zfone_stream_t* stream);

static void* play(void* param)
{
	zfone_stream_t *zfone_stream = (zfone_stream_t*) param;
    unsigned int play_time = 0;

    zrtp_print_log_delim(3, LOG_START_SUBSECTION, "Alert playing");

	// We don't need sound alert for the Video channel.	
	if (zfone_stream->type == ZFONE_SDP_MEDIA_TYPE_VIDEO)
	{
		ZRTP_LOG(3, (_ZTU_, "zrtph_play_alert: Skip alert playing for Video stream.\n"));
		return NULL;
	}

    ZRTP_LOG(3, (_ZTU_, "zrtph_play_alert: start alert sound playing"
					" ctx=%p alert_type = %u.\n",zfone_stream, zfone_stream->alert_code));

    switch (zfone_stream->alert_code)
    {
	case ZRTP_ALERT_PLAY_SECURE:
	    play_time = 500*1000*2;
		break;
	case ZRTP_ALERT_PLAY_CLEAR:
	     play_time = 380*1000*2;
		 break;
	case ZRTP_ALERT_PLAY_ERROR:
	     play_time = 1440*1000*2;
		 break;
	case ZRTP_ALERT_PLAY_NOACTIVE:
	     play_time = 500*1000*2;
		 break;
	default:
	     ZRTP_LOG(1, (_ZTU_, "zrtp_play_alert: Unsupported alert tupe:%d.\n", zfone_stream->alert_code));	
	     return NULL;
    }
    
    // call Mac daemon for alert sound playing
    zrtph_alert_play(zfone_stream);
    
    // sleep for alert sound playing
    usleep(play_time);
    
    // unblock outgoing RTP stream
    ZRTP_LOG(3, (_ZTU_, "zrtph_play_alert: UNBLOCK OUTGOING RTP STREAM.\n"));
    zrtp_print_log_delim(3, LOG_START_SUBSECTION, "Alert playing");
	zfone_stream->alert_code = ZRTP_ALERT_PLAY_NO;

	return NULL;
}

#endif 

//------------------------------------------------------------------------------
void zrtp_play_alert(zfone_stream_t* stream)
{
    pthread_t thread;	
	
	if(stream->zrtp_stream && stream->zrtp_stream->zrtp) {
		uint32_t ssrc = 0;
		zrtp_randstr(stream->zrtp_stream->zrtp, (void*)&ssrc, sizeof(ssrc));
		stream->alert_ssrc = ssrc;
	}
	
    pthread_create(&thread, NULL, play, (void*) stream);
}
