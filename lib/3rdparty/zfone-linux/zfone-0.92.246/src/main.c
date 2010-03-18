/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
*/

#include <sys/file.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkevents.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <gdk/gdkkeysyms.h>

#include "zrtp_types.h"

#include "zfoneg_commands.h"

#include "files.h"
#include "zfoneg_config.h"
#include "zfoneg_ctrl.h"
#include "zfoneg_cb.h"
#include "zfoneg_listbox.h"
#include "zfoneg_tcpconn.h"
#include "zfoneg_pref_form.h"
#include "zfoneg_check_form.h"
#include "zfoneg_secure_params.h"
#include "zfoneg_utils.h"

#include <zrtp_protocol.h>

#define TEST_BUILD		1

#define NAME_FIELD_LENGTH	18

#define ZFONE_LOCK_FILE		"/tmp/zfone.lock"

struct 
{
    char	strings[4][5];
    int		index, burn_flag;
} sas_holder;

enum
{
    MENU_UPDATE = 1,	
    MENU_ABOUT,	
    MENU_PREF,	
    MENU_SEC_PAR,
    MENU_CLEAN_LOGS,
    MENU_RESET_DAEMON,
    MENU_CACHES,
    MENU_REMEMBER,
    MENU_QUIT,
    MENU_REPORT,
    MENU_VERIFIED
};

list_box_t _lb;	// List box containing calls

// file names, which appropriate to states 
char pixmap[][128] = 
{
    NOT_SECURE_PIC,
    PRESS_CLEAR_PIC,
    SECURING_PIC,
    WAITING_PIC,
    IDLE_PATH_PIC,
    NO_FILTER_PIC,
    SECURE_AUDIO_PIC,
    NO_ZRTP_PIC,
    LOOKING_PIC,
    ERROR_PIC
};

static char* rtp_colors_rx[] =
{
	RTP_RX_GREY_PIC,
	RTP_RX_GREY_PIC,
	RTP_RX_RED_PIC,
	RTP_RX_GREEN_PIC
};

static char* rtp_colors_tx[] =
{
	RTP_TX_GREY_PIC,
	RTP_TX_GREY_PIC,
	RTP_TX_RED_PIC,
	RTP_TX_GREEN_PIC
};

static FILE *lock = NULL;

// menu items, which can be disabled
static GtkWidget *pref_menu_item;
static GtkWidget *check_menu_item;
static GtkWidget *sec_par_menu_item;
static GtkWidget *clean_logs_menu_item;
//static GtkWidget *clean_cache_menu_item;
static GtkWidget *reset_menu_item;
static GtkWidget *cache_menu_item;
static GtkWidget *remember_menu_item;
static GtkWidget *verified_menu_item;
static GtkWidget *pref_menu_status;

const gchar *list_item_data_key = "list_item_data";

// main window
static GtkWidget *window;
// main vbox, it contains menu and vbox
static GtkWidget *main_vbox;
// vbox contains all controls. we need it for bordering
static GtkWidget *vbox;
// verifying checkbox
//static GtkWidget *verified;
// connection name
static GtkWidget *name;
// indicators
static GtkWidget *indicator_audio, *indicator_video; 
// hboxes for indicators
static GtkWidget *hbox_audio, *hbox_video;
// clear buttom
static GtkWidget *clear_button;
// sas string
static GtkWidget *label_sas;
// start securing date
static GtkWidget *secure_since;
// secure button
static GtkWidget *secure_button;
// menu
static GtkWidget *menubar;

static GtkWidget *light_in_audio, *light_out_audio, *light_in_video, *light_out_video, *event_box_audio, *event_box_video;
static GtkWidget* padlock, *padlock_eb;

static GtkTooltips *tooltips;

static GtkWidget *label_sas2;

static GtkWidget *status_menu = NULL;

static int	has_prefs;
//! menu callback
static void topmenuitem_response( GtkWidget *w, gpointer data );
//! inits state2pic array
static void init_pixmaps();
static void set_SAS(zrtp_string16_t *sas1, zrtp_string16_t *sas2);
//! reset main view controls
static void secure_state(void);
static void set_secure_button_enabled(const int is_on);
static void set_clear_button_enabled(const int is_on);
//! on press CLEAR button
static void clear_state(void);
//! on press SECURE buton
static void secure_state(void);
//! signals handler
static void sig_handler(int signum);
//! show modal alert
static void show_info(const char* message, GtkMessageType type);
// name field key event handler
static gboolean name_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static void show_about_dialog(void);
//! create application menu
static void get_main_menu( GtkWidget  *window, GtkWidget **menubar );
//! show state picture in the indicator
static void show_state(list_box_item_t *item);
static void rtp_lights(int stream, int direction, int type);
static void change_sas_color(char *color);
static void show_report_dialog();
static void verified_clicked(void* data);

static GtkItemFactoryEntry menu_items[] = 
{
    { "/_Zfone",         				NULL,			NULL, 		      		0, 					"<Branch>" },
    { "/Zfone/_Preferences",			"<control>P",	topmenuitem_response,	MENU_PREF,    		NULL },
    { "/Zfone/Connections _settings",	"<control>S",	topmenuitem_response,	MENU_SEC_PAR, 		NULL },
    { "/Zfone/_Quit",					"<control>Q",	topmenuitem_response,	MENU_QUIT,   		NULL },
    { "/_Edit",         				"<control>E", 	NULL, 		      		0, 					"<Branch>" },
    { "/_Edit/Clear _logs",				"<control>L",	topmenuitem_response, 	MENU_CLEAN_LOGS,	NULL },
    { "/Edit/_Caches",					NULL,			topmenuitem_response, 	MENU_CACHES,  		NULL },
    { "/Edit/_Reset daemon",			NULL,			topmenuitem_response, 	MENU_RESET_DAEMON,  NULL },
    { "/Edit/Note that authentication string checked",NULL,verified_clicked, 	MENU_VERIFIED,		"<CheckItem>" },
    { "/Edit/Register with this _PBX",	NULL,			topmenuitem_response, 	MENU_REMEMBER,      "<CheckItem>" },
    { "/_Help",         				NULL, 			NULL, 		      		0, 					"<Branch>" },
    { "/Help/_About",					NULL,			topmenuitem_response,	MENU_ABOUT,   		NULL },
    { "/Help/_Check for updates",		NULL,			topmenuitem_response,	MENU_UPDATE,  		NULL },
    { "/Help/Create bug _report",		NULL,			topmenuitem_response,	MENU_REPORT,  		NULL },
};

static GtkItemFactory *item_factory;

enum
{
    INDICATOR_VIDEO = 0,
    INDICATOR_AUDIO = 1
};

enum
{
    VERIFIED_DISABLED,
    VERIFIED_CHECKED,
    VERIFIED_UNCHECKED
};

static int main_x = 0;
static int main_y = 0;
static int auto_showed = 0;

static int ignore_verification = 0;
static int ignore_remembering = 0;

static void show_main_window(int automode)
{
	if (GTK_WIDGET_VISIBLE(window))
		return;
	gtk_window_move(GTK_WINDOW(window), main_x, main_y);
	gtk_widget_show(window);
	auto_showed = automode;
}

