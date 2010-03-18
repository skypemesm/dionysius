/*
 * Copyright (c) 2006-2009 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * For licensing and other legal details, see the file zrtp_legal.c.
 *
 * Viktor Krikun <v.krikun at zfoneproject.com>
 */

#include "zrtp.h"

#if defined(ZRTP_DONT_USE_BUILTIN) && (ZRTP_DONT_USE_BUILTIN == 0)

/* Windows kernel have it's own realization in Windows registry*/
#if (ZRTP_PLATFORM != ZP_WIN32_KERNEL)

static mlist_t 	cache_head;
static mlist_t 	mitmcache_head;
static uint8_t	inited = 0;

static zrtp_global_t* zrtp;
static zrtp_mutex_t* def_cache_protector = NULL;


/* Create cache ID like a pair of ZIDs. ZID with lowest value at the beginning */
static void cache_create_id( const zrtp_stringn_t* first_ZID,
							 const zrtp_stringn_t* second_ZID,
							 zrtp_cache_id_t id);

/* Searching for cache element by cache ID */
static zrtp_cache_elem_t* get_elem(const zrtp_cache_id_t id, uint8_t is_mitm);

/* Allows use cache on system with different byte-order */
static void cache_make_cross( zrtp_cache_elem_t* from,
							  zrtp_cache_elem_t* to,
							  uint8_t is_upload);

static zrtp_status_t zrtp_cache_user_init();
static zrtp_status_t zrtp_cache_user_down();


/*===========================================================================*/
/*     libZRTP interface implementation										 */ 
/*===========================================================================*/

#define ZRTP_CACHE_CHECK_ZID(a,b) \
	if ( (a->length != b->length) || \
		 (a->length != sizeof(zrtp_zid_t)) ) \
	{ \
		return zrtp_status_bad_param; \
	}

/*----------------------------------------------------------------------------*/
zrtp_status_t zrtp_def_cache_init(zrtp_global_t* a_zrtp)
{
	zrtp_status_t s = zrtp_status_ok;

	if (!inited) {
		zrtp = a_zrtp;
		s = zrtp_mutex_init(&def_cache_protector);
		if (zrtp_status_ok != s) {
			return s;
		}
		
		init_mlist(&cache_head);
		init_mlist(&mitmcache_head);
		s =  zrtp_cache_user_init();		
		
		inited = 1;
	}

	return s;
}

void zrtp_def_cache_down()
{
	if (inited) {
		mlist_t *node = NULL, *tmp = NULL;				
		
		zrtp_cache_user_down();

		mlist_for_each_safe(node, tmp, &cache_head) {
			zrtp_sys_free(mlist_get_struct(zrtp_cache_elem_t, _mlist, node));
		}
		mlist_for_each_safe(node, tmp, &mitmcache_head) {
			zrtp_sys_free(mlist_get_struct(zrtp_cache_elem_t, _mlist, node));
		}
	
		init_mlist(&cache_head);
		init_mlist(&mitmcache_head);
		
		zrtp_mutex_destroy(def_cache_protector);
		
		inited = 0;
	}
}

/*----------------------------------------------------------------------------*/
zrtp_status_t zrtp_def_cache_set_verified( const zrtp_stringn_t* one_ZID,
										   const zrtp_stringn_t* another_ZID,
										   uint32_t verified)
{
	zrtp_cache_id_t	id;
	zrtp_status_t s = zrtp_status_ok;

	ZRTP_CACHE_CHECK_ZID(one_ZID, another_ZID);
	cache_create_id(one_ZID, another_ZID, id);
	
	zrtp_mutex_lock(def_cache_protector);
	do {
		zrtp_cache_elem_t* new_elem = get_elem(id, 0);
		if (!new_elem) {
			s = zrtp_status_fail;
			break;
		}       
		new_elem->verified = verified;
	} while (0);
	zrtp_mutex_unlock(def_cache_protector);

	return s;
}

zrtp_status_t zrtp_def_cache_get_verified( const zrtp_stringn_t* one_ZID,
										   const zrtp_stringn_t* another_ZID,
										   uint32_t* verified)

