/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
 */

#ifndef __ZFONEG_LISTBOX_H__
#define __ZFONEG_LISTBOX_H__

#include <time.h>

#include <gtk/gtk.h>

#include "zfoneg_cb.h"

#define	ZFONEG_STREAMS_COUNT	2

//==============================================================================
// Left side list entry
//==============================================================================

typedef struct list_box list_box_t;
typedef struct list_conn_info list_conn_info_t;

typedef struct list_streams_info
{
    unsigned int 		state;
    unsigned char		type;

    
    unsigned char		allowclear;			/*! staysecure flag for current connection */
    uint8_t				is_mitm;
    unsigned int		rtp_states[2];
    unsigned char		disclose_bit;
    zrtp_uchar4_t		pkt;			/*! publik keys exchange scheme for current connection */    
	unsigned int 		pbx_reg_ok;
    unsigned int 		cache_ttl;
	unsigned int		passive_peer;
	unsigned char		hear_ctx;
    
    list_conn_info_t	*session;
} list_streams_info_t;


struct list_conn_info
{
    zfone_session_id_t	session_id;
    zrtp_string16_t		zrtp_peer_client;
    zrtp_string8_t		zrtp_peer_version;
    zrtp_string128_t	name;
    unsigned int		secure_since;
    uint8_t				verified;
    
    zrtp_string16_t		sas1;
	zrtp_string16_t		sas2;

    unsigned char		autosecure;			/*! autosecure flag for current connection */

    zrtp_uchar4_t		cipher;			/*! chiper type for current connection */
    zrtp_uchar4_t		atl;			/*! SRTP auth tag length for current connection */
    zrtp_uchar4_t		sas_scheme;		/*! SAS scheme for current connection */
    zrtp_uchar4_t		hash;			/*! HASH calculation scheme for current connection */


	unsigned int		matches;
	unsigned int 		cached;
	

    list_streams_info_t	streams[ZRTP_MAX_STREAMS_PER_SESSION];
};


typedef struct
{
    list_box_t	  		*owner;
    GtkWidget	  		*hbox;
    GtkWidget	  		*btn;
    GtkWidget	  		*icons[ZFONEG_STREAMS_COUNT];
    GtkWidget	  		*label;
    int					sas_burn_flag, sas_index;
    list_conn_info_t    conn_info;
} list_box_item_t;



//==============================================================================
// Left side list itself
//==============================================================================



struct list_box
{
    GtkWidget		*layout; 		// vbox
    GList			*list;   		// Item list
    int				selected_item;
};



//==============================================================================
// List managment functions
//==============================================================================



#define LIST_TO_GTKWIDGET(lb)	((lb)->layout)
/*----------------------------------------------------------------------------*/

// Creating list and items functions
void create_list_box(list_box_t *lb);
void init_list_box(list_box_t *lb);
void show_list_box(list_box_t *lb);

/*----------------------------------------------------------------------------*/

// Managing listbox items
list_box_item_t* selected(list_box_t *lb);
void             select_item(list_box_t *lb, list_box_item_t *item);

// Removes specified entry.
void             remove_entry(list_box_t	  *lb
			     ,zfone_session_id_t   session_id);

list_box_item_t* create_stream(list_box_t    *lb, const char    *name, int state,
	    		       zfone_session_id_t   session_id, stream_id_t   stream_id);


// Lookup for specified entry
list_box_item_t* listbox_find_item_with_session(list_box_t *lb, zfone_session_id_t session_id);

/*----------------------------------------------------------------------------*/

void entry_changed_state(list_box_item_t *item, int state, int stream_id);

void set_phone(list_box_t *lb, zfone_session_id_t session_id, const char* phone);

void relight_icons(list_box_t *lb);

void remove_all_entries(list_box_t *lb);

zrtp_streams_info_t* active_stream(list_box_t* lb);

int check_secured(list_box_item_t *item);

void  set_rtp_activity(list_box_item_t* item, int type, int direction, int activity);

int is_analyzing_state(list_box_item_t* item);

int renew_listbox_items(list_box_t* lb, zrtp_conn_info_t *conn_list, int count);

int is_session_cleared(list_conn_info_t *session);

int is_session_secured(list_conn_info_t *session);

int is_session_allowed_clear(list_conn_info_t *session);

int is_session_pendingclear(list_conn_info_t *session);

list_streams_info_t *get_secured_stream(list_conn_info_t *session);

int is_session_pbx_ok(list_conn_info_t *session);

int is_session_mitm(list_conn_info_t *session);

int is_session_passive_peer(list_conn_info_t *session);

#endif //__ZFONEG_LISTBOX_H__
