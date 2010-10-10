/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */


#if ZRTP_PLATFORM != ZP_WIN32_KERNEL
#include <limits.h>
#include <errno.h>
#endif
#include <string.h>

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone parser"

//! brief parse SDP message
static int32_t parse_sdp(const char* buff, uint32_t length, zfone_sdp_message_t* sdp_message);

/*!
 * \brief SIP methods names
 * \warning Don't forget to change \ref zfone_sip_methods_t if you want
 *          to add some extra method to process.
 */
static const char *zfone_sip_methods[] =
{
    "<Invalid method>",      // pad so that the real methods start at index 1
    "ACK",
    "BYE",
    "CANCEL",
    "INVITE",
};

#define SUPPORTED_SIP_METHODS_COUNT 5

#define SDP_BODY_TYPE 	"application/sdp"
#define SDP_BODY_TYPE_LEN 15

//! Structure for SIP headers names definition
typedef struct
{
    const char *name;			//! full header name
    const char *compact_name;	//! short header name
} zfone_sip_header_t;

/*!
 * \brief SIP headers names
 * \warning Don't forget to change \ref zfone_sip_headers_t if you want
 *          to add some extra headers to process.
 */
static zfone_sip_header_t zfone_sip_headers[] =
{
    { "Unknown-header",	NULL }, //! 0 Pad so that the real headers start at index 1
    { "Call-ID", 		"i"  },
    { "CSeq", 			NULL },
    { "Content-Length",	"l"  },
    { "Content-Type", 	"c"  },
    { "From", 			"f"  },
    { "To",				"t"  },
    { "User-Agent",		NULL }
};

#define SUPPORTED_SIP_HEADERS_COUNT 8


// SIP Parsing flags
#define SIP_FROM_BIT	0x01
#define SIP_TO_BIT		0x02
#define SIP_ID_BIT		0x04
#define SIP_CSEQ_BIT	0x08
#define SIP_CT_BIT		0x10
#define SIP_CL_BIT		0x20

#define SIP_HEADER_BIT_VERIF	0xf

zfone_sip_type_t parse_start_line( const char *buff,
								   uint32_t depth,
								   zfone_start_line_t* start_line );

static zfone_sip_methods_t get_method_by_name(const char* buff, uint32_t len);
static zfone_sip_headers_t get_header_by_name(const char* buff, uint32_t len);

static void print_sip_message(zfone_sip_message_t* msg);


//=============================================================================
//    Zfone SIP/SDP parsing main part
//=============================================================================