static void hide_main_window(int automode)
{
	if (!GTK_WIDGET_VISIBLE(window))
		return;
	if (automode && !auto_showed)
		return;
	gtk_window_get_position(GTK_WINDOW(window), &main_x, &main_y);
	gtk_widget_hide(window);
}

static void set_verified_state(int state)
{
    ignore_verification = 1;
    switch (state)
    {
	case VERIFIED_CHECKED:
	    gtk_image_set_from_file(GTK_IMAGE(padlock), PADLOCK_CHECKED_PIC);
	    gtk_widget_show(padlock);
	    gtk_widget_show(padlock_eb);
	    gtk_widget_set_sensitive(verified_menu_item, TRUE);
	    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(verified_menu_item), 1);
  	    change_sas_color("black");
	    break;
	case VERIFIED_UNCHECKED:
	    gtk_image_set_from_file(GTK_IMAGE(padlock), PADLOCK_PIC);
	    gtk_widget_show(padlock);
	    gtk_widget_show(padlock_eb);
	    gtk_widget_set_sensitive(verified_menu_item, TRUE);
	    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(verified_menu_item), 0);
  	    change_sas_color("red");
	    break;
	default:
	    gtk_widget_hide(padlock);
	    gtk_widget_hide(padlock_eb);
	    gtk_image_set_from_file(GTK_IMAGE(padlock), PADLOCK_DISABLED_PIC);
	    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(verified_menu_item), 0);
	    gtk_widget_set_sensitive(verified_menu_item, FALSE);
  	    change_sas_color("red");
	    break;
    }
    ignore_verification = 0;
}

static void set_remember_state(int value)
{
	ignore_remembering = 1;

	if (value < 0)
	{
		gtk_widget_set_sensitive(remember_menu_item, TRUE);
		gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(remember_menu_item), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(remember_menu_item, value);
		gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(remember_menu_item), FALSE);
	}

	ignore_remembering = 0;
}

//------------------------------------------------------------------------------
void update_main_view(int make_top)
{
    list_box_item_t *item = selected(&_lb);

    gtk_widget_set_sensitive(sec_par_menu_item, FALSE);

    if ( item )
    {
		if (make_top)
			show_main_window(1);	
		pref_form_no_active(0);

		if ( is_analyzing_state(item) )
		{
			clear_main_view();
  			set_indicator(ANALYZING_PIC, ZFONE_SDP_MEDIA_TYPE_AUDIO);
  			set_indicator(IDLE_PATH_PIC, ZFONE_SDP_MEDIA_TYPE_VIDEO);
	  		return;
		}
		list_conn_info_t *conn = &item->conn_info;

		if ( get_secured_stream(conn) )
		{
			set_remember_state(is_session_pbx_ok(conn));
		}

		if ( is_session_secured(conn) )
		{
			int i, ver_en = 1;
	  		set_SAS(&conn->sas1, &conn->sas2);

			for (i = 0; i < ZFONEG_STREAMS_COUNT; i++)
			{
		  		list_streams_info_t *stream = &conn->streams[i];
		  		if ( is_active(stream) ) 
				{
		  			ver_en = stream->cache_ttl;
					break;
				}
			}

			int verified_value;
	    	if ( ver_en && !is_session_mitm(conn) )
				verified_value = conn->verified ? VERIFIED_CHECKED : VERIFIED_UNCHECKED;
		    else
				verified_value = VERIFIED_DISABLED;
			set_verified_state(verified_value);
			
	  		
			gtk_widget_set_sensitive(sec_par_menu_item, TRUE);

	    	    // set secure since
		    struct tm t;
	  	    char secure_since_str[128];
		    memset(secure_since_str, 0, 128);
    	    if ( conn->secure_since )
		    {
            	time_t time = conn->secure_since;
			   	localtime_r((const time_t*)&time, &t);
							   
	        	sprintf(secure_since_str,"Secure since:\n%04i/%02i/%02i %02i:%02i",
                                t.tm_year+1900,
                                t.tm_mon+1,
                                t.tm_mday,
                                t.tm_hour,
                                t.tm_min
                                );
    	    }
		    if ( strlen(secure_since_str) )
		  		gtk_label_set_text(GTK_LABEL(secure_since), secure_since_str);
		    else
				gtk_label_set_text(GTK_LABEL(secure_since), "\n");
			set_clear_button_enabled(is_session_allowed_clear(conn));
		  	set_secure_button_enabled(0);
		}
		else
		{
		    set_SAS(NULL, NULL);
		    gtk_label_set_text(GTK_LABEL(secure_since), "\n");
	    	set_verified_state(VERIFIED_DISABLED);

		    if ( is_session_cleared(conn) )
		    {
				int state = !(is_session_passive_peer(conn) && params.license_mode != ZRTP_LICENSE_MODE_UNLIMITED);
		    	set_secure_button_enabled(state);
				set_clear_button_enabled(0);
		    }
		    else if ( is_session_pendingclear(conn))
		    {
		    	set_secure_button_enabled(0);
		    	set_clear_button_enabled(1);
		    }
		    else
		    {
		    	set_secure_button_enabled(0);
		    	set_clear_button_enabled(0);
		    }
		}

		show_state(item);
		// ----
		//update connection button name
		if (make_top)
		{
			if ( conn->name.length > 0 )
			{
			    if ( conn->name.length > NAME_FIELD_LENGTH )
			    {
				char name_buffer[NAME_FIELD_LENGTH+1];
				strncpy(name_buffer, conn->name.buffer, NAME_FIELD_LENGTH - 3);
				name_buffer[NAME_FIELD_LENGTH - 3] = 0;
				strcat(name_buffer, "...");
				gtk_entry_set_text(GTK_ENTRY(name), name_buffer);	
				name_buffer[NAME_FIELD_LENGTH - 5] = 0;
				strcat(name_buffer, "...");
				gtk_button_set_label(GTK_BUTTON(item->btn), name_buffer);
			    }	    
			    else
	  		    {	    
				char name_buffer[NAME_FIELD_LENGTH+1];
				strncpy(name_buffer, conn->name.buffer, NAME_FIELD_LENGTH);
				name_buffer[NAME_FIELD_LENGTH] = 0;
				gtk_entry_set_text(GTK_ENTRY(name), conn->name.buffer);	
				if (conn->name.length > NAME_FIELD_LENGTH - 5)
				{
					name_buffer[NAME_FIELD_LENGTH - 5] = 0;
					strcat(name_buffer, "...");
				}
				gtk_button_set_label(GTK_BUTTON(item->btn), name_buffer);
			    }
			}
			else
			{
			    gtk_entry_set_text(GTK_ENTRY(name), "detecting...");	
			    gtk_button_set_label(GTK_BUTTON(item->btn), "detecting...");
			}
		}
		int i;
		for (i = 0; i < ZFONEG_STREAMS_COUNT; i++)
		{
		    list_streams_info_t *stream = &conn->streams[i];
		    if ( !is_active(stream) ) continue;
		    rtp_lights(stream->type, 0, stream->rtp_states[0]);
		    rtp_lights(stream->type, 1, stream->rtp_states[1]);
		}
	// ----
    }
    else
    {
		pref_form_no_active(1);
		clear_main_view();
	
		if (ips_holder.ip_count)
		{
		    set_both_indicators(IDLE_PATH_PIC);
		}
		else
		    set_both_indicators(NO_NET_PIC);
		hide_main_window(1);	
    }	
    relight_icons(&_lb);
}