{
	zrtp_cache_id_t	id;
	zrtp_status_t s = zrtp_status_ok;
	
	ZRTP_CACHE_CHECK_ZID(one_ZID, another_ZID);
	cache_create_id(one_ZID, another_ZID, id);
	
	zrtp_mutex_lock(def_cache_protector);
	do {
		zrtp_cache_elem_t* elem = get_elem(id, 0);
		if (!elem) {
			s = zrtp_status_fail;
			break;
		}	
		*verified = elem->verified;
	} while (0);	
	zrtp_mutex_unlock(def_cache_protector);
	
	return s;
}


/*----------------------------------------------------------------------------*/
static zrtp_status_t cache_put( const zrtp_stringn_t* one_ZID,
								const zrtp_stringn_t* another_ZID,
								zrtp_shared_secret_t *rss,
								uint8_t is_mitm )
{
    zrtp_cache_elem_t* new_elem = 0;
	zrtp_cache_id_t	id;
	zrtp_status_t s = zrtp_status_ok;

	ZRTP_CACHE_CHECK_ZID(one_ZID, another_ZID);
	cache_create_id(one_ZID, another_ZID, id);
	
	zrtp_mutex_lock(def_cache_protector);
	do {
		new_elem = get_elem(id, is_mitm);
		if (!new_elem)
		{	
			/* If cache doesn't exist - create ne one */
			if (!( new_elem = (zrtp_cache_elem_t*) zrtp_sys_alloc(sizeof(zrtp_cache_elem_t)) ))	{			
				s = zrtp_status_alloc_fail;
				break;
			}
					
			zrtp_memset(new_elem, 0, sizeof(zrtp_cache_elem_t));		
			ZSTR_SET_EMPTY(new_elem->curr_cache);
			ZSTR_SET_EMPTY(new_elem->prev_cache);
			
			new_elem->secure_since = zrtp_time_now()/1000;
							
			mlist_add_tail(is_mitm ? &mitmcache_head : &cache_head, &new_elem->_mlist);
			zrtp_memcpy(new_elem->id, id, sizeof(zrtp_cache_id_t));
		}
		
		/* Save current cache value as previous one and new as  a current */
		if (!is_mitm) {
			if (new_elem->curr_cache.length > 0) {
				zrtp_zstrcpy(ZSTR_GV(new_elem->prev_cache), ZSTR_GV(new_elem->curr_cache));
			}
		}

		zrtp_zstrcpy(ZSTR_GV(new_elem->curr_cache), ZSTR_GV(rss->value));
		new_elem->lastused_at	= rss->lastused_at;
		if (!is_mitm) {
			new_elem->ttl		= rss->ttl;
		}
	} while (0);	
	zrtp_mutex_unlock(def_cache_protector);

    return s;
}

zrtp_status_t zrtp_def_cache_put( const zrtp_stringn_t* one_ZID,
								  const zrtp_stringn_t* another_ZID,
								  zrtp_shared_secret_t *rss)
{
	return cache_put(one_ZID, another_ZID, rss, 0);
}

zrtp_status_t zrtp_def_cache_put_mitm( const zrtp_stringn_t* one_ZID,
									   const zrtp_stringn_t* another_ZID,
									   zrtp_shared_secret_t *rss)
{
	return cache_put(one_ZID, another_ZID, rss, 1);
}


/*----------------------------------------------------------------------------*/
static zrtp_status_t cache_get( const zrtp_stringn_t* one_ZID,
								const zrtp_stringn_t* another_ZID,
								zrtp_shared_secret_t *rss,
								int prev_requested,
								uint8_t is_mitm)
{
    zrtp_cache_elem_t* curr = 0;
	zrtp_cache_id_t	id;
	zrtp_status_t s = zrtp_status_ok;

	ZRTP_CACHE_CHECK_ZID(one_ZID, another_ZID);
	cache_create_id(one_ZID, another_ZID, id);
	
	zrtp_mutex_lock(def_cache_protector);
    do {		
		curr = get_elem(id, is_mitm);
		if (!curr || (!curr->prev_cache.length && prev_requested)) {
			s = zrtp_status_fail;
			break;
		}    
			
		zrtp_zstrcpy( ZSTR_GV(rss->value),
					  prev_requested ? ZSTR_GV(curr->prev_cache) : ZSTR_GV(curr->curr_cache));
		
		rss->lastused_at = curr->lastused_at;
		if (!is_mitm) {		
			rss->ttl = curr->ttl;
		}
	} while (0);
	zrtp_mutex_unlock(def_cache_protector);

    return s;
}