//-----------------------------------------------------------------------------
int zfone_sip_parse(zfone_packet_t* packet)
{
    uint32_t bytes_left = 0;
    uint32_t offset = 0;
    
    char* sline = NULL;
    char* eline = NULL;
    int32_t line_len = 0;
    
    char*  stok	= NULL;
    uint32_t tok_len = 0;
    
    zfone_sip_message_t* sip_message = NULL;
    
    if (!packet)    
		return -1;    
    
    sip_message = (zfone_sip_message_t* ) packet->extra_data;
    
    // get pointer to the SIP packet buffer and it's size
    sline = (char*)packet->packet + packet->offset;
    bytes_left = packet->size - packet->offset;


    if (bytes_left <= MIN_SIP_MESSAGE_SIZE)
    {
		return -1; // to small for SIP packet
    }
    
    zrtp_print_log_delim(3, LOG_START_SELECT, "process SIP packet");
    
    // prepare packet for parsing
    sip_message->user_agent = ZFONE_AGENT_UNKN;
    
    line_len = zfone_find_line_end(sline, bytes_left, &eline);
    if (line_len <= 0)
    {
		return -2; // can't get status line
    }
    
    if (ZFONE_SIP_UNKNOWN ==  parse_start_line(sline, line_len, &sip_message->start_line))
    {
		return -3; // it's not a SIP at all or unused SIP packet
    }
    bytes_left -= line_len;
    
	sip_message->content_length_start = 0;

    // start headers parsing
    while (1)
    {
		zfone_sip_headers_t header_type = ZFONE_SIP_HEADER_UNKNOWN;
	
		sline = eline;
		line_len = zfone_find_line_end(sline, bytes_left, &eline);
		bytes_left -= line_len;
	
		if (2 > line_len)
		{		
	  		return -4; // can't find CRLF at the end of the line		
		}
	    
		if ((2 == line_len) && !strncmp(sline, "\r\n", 2))
		{
			sline = eline;
	  		break; // this is a blank line separating the message header from the message body.
		}
	
		while (eline && ((char)*eline == ' ' || (char)*eline == '\t'))
		{
	    	// This line end is not a header seperator. It just extends the header
	    	//  with another line. Try to get fool header.
	    	stok = eline;
	    	tok_len = zfone_find_line_end(stok, bytes_left, &eline);
	    	bytes_left -= tok_len;
	    	line_len += tok_len;
		}
		// So now:
		// sline    	- begining of the header (first string)
		// eline    	- header end (end of the last string)
		// line_len	- header length (all strings)
		// bytes_left 	- bytes left in buffer after all header strings
	
		// get header name
		stok = zfone_get_next_token(sline, line_len, " :", 0, &offset, &tok_len);
		if ( !stok )
		{
	  		return -5; // can't get header name
		}
	
		// check for colon after the field name
		if ( NULL == zfone_find_char(stok, line_len-offset, ":", NULL) )
		{
	  		return -6; // wrong header fiels - no colon after the name		 
		}
	
		// determine header type
		header_type = get_header_by_name(stok, tok_len);
		if (ZFONE_SIP_HEADER_UNKNOWN == header_type)
		{
	  		continue; // skip unknown or unsupported headers
		}
		stok += tok_len;
		line_len -= offset + tok_len;
		
		// get header value
		stok = zfone_get_next_token(stok, line_len, " :;\t\r\n", 0, &offset, &tok_len);
		if ( !stok )
		{
	  		return -7; //can't get headerv value
		}
		line_len -= offset;
	
		switch ( header_type )
		{
	  		case ZFONE_SIP_HEADER_CALLID:
	  		{	    
				// copy Call-ID value to zfone sip-message body
				if (tok_len > MAX_CALL_ID_SIZE)
				{
		  			return -8; //Call-Id is to long
				}
				strncpy(sip_message->call_id, stok, tok_len);
		
				sip_message->parsed_flags |= SIP_ID_BIT;
			}break;
	    
	  		// the sequence number MUST be expressible as a 32-bit unsigned integer.
	  		// the method part of CSeq is case-sensitive.
	  		case ZFONE_SIP_HEADER_CSEQ:
	  		{
				// parse Cseq number
	  			if (0 > zfone_strtoui(stok, tok_len, &sip_message->cseq.seq))
				{
		  			return -9; // can't get Cseq number
				}
				stok += tok_len;
				line_len -= tok_len;
			
				// parse name part
				stok = zfone_get_next_token(stok, line_len, " \r\n", 0, NULL, &tok_len);
				if ( !stok )
				{
		  			return -10; //can't get CSeq method part
				}
				sip_message->cseq.name = get_method_by_name(stok, tok_len);
		
				sip_message->parsed_flags |= SIP_CSEQ_BIT;
	  		}break;
	    
	  		case ZFONE_SIP_HEADER_CONTENT_LENGTH:
	  		{			
				if (0 > zfone_strtoui(stok, tok_len, &sip_message->clength))
				{
		  			return -11; // can't convert
				}
				//WIN64
				sip_message->content_length_start = (uint32_t)((char*)stok - (char*)packet->packet);
		
				sip_message->parsed_flags |= SIP_CL_BIT;
	  		}break;
	    
	  		case ZFONE_SIP_HEADER_CONTENT_TYPE:
	  		{
				// copy Call-ID value to zfone sip-message body
				if (tok_len != SDP_BODY_TYPE_LEN)
				{
		  			return -12; // wrong SDP body type length
				}
				
				if ( !zfone_str_nocase_cmp(stok, SDP_BODY_TYPE, SDP_BODY_TYPE_LEN))
				{
		  			sip_message->ctype = ZFONE_SIP_BODY_SDP;
				}
				else
				{
		  			sip_message->ctype = ZFONE_SIP_BODY_UNKNOWN;
				}		
		
				sip_message->parsed_flags |= SIP_CT_BIT;
	  		}break;
	    
	  		case ZFONE_SIP_HEADER_FROM:
	  		case ZFONE_SIP_HEADER_TO:
	  		{
				// if we have a SIP/SIPS uri enclosed in <> - everything before uri is display name
				if ( zfone_find_char(stok, line_len-offset, "<", &offset) )
				{
		  			// id there is something info arount of URI. Store this token like display-name
		  			if (offset > 0)
		  			{
						char* name_ptr = stok;
						// strip quotes from SIP name
						name_ptr = zfone_get_next_token(stok, offset, " :<\"", 0, NULL, &tok_len);
						if (!name_ptr)
						{
			  				return -13; //can't get display name
						}
		    
						if (tok_len > MAX_SIP_URI_SIZE)
						{
			  				return -13; //to long display name
						}
			
						if (ZFONE_SIP_HEADER_FROM == header_type)
			  				strncpy(sip_message->from.name, name_ptr, tok_len);
						else
			  				strncpy(sip_message->to.name, name_ptr, tok_len);


		  			}
		  			stok += offset+1; //pointed to the next character after "<"
		  			line_len -= offset+1;
		
		  			// RFC3261 sec. 20:
		  			// The Contact, From, and To header fields contain a URI.  If the URI
		  			// contains a comma, question mark or semicolon, the URI MUST be enclosed
		  			// in angle brackets (< and >).  Any URI parameters are contained within
		  		    // these brackets.  If the URI is not enclosed in angle brackets, any
		  			// semicolon-delimited parameters are header-parameters, not URI parameters.
		  			stok = zfone_get_next_token(stok, line_len, ">,;?", 0, &offset, &tok_len);
		  			if (!stok)		    
		  			{
						return -14; // can't find URI end
		  			}
		  			line_len -= offset;
				}
		
				// extract SIP URI
				if (tok_len > MAX_SIP_URI_SIZE)
				{
		  			return -15; // to long SIP URI
				}
		
				if (ZFONE_SIP_HEADER_FROM == header_type)
				{
		  			strncpy(sip_message->from.uri, stok, tok_len);    
		  			sip_message->parsed_flags |= SIP_FROM_BIT;
				}
				else
				{    
		  			strncpy(sip_message->to.uri, stok, tok_len);    
		  		    sip_message->parsed_flags |= SIP_TO_BIT;
				}		
	  		}break;
	    	    
	  		case ZFONE_SIP_HEADER_AGENT:
	  		{
				// We don't now format of Gizmo name for sure, so looking just for the
				// part of the name at first
				if (NULL != zfone_find_str(stok, tok_len+1, ZFONE_AGENT_GIZMO_NAME, NULL))
				{
		  			// this is a packet from Gizmo user agent - mark packet
		  			sip_message->user_agent = ZFONE_AGENT_GIZMO;
				}
				else if (NULL != zfone_find_str(stok, tok_len+1, ZFONE_AGENT_ICHAT_NAME, NULL))
				{
		  			// this is a packet from Gizmo user agent - mark packet
		  			sip_message->user_agent = ZFONE_AGENT_ICHAT;
				}
/*
				TIVI doesnt use this attribute
				else if (NULL != zfone_find_str(stok, tok_len+1, ZFONE_AGENT_TIVI_NAME, NULL))
				{
		  			// this is a packet from Gizmo user agent - mark packet
					sip_message->user_agent = ZFONE_AGENT_TIVI;
				}
*/
			}
	  		default: break;
		}
    }
    
    // What have been parsed?
    
    // Check main headers 
    if ( !(sip_message->parsed_flags & SIP_HEADER_BIT_VERIF) )		
		return -25; // not all obligatory were parsed
    
    // Parse SDP body if Content Length not zero
    if ((sip_message->parsed_flags & SIP_CT_BIT) && (sip_message->parsed_flags & SIP_CL_BIT))
    {
		if ((ZFONE_SIP_BODY_SDP  == sip_message->ctype) && (sip_message->clength > 0)) 
		{
	  		int32_t sdp_status = -1;
	
	  		// Check SDP mesasge length
	  		if (sip_message->clength > bytes_left)	  		
				return -31; // wrong SDP message size	  		
	
	  		// Parse SDP message
	  		sdp_status = parse_sdp(sline, sip_message->clength, &sip_message->body);
	  		if (sdp_status < 0)	  		
				return sdp_status; // error during SDP parsing	  		
	    
			if ( sip_message->body.zrtp_offset )		
			{
				//WIN64
				sip_message->body.zrtp_offset += (uint32_t)(sline - (char*)packet->packet);
			}

			// check all parsed fields
	  		if (sip_message->body.active_streams > 0)
	  		{
				// TODO: restore this when IP=0.0.0.0 will be a appropriate value for the parser
				/*
				if (0 == sip_message->body.contact.base)
	  			{
		  			uint32_t i=0;
		    
		  			// There is no connection information in Session part. Try to find it
		  			// every media sections.
		  			for (i=0; i<sip_message->body.active_streams; i++)
		  			{
						if (0 == sip_message->body.streams[i].contact.base)
						{
			  				return -60; // on of the media sections haven't connection info
						}
		  			}
				}
				*/
	  		}
	  		else
	  		{
				return -61; // SDP parset can't find any media streams
	  		}
		}
    }
    
    // every fields was parsed successfully - calculate session ID
    {
		sha256_ctx sha_ctx;
		uint8_t	sha_buff[64];
		
		sha256_begin(&sha_ctx);
		sha256_hash((unsigned char*)sip_message->call_id, strlen(sip_message->call_id), &sha_ctx);
		
		if ( ((ZFONE_IO_IN == packet->direction) && (ZFONE_SIP_REQUEST == sip_message->start_line.type)) ||
			((ZFONE_IO_OUT == packet->direction) && (ZFONE_SIP_RESPONSE == sip_message->start_line.type)) )
		{
			sha256_hash((unsigned char*)sip_message->to.uri, strlen(sip_message->to.uri), &sha_ctx);
			sha256_hash((unsigned char*)sip_message->from.uri, strlen(sip_message->from.uri), &sha_ctx);	
		}
		else if ( ((ZFONE_IO_OUT == packet->direction) && (ZFONE_SIP_REQUEST == sip_message->start_line.type)) ||
			((ZFONE_IO_IN == packet->direction) && (ZFONE_SIP_RESPONSE == sip_message->start_line.type)) )
		{
			sha256_hash((unsigned char*)sip_message->from.uri, strlen(sip_message->from.uri), &sha_ctx);	
			sha256_hash((unsigned char*)sip_message->to.uri, strlen(sip_message->to.uri), &sha_ctx);
		}
			
		sha256_end(sha_buff, &sha_ctx);
		zrtp_memcpy(sip_message->session_id, sha_buff, SIP_SESSION_ID_SIZE);
    }
    
    print_sip_message(sip_message);
    
    return 0;
}