//------------------------------------------------------------------------------
void set_tooltip(GtkWidget *widget, char *message)
{
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), widget, message, "");
}

/*!
    \ingroup ControlClient
    callback which is called upon successfull creation of TCP connection.
*/
void ipf_ok(void)
{
    gui_run_ctrl_client(NULL);

    update_main_view(1);
    
    enable_menus();
    //request connections states from daemon
    zfone_session_id_t session_id;
    memset(session_id, 0, ZFONE_SESSION_ID_SIZE);
    send_cmd(REFRESH, session_id, 0, NULL, 0);
}

/*!
    \ingroup ControlClient
    callback which is called upon TCP connection failure. 
    \sa tcp_conn tcp_conn#is_ok
*/
//------------------------------------------------------------------------------
void ipf_failed(void)
{
    remove_all_entries(&_lb);    
    disable_menus();
    clear_main_view();
    set_both_indicators(NO_FILTER_PIC);
    has_prefs = 0;
    ips_holder.ip_count = 0;
    disable_tcp(&tconn);
    tconn.create(&tconn);
}

//------------------------------------------------------------------------------
void show_crash_dialog(void)
{
    GtkWidget *msg = gtk_message_dialog_new(
				GTK_WINDOW(window)
			       ,GTK_DIALOG_DESTROY_WITH_PARENT
			       ,GTK_MESSAGE_WARNING
			       ,GTK_BUTTONS_OK
			       ,"Zfone IP filter crashed\n"
			 );

    gtk_dialog_run ( GTK_DIALOG (msg) );
    gtk_widget_destroy( GTK_WIDGET (msg) );
	ipf_failed();
	show_report_dialog();
}

//------------------------------------------------------------------------------
void show_warning(const char* message)
{
    show_info(message, GTK_MESSAGE_WARNING);
}

//------------------------------------------------------------------------------
void show_error(const char* message)
{
    show_info(message, GTK_MESSAGE_ERROR);
}

//------------------------------------------------------------------------------
void gui_cb_send_version(zrtp_cmd_t *cmd)
{
    update_check_info(cmd, window);
}

//------------------------------------------------------------------------------
void clear_main_view(void)
{
    set_SAS(NULL, NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(secure_button), 0);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_button), 0);
    set_verified_state(VERIFIED_DISABLED);
    gtk_label_set_text(GTK_LABEL(secure_since), "\n");
    gtk_entry_set_text(GTK_ENTRY(name), "");
    gtk_image_set_from_file(GTK_IMAGE(light_in_audio), RTP_RX_GREY_PIC);
    gtk_image_set_from_file(GTK_IMAGE(light_out_audio), RTP_TX_GREY_PIC);
    gtk_image_set_from_file(GTK_IMAGE(light_in_video), RTP_RX_GREY_PIC);
    gtk_image_set_from_file(GTK_IMAGE(light_out_video), RTP_TX_GREY_PIC);
	set_remember_state(0);
}

//-------------------------------------------------------------------------------
GdkPixbuf* get_app_icon()
{
    static GdkPixbuf *pxBuf;

    GError *er = NULL;	
    if ( pxBuf )
    {
	return pxBuf;
    }

    pxBuf = gdk_pixbuf_new_from_file(ZFONE_ICON, &er);
    return pxBuf;
}

//------------------------------------------------------------------------------
void disable_menus()
{
    gtk_widget_set_sensitive(pref_menu_item, FALSE);
    gtk_widget_set_sensitive(check_menu_item, FALSE);
    gtk_widget_set_sensitive(clean_logs_menu_item, FALSE);
    gtk_widget_set_sensitive(reset_menu_item, FALSE);
    gtk_widget_set_sensitive(cache_menu_item, FALSE);
	gtk_widget_set_sensitive(sec_par_menu_item, FALSE);
//    gtk_widget_set_sensitive(clean_cache_menu_item, FALSE); <- was before build
}

//------------------------------------------------------------------------------
void enable_menus()
{
    if (has_prefs)
	gtk_widget_set_sensitive(pref_menu_item, TRUE);
    else
	gtk_widget_set_sensitive(pref_menu_item, FALSE);

    gtk_widget_set_sensitive(check_menu_item, TRUE);
    gtk_widget_set_sensitive(clean_logs_menu_item, TRUE);
    gtk_widget_set_sensitive(reset_menu_item, TRUE);
    gtk_widget_set_sensitive(cache_menu_item, TRUE);
//    gtk_widget_set_sensitive(clean_cache_menu_item, TRUE); <- was before build
}

void enable_prefs()
{
    gtk_widget_set_sensitive(pref_menu_item, TRUE);
    has_prefs = 1;
}

// ------------------------------- STATIC FUNCS --------------------------------

//------------------------------------------------------------------------------
static void init_pixmaps()
{
    state2pic[ZRTP_STATE_NONE] 			= IMG_Idle;
    state2pic[ZRTP_STATE_ACTIVE]		= IMG_Idle;
    state2pic[ZRTP_STATE_START]			= IMG_LookingZrtp;

    state2pic[ZRTP_STATE_WAIT_HELLOACK] 	= IMG_LookingZrtp;
    state2pic[ZRTP_STATE_WAIT_HELLO] 		= IMG_LookingZrtp;
    state2pic[ZRTP_STATE_CLEAR]			= IMG_IsNotSecure;
    state2pic[ZRTP_STATE_INITIATINGSECURE]	= IMG_Securing;
    state2pic[ZRTP_STATE_WAIT_CONFIRM1]		= IMG_Securing;
    state2pic[ZRTP_STATE_WAIT_CONFIRMACK]	= IMG_Securing;
    state2pic[ZRTP_STATE_INITIATINGSECURE]	= IMG_Securing;
    state2pic[ZRTP_STATE_PENDINGSECURE]		= IMG_Securing;
    state2pic[ZRTP_STATE_WAIT_CONFIRM2]		= IMG_Securing;
	state2pic[ZRTP_STATE_SASRELAYING]		= IMG_IsSecure;
    state2pic[ZRTP_STATE_SECURE]			= IMG_IsSecure;
    state2pic[ZRTP_STATE_INITIATINGCLEAR]	= IMG_Waiting;
    state2pic[ZRTP_STATE_PENDINGCLEAR]		= IMG_PressClear;
    state2pic[ZRTP_STATE_ERROR]				= IMG_Error;
    state2pic[ZRTP_STATE_NO_ZRTP]			= IMG_NoZrtp;
}

//------------------------------------------------------------------------------
void set_both_indicators(char *picture)
{
    set_indicator(picture, ZFONE_SDP_MEDIA_TYPE_AUDIO);
    set_indicator(picture, ZFONE_SDP_MEDIA_TYPE_VIDEO);
}


//------------------------------------------------------------------------------
void set_indicator(char *picture, zfone_media_type_t stream_type)
{
    if (stream_type == ZFONE_SDP_MEDIA_TYPE_AUDIO)
    {
	gtk_image_set_from_file(GTK_IMAGE(indicator_audio), picture);
    }
    else
    {
	if ( !strcmp(picture, IDLE_PATH_PIC) || !strcmp(picture, NO_FILTER_PIC) || !strcmp(picture, NO_NET_PIC) )
	{
	    gtk_widget_hide(hbox_video);
	}
	else
	{
	    gtk_widget_show(hbox_video);
	}
	gtk_image_set_from_file(GTK_IMAGE(indicator_video), picture);
    }
    gtk_widget_queue_draw( window );
    gtk_main_iteration_do(FALSE);
}

