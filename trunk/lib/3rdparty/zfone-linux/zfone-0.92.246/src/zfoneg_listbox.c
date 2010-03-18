/* Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
 */


#include <stdlib.h>
#include <string.h>
#include "zfoneg_config.h"

#include "files.h"
#include "zfoneg_listbox.h"
#include "zfoneg_ctrl.h"
#include "zfoneg_utils.h"

// colors for states
enum {RED = 0, ORANGE = 1, GREEN = 2, GREY = 3, BLUE = 4};

//static GdkPixbuf *led_pb[4];
static char *led_pb[] =
{
    ICON_RED, 
    ICON_YELLOW,
    ICON_GREEN,
    ICON_GREY,
    ICON_BLUE
};

//! maps states to colors
static int led_color[] = { RED, ORANGE, ORANGE, ORANGE, GREY, GREY, GREEN, RED, RED, RED };

//==================== CREATION ================================================

//! Staff for manipulating state visuals
void create_list_box(list_box_t *lb)
{
    if ( !lb ) return;
    
    lb->layout = gtk_vbox_new(TRUE, 1);
    gtk_box_set_homogeneous(GTK_BOX(lb->layout), FALSE);
    lb->list   = NULL; // we'll add entry later here
}

//------------------------------------------------------------------------------
void init_list_box(list_box_t *lb)
{
}



//==================== SEARCH ==================================================

static int search_counter;
static int disable_clickings;
//------------------------------------------------------------------------------
//! compares items button and the given one and sets found item as current
void list_finder(gpointer p1, gpointer btn)
{
    GtkWidget       *button_widget = GTK_WIDGET(btn);
    list_box_item_t *item          = (list_box_item_t*)p1;
    
    if ( item->btn == button_widget )
    {
		item->owner->selected_item = search_counter;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->btn), TRUE);
		update_main_view(1);
    }
    else
    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->btn), FALSE);
    }
    search_counter++;
}

//------------------------------------------------------------------------------
//! on call button pressed
void entry_clicked(gpointer btn, gpointer glb)
{
    list_box_t *lb = (list_box_t*)glb;

    if ( !disable_clickings )
    {
        disable_clickings = 1;
	search_counter = 0;    
        
        g_list_foreach(lb->list, list_finder, btn );
	
	disable_clickings = 0;
    }
}

//------------------------------------------------------------------------------
struct find_result
{
    list_box_item_t *entry;
    zfone_session_id_t     session_id;
    unsigned short   padding;
};

//------------------------------------------------------------------------------
//! find item by ip and port
void entry_finder(gpointer p1, gpointer res)
{
    struct find_result* r          = (struct find_result*)res;
    list_box_item_t *item     	   = (list_box_item_t*)p1;
    
    if ( !memcmp(item->conn_info.session_id, r->session_id, ZFONE_SESSION_ID_SIZE) )
    {
	r->entry = item;
    }
}

//------------------------------------------------------------------------------
//! return item with given ip and port
list_box_item_t* listbox_find_item_with_session(list_box_t* lb, zfone_session_id_t session_id)
{
    struct find_result res;
    memcpy(res.session_id, session_id, ZFONE_SESSION_ID_SIZE);
    res.entry = NULL;
    
    g_list_foreach(lb->list, entry_finder, &res);
    
    return res.entry;
}

//==================== ADD =====================================================
//------------------------------------------------------------------------------
//! adds new call
list_box_item_t* add_entry(list_box_t *lb, const char *name)
{
    int i;
    GList*	updated_list     = NULL;
	char name_buf[20];
    
    list_box_item_t *item = (list_box_item_t*)malloc(sizeof(list_box_item_t));

    if ( !item ) 
    {
	return NULL;
    }
    
	
	if (strlen(name) > 13)
	{
  		strncpy(name_buf, name, 13);
		name_buf[13] = 0;
		strcat(name_buf, "...");
	}
	else
		strcpy(name_buf, name);
    // new box
    item->hbox 	   = gtk_hbox_new(TRUE, 5);
    item->btn      = gtk_toggle_button_new_with_label(name_buf);
    item->owner    = lb;

    // new image view depending on items state
    gtk_box_set_homogeneous(GTK_BOX(item->hbox), FALSE);
    for (i = 0; i < ZFONEG_STREAMS_COUNT; i++)
    {
	item->icons[i]  = gtk_image_new_from_file(ICON_GREY/*led_pb[led_color[state2pic[state]]]*/);
	gtk_box_pack_start(GTK_BOX(item->hbox), item->icons[i], FALSE, FALSE, 0);
	gtk_widget_show(item->icons[i]);
    }
    gtk_box_pack_start(GTK_BOX(item->hbox), item->btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(lb->layout), item->hbox, FALSE, FALSE, 0);

    gtk_signal_connect (GTK_OBJECT (item->btn), "clicked",
            		    GTK_SIGNAL_FUNC (entry_clicked), lb);
	
    // add new item to the list
    updated_list = g_list_append(lb->list, item);

    if ( updated_list )
    {
        lb->list = updated_list;
    }
    	
    // we'll show call buttons if there are two or more buttons
    if ( g_list_length(lb->list) > 1 )
    {
        gtk_widget_show(lb->layout);
    }

    gtk_widget_show(item->hbox);
    gtk_widget_show(item->btn);

    return item;
}


