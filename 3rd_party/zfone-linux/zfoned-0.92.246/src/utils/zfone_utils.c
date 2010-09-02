/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone utils"

char* zfone_media_type_names[ZFONE_SDP_MEDIA_TYPE_COUNT] =
{
	"UNKNOWN",
	"VIDEO",
	"AUDIO",
	"ZRTP"
};

char* zfone_direction_names[ZFONE_IO_COUNR] =
{
	"<---",
	"--->",
	"x--x"
};

char* zfone_prtps_state_names[ZFONE_PRTPS_STATE_COUNT] = 
{
	"PASIVE",
	"ACTIVE",
	"PRE_READY",
	"READY",
	"PRE_ESTABL",
	"ESTABL",
	"SLEEP"	
};

char* zfone_confirm_mode_names[ZFONE_CONF_MODE_COUNT] = 
{
	"UNKN",
	"CONFIRM",
	"LINKED",
	"CLOSED"
};


//------------------------------------------------------------------------------
int zfone_sipidcmp(const zfone_sip_id_t id1, const zfone_sip_id_t id2)
{
	return zrtp_memcmp(id1, id2, SIP_SESSION_ID_SIZE);
}

//------------------------------------------------------------------------------
char* zfone_get_next_token( const char* buff,
						    uint32_t depth,
							const char* delem,
							uint32_t segmented,
							uint32_t* offset,
							uint32_t* tok_length )
{
    char* tok_begin	= (char*) buff;
    char* tok_end	= (char*) buff;
    uint32_t last_end	= 0;
    
    if (!buff || !delem || !depth)
		return NULL;
    
    if (offset) *offset = 0;
    if (tok_length) *tok_length = 0;
    
    // looking for token begin
    while ( strchr(delem, buff[last_end]) )
    {
		if (++last_end == depth)
			return NULL;
    }
    tok_begin = (char*)buff+last_end;
    
    // looking for token end
    while ( !strchr(delem, buff[last_end]) )
    {
		if (++last_end == depth)
		{
			// if we reacher and of the buffer and can't find token end - look at segmented flag
			if (segmented)
				break;	// buffer may be segmented - continue processing
			else
				return NULL;	// buffer isn't segmented - error
		}
    }
    tok_end = (char*)buff+last_end;
    
    //WIN64
	if (tok_length) *tok_length = (uint32_t)(tok_end - tok_begin);
    if (offset) *offset = (uint32_t)(tok_begin - buff);

    return tok_begin;
}

//------------------------------------------------------------------------------
char* zfone_find_char(const char* buff, uint32_t depth, const char* chr, uint32_t* offset)
{
    uint32_t last_end = 0;
    if (offset) *offset = 0;
    
    // looking for any character from needle string
    while ( !strchr(chr, buff[last_end]) )
    {
		if (++last_end == depth)
			return NULL;
    }
    
    if (offset) *offset = last_end;
	return (char*)buff+last_end;
}
//------------------------------------------------------------------------------
char* zfone_find_str(const char *buff, uint32_t depth, const char *needle, uint32_t* offset)
{
    uint32_t i,j = 0;
    uint32_t coincided = 0;
    uint32_t need_coincided = 0; 
    
    if ( !buff || !needle || !depth)
		return NULL;
    
    if (offset) *offset = 0;
	
    need_coincided = strlen(needle);
    if ( (depth < need_coincided) )
		return NULL;

    i = 0;    
    while ( (i < (depth-need_coincided)) && (buff[i] != 0) )
    {
		// if we found first character of needle string - start comparison
		if (buff[i] == needle[0])
		{
			coincided = 0;
			j=0;
			while ( (j<need_coincided) && (buff[i+j] != 0) )
			{
				if (buff[i+j] == needle[j])
					coincided++;
				else
					break;	// break if just one character doesn't match		
				j++;
			}
			
			// break if we found all characters from needle string
			if (coincided == need_coincided)
				break;
		}
		
		i++;
    }
    
    // return pointer if all characters are matched after comparing
    if (coincided == need_coincided)
    {
		if (offset) *offset = i;
		return (char*) (buff+i);
    }
    else 
    {
		return NULL;
    }
}