//------------------------------------------------------------------------------
static void show_state(list_box_item_t *item)
{
    if (!item)
    {
		ips_holder.ip_count ? set_both_indicators(IDLE_PATH_PIC) : set_both_indicators(NO_NET_PIC);
    }
    else 
    {
		int i;
		int channels_set = 0;
	
		for (i = 0; i < ZRTP_MAX_STREAMS_PER_SESSION; i++)
		{
		    list_streams_info_t *stream = &item->conn_info.streams[i];
		    
		    if ( !is_active(stream) )
		    {
				continue;
		    }
		    
		    channels_set |= stream->type;
			int set = 0;
		    if (stream->state == ZRTP_STATE_SECURE || stream->state == ZRTP_STATE_SASRELAYING)
		    {
				int activity = (stream->rtp_states[ZFONE_IO_OUT] == ZFONED_NO_ALERTED_ACTIV || 
					stream->rtp_states[ZFONE_IO_OUT] == ZFONED_NO_ACTIV);
				if ( stream->disclose_bit )
					set_indicator(activity ? SECURE_DISCLOSE_FADED_PIC : SECURE_DISCLOSE_PIC, stream->type);
				else if ( stream->is_mitm  )
					set_indicator(activity ? SECURE_MITM_FADED_PIC : SECURE_MITM_PIC, stream->type);
				else if (stream->type == ZFONE_SDP_MEDIA_TYPE_AUDIO )
					set_indicator(activity ? SECURE_AUDIO_FADED_PIC : SECURE_AUDIO_PIC, stream->type);
				else
					set_indicator(activity ? SECURE_VIDEO_FADED_PIC : SECURE_VIDEO_PIC, stream->type);
				set = 1;
		    }
		    else if (stream->state == ZRTP_STATE_CLEAR)
			{
				if (stream->passive_peer && params.license_mode != ZRTP_LICENSE_MODE_UNLIMITED)
				{
					set_indicator(NOT_SECURE_PASSIVE_PIC, stream->type);
					set = 1;
				}
			}
			if (!set)
		    {
			    set_indicator(pixmap[state2pic[stream->state]], stream->type);
		    }
		}
		if ( !(channels_set & ZFONE_SDP_MEDIA_TYPE_AUDIO) )
	  		set_indicator(IDLE_PATH_PIC, ZFONE_SDP_MEDIA_TYPE_AUDIO);	
		if ( !(channels_set & ZFONE_SDP_MEDIA_TYPE_VIDEO) )
  			set_indicator(IDLE_PATH_PIC, ZFONE_SDP_MEDIA_TYPE_VIDEO);	
    }
    
    gtk_widget_queue_draw( window );
    // Force widget to be updated
    gtk_main_iteration_do(FALSE);
}

//------------------------------------------------------------------------------
static void set_SAS(zrtp_string16_t *sas1, zrtp_string16_t *sas2)
{
    if ( !sas1 )
    {
		gtk_label_set_text(GTK_LABEL(label_sas), " ");
		gtk_widget_hide(label_sas2);
		return;
    }

 	gtk_label_set_text(GTK_LABEL(label_sas), sas1->buffer);
	if ( sas2->length)
	{
		gtk_label_set_text(GTK_LABEL(label_sas2), sas2->buffer);
		gtk_widget_show(label_sas2);
 	}
 	else
		gtk_widget_hide(label_sas2);
 	
	return;
}

//------------------------------------------------------------------------------
static void set_secure_button_enabled(const int is_on)
{
    gtk_widget_set_sensitive(GTK_WIDGET(secure_button), is_on);
}

//------------------------------------------------------------------------------
static void set_clear_button_enabled(const int is_on)
{
    gtk_widget_set_sensitive(GTK_WIDGET(clear_button), is_on);
}

//------------------------------------------------------------------------------
static void secure_state(void)
{    
    list_box_item_t* item = selected(&_lb);

    if ( item )
		send_cmd(GO_SECURE, item->conn_info.session_id, 0, NULL, 0);
    else
		printf("!!!!!! ERROR !!!!!!!: Zfone GUI secure_state: No current selection.\n");
}


//------------------------------------------------------------------------------
static void clear_state(void)
{
    list_box_item_t* item = selected(&_lb);

    if ( item )
	send_cmd(GO_CLEAR, item->conn_info.session_id, 0, NULL, 0);
    else
	printf("!!!!!! ERROR !!!!!!!: Zfone GUI clear_state: No current selection\n");
}

// on verifed clicked
//------------------------------------------------------------------------------
static void verified_clicked(void *data)
{
    if (ignore_verification)
	return;
    list_box_item_t *current = selected(&_lb);
    if ( current )
    {	
	zrtp_cmd_set_verified_t cmd_verified;
        int verified_state;
	if (data)
	    verified_state = !GTK_CHECK_MENU_ITEM(verified_menu_item)->active;
	else
	    verified_state = GTK_CHECK_MENU_ITEM(verified_menu_item)->active;
	cmd_verified.is_verified = verified_state ? 1: 0;
        	
	send_cmd(SET_VERIFIED, current->conn_info.session_id, 0, &cmd_verified, sizeof(cmd_verified));
	current->conn_info.verified = verified_state;
	if (verified_state)
	{
	    set_verified_state(VERIFIED_CHECKED);
	}
	else
	    set_verified_state(VERIFIED_UNCHECKED);
    }
    else
    {
	printf("!!!!!! ERROR !!!!!!!: Zfone GUI: VERIFIED CLICKED  - no current selection\n");
    }
}

/*
DOESNT WORK!

static void check_hear_ctx(int audio, int on)
{
    if (!params.hear_ctx)
	{
		return;
	}
    list_box_item_t *current = selected(&_lb);
    if ( current )
    {	
		zrtp_cmd_hear_ctx_t cmd;
		cmd.disable_zrtp = on;
		printf("sent hear_ctx command %d, on = %d\n", audio, on);
		send_cmd(HEAR_CTX, current->conn_info.session_id, audio ? ZFONE_SDP_MEDIA_TYPE_AUDIO : ZFONE_SDP_MEDIA_TYPE_VIDEO, &cmd, sizeof(cmd));
	}
	else
		printf("NOT sent hear_ctx command\n");
}

static void audio_clicked(GtkWidget *widget, void *data)
{
	check_hear_ctx(1, (int)data);
}

static void video_clicked(GtkWidget *widget, void *data)
{
	check_hear_ctx(0, (int)data);
}
*/

//------------------------------------------------------------------------------
static void sig_handler(int signum)
{
    if ( signum == SIGALRM )
    {	
	// check TCP connection
	tconn.create(&tconn);
	alarm(5);
    }
    else
    {
        if ( SIGINT == signum )
	{
	    gui_stop_ctrl_client(NULL, G_IO_HUP, NULL);
	}
	signal (signum, SIG_DFL);
        raise (signum);
    }
}



