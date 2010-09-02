/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZFONED_UTILS_H__
#define __ZFONED_UTILS_H__

#include <zrtp.h>
#include "zfone_siprocessor.h"

char* zfone_get_next_token(const char* buff, uint32_t depth, const char* delem,
			    uint32_t segmented, uint32_t* offset, uint32_t* tok_length );
			     			     
char* zfone_find_char(const char* buff, uint32_t depth, const char* chr, uint32_t* offset);

char* zfone_find_str(const char *buff, uint32_t depth, const char *needle, uint32_t* offset);

int zfone_str_nocase_cmp(const char* s1, const char* s2, uint32_t count);

int32_t zfone_strtoui(const char* buff, uint32_t depth, uint32_t* num);
int32_t zfone_strtoi16(const char* nptr, uint32_t depth, uint32_t* num);

int32_t zfone_find_line_end(const char* buff, uint32_t depth, char** eptr);

int zfone_sipidcmp(const zfone_sip_id_t id1, const zfone_sip_id_t id2);

/*!
 * \brief Converts host mode IP to string
 * \warning allocated buff size mast be greater then 16 bytes
 * \param buff - buffer for converted IP
 * \param ip - IP to convert
 * \return 
 * 	- pointer to the buff on success
 * 	- NULL on error
 */
char* zfone_ip2str(char* buff, int size, uint32_t ip);

/*!
 * \brief Converts string IP to unsigned integer
 * \param str - string IP representation (something like: "192.168.1.27")
 * \return
 * 	- integer IP on success
 * 	- -1 on error
 */
uint32_t zfone_str2ip(const char* str);

int zfone_insert_cs(char* packet, uint32_t size);

char* zrtph_get_time_str(char* buff, int size);
uint64_t	zrtp_time_now();

const wchar_t* hex2wstr(const char *bin, int bin_size, wchar_t *buff, int buff_size);

wchar_t* hex2wchar(wchar_t *dst, unsigned char b);


#if ZRTP_PLATFORM == ZP_WIN32_KERNEL

#include <ndis.h>
#include <stdarg.h>

//void unicode2ansi(WCHAR *wstr, int wlen, char *cstr, int clen);


NTSTATUS
RtlStringCchVPrintfA(
    OUT LPSTR  pszDest,
    IN  size_t  cchDest,
    IN  LPCSTR pszFormat,
    IN  va_list argList
    );

NTSTATUS
  RtlStringCchPrintfA(
    OUT LPSTR  pszDest,
    IN size_t  cchDest,
    IN LPCSTR  pszFormat,
    ...
    );

NTSTATUS
  RtlStringCchCopyW(
    OUT LPWSTR  pszDest,
    IN size_t  cchDest,
    IN LPCWSTR  pszSrc
    );

NTSTATUS
  RtlStringCchCatNA(
    IN OUT LPSTR  pszDest,
    IN size_t  cchDest,
    IN LPCSTR  pszSrc,
    IN size_t  cchMaxAppend
    );

NTSTATUS
  RtlStringCchLengthA(
    IN LPCSTR  psz,
    IN size_t  cchMax,
    OUT size_t*  pcch  OPTIONAL
    );

NTSTATUS reg_get_string(HANDLE hkey, PUNICODE_STRING name, zrtp_stringn_t *value);
NTSTATUS reg_set_string(HANDLE hkey, PUNICODE_STRING name, const zrtp_stringn_t *value);
NTSTATUS reg_get_binary(HANDLE hkey, PUNICODE_STRING name, zrtp_stringn_t *value);
NTSTATUS reg_set_binary(HANDLE hkey, PUNICODE_STRING name, const zrtp_stringn_t *value);
NTSTATUS reg_get_dword(HANDLE hkey, PUNICODE_STRING name, ULONG *value);
NTSTATUS reg_set_dword(HANDLE hkey, PUNICODE_STRING name, ULONG value);

int zfone_send_ip(zfone_adapter_info_t *adapter_info, char* packet, unsigned int length);

#endif

/*!
 * \brief unix-like IP header for internal use only
 */
#pragma	pack(push, 1)
struct zrtp_iphdr
  {
#if ZRTP_BYTE_ORDER == ZBO_LITTLE_ENDIAN
    unsigned char 	ihl:4;
    unsigned char 	version:4;
#elif ZRTP_BYTE_ORDER == ZBO_BIG_ENDIAN
    unsigned char 	version:4;
    unsigned char 	ihl:4;
#endif
    uint8_t			tos;
    uint16_t		tot_len;
    uint16_t		id;
    uint16_t		frag_off;
    uint8_t			ttl;
    uint8_t			protocol;
    uint16_t		check;
    uint32_t		saddr;
    uint32_t		daddr;
  };
#pragma	pack(pop)


/*!
 * \brief unix-like TCP header for internal use only
 */
#pragma	pack(push, 1)
struct zrtp_tcphdr
  {
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
#  if ZRTP_BYTE_ORDER == ZBO_LITTLE_ENDIAN
    uint16_t res1:4;
    uint16_t doff:4;
    uint16_t fin:1;
    uint16_t syn:1;
    uint16_t rst:1;
    uint16_t psh:1;
    uint16_t ack:1;
    uint16_t urg:1;
    uint16_t res2:2;
#  elif ZRTP_BYTE_ORDER == ZBO_BIG_ENDIAN
    uint16_t doff:4;
    uint16_t res1:4;
    uint16_t res2:2;
    uint16_t urg:1;
    uint16_t ack:1;
    uint16_t psh:1;
    uint16_t rst:1;
    uint16_t syn:1;
    uint16_t fin:1;
#  else
#   error "Adjust your <bits/endian.h> defines"
#  endif
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};
#pragma	pack(pop)


/*!
 * \brief unix-like UDP header for internal use only
 */
#pragma	pack(push, 1)
struct zrtp_udphdr
{
  uint16_t source;
  uint16_t dest;
  uint16_t len;
  uint16_t check;
};
#pragma	pack(pop)

#endif //__ZFONED_UTILS_H__