static int check_voicemail(char *str, int length)
{
	char buffer[40];
	unsigned int tok_len;

	memset(buffer, 0, sizeof(buffer));
	zfone_get_next_token(str, 39, "@\r\n", 0, NULL, &tok_len);
	memcpy(buffer, str, tok_len ? tok_len : 39);

	return strstr(buffer, "sip:voicemail_sipphone+") != NULL;
}

//------------------------------------------------------------------------------
//    Zfone SIP/SDP parsing utility part
//------------------------------------------------------------------------------

/*
 * from RFC 3261:
 * (section 7  ) start line = request line / status line
 * (section 7.1) request line = Method SP request-URI SP SIP-Version CRLF
 * (section 7.2) request line = SIP-Version SP Status-Code SP Reason-Phrase  CRLF
 */
zfone_sip_type_t parse_start_line(const char *buff, uint32_t depth, zfone_start_line_t* start_line)
{    
	char* sline = (char*) buff;
	uint32_t tok_len = 0;
	uint32_t offset = 0;
	uint32_t bytes_left = depth;
	
	if (!buff || !depth)
	    return ZFONE_SIP_ERROR;

	// get first token
	sline = zfone_get_next_token(sline, bytes_left, " ", 0, &offset, &tok_len);
	if (!sline)
	{
	    return ZFONE_SIP_UNKNOWN; // can't find no one token in start line
	}
	bytes_left -= offset;
	
	// Is the first token a version string?
	if ( (SIP2_HDR_LEN == tok_len) && !zfone_str_nocase_cmp(sline, SIP2_HDR, SIP2_HDR_LEN) )
	{
	    // It's s Status-Line or something else but not Request-Line
	    // To be a Status-Line, the second token must be a 3-digit number (Status-Code)
	    sline += tok_len;
	    bytes_left -= tok_len;
	    
	    sline = zfone_get_next_token(sline, bytes_left, " ", 0, &offset, &tok_len);
	    if (!sline)
	    {
			return ZFONE_SIP_UNKNOWN; // can't find Status-Code so it isn't a Status-Line
	    }
	    
	    if (3 != tok_len)
	    {
			return ZFONE_SIP_UNKNOWN; // we can't find 3-character status code
	    }
	    
	    if ( (('0' > buff[sline-buff]) || ('9' < buff[sline-buff]))     ||
	         (('0' > buff[sline-buff+1]) || ('9' < buff[sline-buff]+1)) ||
	         (('0' > buff[sline-buff+2]) || ('9' < buff[sline-buff]+2))  )
	    {
			return ZFONE_SIP_UNKNOWN; // we gott 3 characters but they are not digits
	    }
	    
	    // Ok. It looks like status line - save protocol version, Status-code and exit
	    if (start_line)
	    { 
			start_line->type = ZFONE_SIP_RESPONSE;
			start_line->version = 2*10+0; //TODO: parse SIP version
			if (0 > zfone_strtoui(sline, 3, &start_line->status_code))
			{
		  		return ZFONE_SIP_ERROR; // can't convert
			}
			start_line->is_voicemail = (uint8_t)check_voicemail(sline + tok_len, bytes_left - tok_len);
	    }
	    
		
	    return ZFONE_SIP_RESPONSE;
    }
    else
    {
		uint32_t col_offset  = 0;
		char*	 method_ptr  = NULL;
		uint32_t method_len  = 0;
		uint32_t  method     = ZFONE_SIP_METHOD_INVALID;
		int is_vm;
		
		// This is a Request-Line or something other than a Status-Line.
		// To be a Request-Line, the second token must be a URI and the third token must
		// be a version string.
		
		if (tok_len < 3)
		{
    		return ZFONE_SIP_UNKNOWN;	// wrong Method-name size
		}
		method_ptr = sline;
		method_len = tok_len;
		    
		is_vm = check_voicemail(sline + tok_len, bytes_left - tok_len);

			// get SIP URI and try to find colon there
		sline += tok_len;
		bytes_left -= tok_len;
		sline = zfone_get_next_token(sline, bytes_left, " >", 0, &offset, &tok_len);
		if (!sline)
		{
			return ZFONE_SIP_UNKNOWN; // can't find SIP URI in Reqest String
		}


		bytes_left -= offset;
		    
		if ( !zfone_find_char(sline, bytes_left, ":", &col_offset) )
		{
			return ZFONE_SIP_UNKNOWN; // there is no colon after the method, so the URI isn't valid.
		}
		    
		if (offset+tok_len < col_offset)
		{
			return ZFONE_SIP_UNKNOWN; // col is in Version fiels
		}
		sline = sline + tok_len;
		bytes_left -= tok_len;	    

		// check SIP version
		sline = zfone_get_next_token(sline, bytes_left, " >\r\n", 0, NULL, &tok_len);
		if (!sline)
		{
			return ZFONE_SIP_UNKNOWN; // can't find SIP Version at Reqest string
		}    
		if ( (SIP2_HDR_LEN != tok_len) || zfone_str_nocase_cmp(sline, SIP2_HDR, SIP2_HDR_LEN) )
		{
			return ZFONE_SIP_UNKNOWN; // incorrect SIP version
		}
		
		// it looks like everything is ok with Reqest-line - storing all needed params
		method = get_method_by_name(method_ptr, method_len);
		if (ZFONE_SIP_METHOD_INVALID == method)
		{
			return ZFONE_SIP_UNKNOWN; // unsupported method name
		}
		
		if (start_line)
		{
			start_line->type = ZFONE_SIP_REQUEST;;
			start_line->version = 2*10+0; //TODO: parse SIP version
			start_line->method =method;
			start_line->is_voicemail = (uint8_t)is_vm;
		}
		
		return ZFONE_SIP_REQUEST;
    }
}