//------------------------------------------------------------------------------
static void destroy(void)
{
    gui_stop_ctrl_client(NULL, G_IO_HUP, NULL);
	flock(fileno(lock), LOCK_UN);
	fclose(lock);
	unlink(ZFONE_LOCK_FILE);
	
    gtk_exit (0);
}

//------------------------------------------------------------------------------
static void show_info(const char* message, GtkMessageType type)
{
    GtkWidget *msg = gtk_message_dialog_new(
				GTK_WINDOW(window)
			       ,GTK_DIALOG_DESTROY_WITH_PARENT
			       ,type
			       ,GTK_BUTTONS_OK
			       ,message
			 );
 
	g_signal_connect_swapped(GTK_OBJECT(msg), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(msg));
    gtk_window_set_position(GTK_WINDOW(msg), GTK_WIN_POS_CENTER);
	gtk_widget_show(msg);
}

//------------------------------------------------------------------------------
static gboolean name_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    if ( !event )
	return FALSE;

    if ( event->type == GDK_KEY_PRESS )
    {
	// get current call
	list_box_item_t *item = selected(&_lb);
			
	if ( event->keyval == GDK_KP_Enter || event->keyval == GDK_Return)
	{
	    if ( item )
	    {
		unsigned int text_length;
		list_conn_info_t *info = &item->conn_info;
		char* text = (char*)gtk_entry_get_text(GTK_ENTRY(widget));
		if ( text )
		{
			zrtp_cmd_set_name_t cmd_name;
			memset(&cmd_name, 0, sizeof(cmd_name));
	
			text_length = (strlen(text) < sizeof(info->name.buffer)-1) ? strlen(text) : sizeof(info->name.buffer)-1;
			
			//change name in item
			strncpy(info->name.buffer, text, text_length);
	  		info->name.buffer[text_length] = 0;
			info->name.length = text_length;
		
		    cmd_name.name_length = text_length > sizeof(cmd_name.name) - 1 ? sizeof(cmd_name.name) - 1 : text_length;
		    strncpy(cmd_name.name, text, cmd_name.name_length);
			cmd_name.name[cmd_name.name_length++] = 0;
		
		    send_cmd(SET_NAME, item->conn_info.session_id, 0, &cmd_name, sizeof(cmd_name));
		}
		else
		{
		    printf("Name is empty\n");
		}
		
		update_main_view(1);
	    }
	}
    }

    return FALSE;
}

//------------------------------------------------------------------------------
static void show_about_dialog(void)
{
    char copyright[2024];
	char *lics[] = {"Passive License", "Active License", "Unlimited License", "Unknown License"};
	
	int lIndex = params.license_mode;
	if (lIndex > ZRTP_LICENSE_MODE_UNLIMITED)
	  lIndex = 3;
	
    sprintf(copyright, 
		"Zfone %sSoftware %s\n%s\n"
		"Copyright (c) 2005, 2009 Philip Zimmermann.\n"
		"All rights reserved.\n"
		"Contact Phil at: www.philzimmermann.com\n"
		"Visit the Zfone Project Home Page\n"
		"Report bugs via the Zfone Bugs Page\n"
		"Zfone secures VoIP phone calls with strong encryption.\n"
		"It lets you whisper in someone's ear, even if their ear is\n"
		"a thousand miles away.\n"
		"Created by Phil Zimmermann.\n"
		"Developers:\n"
		"\tViktor Krikun\n"
		"\tNikolay Popok\n"
		"\tVitaly Rozhkov\n"
		"\tAndrey Rozinko\n"
		"\tBryce Wilcox-O'Hearn\n"
		"Thanks to:\n"
		"\tAlan Johnston\n"
		"\tJon Callas\n"
		"\tHal Finney\n"
		"\tSagar Pai\n"
        "\tL. Amber Wilcox-O'Hearn\n"
		"\tColin Plumb\n"
		"\tAriel Boston\n"
		"\tDonovan Preston\n"
		"Software development services provided by Svitla Systems and Soft-Industry.com.\n"
		"Portions of this software are available under open source licenses from other authors.\n"
		"Notably, Brian Gladman's AES implementation, and David McGrew's libSRTP package.\n",
		params.is_ec ? "Enterprise " : "", lics[lIndex], version);
	
	
	// TODO: look at about dialog
    GtkWidget *about_dialog = gtk_message_dialog_new(
				GTK_WINDOW(window)
			       ,GTK_DIALOG_DESTROY_WITH_PARENT
			       ,GTK_MESSAGE_INFO
			       ,GTK_BUTTONS_OK
			       ,copyright
			 );
 
//    gtk_widget_set_size_request(GTK_WIDGET(about_dialog), 450, 550);
    gtk_window_set_position(GTK_WINDOW(about_dialog), GTK_WIN_POS_CENTER);
    gtk_dialog_run ( GTK_DIALOG (about_dialog) );
    gtk_widget_destroy( GTK_WIDGET (about_dialog) );

}

/*/------------------------------------------------------------------------------
static void menuitem_response( gchar *string )
{
    if ( !strcmp(string, "update") )
	send_cmd(GET_VERSION, 0, 0, 0, 0);
    else if ( !strcmp(string, "about") )
	show_about_dialog();
}
*/
//------------------------------------------------------------------------------

static void show_report_dialog()
{
	int state = create_report();
    GtkWidget *report_dialog = gtk_message_dialog_new(
					GTK_WINDOW(window)
			       ,GTK_DIALOG_DESTROY_WITH_PARENT
			       ,state < 0 ? GTK_MESSAGE_ERROR : GTK_MESSAGE_INFO
			       ,GTK_BUTTONS_OK
			       ,state < 0 ? "Error occured while creating report" : 
				  	  "Zfone has prepared a bug report file which you should email to bug-reports@zfoneproject.com. "
					  "This bug report file is called zfone-bug-report-(date).rep, on your desktop."
			 );
 
    gtk_window_set_position(GTK_WINDOW(report_dialog), GTK_WIN_POS_CENTER);
    gtk_dialog_run ( GTK_DIALOG (report_dialog) );
    gtk_widget_destroy( GTK_WIDGET (report_dialog) );
}