//------------------------------------------------------------------------------
uint32_t zfone_str2ip(const char* str)
{
    int i = 0;
    uint32_t ip = 0;
    
    for(i=0; i<4; i++)
    {
		int part = 0;
	
		while(*str>='0' && *str<='9')
		{
			part *= 10;
			part += (*str - '0');
			str++;
		}

		if(*str == '.')
			str++;

		ip <<= 8;
		ip += part;
    }

    return ip;
}

//------------------------------------------------------------------------------
int32_t zfone_find_line_end(const char* buff, uint32_t depth, char** eptr)
{
    uint32_t offset = 0;
    if (eptr) *eptr = NULL;
    
    // Look either for a CR or an LF.
    if ( zfone_find_char(buff, depth, "\r\n", &offset) )
    {    
		 // Is it a CR?
		if (buff[offset] == '\r')
		{
			//is it followed by an LF?
			if (offset + 1 >= depth)
			{
				// next byte isn't in this buffer
				return -1;
			}
			else
			{
				if (buff[offset+1] == '\n')			
					offset++; // yes it's LF after CR - make one more step				
				
				// return the offset of the character after the last
				// character in the line, skipping over the last character
				// in the line terminator.
				if (eptr) *eptr = (char*)(buff + offset+1);
					return offset+1;
			}
		}
    }
    
    return -1;
}


#define CKSUM_CARRY(x) (x = (x >> 16) + (x & 0xffff), (~(x + (x >> 16)) & 0xffff))


//-----------------------------------------------------------------------------
static int in_cksum(uint16_t *addr, int len)
{
    int sum = 0;
    while (len > 1)
    {
		sum += *addr++;
		len -= 2;
    }
    if (len == 1)
	sum += *(uint16_t *)addr;
    
    return sum;
}


//-----------------------------------------------------------------------------
int zfone_insert_cs(char* packet, uint32_t size)
{
    int sum			= 0;
    int incSize		= 0;
    int memIncSize	= 0;
    
    struct zrtp_iphdr* ipHdr = NULL;    

    if ( (size < 20) || (!packet) )
		return -1;
    
    ipHdr = (struct zrtp_iphdr*) packet;
    incSize = size - ipHdr->ihl*4;		
    memIncSize  = incSize;
    
    if (incSize%2 != 0)
    {
    	memset(packet + size, 0, 1);
		++memIncSize;
    }
    
    // Calculate IP header checksum
    ipHdr->check = 0;
    sum = in_cksum((uint16_t *)ipHdr, ipHdr->ihl*4);
    ipHdr->check = CKSUM_CARRY(sum);
    
    // calculate checksum of incapsulated packet deffineing on protocol
    switch (ipHdr->protocol)
    {
	case voip_proto_TCP:
		{
	    struct zrtp_tcphdr* tcpHdr = (struct zrtp_tcphdr*) (packet + ipHdr->ihl*4);
	    tcpHdr->check = 0;
	    sum  = in_cksum((uint16_t *)&ipHdr->saddr, 8);
	    sum += zrtp_ntoh16(ipHdr->protocol + incSize);
	    sum += in_cksum((uint16_t *)tcpHdr, memIncSize);
	    tcpHdr->check = CKSUM_CARRY(sum);
		} break;
	case voip_proto_UDP:
		{
	    struct zrtp_udphdr* udpHdr = (struct zrtp_udphdr*) (packet + ipHdr->ihl*4);
	    udpHdr->check = 0;
	    sum  = in_cksum((uint16_t *)&ipHdr->saddr, 8);
	    sum += zrtp_ntoh16(ipHdr->protocol + incSize);
	    sum += in_cksum((uint16_t *)udpHdr, memIncSize);
	    udpHdr->check = CKSUM_CARRY(sum);
		} break;
	default:
		{
	    return -1;
		}
    }

    return 0;
}