//==================== DELETE ==================================================

//------------------------------------------------------------------------------
//! removes item
void free_entry(gpointer entry, gpointer data)
{
    list_box_t *lb = (list_box_t *)data;
    list_box_item_t *item     = (list_box_item_t*)entry;
    gtk_container_remove( GTK_CONTAINER(lb->layout), GTK_WIDGET(item->hbox));
    free(entry);    
}

//------------------------------------------------------------------------------
//! clear the list
void remove_all_entries(list_box_t *lb)
{
    if (!lb)
    {
	return;
    }
    lb->selected_item = -1;

    gtk_widget_hide(lb->layout);
    g_list_foreach(lb->list, free_entry, lb);
    lb->list = NULL;
    g_list_free(lb->list);
}

//------------------------------------------------------------------------------
static void disable_buttons(gpointer p1, gpointer arg)
{
    list_box_item_t *item = (list_box_item_t*)p1;
    if ( !search_counter++ )
    {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->btn), TRUE);
    }
    else
    {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->btn), FALSE);
    }
}


//------------------------------------------------------------------------------
static void set_zero_active(list_box_t* lb)
{
    search_counter = 0;
    disable_clickings = 1;
    g_list_foreach(lb->list, disable_buttons, NULL);
    disable_clickings = 0;
}

static void do_remove_entry(list_box_t *lb, list_box_item_t *item)
{
    if ( item )
    {
	if ( g_list_length(lb->list) <= 2 )
	{
	    //gtk_widget_hide(lb->layout);
	}

	// 2. remove widget from lb->layout
	gtk_container_remove( GTK_CONTAINER(lb->layout), GTK_WIDGET(item->hbox));

	// 4. remove item from GList (lb->list)
	lb->list = g_list_remove(lb->list, item);
	free(item);
	
	if ( lb->list && g_list_length(lb->list) )
	{
	    lb->selected_item = 0;
	    set_zero_active(lb);
//	    update_main_view();
	}
	else
	{
	    lb->selected_item = -1;
//	    set_both_indicators(IDLE_PATH_PIC);
	}
    }
    else
    {
		printf("!!!!!!ERROR!!!!!: zfoneg_listbox, remove_entry: was trying to remove session while its not in the list\n");
    }
}

//------------------------------------------------------------------------------
//! remove item by ip and port
void remove_entry(list_box_t *lb, zfone_session_id_t session_id)
{
    struct find_result res;
    memcpy(res.session_id, session_id, ZFONE_SESSION_ID_SIZE);
    
    res.entry = NULL;
    g_list_foreach(lb->list, entry_finder, &res);
    if ( res.entry )
    {
	do_remove_entry(lb, res.entry);
    }
}

//------------------------------------------------------------------------------
void show_list_box(list_box_t *lb)
{
    gtk_widget_show(lb->layout);
}

void relight_icons(list_box_t *lb)
{
    int j;
    for (j = 0; j  < g_list_length(lb->list); j++)
    {
		int i, state;
		list_box_item_t *item = g_list_nth_data(lb->list, j);
		gtk_image_set_from_file(GTK_IMAGE(item->icons[0]), ICON_GREY);
		gtk_image_set_from_file(GTK_IMAGE(item->icons[1]), ICON_GREY);
		for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
		{
	  		if ( is_active(&item->conn_info.streams[i]) )
	  		{	
				int index = (item->conn_info.streams[i].type == ZFONE_SDP_MEDIA_TYPE_AUDIO ? 0 : 1);
				state = item->conn_info.streams[i].state;
				gtk_image_set_from_file(GTK_IMAGE(item->icons[index]), led_pb[led_color[state2pic[state]]]);
	  		}
		}
		gtk_widget_queue_draw(item->hbox);
    }
}

