/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */
 
#ifndef __ZFONE_CACHE_H__
#define __ZFONE_CACHE_H__

#include <zrtp.h>


#if defined(__cplusplus)
extern "C"
{
#endif

//! Down secret cache.
/*!
    Rewrite cache file with lastest values and free all used resources.
    \return zrtp_status_t on success and error code on other case.
*/
zrtp_status_t zrtp_def_cache_init(zrtp_global_t * zrtp);

void zrtp_def_cache_down(); 

zrtp_status_t zrtp_def_cache_put_name( const zrtp_stringn_t* one_ZID,
				   const zrtp_stringn_t* another_ZID,
				   const zrtp_stringn_t* name );

zrtp_status_t zrtp_def_cache_get_name( const zrtp_stringn_t* one_ZID,
				   const zrtp_stringn_t* another_ZID,
				   zrtp_stringn_t* name );

zrtp_status_t zrtp_def_cache_get_since( const zrtp_stringn_t* one_ZID,
				    const zrtp_stringn_t* another_ZID,
				    uint32_t* since );

zrtp_status_t zrtp_cache_clear( const zrtp_stringn_t* one_ZID,
				    const zrtp_stringn_t* another_ZID);


#define ZRTP_MAX_CACHE_INFO_RECORD	50
/*
 * Cache info record is used to store brief information about cache entry
 */
typedef struct zrtp_cache_info_record
{
	zrtp_cache_id_t		id;								/* cache element identifier */
	char				name[ZFONE_CACHE_NAME_LENGTH];  /* name of the user associated with this cache entry */
	uint8_t				verified;						/* is verified */		
	uint32_t			time_created;					/* time when cache entry was created*/ 
	uint32_t			time_accessed;					/* the last access time */ 
	uint8_t				oper;							/* operation which performed over entry (modification or deleting or smth else)*/
	uint8_t				is_mitm;
	uint8_t				is_expired;
} zrtp_cache_info_record_t;

/*
 *	Structure is used for storing array of cache info records, its used in 
 *  zrtp_get_cache_info and zrtp_set_cache_info
*/
typedef struct zrtp_cache_info
{
    uint32_t				 count;			/* count of record in the array */	
    zrtp_cache_info_record_t list[ZRTP_MAX_CACHE_INFO_RECORD]; /* array of cache info records */
} zrtp_cache_info_t;

/* function is used for retriving brief information about avaliable cache entries. */
uint32_t zrtp_get_cache_info(zrtp_cache_info_t *info);
/* function is used to modificate cache entries according to the records which are stored in the info parameter. */
uint32_t zrtp_set_cache_info(zrtp_cache_info_t *info);


#if (ZRTP_PLATFORM == ZP_WIN32_KERNEL)
zrtp_status_t zfone_cache_init(int *ready);
#endif

#if defined(__cplusplus)
}
#endif


#endif //__ZFONE_CACHE_H__