//------------------------------------------------------------------------------
zfone_sip_methods_t get_method_by_name(const char* buff, uint32_t len)
{
    uint32_t method_pos = 1;
    
    for (method_pos=0; method_pos<SUPPORTED_SIP_METHODS_COUNT; method_pos++)
    {
        if ( (strlen(zfone_sip_methods[method_pos]) == len) &&
	      !zfone_str_nocase_cmp(zfone_sip_methods[method_pos], buff, len) )
		{
			return method_pos;
		}
    }

    return ZFONE_SIP_METHOD_INVALID;
}

//------------------------------------------------------------------------------
zfone_sip_headers_t get_header_by_name(const char* buff, uint32_t len)
{
    uint32_t header_pos = 0;
    
    for (header_pos=0; header_pos<SUPPORTED_SIP_HEADERS_COUNT; header_pos++)
    {
        if ( (strlen(zfone_sip_headers[header_pos].name) == len) &&
	      !zfone_str_nocase_cmp(zfone_sip_headers[header_pos].name, buff, len) )
		{		
	  		return header_pos;
		}
    }	

    return ZFONE_SIP_HEADER_UNKNOWN;
}

//------------------------------------------------------------------------------
static int32_t parse_sdp(const char* buff, uint32_t length, zfone_sdp_message_t* sdp_message)
{
    uint32_t bytes_left = length;
    uint32_t offset = 0;
    
    char* sline = (char*) buff;
    char* eline = (char*) buff;
    int32_t line_len = 0;
    
    char*  stok	= NULL;
    uint32_t tok_len = 0;
    
    uint16_t	is_session_parsed = 0;
    
    memset(sdp_message, 0, sizeof(zfone_sdp_message_t));
	sdp_message->is_zrtp_present = 0;
    sdp_message->active_streams = 0;
	sdp_message->zrtp_offset = 0;
	
    // Parsing SDP message
    while (1)
    {
		char type = ' ';
    
		if ((line_len > 0) && (0 == bytes_left))
		{
	  		break; // all SDP data have been parse		    
		}
		
		sline = eline;
		line_len = zfone_find_line_end(sline, bytes_left, &eline);
		bytes_left -= line_len;
	
		if (2 > line_len)
		{		
	  		return -41; // can't find CRLF at the end of the line
		}
	
		// check header type 
		type = *sline;
		sline++;
	
		if ( !((type >= 'A') && (type <= 'Z')) && !((type >= 'a') && (type <= 'z')) )
		{
	  		continue; // Field type must be a character. Skip such line
		}
		if ( *sline != '=')
		{
	  		continue; // '=' must follow after the header type. Skip such line
		}
		sline++;
		line_len -= 2;
		stok = sline;
	
		// now:
		//     sline pointed to the firts character after the 'X='
  		//     stok pointed to the begining of the line
		//     line_len set to actual value
		switch (type)
		{
	  		case 'v':
	  		{
				stok = zfone_get_next_token(stok, line_len, " \r\n", 0, NULL, &tok_len);
				if (!stok)
				{
		  			return -42; // can't find version
				}
				if ((tok_len != 1) || (*stok != '0'))
				{
		  			return -43; // unsupported or wrong SDP version
				}
	  		} break;
	  		case 'c':
	  		{
				// RFC 2327 appendix A	
				// connection-field =  ["c=" nettype space addrtype space connection-address CRLF]
				// a connection field must be present in every media description or at the session-level
				// RFC 2327 par 6.	
				// "The first sub-field is the network type, which is a text string
    			// giving the type of network.  Initially "IN" is defined to have the
				// meaning "Internet" " We are support just IN now.
		
				// check <nettype>
				stok = zfone_get_next_token(stok, line_len, " ", 0, NULL, &tok_len);
				if (!stok)
				{
		  			return -44; // can't find Network type
				}
				if ((tok_len != 2) || zfone_str_nocase_cmp(stok, "IN", 2))
				{
		  			return -45; //unsupported or wrong network type
				}
				stok += tok_len;
				line_len -= tok_len;
		
				// check <addrtype>
				stok = zfone_get_next_token(stok, line_len, " ", 0, NULL, &tok_len);
				if (!stok)
				{
		  			return -46; // can't find Address type
				}
				if ((tok_len != 3) || zfone_str_nocase_cmp(stok, "IP4", 3))
				{
		  			return -47; // unsupported or wrong address type
				}
				stok += tok_len;
				line_len -= tok_len;
		
				// get connection address
				// RFC 2327 par 6 "<connection-address> = <base multicast address>/<ttl>/<number of addresses>"
				stok = zfone_get_next_token(stok, line_len, " \r\n", 0, NULL, &tok_len);
				if (!stok)
				{
		  			return -48; // can't get connection address block
				}
				if (tok_len < 7)
				{
		  			return -49; // to small token for IP address 
				}
		
				// try to convert IP address. zfone_ip2str will stops at first wrong 
				// character so we can parse TTL and Range after IP extracting
		
				// Session section parsed - this field relates to one of media sections
				if ( !is_session_parsed )
				{
				    sdp_message->contact.range = 0;
				    sdp_message->contact.base  = zfone_str2ip(stok);
					// TODO: sometimes it may be 0 for few VoIP clients
					// TODO: provide some error code for zfone_str2ip()
					/*
				    if ( !sdp_message->contact.base )
				    {
						ZRTP_LOG(1, (_ZTU_, "zfoned siparser: ERROR -50 can't convert IP:%.25s\n packet:%s.\n", stok, buff));
						return -50; // can't convert IP
		  		    }
					*/
				}
				else
				{
		  		    sdp_message->streams[sdp_message->active_streams-1].contact.range = 0;
				    sdp_message->streams[sdp_message->active_streams-1].contact.base = zfone_str2ip(stok);
					// TODO: sometimes it may be 0 for few VoIP clients
					// TODO: provide some error code for zfone_str2ip()
					/*
				    if ( !sdp_message->streams[sdp_message->active_streams-1].contact.base )
			  			return -50; // can't convert IP
					*/
				}
		
				//  check for multicast address. At first TTL field and the address range
				if ( zfone_find_char(stok, tok_len, "/", &offset) )
				{
		  		    stok += offset + 1;
				    tok_len -= offset + 1;
				    // Session section parsed - this field relates to one of media sections
				    if ( !is_session_parsed )
				    {
						// from the last '/' and to the end of the token must be range value
						if (0 > zfone_strtoui(stok, tok_len, &sdp_message->contact.range))
						{
						    return -52; // can't convert range part for multicast 
						}			
				    }
				    else
				    {
						if (0 > zfone_strtoui(stok, tok_len, &sdp_message->streams[sdp_message->active_streams-1].contact.range))
						{
						    return -53; // can't convert range part for multicast
						}			
				    }
				}
	  	    } break;
	  	    case 'm':
		    {
		  		// RFC 2327 appendix A
				// media-field =  "m=" media space port ["/"integer] space proto 1*(space fmt) CRLF			    
				//  	media  =  typically "audio", "video", "application" or "data"
				//		fmt    =  typically an RTP payload type for audio and video media
				//      proto  =  typically "RTP/AVP" or "udp" for IP4																		    
				//      port   =  should in the range "1024" to "65535" inclusive
			
				if (!is_session_parsed)
				{
				    // first media session have been found. This means Session section
				    // closed and all following atributes related to current media stream.
				    is_session_parsed = 1;
				}
		
				if ( !sdp_message->zrtp_offset )		
				{
					sdp_message->zrtp_offset = length - bytes_left - line_len - 2;
				}

				if ( !sdp_message->zrtp_offset )		
				{
					sdp_message->zrtp_offset = length - bytes_left;
				}


				if (sdp_message->active_streams == MAX_SDP_RTP_CHANNELS)
				{
				    return -39;	// can't handle more then MAX_SDP_RTP_CHANNELS streams
				}
		
				// adding one more active stream
				sdp_message->active_streams++;

				// define media type
				stok = zfone_get_next_token(stok, line_len, " ", 0, NULL, &tok_len);
				if (!stok)
				{
				    return -54; // can't media type
				}
		
				if ((tok_len == 5) && !zfone_str_nocase_cmp(stok, "video", 5))
				{
				    sdp_message->streams[sdp_message->active_streams-1].type = ZFONE_SDP_MEDIA_TYPE_VIDEO;
				}
				else if ((tok_len == 5) && !zfone_str_nocase_cmp(stok, "audio", 5))
				{
				    sdp_message->streams[sdp_message->active_streams-1].type = ZFONE_SDP_MEDIA_TYPE_AUDIO;
				}
				else
				{
		  		    // it's unsupported stream media typr - free stream and skipp this line
				    sdp_message->streams[sdp_message->active_streams-1].type = ZFONE_SDP_MEDIA_TYPE_UNKN;
				    sdp_message->active_streams--;
				    continue;
				}		
		  		stok += tok_len;
				line_len -= tok_len;
		
				// parse ports space
				stok = zfone_get_next_token(stok, line_len, " ", 0, NULL, &tok_len);
				if (!stok)
				    return -55; // can't get ports block
  		
		  		// converting base port number. strtol() must breaks at first '/' or ' ' character
				if (0 > zfone_strtoui(stok, tok_len, &sdp_message->streams[sdp_message->active_streams-1].rtp_ports.base))
		  		    return -56; // can't convert
		
		
				// is it multicast block?
				if ( zfone_find_char(stok, tok_len, "/", &offset) )
				{
	  			    // yes it looks like multicast - try to convert range part
				    if (0 > zfone_strtoui(stok+offset+1, tok_len-offset, &sdp_message->streams[sdp_message->active_streams-1].rtp_ports.range))
						return -57; // can't convert
				}
				stok += tok_len;
				line_len -= tok_len;
		
				// check transport sub-field
				stok = zfone_get_next_token(stok, line_len, " ", 0, NULL, &tok_len);
				if (!stok)
				    return -58; // can't transport block
		
				if ((tok_len != 7) || zfone_str_nocase_cmp(stok, "RTP/AVP", 7))
				{
				    // it's unsupported transport layer - free stream and skipp this line
		  		    sdp_message->streams[sdp_message->active_streams-1].type = ZFONE_SDP_MEDIA_TYPE_UNKN;
				    sdp_message->active_streams--;
				    continue;
				}
	  	    } break;
	  	    case 'a':
		    {

				zfone_sdp_stream_t *stream;
				// Looking for ZRTP atribute in SDP message. According to the ZRTP Internet
				// draft Zfone steps back and apper layer application should provide encryption.
				stok = zfone_get_next_token(stok, line_len, ": \r\n", 0, NULL, &tok_len);
				line_len -= tok_len;
				if ( tok_len >= 8 && 
					 (!zfone_str_nocase_cmp(stok, "zrtp-zid", 8) || !zfone_str_nocase_cmp(stok, "zrtp-sas", 8)) )
	  			{
	  				sdp_message->is_zrtp_present = 1;
					break;
	  			}

				if ( tok_len >= 15 && 
					 (!zfone_str_nocase_cmp(stok, "zrtp-hello-hash", 15) || !zfone_str_nocase_cmp(stok, "zrtp-sas", 8)) )
	  			{
					char tmp_buffer[16], print_buffer[33];
	  				sdp_message->is_zrtp_present = 1;
					stok += tok_len;
					if ( *stok != ':' )
					{
						ZRTP_LOG(1, (_ZTU_, "zfoned (parse sdp) : failed to find ':' during zrtp-hello-hash parsing.\n\n"));
						break;
					}

					stok = zfone_get_next_token(++stok, line_len, " \r\n", 0, NULL, &tok_len);
					line_len -= tok_len;					
					if ( tok_len != 32 )
					{
						ZRTP_LOG(1, (_ZTU_, "zfoned (parse sdp) : zrtp-hello-hash value has wrong size: %d %0.20s.\n\n",
							tok_len, stok));
						break;
					}

					str2hex(stok, 32, tmp_buffer, 16);
					ZRTP_LOG(3, (_ZTU_, "zfoned (PARSE SDP) : hello-hash is %s\n\n", hex2str(tmp_buffer, 16, print_buffer, 33)));
					
					break;
	  			}

				if ( !sdp_message->active_streams || sdp_message->active_streams > MAX_SDP_RTP_CHANNELS )
				{
					ZRTP_LOG(1, (_ZTU_, "zfoned (parse sdp) : cant get correct active stream.\n\n"));
					break;
				}

				stream = &sdp_message->streams[sdp_message->active_streams-1];
				if ( stream->type == ZFONE_SDP_MEDIA_TYPE_UNKN )
				{
					ZRTP_LOG(1, (_ZTU_, "zfoned (parse sdp) : active stream type is unknown.\n\n"));
					break;
				}

				// Tivi VoIP client use the same UDP port for video and voice. x-ssrc param is
				// attached to every stream to distinguish type of media. We use this field to
				// create several ZRTP streams
				if ( tok_len >= 6 && !zfone_str_nocase_cmp(stok, "x-ssrc", 6) )
				{
					stok += tok_len;
					if ( *stok != ':' )
					{
						ZRTP_LOG(1, (_ZTU_, "zfoned (parse sdp) : failed to find ':' during x-ssrc parsing.\n"));
						break;
					}

					stok = zfone_get_next_token(++stok, line_len, " \r\n", 0, NULL, &tok_len);
					line_len -= tok_len;					
					if ( tok_len > 8 )
					{
						ZRTP_LOG(1, (_ZTU_, "zfoned (parse sdp) : ssrc value is bigger than 8 symbols %d %0.20s.\n",
							tok_len, stok));
						break;
					}

					zfone_strtoi16(stok, tok_len, &stream->extra);
				}
				else if ( tok_len >= 3 && !zfone_str_nocase_cmp(stok, "alt", 3) )
				{
					int i;
					stok += tok_len;
			
					if ( *stok != ':' )
					{
						ZRTP_LOG(1, (_ZTU_, "zfoned (parse sdp) : failed to find ':' during ice parsing.\n"));
						break;
					}
					
					stok++;
					for (i = 0; i < 5; i++)
					{
						stok = zfone_get_next_token(stok, line_len, " \r\n", 0, NULL, &tok_len);
						if (stok)
						{
							stok += tok_len;
							line_len -= tok_len;
							if ( *stok != ' ' )
								break;
						}						
					}
					
					if (i == 5)
					{
						stok = zfone_get_next_token(stok, line_len, " \n\r", 0, NULL, &tok_len);
						stream->ice_ip = zfone_str2ip(stok);
						line_len -= tok_len;
						stok += tok_len;

						if (*stok == ' ')
						{
							stok = zfone_get_next_token(stok + 1, line_len, " \n\r", 0, NULL, &tok_len);
							line_len -= tok_len;
							zfone_strtoui(stok, tok_len, &stream->ice_port);
						}
					}
					ZRTP_LOG(3, (_ZTU_, "ICE attribute was found %x %d\n", stream->ice_ip, stream->ice_port));
				}
				else if ( tok_len >= 6 && !zfone_str_nocase_cmp(stok, "rtpmap", 6))
				{
					uint32_t value;
					stok += tok_len;
			
					if ( *stok != ':' )
					{
						ZRTP_LOG(1, (_ZTU_, "zfoned (parse sdp) : failed to find ':' during ice parsing.\n"));
						break;
					}
					
					stok = zfone_get_next_token(++stok, line_len, " \r\n", 0, NULL, &tok_len);
					line_len -= tok_len;
					
					zfone_strtoui(stok, tok_len, &value);
					ZRTP_LOG(3, (_ZTU_, "[TEST] : parsed codec %d %d\n", stream->type, value));
					if (!value)
						break;

					if (sdp_message->codecs.count >= ZFONE_SDP_CODECS_MAX)
						break;
						
					sdp_message->codecs.type[sdp_message->codecs.count]   = stream->type;
					sdp_message->codecs.code[sdp_message->codecs.count++] = value;
				}
				else if ( tok_len >= 8 && !zfone_str_nocase_cmp(stok, "inactive", 8) )
				{
					ZRTP_LOG(3, (_ZTU_, "[TEST] : inactive flag was set to media stream\n"));
					stream->dir_attr = ZFONE_SDP_DIR_ATTR_INACTIVE;
				}
				else if ( tok_len >= 8 && !zfone_str_nocase_cmp(stok, "sendonly", 8) )
				{
					ZRTP_LOG(3, (_ZTU_, "[TEST] : sendonly flag was set to media stream\n"));
					stream->dir_attr = ZFONE_SDP_DIR_ATTR_SENDONLY;
				}
				else if ( tok_len >= 8 && !zfone_str_nocase_cmp(stok, "recvonly", 8) )
				{
					ZRTP_LOG(3, (_ZTU_, "[TEST] : recvonly flag was set to media stream\n"));
					stream->dir_attr = ZFONE_SDP_DIR_ATTR_RECVONLY;
				}
		    } break;
			case 'i':
			{
				// Gizmo VoIP client use bonjour in LAN. The 5-th param in this field
				// determines RTP port. Heurist will use this information to handle Gizmo
				// calls. If we can't obtain this option - nothing wrong with this. 
				int i;
				for (i = 0; i < 5;i++)
				{
					stok = zfone_get_next_token(stok, line_len, " \n\r", 0, NULL, &tok_len);
					line_len -= tok_len;
					stok += tok_len;
					if ( *stok != ' ' )
					{
						break;
					}
				}
				if ( i == 5 )				
				{
					stok = zfone_get_next_token(stok, line_len, " \n\r", 0, NULL, &tok_len);
					line_len -= tok_len;
					zfone_strtoui(stok, tok_len, &sdp_message->extra);
				}
			} break;
		    default: break;	// skip all unsupported headers
		}
    }
    
    return 0;
}