//=============================================================================
//  Utility functions for Unix-like platforms
//=============================================================================


#if ZRTP_PLATFORM == ZP_LINUX || ZRTP_PLATFORM == ZP_DARWIN

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

//------------------------------------------------------------------------------
int zfone_str_nocase_cmp(const char* s1, const char* s2, uint32_t count)
{
    return strncasecmp(s1, s2, count);
}

//------------------------------------------------------------------------------
char* zfone_ip2str(char* buff, int size, uint32_t ip)
{
	if (size <= 20)
	  return "Buffer to small";
	  
    snprintf(buff, size, "%i.%i.%i.%i", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
    return buff;
}

//------------------------------------------------------------------------------
int32_t zfone_strtoui(const char* buff, uint32_t depth, uint32_t* num)
{
    char* endptr = NULL;

    if (!buff || !num)
	return -1;
		
    errno = 0;
    *num = strtol(buff, &endptr, 10);

    // Check for various possible errors
    if ( ((errno == ERANGE) && (*num == LONG_MAX)) ||
		 ((errno != 0) && (*num == 0)) )
    {
		*num = 0;
		return -1; // error during parsing
    }

    if (endptr == buff)
    {
		*num = 0;
		return -1; // no digits were found
    }
    
    if ((uint32_t)(endptr - buff) != depth)
    {
		return 1;
    }
    
    return 0;
}

//------------------------------------------------------------------------------
char* zrtph_get_time_str(char* buff, int size)
{
    struct tm t;
    struct timeval tv;    

    if (!buff || (size < 20))
    {
	return NULL;
    }
    
    // get local time
    gettimeofday(&tv, 0);
    localtime_r((const time_t*)&tv.tv_sec, &t);    
    snprintf(buff, size, "%04i:%02i:%02i %02i:%02i:%02i",
		t.tm_year+1900,
		t.tm_mon+1,
		t.tm_mday,
		t.tm_hour,
		t.tm_min,
		t.tm_sec
    	   );
    
    return buff;
}



//=============================================================================
//  Utility functions for windows Kernel
//=============================================================================

#elif ZRTP_PLATFORM == ZP_WIN32_KERNEL

//------------------------------------------------------------------------------
int zfone_str_nocase_cmp(const char* s1, const char* s2, uint32_t count)
{
	return _strnicmp(s1, s2, count);
}

//-----------------------------------------------------------------------------
wchar_t* hex2wchar(wchar_t *dst, unsigned char b)
{
	unsigned char v = b >> 4;
	*dst++ = (v<=9) ? L'0'+v : L'a'+ (v-10);
	v = b & 0x0f;
	*dst++ = (v<=9) ? L'0'+v : L'a'+ (v-10);
	return dst;
}

static char* zfone_print_byte(char* buff, uint8_t b)
{
	if (b >= 100) {
		uint8_t m = b / 100;
		*buff++ = '0' + m;
		b -= m*100;
		m = b / 10;
		*buff++ = '0' + m;
		b -= m*10;
		*buff++ = '0' + b;
	} else {
		if (b >= 10) {
			uint8_t m = b / 10;
			*buff++ = '0' + m;
			b -= m*10;
			*buff++ = '0' + b;
		} else {
			*buff++ = '0' + b;
		}
	}
	return buff;
}


//-----------------------------------------------------------------------------
const wchar_t* hex2wstr(const char *bin, int bin_size, wchar_t *buff, int buff_size)
{
    wchar_t *nptr = buff;

	if (NULL == buff)
	{
		return L"buffer is NULL";
	}
    
	if (buff_size < bin_size*2+1) {
		return L"buffer too small";
	}

	while (bin_size--) {
		nptr = hex2wchar(nptr, *bin++);
    }
	*nptr = 0;
    
    return buff;
}


//-----------------------------------------------------------------------------
int32_t zfone_strtoui(const char* nptr, uint32_t depth, uint32_t* num)
{
	uint32_t i = 0;

	if (!nptr || !num)
		return -1;

    *num = 0;
	while (*nptr >= '0' && *nptr <= '9' && (i<depth))
    {
		*num *= 10;
		*num += *nptr - '0';
		nptr++;
		i++;
	}
    
	return (i == depth) ? 1 : 0;	
}

//-----------------------------------------------------------------------------
/*
static uint32_t wstr2ip(const WCHAR *str)
{
    int i = 0;
    uint32_t ip = 0;
    
    for (i = 0; i < 4 && *str; i++)
    {
		int part = 0;

		while (*str >= L'0' && *str <= L'9')
		{
    		part *= 10;
			part += (*str - L'0');
			str++;
		}
		ip <<= 8;
		ip += part;
		if (*str == L'.') {
			str++;
	    } else {
			break;
		}
    }

    return ip;
}
*/

//------------------------------------------------------------------------------
char* zfone_ip2str(char* buff, int size, uint32_t ip)
{
	char* ret = buff;

	if (size < 16)
		return NULL;

	buff = zfone_print_byte(buff, (ip >> 24) & 0xff);
	*buff++ = '.';
	buff = zfone_print_byte(buff, (ip >> 16) & 0xff);
	*buff++ = '.';
	buff = zfone_print_byte(buff, (ip >> 8) & 0xff);
	*buff++ = '.';
	buff = zfone_print_byte(buff, ip & 0xff);
	*buff = 0;
    return ret;
}

//------------------------------------------------------------------------------
char* zrtph_get_time_str(char* buff, int size)
{
	LARGE_INTEGER sys_current;
	LARGE_INTEGER loc_current;
	TIME_FIELDS   time;

    if (!buff || (size < 20))
    {
		return NULL;
    }
    
	// construct data part: "hh:mm::ss"
	KeQuerySystemTime(&sys_current);
	ExSystemTimeToLocalTime(&sys_current, &loc_current);
	RtlTimeToTimeFields(&loc_current, &time);
	RtlStringCchPrintfA(buff, size, "%04i:%02i:%02i %02i:%02i:%02i", time.Year, time.Month, time.Day, 
		time.Hour, time.Minute, time.Second);
	
	return buff;
}

/*
void unicode2ansi(WCHAR *wstr, int wlen, char *cstr, int clen)
{
	UNICODE_STRING unicode = { (USHORT)wlen, (USHORT)wlen, wstr };
	ANSI_STRING ansi = { 0, clen-1, clen };
	RtlUnicodeStringToAnsiString(&ansi, &unicode, FALSE);
}
*/

//-----------------------------------------------------------------------------
int zfone_send_ip(zfone_adapter_info_t *adapter_info, char* packet, unsigned int length)
{
    NDIS_STATUS			Status;	
	unsigned char*		_new_data;
   	struct eth_hdr*		ethHdr;

	// Allocate memory for the new packet here
	Status = NdisAllocateMemoryWithTag(&_new_data, length + ETHERNET_HEADER_SIZE + 16, TAG);
	if ( Status != NDIS_STATUS_SUCCESS )
	{
		return -1;
	}

	ethHdr = (struct eth_hdr*)_new_data;
	NdisMoveMemory(ethHdr, &adapter_info->_eth_hdr, ETHERNET_HEADER_SIZE);	
	NdisMoveMemory(_new_data + ETHERNET_HEADER_SIZE, packet, length);

	{
		char sip[32];
		char dip[32];

		struct zrtp_iphdr*	ipHdr =  (struct zrtp_iphdr*) (_new_data + ETHERNET_HEADER_SIZE);
		struct zrtp_udphdr*	udpHdr = (struct zrtp_udphdr*)(_new_data + 20 + ETHERNET_HEADER_SIZE);

		ZRTP_LOG(3, (_ZTU_, "[zfone_send_ip]: Send %d bytes packet FROM %s:%d TO %s:%d.\n",
						length,
						zfone_ip2str(sip, 32, zrtp_ntoh32(ipHdr->saddr)),
						zrtp_ntoh16(udpHdr->source),
						zfone_ip2str(dip, 32, zrtp_ntoh32(ipHdr->daddr)),
						zrtp_ntoh16(udpHdr->dest)));
		ZRTP_LOG(3, (_ZTU_, "%d %d %d %x %d %d\n", 
			ipHdr->ihl, zrtp_ntoh16(ipHdr->tot_len), ipHdr->version, ipHdr->frag_off, ipHdr->ttl, ipHdr->protocol));
	}

	__try
	{
		PNDIS_PACKET	_new_packet;
		PNDIS_BUFFER	_new_buf = NULL;
		PADAPT pAdapt = (PADAPT)adapter_info->adapter;

		// Build and chain new buffer to a new packet and after that send the packet
		NdisAcquireSpinLock(&pAdapt->Lock);
		if (pAdapt->PTDeviceState > NdisDeviceStateD0)
		{
			NdisReleaseSpinLock(&pAdapt->Lock);
			if ( _new_data )
			{
				NdisFreeMemory(_new_data, 0, 0);
			}
			return -1;
		}
		pAdapt->OutstandingSends++;
		NdisReleaseSpinLock(&pAdapt->Lock);

		NdisAllocatePacket(&Status, &_new_packet, pAdapt->SendPacketPoolHandle);
		if (Status == NDIS_STATUS_SUCCESS)
		{
			PSEND_RSVD        SendRsvd;

			NdisAllocateBuffer(&Status, &_new_buf, pAdapt->SendPacketPoolHandle, _new_data, length + ETHERNET_HEADER_SIZE);
			if (NDIS_STATUS_SUCCESS != Status)
			{
				NdisFreePacket(_new_packet);
				if ( NULL != _new_data )
				{
					NdisFreeMemory(_new_data, 0, 0);
				}
				ADAPT_DECR_PENDING_SENDS(pAdapt);				
				return -1;
			}

			SendRsvd = (PSEND_RSVD)(_new_packet->ProtocolReserved);
			SendRsvd->OriginalPkt = NULL;
			SendRsvd->bNewBuffer = TRUE;

			_new_buf->Next = NULL;
			NdisChainBufferAtFront( _new_packet, _new_buf);
			NdisSetPacketFlags(_new_packet, NDIS_FLAGS_DONT_LOOPBACK);
			NdisSendPackets(pAdapt->BindingHandle, &_new_packet, 1);
			Status = NDIS_STATUS_SUCCESS;			
		}
		else
		{
			ADAPT_DECR_PENDING_SENDS(pAdapt);			
			return -1;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		ZRTP_LOG(1, (_ZTU_, "zfoned: (zrtp_send_ip): EXCEPTION %x.\n", GetExceptionCode()));
		return -1;
	}

	return 0;
}

#endif // architecture specific functions

//-----------------------------------------------------------------------------
int32_t zfone_strtoi16(const char* nptr, uint32_t depth, uint32_t* num)
{ 
	uint32_t i = 0;

	*num = 0;
	while ( *nptr && (i++ < depth) )
	{
		int value = -1;

		if (*nptr >= '0' && *nptr <= '9')
		{
			value = *nptr - '0';
		}
		else if (*nptr >= 'a' && *nptr <= 'f')
		{
			value = *nptr - 'a' + 10;
		}
		else if (*nptr >= 'A' && *nptr <= 'F')
		{
			value = *nptr - 'A' + 10;
		}

		if ( value < 0 )
		{
			break;
		}
	
		*num <<= 4;
		*num |= value;

		nptr++;
	}

	return (i == depth) ? 1 : 0;	
}