static void topmenuitem_response( GtkWidget *w, gpointer data )
{
    switch ((long)data)
    {
	case MENU_UPDATE:
	{
	    create_check_form(version, NULL);
	    send_cmd(GET_VERSION, 0, 0, 0, 0);
	    break;
	}
	case MENU_ABOUT:
	{
	    show_about_dialog();
	    break;
	}
	case MENU_PREF:
	{
  		list_box_item_t *item = selected(&_lb);
	    create_pref_form(item == NULL);
	    break;
	}
	case MENU_SEC_PAR:
	{
  		list_box_item_t *item = selected(&_lb);
	    create_secure_params_form(item);
	    break;
	}
	case MENU_CLEAN_LOGS:
	{
	    send_cmd(CLEAN_LOGS, 0, 0, 0, 0);
	    break;
	}
	case MENU_CACHES:
	{
	    send_cmd(GET_CACHE_INFO, 0, 0, NULL, 0);
		break;
	}
	case MENU_RESET_DAEMON:
	{
	    int result;
	    GtkWidget *verify_dialog = gtk_message_dialog_new(
                                    GTK_WINDOW(window)
                                    ,GTK_DIALOG_DESTROY_WITH_PARENT
                                    ,GTK_MESSAGE_WARNING
                                    ,GTK_BUTTONS_NONE
                                    ,"Daemon reset during call may cause weird things. Continue?"
                                    );
	    gtk_dialog_add_button(GTK_DIALOG(verify_dialog), "Reset", GTK_RESPONSE_YES);
	    gtk_dialog_add_button(GTK_DIALOG(verify_dialog), "Cancel", GTK_RESPONSE_NO);
        gtk_window_set_position(GTK_WINDOW(verify_dialog), GTK_WIN_POS_CENTER);
        result = gtk_dialog_run ( GTK_DIALOG (verify_dialog) );
        gtk_widget_destroy( GTK_WIDGET (verify_dialog) );
	    
	    if (result == GTK_RESPONSE_YES)
			send_cmd(RESET_DAEMON, 0, 0, 0, 0);
		
	    break;
	}
	case MENU_REMEMBER:
	{
	    int result;
		if (ignore_remembering)
			break;

  		list_box_item_t *item = selected(&_lb);
		if (!item)
		{
			set_remember_state(0);
			break;
		}

		int f = is_session_pbx_ok(&item->conn_info);

	    if (f > 0)
		{
			GtkWidget *verify_dialog = gtk_message_dialog_new(
                                    GTK_WINDOW(window)
                                    ,GTK_DIALOG_DESTROY_WITH_PARENT
                                    ,GTK_MESSAGE_WARNING
                                    ,GTK_BUTTONS_NONE
                                    ,"Are you sure that this call is your first call to PBX and it has to be remembered?"
                                    );
		    gtk_dialog_add_button(GTK_DIALOG(verify_dialog), "Yes", GTK_RESPONSE_YES);
  	    	gtk_dialog_add_button(GTK_DIALOG(verify_dialog), "No", GTK_RESPONSE_NO);
	        gtk_window_set_position(GTK_WINDOW(verify_dialog), GTK_WIN_POS_CENTER);
	        result = gtk_dialog_run ( GTK_DIALOG (verify_dialog) );
	        gtk_widget_destroy( GTK_WIDGET (verify_dialog) );
	    
		    if (result == GTK_RESPONSE_YES)
			{
	  			list_box_item_t* item = selected(&_lb);
		  		if ( item )
		  		{
					send_cmd(REMEMBER_CALL, item->conn_info.session_id, 0, NULL, 0);
					f = -1;
		  		}
				else
					f = 0;
			}
		}
		else if (f < 0)
		{
  			GtkWidget *msg = gtk_message_dialog_new(
					GTK_WINDOW(window)
			       ,GTK_DIALOG_DESTROY_WITH_PARENT
			       ,GTK_MESSAGE_WARNING
			       ,GTK_BUTTONS_OK
			       ,"Already registered with this pbx. Delete appropriate cache if you want to reregister."
			 );

		    gtk_dialog_run ( GTK_DIALOG (msg) );
		    gtk_widget_destroy( GTK_WIDGET (msg) );
		}
		
		set_remember_state(f);
	    break;
	}
	case MENU_QUIT:
		destroy();
		break;
	case MENU_REPORT:
		show_report_dialog();
		break;
    }
}

/*------------------------------------------------------------------------------
static void popup_menu(guint button, guint32 time)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *about_item =  gtk_menu_item_new_with_label("About");
    GtkWidget *update_item =  gtk_menu_item_new_with_label("Check for Updates");
	    
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), about_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), update_item);

    g_signal_connect_swapped (G_OBJECT (about_item), "activate",
                    	      G_CALLBACK (menuitem_response),
                    	      (gpointer) "about");
    g_signal_connect_swapped (G_OBJECT (update_item), "activate",
                    	      G_CALLBACK (menuitem_response),
    	            	      (gpointer) "update");
				      
    gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL
		   , button, time);

    gtk_widget_show(about_item);
    gtk_widget_show(update_item);
}

//------------------------------------------------------------------------------
static gboolean show_popup(GtkWidget *widget, gpointer   user_data) 
{
    popup_menu(0, gtk_get_current_event_time());

    return TRUE;
}

//------------------------------------------------------------------------------
static gboolean btn_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    GdkEventButton *event_button;

    if ( event && event->type == GDK_BUTTON_RELEASE )
    {
	event_button = (GdkEventButton *) event;

	if ( event_button->button == 3 || event_button->button == 2)
	{
	    popup_menu( event_button->button, event_button->time);
	    return TRUE;
	}
    }

    return TRUE;
}
*/
//-----------------------------------------------------------------------------
static void get_main_menu( GtkWidget  *window, GtkWidget **menubar )
{
    GtkAccelGroup *accel_group;
    gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
			  
    accel_group = gtk_accel_group_new ();

    item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);

    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
    if (menubar)
    {
	*menubar = gtk_item_factory_get_widget(item_factory, "<main>");
    }
}

//-----------------------------------------------------------------------------
// test function
static void rtp_lights(int stream, int direction, int type)
{
    GtkWidget *light;
    char *picture;
	int index = type - ZFONED_NO_ACTIV;

	if ( !type )
	{
		index = 0;
	}
	else if (type > ZFONED_PASS_ACTIV || type < ZFONED_NO_ACTIV)
	{
		printf("[ERROR] : type of rtp activity is wrong. %d\n", type);
		return;
	}

    if (direction == ZFONE_IO_IN)
    {
		light = stream == ZFONE_SDP_MEDIA_TYPE_AUDIO ? light_in_audio : light_in_video;
		picture = rtp_colors_rx[index];
    }
    else
    {
		light = stream == ZFONE_SDP_MEDIA_TYPE_AUDIO ? light_out_audio : light_out_video;
		picture = rtp_colors_tx[index];
    }
    
    gtk_image_set_from_file(GTK_IMAGE(light), picture);
}

void tray_icon_on_click()
{
	if ( GTK_WIDGET_VISIBLE(window) )
		hide_main_window(0);	
	else
		show_main_window(0);	
}

static void change_sas_color(char *color_name)
{
	GdkColor color;
	gdk_color_parse(color_name, &color);
	gtk_widget_modify_fg(label_sas,  GTK_STATE_NORMAL, &color);
	gtk_widget_modify_fg(label_sas2, GTK_STATE_NORMAL, &color);
}

static void tray_popup(GtkStatusIcon *status_icon,
                guint button,
                guint activate_time,
                gpointer user_data)
{
    if (!status_menu)
    {
        GtkWidget *item;
        status_menu = gtk_menu_new();

        pref_menu_status = gtk_menu_item_new_with_label(" Preferences ");
        gtk_menu_append(status_menu, pref_menu_status);
        g_signal_connect (G_OBJECT(pref_menu_status), "activate",
                          G_CALLBACK(topmenuitem_response), 
                          (void*)MENU_PREF);
		gtk_widget_set_sensitive(pref_menu_status, GTK_WIDGET_SENSITIVE(pref_menu_item));
		

        item = gtk_menu_item_new_with_label(" Quit ");
        gtk_menu_append(status_menu, item);
        g_signal_connect (G_OBJECT(item), "activate",
                          G_CALLBACK(destroy), 
                          NULL);
		//gtk_menu_item_toggle_size_request(item, 100);
    }
	else
		gtk_widget_set_sensitive(pref_menu_status, GTK_WIDGET_SENSITIVE(pref_menu_item));
	  
        
    gtk_widget_show_all(status_menu);
    gtk_menu_popup(GTK_MENU(status_menu),
                        NULL,
                        NULL,
                        gtk_status_icon_position_menu,
                        status_icon,
                        button,
                        activate_time);
}