//==================== PARAMETERS ==============================================
//------------------------------------------------------------------------------
//! sets call name
void set_phone(list_box_t *lb, zfone_session_id_t session_id, const char* phone)
{
    list_box_item_t *item = listbox_find_item_with_session(lb, session_id);

    if ( item )
    {
	gtk_button_set_label(GTK_BUTTON(item->btn), phone);
    }
}

//------------------------------------------------------------------------------
int check_secured(list_box_item_t *item)
{
    int result = 0;
/*
    if (!item) 
    {
	return 0;
    }
	
    if ( is_active(&item->conn_info.streams[STREAM_TYPE_AUDIO]) )
    {
	result |= (item->conn_info.streams[STREAM_TYPE_AUDIO].state == ZRTP_STATE_SECURE );
	printf("audio state = %d\n", item->conn_info.streams[STREAM_TYPE_AUDIO].state);
    }
    if ( is_active(&item->conn_info.streams[STREAM_TYPE_VIDEO]) )
    {
	result |= (item->conn_info.streams[STREAM_TYPE_VIDEO].state == ZRTP_STATE_SECURE );
	printf("video state = %d\n", item->conn_info.streams[STREAM_TYPE_AUDIO].state);
    }
*/
    return result;
}



//==================== SELECTION ===============================================
//------------------------------------------------------------------------------
//! returms selected item
list_box_item_t* selected(list_box_t* lb)
{
    return g_list_nth_data(lb->list, lb->selected_item);
}

//==================== RENEW ===================================================
//------------------------------------------------------------------------------
static void renew_item(list_box_item_t *item, zrtp_conn_info_t *conn)
{
    int i;
    list_conn_info_t *session = &item->conn_info;

    memcpy(session->session_id, conn->session_id, sizeof(zfone_session_id_t));
    memcpy(&session->zrtp_peer_client, &conn->zrtp_peer_client, sizeof(session->zrtp_peer_client));
    memcpy(&session->zrtp_peer_version, &conn->zrtp_peer_version, sizeof(session->zrtp_peer_version));
    memcpy(&session->name, &conn->name, sizeof(session->name));
    session->secure_since = conn->secure_since;
    session->verified = conn->is_verified;
    memcpy(&session->sas1, &conn->sas1, sizeof(zrtp_string16_t));
    memcpy(&session->sas2, &conn->sas2, sizeof(zrtp_string16_t));

	session->matches = conn->matches;
	session->cached = conn->cached;

	session->autosecure = conn->is_autosecure;
	memcpy(session->cipher, conn->cipher, sizeof(zrtp_uchar4_t));
	memcpy(session->atl,    conn->atl,    sizeof(zrtp_uchar4_t));
	memcpy(session->sas_scheme, conn->sas_scheme, sizeof(zrtp_uchar4_t));
	memcpy(session->hash,   conn->hash,   sizeof(zrtp_uchar4_t));
	
    for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
    {
		list_streams_info_t *stream = &session->streams[i];
		zrtp_streams_info_t *zrtp_stream = &conn->streams[i];
		
	  	stream->state         = zrtp_stream->state;
		stream->type          = zrtp_stream->type;
		stream->allowclear = zrtp_stream->allowclear;
		stream->disclose_bit = zrtp_stream->disclose_bit;

		memcpy(stream->pkt,    zrtp_stream->pkt,    sizeof(zrtp_uchar4_t));

		stream->cache_ttl = zrtp_stream->cache_ttl;
		stream->session = session;
		stream->rtp_states[ZFONE_IO_IN]  = zrtp_stream->rx_state;
		stream->rtp_states[ZFONE_IO_OUT] = zrtp_stream->tx_state;
		stream->is_mitm  = zrtp_stream->is_mitm;
		stream->pbx_reg_ok = zrtp_stream->pbx_reg_ok;
		stream->passive_peer = zrtp_stream->passive_peer;
		stream->hear_ctx = zrtp_stream->hear_ctx;
    }
}