zrtp_status_t zrtp_def_cache_get( const zrtp_stringn_t* one_ZID,
								  const zrtp_stringn_t* another_ZID,
								  zrtp_shared_secret_t *rss,
								  int prev_requested)
{
	return cache_get(one_ZID, another_ZID, rss, prev_requested, 0);
}

zrtp_status_t zrtp_def_cache_get_mitm( const zrtp_stringn_t* one_ZID,
									   const zrtp_stringn_t* another_ZID,
									   zrtp_shared_secret_t *rss)
{
	return cache_get(one_ZID, another_ZID, rss, 0, 1);
}


/*-----------------------------------------------------------------------------*/
static void cache_create_id( const zrtp_stringn_t* first_ZID,
							 const zrtp_stringn_t* second_ZID,
							 zrtp_cache_id_t id )
{	
	if (0 < zrtp_memcmp(first_ZID->buffer, second_ZID->buffer, sizeof(zrtp_zid_t))) {
		const zrtp_stringn_t* tmp_ZID = first_ZID;
		first_ZID = second_ZID;
		second_ZID = tmp_ZID;
	}

	zrtp_memcpy(id, first_ZID->buffer, sizeof(zrtp_zid_t));
	zrtp_memcpy((char*)id+sizeof(zrtp_zid_t), second_ZID->buffer, sizeof(zrtp_zid_t));
}

/*-----------------------------------------------------------------------------*/
zrtp_cache_elem_t* zrtp_def_cache_get2(const zrtp_cache_id_t id, int is_mitm)
{
	return get_elem(id, is_mitm);
}

/*-----------------------------------------------------------------------------*/
static zrtp_cache_elem_t* get_elem(const zrtp_cache_id_t id, uint8_t is_mitm)
{
	mlist_t* node = NULL;
	mlist_t* head = is_mitm ? &mitmcache_head : &cache_head;
	mlist_for_each(node, head) {
		zrtp_cache_elem_t* elem = mlist_get_struct(zrtp_cache_elem_t, _mlist, node);
		if (!zrtp_memcmp(elem->id, id, sizeof(zrtp_cache_id_t))) {
			return elem;
		}
    }
    
    return NULL;	
}

/*----------------------------------------------------------------------------*/
static void cache_make_cross( zrtp_cache_elem_t* from,
							  zrtp_cache_elem_t* to,
							  uint8_t is_upload)
{
	if (!to) {
		return;
	}

	if (from) {
		zrtp_memcpy(to, from, sizeof(zrtp_cache_elem_t));
	}

	if (is_upload) {
		to->verified 	= zrtp_ntoh32(to->verified);
		to->secure_since= zrtp_ntoh32(to->secure_since);
		to->lastused_at = zrtp_ntoh32(to->lastused_at);
		to->ttl			= zrtp_ntoh32(to->ttl);
		to->name_length	= zrtp_ntoh32(to->name_length);
		to->curr_cache.length = zrtp_ntoh16(to->curr_cache.length);
		to->prev_cache.length = zrtp_ntoh16(to->prev_cache.length);
	} else {
		to->verified	= zrtp_hton32(to->verified);
		to->secure_since= zrtp_hton32(to->secure_since);
		to->lastused_at = zrtp_hton32(to->lastused_at);
		to->ttl			= zrtp_hton32(to->ttl);
		to->name_length	= zrtp_hton32(to->name_length);
		to->curr_cache.length = zrtp_hton16(to->curr_cache.length);
		to->prev_cache.length = zrtp_hton16(to->prev_cache.length);
	}
}


/*===========================================================================*/
/*     ZRTP cache realization as a simple binary file						 */
/*===========================================================================*/