//------------------------------------------------------------------------------
static void print_sip_message(zfone_sip_message_t* msg)
{
    char ip_buff[25];
	char buff[SIP_SESSION_ID_SIZE*2+2];
    char method[ MAX_CSEQ_METHOD_SIZE];
    int i = 0;
    
    memset(method, 0, sizeof(method));
    memset(ip_buff, 0, sizeof(ip_buff));
    
    // print start line information
#if ZRTP_PLATFORM == ZP_WIN32_KERNEL
	RtlStringCchPrintfA(ip_buff, sizeof(ip_buff), "%d", msg->start_line.status_code);
#else
    snprintf(ip_buff, sizeof(ip_buff), "%d", msg->start_line.status_code);
#endif	
    ZRTP_LOG(3, (_ZTU_, "%s SIP %s packet. version %d.\n", 
		   (msg->start_line.type == ZFONE_SIP_REQUEST) ? "REQEST" : "RESPONSE",
		   (msg->start_line.type == ZFONE_SIP_REQUEST) ? zfone_sip_methods[msg->start_line.method] : ip_buff,
		   msg->start_line.version ));
    
    // print headers
    ZRTP_LOG(3, (_ZTU_, "From:<%s> <%s>.\n", msg->from.name, msg->from.uri));
    ZRTP_LOG(3, (_ZTU_, "To:<%s> <%s>.\n", msg->to.name, msg->to.uri));
    ZRTP_LOG(3, (_ZTU_, "Call-ID:%s.\n", msg->call_id));
    ZRTP_LOG(3, (_ZTU_, "Packet-ID:%s.\n", hex2str((char*)msg->session_id, SIP_SESSION_ID_SIZE, buff, sizeof(buff))));
    ZRTP_LOG(3, (_ZTU_, "CSeq:%d %s.\n", msg->cseq.seq, zfone_sip_methods[msg->cseq.name]));
    ZRTP_LOG(3, (_ZTU_, "Conttent type:%s.\n", (msg->ctype == ZFONE_SIP_BODY_SDP) ? "SDP" : "Unknown"));
    ZRTP_LOG(3, (_ZTU_, "Conttent length:%d.\n", msg->clength));
    ZRTP_LOG(3, (_ZTU_, "User-Agent:%s.\n", (ZFONE_AGENT_GIZMO == msg->user_agent) ? "GIZMO" : "DEFAULT"));
    
    // print body
    if ((msg->ctype == ZFONE_SIP_BODY_SDP) && msg->clength)
    {
		ZRTP_LOG(3, (_ZTU_, "SDP body:\n"));
		ZRTP_LOG(3, (_ZTU_, "\tversion:%d\n", msg->body.version));
		ZRTP_LOG(3, (_ZTU_, "\tcontact:%s/%d\n",
				zfone_ip2str(ip_buff, 25, msg->body.contact.base),
				msg->body.contact.range ));
		for (i=0; i<msg->body.active_streams; i++)
		{
			ZRTP_LOG(3, (_ZTU_, "\t%s IP4 RTP %d/%d\n",
				(msg->body.streams[i].type == ZFONE_SDP_MEDIA_TYPE_VIDEO) ? "video" : "audio",
				msg->body.streams[i].rtp_ports.base, msg->body.streams[i].rtp_ports.range));
			if (msg->body.streams[i].contact.base)
			{
				ZRTP_LOG(3, (_ZTU_, "\t contact  %s/%d\n",
					zfone_ip2str(ip_buff, 25, msg->body.streams[i].contact.base), msg->body.streams[i].contact.range));
			}
		}
    }
    
    zrtp_print_log_delim(3, LOG_SPACE, "");    
}