//------------------------------------------------------------------------------
int renew_listbox_items(list_box_t* lb, zrtp_conn_info_t *conn_list, int count)
{
    int i, tmp, length = g_list_length(lb->list);

    for (i = 0; i < count; i++)
    {
		zrtp_conn_info_t *conn = &conn_list[i];
		list_box_item_t *item;
		if (i >= length)
  		{
	  		item = add_entry(lb, conn->name.buffer);
		}
		else
		{
	  		item = g_list_nth_data(lb->list, i);
		}
		renew_item(item, conn);
    }
    if ((i && (lb->selected_item < 0 || lb->selected_item >= i)) || (!length && count))
    {
		lb->selected_item   = 0;
		set_zero_active(lb);
    }
    tmp = i;
    for (; i < length; i++)
    {
		do_remove_entry(lb, g_list_nth_data(lb->list, tmp));
    }

	if (count < 2)
	{
  		gtk_widget_hide(lb->layout);
	}
    return count;
}

void  set_rtp_activity(list_box_item_t* item, int type, int direction, int activity)
{
	int i;
    for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
    {
		if (item->conn_info.streams[i].type == type)
		{
			item->conn_info.streams[i].rtp_states[direction] = activity;
		}
	}	
}

int is_analyzing_state(list_box_item_t* item)
{
	return find_active_stream(item) == -1;
}

int is_session_in_state(list_conn_info_t *session, zrtp_state_t state)
{
	int i, hitted = 0;
	for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
	{
		list_streams_info_t *stream = &session->streams[i];
		if ( !is_active(stream) )
		{
			continue;
		}
	
		if (stream->state != state)
			return 0;
		hitted = 1;
	}
	return hitted;
}

int is_session_cleared(list_conn_info_t *session)
{
	return is_session_in_state(session, ZRTP_STATE_CLEAR);
}

int is_session_secured(list_conn_info_t *session)
{
	return is_session_in_state(session, ZRTP_STATE_SECURE);
}

int is_session_allowed_clear(list_conn_info_t *session)
{
	int i, result = 0;
	for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
	{
		list_streams_info_t *stream = &session->streams[i];
		if ( !is_active(stream) )
			continue;
	
		if (stream->state != ZRTP_STATE_SECURE)
			return 0;
		result |= stream->allowclear;
	}
	return result;
}

int is_session_pbx_ok(list_conn_info_t *session)
{
	int i;
	for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
	{
		list_streams_info_t *stream = &session->streams[i];
		if ( !is_active(stream) )
			continue;
		if (stream->pbx_reg_ok)
		{
			return stream->pbx_reg_ok;
		}
	}
	return 0;
}

int is_session_mitm(list_conn_info_t *session)
{
	int i;
	for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
	{
		list_streams_info_t *stream = &session->streams[i];
		if ( !is_active(stream) )
			continue;
	
		if (stream->is_mitm)
			return 1;
	}
	return 0;
}


/*
void get_session_sasses(list_conn_info_t *session, zrtp_string16_t *sas1, zrtp_string16_t *sas2)
{
	int i;
	sas1->length = sas2->length = 0;
	for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
	{
		list_streams_info_t *stream = &session->streams[i];
		if ( !is_active(stream) )
		{
			continue;
		}
		if ( stream->state == ZRTP_STATE_SECURE )
		{
			if ( stream->sas2.length )
				memcpy(sas2, &stream->sas2, sizeof(zrtp_string16_t));
			if ( stream->sas1.length )
			{
				memcpy(sas1, &stream->sas1, sizeof(zrtp_string16_t));
				return;
			}
		}
	}
}
*/

int is_session_pendingclear(list_conn_info_t *session)
{
	int i;
	for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
	{
		list_streams_info_t *stream = &session->streams[i];
		if ( !is_active(stream) )
			continue;
	
		if (stream->state == ZRTP_STATE_PENDINGCLEAR)
			return 1;
	}
	return 0;
}

list_streams_info_t *get_secured_stream(list_conn_info_t *session)
{
	list_streams_info_t *result = NULL;
	int i;
	for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
	{
		list_streams_info_t *stream = &session->streams[i];
		if ( !is_active(stream) )
			continue;
	
		if (stream->state == ZRTP_STATE_SECURE)
		{
			if (memcmp(stream->pkt, ZRTP_MULT, 4))
				return stream;
			result = stream;
		}
	}
	return result;
}

int is_session_passive_peer(list_conn_info_t *session)
{
	int i;
	for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
	{
		list_streams_info_t *stream = &session->streams[i];
		if ( !is_active(stream) )
			continue;
		if (stream->passive_peer)
			return 1;
	}
	return 0;
}