#if ZRTP_HAVE_STDIO_H == 1
	#include <stdio.h>
#endif

/*---------------------------------------------------------------------------*/
#define ZRTP_INT_CACHE_BREAK(s, status) \
{ \
	if (!s) s = status; \
	break; \
}\

zrtp_status_t zrtp_cache_user_init()
{
	FILE* 	cache_file = 0;
	zrtp_cache_elem_t* new_elem = 0;
	zrtp_status_t s = zrtp_status_ok;	
	uint32_t cache_elems_count = 0;
	uint32_t mitmcache_elems_count = 0;
	uint32_t i = 0;
    
    /* Try to open existing file. If ther is no cache file - start with empty cache */
    if (0 == (cache_file = fopen(zrtp->def_cache_path.buffer, "rb"))) {
		return zrtp_status_ok;
	}

	/*
	 *  Load MitM caches: first 32 bits is a mitm secrets counter. Read it and then
	 *  upload appropriate number of MitM secrets.
	 */
	do {
		if (fread(&mitmcache_elems_count, 4, 1, cache_file) <= 0) {
			ZRTP_INT_CACHE_BREAK(s, zrtp_status_read_fail);
		}
		
		mitmcache_elems_count = zrtp_ntoh32(mitmcache_elems_count);		
		for (i=0; i<mitmcache_elems_count; i++)
		{
			new_elem = (zrtp_cache_elem_t*) zrtp_sys_alloc(sizeof(zrtp_cache_elem_t));
			if (!new_elem) {
				ZRTP_INT_CACHE_BREAK(s, zrtp_status_alloc_fail);
			}
			
			if (fread(new_elem, ZRTP_MITMCACHE_ELEM_LENGTH, 1, cache_file) <= 0) {
				zrtp_sys_free(new_elem);
				ZRTP_INT_CACHE_BREAK(s, zrtp_status_read_fail);
			}

			cache_make_cross(NULL, new_elem, 1);
			mlist_add_tail(&mitmcache_head, &new_elem->_mlist);
		}

		if (i != mitmcache_elems_count)
			ZRTP_INT_CACHE_BREAK(s, zrtp_status_read_fail);
	} while(0);
	if (s != zrtp_status_ok) {
		fclose(cache_file);
		zrtp_def_cache_down();
		return s;
	}

	/*
	 * Load regular caches: first 32 bits is a secrets counter. Read it and then
	 * upload appropriate number of regular secrets.
	 */
	if (fread(&cache_elems_count, 4, 1, cache_file) <= 0) {
		fclose(cache_file);
		zrtp_def_cache_down();
		return zrtp_status_read_fail;
	}		

	cache_elems_count = zrtp_ntoh32(cache_elems_count);
	for (i=0; i<cache_elems_count; i++)
	{
		new_elem = (zrtp_cache_elem_t*) zrtp_sys_alloc(sizeof(zrtp_cache_elem_t));
		if (!new_elem) {
			ZRTP_INT_CACHE_BREAK(s, zrtp_status_alloc_fail);
		}

		if (fread(new_elem, sizeof(zrtp_cache_elem_t)-sizeof(mlist_t), 1, cache_file) <= 0) {
			zrtp_sys_free(new_elem);
			ZRTP_INT_CACHE_BREAK(s, zrtp_status_read_fail);			
		}

		cache_make_cross(NULL, new_elem, 1);
		mlist_add_tail(&cache_head, &new_elem->_mlist);
	}
	if (i != cache_elems_count) {		
		s = zrtp_status_read_fail;
	}			

    if (0 != fclose(cache_file)) {
		zrtp_def_cache_down();
		return zrtp_status_fail;
    }

	return s;
}

/*---------------------------------------------------------------------------*/
#define ZRTP_DOWN_CACHE_RETURN(s, f) \
{\
	if (f) fclose(f);\
	return s;\
};

