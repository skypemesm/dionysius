/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok <chaser@soft-industry.com>
 */
 
#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfone cache"

extern zrtp_global_t 	*zrtp_global;	//!< zrtp context for global data storing

// just copy info from cache entry into record
/*---------------------------------------------------------------------------*/
static int get_cache_info_callback(zrtp_cache_elem_t* elem, int is_mitm, void *data, int *delete)
{
	zrtp_cache_info_t *info = (zrtp_cache_info_t *)data;
	zrtp_cache_info_record_t *record = &info->list[info->count];

	if (info->count >= ZRTP_MAX_CACHE_INFO_RECORD)	
		return 0;	

	zrtp_memcpy(record->id, elem->id, sizeof(zrtp_cache_id_t));
	zrtp_memcpy(record->name, elem->name, ZFONE_CACHE_NAME_LENGTH);

	ZRTP_LOG(3, (_ZTU_, "ZFONED get_cache_info_callback(): TEST name=%.32s\n", record->name));

	record->verified = elem->verified;
	record->time_created = elem->secure_since;
	record->time_accessed = elem->lastused_at;
	record->oper = 0;
	record->is_mitm = is_mitm;

	if (zfone_cfg.params.zrtp.cache_ttl != SECRET_NEVER_EXPIRES)
	{
	
		record->is_expired = (elem->lastused_at + zfone_cfg.params.zrtp.cache_ttl <	zrtp_time_now()*1000);
	}
	else
		record->is_expired = 0;

	*delete = 0;	
	info->count++;

	return 1;
}

uint32_t zrtp_get_cache_info(zrtp_cache_info_t *info)
{
	info->count = 0;
	zrtp_def_cache_foreach(zrtp_global, 1, get_cache_info_callback, (void*)info);
	zrtp_def_cache_foreach(zrtp_global, 0, get_cache_info_callback, (void*)info);
	return info->count;
}


/*---------------------------------------------------------------------------*/
// help structure for cache info setting
typedef struct cache_set_helper
{
	zrtp_cache_info_t	*info;
	uint32_t 			current;	//current record in the array
} cache_set_helper_t;

int set_cache_info_callback(zrtp_cache_elem_t* elem, int is_mitm, void *data, int *delete)
{
	uint32_t i = 0;
	cache_set_helper_t *helper = (cache_set_helper_t *)data;
	
	if (helper->current == helper->info->count)
		return 0;

  	*delete = 0;

	// lets find appropriate record in the array. we'll start from helper->current
	// index as we dont want to search over whole array again and again. Cache
	// entries order wont be changed between getting info and call of this function.
  	for (i=helper->current; i<helper->info->count; i++)
	{
		zrtp_cache_info_record_t *record = &helper->info->list[i];

		//check if this our record
		if (zrtp_memcmp(elem->id, record->id, sizeof(zrtp_cache_id_t)))
			continue;

		if (record->is_mitm != is_mitm)
			continue;

		// if this is the first record we checked, we can move helper->current
		// to the next record for next iteration of this function call
		if (i == helper->current)
			helper->current++;	

		switch ( record->oper )
		{		
		case 'm': // record was modified
			elem->name[ZFONE_CACHE_NAME_LENGTH-1] = 0;
			zrtp_memcpy(elem->name, record->name, ZFONE_CACHE_NAME_LENGTH-1);
			elem->name_length = strlen((char*)elem->name);
			elem->verified = record->verified;

			ZRTP_LOG(3, (_ZTU_, "ZFONED set_cache_info_callback(): cache entry has been changed: len=%d, name=%.32s\n", elem->name_length, elem->name));
			ZRTP_LOG(3, (_ZTU_, "ZFONED set_cache_info_callback(): V=%d, len=%d, name=%.32s\n", elem->verified, elem->name_length, elem->name));
			break;

		case 'd': // record was deleted
			*delete = 1;
			ZRTP_LOG(3, (_ZTU_, "ZFONED set_cache_info_callback(): cache entry has been removed: len=%d, name=%.32s\n", elem->name_length, elem->name));
			break;

		default:
			break;
		}
	}

	return 1;
}

/*---------------------------------------------------------------------------*/
uint32_t zrtp_set_cache_info(zrtp_cache_info_t *info)
{
	cache_set_helper_t helper;

	helper.info = info;
	helper.current = 0;
	
	zrtp_def_cache_foreach(zrtp_global, 1, set_cache_info_callback, (void*)&helper);		
	zrtp_def_cache_foreach(zrtp_global, 0, set_cache_info_callback, (void*)&helper);		
	return zrtp_status_ok;
}