static int close_window()
{
	gtk_widget_hide(window);
	return TRUE;
}

//-----------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    GtkWidget *sas_hbox, *sas_inner_vbox;
    
	lock = fopen(ZFONE_LOCK_FILE, "w");
	if ( lock && flock(fileno(lock), LOCK_EX|LOCK_NB) < 0)
	{
		fclose(lock);
		return -1;
	}

    ips_holder.ip_count = 0;
    version[0] = '\0';

    has_prefs = 0;
    // Secure & Clear buttons HBox
    GtkWidget *hbox_btns, *lights_vbox, *light_ao_eb, *light_ai_eb, *light_vo_eb, *light_vi_eb; 
    GtkWidget *label_sas_frame;
    
    gtk_init (&argc, &argv);

    // main window
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);    
    gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);    
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
//    gtk_signal_connect (GTK_OBJECT (window), "destroy", GTK_SIGNAL_FUNC (destroy), NULL);
    gtk_signal_connect (GTK_OBJECT (window), "delete-event", GTK_SIGNAL_FUNC (close_window), NULL);
    gtk_container_border_width (GTK_CONTAINER (window), 0);
    gtk_window_set_title(GTK_WINDOW(window), "ZFone");
    gtk_window_set_icon(GTK_WINDOW(window), get_app_icon());
    
    // Create containers to handle widgets
    main_vbox = gtk_vbox_new (FALSE, 1);
    gtk_container_border_width (GTK_CONTAINER (main_vbox), 0);
    vbox      = gtk_vbox_new(TRUE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    hbox_btns = gtk_hbox_new(TRUE, 5);
    gtk_container_border_width (GTK_CONTAINER (hbox_btns), 5);

//    gtk_signal_connect (GTK_OBJECT (window), "button-release-event",
//            		GTK_SIGNAL_FUNC (btn_release), NULL);
//    gtk_signal_connect (GTK_OBJECT (window), "popup-menu",
//            		GTK_SIGNAL_FUNC (show_popup), NULL);

    // Create secure and clear buttons
    clear_button = gtk_button_new_with_label ("Clear");
    gtk_signal_connect (GTK_OBJECT (clear_button), "clicked",
            			       GTK_SIGNAL_FUNC (clear_state), NULL);

    secure_button = gtk_button_new_with_label ("Secure");
    gtk_signal_connect (GTK_OBJECT (secure_button), "clicked",
            			        GTK_SIGNAL_FUNC (secure_state), NULL);

    // Create name field
    name = gtk_entry_new_with_max_length(120);
    gtk_signal_connect (GTK_OBJECT (name), "key-press-event",
           	        GTK_SIGNAL_FUNC (name_key_pressed), NULL);

    // Sas label creation
    label_sas  	     = gtk_label_new ("    ");
    label_sas2       = gtk_label_new ("    ");
    label_sas_frame  = gtk_frame_new( "Compare with partner:" );

    sas_hbox = gtk_hbox_new(FALSE, 1);
    gtk_box_set_homogeneous(GTK_BOX(sas_hbox), FALSE);
    gtk_container_border_width(GTK_CONTAINER(sas_hbox), 1);
    gtk_container_add(GTK_CONTAINER(label_sas_frame), sas_hbox);
    gtk_widget_show(sas_hbox);

    padlock_eb = gtk_event_box_new();
    gtk_widget_set_events(padlock_eb, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect (GTK_OBJECT(padlock_eb), "button_press_event",
            			   GTK_SIGNAL_FUNC (verified_clicked), NULL);

    padlock = gtk_image_new_from_file(PADLOCK_DISABLED_PIC);
    gtk_container_add (GTK_CONTAINER (padlock_eb), padlock);

    sas_inner_vbox = gtk_vbox_new(FALSE, 1);
    gtk_widget_set_size_request (GTK_WIDGET(sas_inner_vbox), 110, 35);
    gtk_container_border_width(GTK_CONTAINER(sas_inner_vbox), 1);
    gtk_widget_show(sas_inner_vbox);

    gtk_widget_show(window);
    gtk_widget_add_events(GTK_WIDGET(window), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    
    // init state2pic array 
    init_pixmaps();

    // HBOX
    hbox_audio = gtk_hbox_new(FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (hbox_audio), 5);
    gtk_widget_show(hbox_audio);

    // AUDIO INDICATOR
    event_box_audio = gtk_event_box_new();
    gtk_widget_show(event_box_audio);
/*
    gtk_signal_connect (GTK_OBJECT(event_box_audio), "button_press_event",
            			   GTK_SIGNAL_FUNC (audio_clicked), (void*)1);
    gtk_signal_connect (GTK_OBJECT(event_box_audio), "button_release_event",
            			   GTK_SIGNAL_FUNC (audio_clicked), (void*)0);
*/
    // create image view, which displays current state
    indicator_audio = gtk_image_new_from_file(NO_FILTER_PIC);
    gtk_container_add (GTK_CONTAINER (event_box_audio), indicator_audio);
    gtk_widget_set_events (event_box_audio, GDK_BUTTON_PRESS_MASK);
    gtk_widget_show(indicator_audio);
    // ADD INDICATOR TO HBOX
    gtk_container_add(GTK_CONTAINER(hbox_audio), event_box_audio);

    // LIGHTS
    light_ai_eb = gtk_event_box_new();
    light_in_audio  = gtk_image_new_from_file(RTP_RX_GREY_PIC);
    light_ao_eb = gtk_event_box_new();
    light_out_audio = gtk_image_new_from_file(RTP_TX_GREY_PIC);
    gtk_widget_show(light_ai_eb);
    gtk_widget_show(light_in_audio);
    gtk_widget_show(light_ao_eb);
    gtk_widget_show(light_out_audio);
    
    // LIGHTS VBOX    
    lights_vbox = gtk_vbox_new(TRUE, 0);
    gtk_container_border_width (GTK_CONTAINER (lights_vbox), 0);
    gtk_widget_show(lights_vbox);
    // ADD LIGHTS TO VBOX
    gtk_container_add (GTK_CONTAINER (light_ai_eb), light_in_audio);
    gtk_container_add (GTK_CONTAINER (light_ao_eb), light_out_audio);
    gtk_container_add (GTK_CONTAINER (lights_vbox), light_ao_eb);
    gtk_container_add (GTK_CONTAINER (lights_vbox), light_ai_eb);

    // ADD VBOX TO HBOX
    gtk_container_add (GTK_CONTAINER (hbox_audio), lights_vbox);

    // ----------------------------------------------------------------------------------
    // HBOX
    hbox_video = gtk_hbox_new(FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (hbox_video), 5);
//    gtk_widget_show(hbox_video);

    // VIDEO INDICATOR
    event_box_video = gtk_event_box_new ();
    gtk_widget_show(event_box_video);
/*
    gtk_signal_connect (GTK_OBJECT(event_box_video), "button_press_event",
            			   GTK_SIGNAL_FUNC (video_clicked), (void*)1);
    gtk_signal_connect (GTK_OBJECT(event_box_video), "button_release_event",
            			   GTK_SIGNAL_FUNC (video_clicked), (void*)0);
*/					
    indicator_video = gtk_image_new_from_file(NO_FILTER_PIC);
    gtk_container_add (GTK_CONTAINER(event_box_video), indicator_video);
    gtk_widget_set_events (event_box_video, GDK_BUTTON_PRESS_MASK);
    gtk_widget_show(indicator_video);

    // ADD INDICATOR TO HBOX
    gtk_container_add (GTK_CONTAINER (hbox_video), event_box_video);

    // LIGHTS
    light_vi_eb = gtk_event_box_new();
    light_in_video  = gtk_image_new_from_file(RTP_RX_GREY_PIC);
    light_vo_eb = gtk_event_box_new();
    light_out_video = gtk_image_new_from_file(RTP_TX_GREY_PIC);
    gtk_widget_show(light_vi_eb);
    gtk_widget_show(light_in_video);
    gtk_widget_show(light_vo_eb);
    gtk_widget_show(light_out_video);

    // LIGHTS VBOX    
    lights_vbox = gtk_vbox_new(TRUE, 0);
    gtk_container_border_width (GTK_CONTAINER (lights_vbox), 0);
    gtk_widget_show(lights_vbox);

    // ADD LIGHTS TO VBOX
    gtk_container_add (GTK_CONTAINER (light_vi_eb), light_in_video);
    gtk_container_add (GTK_CONTAINER (light_vo_eb), light_out_video);
    gtk_container_add (GTK_CONTAINER (lights_vbox), light_vo_eb);
    gtk_container_add (GTK_CONTAINER (lights_vbox), light_vi_eb);
    // ADD VBOX TO HBOX
    gtk_container_add (GTK_CONTAINER (hbox_video), lights_vbox);

    secure_since = gtk_label_new("Secure since: ");

    // create list box for connections
    create_list_box(&_lb);

    get_main_menu (window, &menubar);
    gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(sas_hbox), padlock_eb);
    gtk_container_add(GTK_CONTAINER(sas_hbox), sas_inner_vbox);
    gtk_container_add(GTK_CONTAINER(sas_inner_vbox), label_sas);
    gtk_container_add(GTK_CONTAINER(sas_inner_vbox), label_sas2);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);
    gtk_container_add(GTK_CONTAINER(main_vbox), vbox);

    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_add(GTK_CONTAINER(vbox), name);
    gtk_container_add(GTK_CONTAINER(vbox), label_sas_frame);

    gtk_container_add(GTK_CONTAINER(vbox), hbox_audio);
    gtk_container_add(GTK_CONTAINER(vbox), hbox_video);
    gtk_container_add(GTK_CONTAINER(vbox), secure_since);

#ifdef RIPCORD_LOGO
	GtkWidget *ripcord = gtk_image_new_from_file(RIPCORD_PIC);
    gtk_container_add(GTK_CONTAINER(vbox), ripcord);
    gtk_widget_show(ripcord);
#endif

    gtk_container_add(GTK_CONTAINER(vbox), hbox_btns);
    gtk_container_add(GTK_CONTAINER(hbox_btns), secure_button);
    gtk_container_add(GTK_CONTAINER(hbox_btns), clear_button);


    gtk_container_add(GTK_CONTAINER(vbox), _lb.layout);
    
    gtk_widget_show(vbox);
    gtk_widget_show(main_vbox);
    gtk_widget_show(menubar);
    gtk_widget_show(hbox_btns);
    gtk_widget_show(name);
    gtk_widget_show(label_sas_frame);
    gtk_widget_show(label_sas);
    gtk_widget_show(secure_since);
    gtk_widget_show(clear_button);
    gtk_widget_show(secure_button);

    // init list box
    init_list_box(&_lb);

    // update main window
    //update_main_view_with_state(STOPPED);
//    set_both_indicators(NO_FILTER_PIC);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, sig_handler);
    
    // lets memorize menu items for enabling and disabling
    pref_menu_item        = gtk_item_factory_get_widget(item_factory, "<main>/Zfone/Preferences");
    check_menu_item       = gtk_item_factory_get_widget(item_factory, "<main>/Help/Check for updates");
    sec_par_menu_item     = gtk_item_factory_get_widget(item_factory, "<main>/Zfone/Connections settings");
    clean_logs_menu_item  = gtk_item_factory_get_widget(item_factory, "<main>/Edit/Clear logs");
//    clean_cache_menu_item = gtk_item_factory_get_widget(item_factory, "<main>/Edit/Clear cache");
    reset_menu_item       = gtk_item_factory_get_widget(item_factory, "<main>/Edit/Reset daemon");
    cache_menu_item       = gtk_item_factory_get_widget(item_factory, "<main>/Edit/Caches");
    remember_menu_item    = gtk_item_factory_get_widget(item_factory, "<main>/Edit/Register with this PBX");
    verified_menu_item    = gtk_item_factory_get_widget(item_factory, "<main>/Edit/Note that authentication string checked");

    // disable menu as we are not connected to daemon yet
    disable_menus();
    // disable connection params menu item
    gtk_widget_set_sensitive(sec_par_menu_item, FALSE);
	gtk_widget_set_sensitive(remember_menu_item, FALSE);
//    gtk_widget_set_sensitive(clean_cache_menu_item, FALSE);

    // disable controls
    set_clear_button_enabled(FALSE);
    set_secure_button_enabled(FALSE);

    gui_init_ctrl_client();
    gui_run_ctrl_client(NULL);

    // set tooltips
    tooltips = gtk_tooltips_new();
    set_tooltip(clear_button, "Go clear");
    set_tooltip(secure_button, "Go secure");
    set_tooltip(name, "Name to associate with phone number");
    set_tooltip(label_sas, "Compare this short authentication string with your partner. If they don't match, a wiretapper is present");
    set_tooltip(secure_since, "No wiretappers since this date, when we first cached a shared secret with other party");
//    set_tooltip(indicator_audio, "Audio status indicator");
//    set_tooltip(indicator_video, "Video status indicator");
    set_tooltip(event_box_audio, "Audio status indicator");
    set_tooltip(event_box_video, "Video status indicator");
    set_tooltip(light_ai_eb, "Audio rtp/srtp rx");
    set_tooltip(light_ao_eb, "Audio rtp/srtp tx");
    set_tooltip(light_vi_eb, "Video rtp/srtp rx");
    set_tooltip(light_vo_eb, "Video rtp/srtp tx");
    set_tooltip(padlock_eb, "Check to remind yourself that you have already checked the authentication string");

	GtkStatusIcon *tray_icon;

    tray_icon = gtk_status_icon_new_from_file(ZFONE_ICON);
    g_signal_connect(G_OBJECT(tray_icon), "activate", 
		     G_CALLBACK(tray_icon_on_click), NULL);
    g_signal_connect(G_OBJECT(tray_icon), "popup-menu",
		     G_CALLBACK (tray_popup), NULL);
    gtk_status_icon_set_tooltip(tray_icon, "Zfone");
	
    alarm(5);
    
    gtk_widget_hide(_lb.layout);

    gtk_main ();

    return 0;
}