zrtp_status_t zrtp_cache_user_down()
{
	FILE* cache_file = 0;	
	zrtp_cache_elem_t tmp_elem;
	mlist_t *node = 0;
	uint32_t count = 0;
	uint32_t pos = 0;

    /* Open/create file for writing */
	cache_file = fopen(zrtp->def_cache_path.buffer, "wb+");
	if (!cache_file) {
		return zrtp_status_open_fail;
	}

    /* Store PBX secrets first. Format: <secrets count>, <secrets' data> */
	count = 0;
	fwrite(&count, sizeof(count), 1, cache_file);
	mlist_for_each(node, &mitmcache_head) {
		zrtp_cache_elem_t* elem = mlist_get_struct(zrtp_cache_elem_t, _mlist, node);
		cache_make_cross(elem, &tmp_elem, 0);
		if (fwrite(&tmp_elem, ZRTP_MITMCACHE_ELEM_LENGTH, 1, cache_file) != 1) {
			ZRTP_DOWN_CACHE_RETURN(zrtp_status_write_fail, cache_file);
		}
		count++;
	}

	fseek(cache_file, 0L, SEEK_SET);
	count = zrtp_hton32(count);
	if (fwrite(&count, sizeof(count), 1, cache_file) != 1) {
		ZRTP_DOWN_CACHE_RETURN(zrtp_status_write_fail, cache_file);
	}

	fseek(cache_file, 0L, SEEK_END);
	
	/* Store reqular secrets. Format: <secrets count>, <secrets' data> */
	pos = ftell(cache_file);
	count = 0;
	fwrite(&count, sizeof(count), 1, cache_file);
	mlist_for_each(node, &cache_head) {
		zrtp_cache_elem_t* elem = mlist_get_struct(zrtp_cache_elem_t, _mlist, node);
		cache_make_cross(elem, &tmp_elem, 0);
		if (fwrite(&tmp_elem, ZRTP_CACHE_ELEM_LENGTH, 1, cache_file) != 1) {
			ZRTP_DOWN_CACHE_RETURN(zrtp_status_write_fail, cache_file);
		}
		count++;
	}

	fseek(cache_file, pos, SEEK_SET);
	count = zrtp_hton32(count);
	if (fwrite(&count, sizeof(count), 1, cache_file) != 1) {
		ZRTP_DOWN_CACHE_RETURN(zrtp_status_write_fail, cache_file);
	}

	ZRTP_DOWN_CACHE_RETURN(zrtp_status_ok, cache_file);	
}


/*==========================================================================*/
/*						Utility  functions.								    */
/* These functions are example how cache can be used for internal needs     */
/*==========================================================================*/


/*----------------------------------------------------------------------------*/
static zrtp_status_t put_name( const zrtp_stringn_t* one_ZID,
							   const zrtp_stringn_t* another_ZID,
							   const zrtp_stringn_t* name,
							   uint8_t is_mitm)
{
    zrtp_cache_elem_t* new_elem = 0;
	zrtp_cache_id_t	id;
	zrtp_status_t s = zrtp_status_ok;

	ZRTP_CACHE_CHECK_ZID(one_ZID, another_ZID);   
	cache_create_id(one_ZID, another_ZID, id);
	
	zrtp_mutex_lock(def_cache_protector);
	do {
		new_elem = get_elem(id, is_mitm);
		if (!new_elem) {			
			s = zrtp_status_fail;
			break;
		}

		/* Update regular cache name*/
		new_elem->name_length = ZRTP_MIN(name->length, ZFONE_CACHE_NAME_LENGTH-1);
		zrtp_memset(new_elem->name, 0, sizeof(new_elem->name));
		zrtp_memcpy(new_elem->name, name->buffer, new_elem->name_length);
	} while (0);
	zrtp_mutex_unlock(def_cache_protector);
	
	return s;
}


zrtp_status_t zrtp_def_cache_put_name( const zrtp_stringn_t* one_ZID,
									   const zrtp_stringn_t* another_ZID,
									   const zrtp_stringn_t* name)
{
	return put_name(one_ZID, another_ZID, name, 0);
}


/*----------------------------------------------------------------------------*/
static zrtp_status_t get_name( const zrtp_stringn_t* one_ZID,
							   const zrtp_stringn_t* another_ZID,
							   zrtp_stringn_t* name,
							   uint8_t is_mitm)
{
    zrtp_cache_elem_t* new_elem = 0;
	zrtp_cache_id_t	id;
	zrtp_status_t s = zrtp_status_fail;

	ZRTP_CACHE_CHECK_ZID(one_ZID, another_ZID);	
	cache_create_id(one_ZID, another_ZID, id);
	
	zrtp_mutex_lock(def_cache_protector);
	do {
		new_elem = get_elem(id, is_mitm);
		if (!new_elem) {			
			s = zrtp_status_fail;
			break;
		}
		
		name->length = new_elem->name_length;
		zrtp_memcpy(name->buffer, new_elem->name, name->length);
		s = zrtp_status_ok;
	} while (0);
	zrtp_mutex_unlock(def_cache_protector);

	return s;
}

zrtp_status_t zrtp_def_cache_get_name( const zrtp_stringn_t* one_zid,
									   const zrtp_stringn_t* another_zid,
									   zrtp_stringn_t* name)
{
	return get_name(one_zid, another_zid, name, 0);
}


/*----------------------------------------------------------------------------*/
zrtp_status_t zrtp_def_cache_get_since( const zrtp_stringn_t* one_ZID,
									    const zrtp_stringn_t* another_ZID,
									    uint32_t* since)
{
    zrtp_cache_elem_t* new_elem = 0;
	zrtp_cache_id_t	id;
	zrtp_status_t s = zrtp_status_ok;

	ZRTP_CACHE_CHECK_ZID(one_ZID, another_ZID);	   
	cache_create_id(one_ZID, another_ZID, id);

	zrtp_mutex_lock(def_cache_protector);
	do {
		new_elem = get_elem(id, 0);
		if (!new_elem) {
			s = zrtp_status_fail;
			break;
		}
		
		*since = new_elem->secure_since;
	} while (0);
	zrtp_mutex_unlock(def_cache_protector);

	return s;
}

zrtp_status_t zrtp_def_cache_reset_since( const zrtp_stringn_t* one_zid,
										  const zrtp_stringn_t* another_zid)
{
	zrtp_cache_elem_t* new_elem = 0;
	zrtp_cache_id_t	id;
	zrtp_status_t s = zrtp_status_ok;
	
	ZRTP_CACHE_CHECK_ZID(one_zid, another_zid);	   
	cache_create_id(one_zid, another_zid, id);
	
	zrtp_mutex_lock(def_cache_protector);
	do {
		new_elem = get_elem(id, 0);
		if (!new_elem) {
			s = zrtp_status_fail;
			break;
		}
		
		new_elem->secure_since = zrtp_time_now()/1000;
	} while (0);
	zrtp_mutex_unlock(def_cache_protector);
	
	return s;
}


/*----------------------------------------------------------------------------*/
void zrtp_def_cache_foreach( zrtp_global_t *global,
							 int is_mitm,
							 zrtp_cache_callback_t callback,
							 void *data)
{
	int delete, result;
	mlist_t* node = NULL, *tmp_node = NULL;

	zrtp_mutex_lock(def_cache_protector);	
	mlist_for_each_safe(node, tmp_node, (is_mitm ? &mitmcache_head : &cache_head))
    {		
		zrtp_cache_elem_t* elem = mlist_get_struct(zrtp_cache_elem_t, _mlist, node);
		result = callback(elem, is_mitm, data, &delete);
		if (delete) {
			mlist_del(&elem->_mlist);
		}
		if (!result) {
			break;
		}
	}
	zrtp_mutex_unlock(def_cache_protector);
	
	return;
}

/*----------------------------------------------------------------------------*/
zrtp_status_t zrtp_def_cache_store(zrtp_global_t *zrtp)
{
	zrtp_mutex_lock(def_cache_protector);
	zrtp_cache_user_down();
	zrtp_mutex_unlock(def_cache_protector);
	
	return zrtp_status_ok;
}

#endif /* ZRTP_PLATFORM != ZP_WIN32_KERNEL */

#endif /* ZRTP_DONT_USE_BUILTIN */
