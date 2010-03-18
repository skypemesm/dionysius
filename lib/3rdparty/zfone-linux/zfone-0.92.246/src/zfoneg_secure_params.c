/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok mailto: <chaser@soft-industry.com>
*/


#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <gdk/gdkkeysyms.h>

#include "zfoneg_listbox.h"
#include "zfoneg_config.h"
#include "zfoneg_cb.h"
#include "zfoneg_utils.h"

#include "files.h"

#define X_OFFSET	10
#define X_OFFSET2	50

static GtkWidget *window_;

//! form destroying event handler 
static void sec_par_destroy()
{
    window_ = NULL;
}

static void sec_par_close()
{
    gtk_widget_destroy(window_);
}

void add_new_picture(unsigned int value, char *name, char *color_file, GtkWidget *parent)
{
	int i;
	GtkWidget *hbox, *label, *align = gtk_fixed_new();

    hbox = gtk_hbox_new(FALSE, 1);
//    gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
    gtk_container_add(GTK_CONTAINER(parent), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
	gtk_container_add(GTK_CONTAINER(hbox), align);
    gtk_widget_show(align);

    label = gtk_label_new(name);    
    gtk_widget_show(label);
//	put_to_offset(hbox, label, X_OFFSET);
	gtk_fixed_put(GTK_FIXED(align), label, X_OFFSET, 0);

    for (i = 1; i < 5; i++)
	{
		GtkWidget *img;
		if (value & (1 << i))
			img = gtk_image_new_from_file(SEC_GREEN_PIC);
		else
			img = gtk_image_new_from_file(color_file);
		
  		gtk_widget_show(img);
//		gtk_container_add(GTK_CONTAINER(parent), img);
//		put_to_offset(parent, img, 20 + i*5);
		gtk_fixed_put(GTK_FIXED(align), img, 175 + 25*i, 0);
	}
}

static gboolean form_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_Escape)
	{
		sec_par_close();
		return TRUE;
	}
	
	return FALSE;
}

//------------------------------------------------------------------------------
int create_secure_params_form(list_box_item_t *item, int active)
{
    GtkWidget *vbox, *label, *frame, *inner_vbox, *hbox;
    list_conn_info_t *session;
    list_streams_info_t *stream;
    
    // if window is already created or we dont have call item
    if ( !item || window_ )
		return 0;
    
    session = &item->conn_info;
    stream = get_secured_stream(&item->conn_info);

    // window creation
    window_ = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
    gtk_window_set_policy(GTK_WINDOW(window_), FALSE, FALSE, FALSE);
    gtk_window_set_resizable(GTK_WINDOW(window_), FALSE);
    gtk_container_border_width (GTK_CONTAINER (window_), 0);
    gtk_window_set_title(GTK_WINDOW(window_), "Connections settings");
	gtk_widget_set_size_request (GTK_WIDGET(window_), 300, 400);
    
    gtk_signal_connect (GTK_OBJECT (window_), "destroy", GTK_SIGNAL_FUNC (sec_par_destroy), NULL);
    gtk_window_set_icon(GTK_WINDOW(window_), get_app_icon());
    gtk_widget_show(window_);

    // main vbox
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window_), vbox);
    gtk_widget_show(vbox);
    gtk_container_border_width(GTK_CONTAINER(vbox), 4);
    
	{
	  char buffer[512];
	  snprintf(buffer, 512, "Settings for: %s", session->name.buffer);
      label = gtk_label_new(buffer);    
  	  gtk_widget_show(label);
	  put_to_offset(vbox, label, X_OFFSET);
	}

/*
    combo = gtk_combo_box_new_text();    
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), session->name.buffer);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), 0);

//    glist = g_list_append (glist, session->name.buffer);
//    gtk_combo_set_popdown_strings (GTK_COMBO (combo), glist);
    
	gtk_combo_box_append_text(combo, session->name.buffer);


    gtk_container_add (GTK_CONTAINER (vbox2), combo);
    gtk_widget_show(combo);
*/
    
    frame = gtk_frame_new("ZRTP Peer Client");
    gtk_container_border_width (GTK_CONTAINER (frame), 5);
    gtk_container_add (GTK_CONTAINER (vbox), frame);
    gtk_widget_show(frame);

    inner_vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add (GTK_CONTAINER (frame), inner_vbox);
    gtk_widget_show(inner_vbox);

    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    label = gtk_label_new("ZRTP Client ID: ");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);
    label = gtk_label_new(session->zrtp_peer_client.buffer);    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);

    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    label = gtk_label_new("Protocol version: ");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);
    label = gtk_label_new(session->zrtp_peer_version.buffer);    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);


    // Connection flags controls
    frame = gtk_frame_new("Connection Flags");
    gtk_container_border_width (GTK_CONTAINER (frame), 5);
    gtk_container_add (GTK_CONTAINER (vbox), frame);
    gtk_widget_show(frame);
    inner_vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add (GTK_CONTAINER (frame), inner_vbox);
    gtk_widget_show(inner_vbox);
    
    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
    label = gtk_label_new("Auto secure:");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);
    if (session->autosecure)
		label = gtk_label_new("enabled");    
    else
		label = gtk_label_new("disabled");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);


    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
    label = gtk_label_new("Allow switching to plain RTP:");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);
    if (stream->allowclear)
		label = gtk_label_new("enabled");    
    else
		label = gtk_label_new("disabled");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);

    // Profile controls
    frame = gtk_frame_new("Crypto Components");
    gtk_container_border_width (GTK_CONTAINER (frame), 5);
    gtk_container_add (GTK_CONTAINER (vbox), frame);
    gtk_widget_show(frame);
    inner_vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add (GTK_CONTAINER (frame), inner_vbox);
    gtk_widget_show(inner_vbox);
    
    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
    label = gtk_label_new("Cipher type:");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);
    
	if (!memcmp(session->cipher, ZRTP_AES1, 4))
  		label = gtk_label_new("AES-128");    
	else if (!memcmp(session->cipher, ZRTP_AES3, 4))
  		label = gtk_label_new("AES-256");    
	else
		label = gtk_label_new("Unknown");
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);

    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
    label = gtk_label_new("SRTP auth tag:");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);

	if (!memcmp(session->atl, ZRTP_HS32, 4))
  		label = gtk_label_new("HMAC-32");
	else if (!memcmp(session->atl, ZRTP_HS80, 4))
  		label = gtk_label_new("HMAC-80");
	else
  		label = gtk_label_new("Unknown");
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);

    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
    label = gtk_label_new("Sas type:");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);

	if (!memcmp(session->sas_scheme, ZRTP_B32, 4))
  		label = gtk_label_new("Base 32");
	else if (!memcmp(session->sas_scheme, ZRTP_B256, 4))
  		label = gtk_label_new("Base 256");
	else
  		label = gtk_label_new("Unknown");
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);

    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
    label = gtk_label_new("Hash algorithm:");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);

	if (!memcmp(session->hash, ZRTP_S128, 4))
  		label = gtk_label_new("SHA-128");
	else if (!memcmp(session->hash, ZRTP_S256, 4))
  		label = gtk_label_new("SHA-256");
	else
  		label = gtk_label_new("Unknown");
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);

    hbox = gtk_hbox_new(TRUE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
    label = gtk_label_new("Key exchange scheme:");    
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET);

	if (!memcmp(stream->pkt, ZRTP_DH3K, 4))
  		label = gtk_label_new("DH-3072");
#if (defined(ZRTP_USE_ENTERPRISE) && (ZRTP_USE_ENTERPRISE == 1))
	else if (!memcmp(stream->pkt, ZRTP_EC256P, 4))
  		label = gtk_label_new("ECDH-256");
	else if (!memcmp(stream->pkt, ZRTP_EC384P, 4))
  		label = gtk_label_new("ECDH-384");
	else if (!memcmp(stream->pkt, ZRTP_EC521P, 4))
  		label = gtk_label_new("ECDH-521");
#endif
	else if (!memcmp(stream->pkt, ZRTP_MULT, 4))
  		label = gtk_label_new("Multistream");
	else if (!memcmp(stream->pkt, ZRTP_PRESHARED, 4))
  		label = gtk_label_new("Preshared");
	else
  		label = gtk_label_new("Unknown");
    gtk_widget_show(label);
	put_to_offset(hbox, label, X_OFFSET2);

    // Secrets expiration controls
    frame = gtk_frame_new(NULL);
    gtk_container_border_width (GTK_CONTAINER (frame), 5);
    gtk_container_add (GTK_CONTAINER (vbox), frame);
    gtk_widget_show(frame);
    
    if (stream->cache_ttl == SECRET_NEVER_EXPIRES)
	{
  		hbox = gtk_hbox_new(TRUE, 1);
	    gtk_container_border_width (GTK_CONTAINER (hbox), 5);
	    gtk_container_add(GTK_CONTAINER(frame), hbox);
	    gtk_widget_show(hbox);
		
		label = gtk_label_new("Shared secrets will be retained forever");    
	    gtk_widget_show(label);
		put_to_offset(hbox, label, X_OFFSET);
	}
    else if (stream->cache_ttl == 1)
	{
  		hbox = gtk_hbox_new(TRUE, 1);
	    gtk_container_border_width (GTK_CONTAINER (hbox), 5);
	    gtk_container_add(GTK_CONTAINER(frame), hbox);
	    gtk_widget_show(hbox);
		
		label = gtk_label_new("Shared secrets will be changed tomorrow");    
	    gtk_widget_show(label);
		put_to_offset(hbox, label, X_OFFSET);
	}
    else if (stream->cache_ttl == 0)
	{
		GtkWidget *vbox_caches;
  		vbox_caches = gtk_vbox_new(FALSE, 1);
  		gtk_container_add(GTK_CONTAINER(frame), vbox_caches);
	    gtk_widget_show(vbox_caches);

		label = gtk_label_new("Shared secrets are expired");
	    gtk_widget_show(label);
  		gtk_container_add(GTK_CONTAINER(vbox_caches), label);
		label = gtk_label_new("and will be changed");    
	    gtk_widget_show(label);
  		gtk_container_add(GTK_CONTAINER(vbox_caches), label);
	}
    else
    {
		char buffer[512];

		GtkWidget *vbox_caches;
  		vbox_caches = gtk_vbox_new(FALSE, 1);
  		gtk_container_add(GTK_CONTAINER(frame), vbox_caches);
	    gtk_widget_show(vbox_caches);

		label = gtk_label_new("Shared secrets will be retained");
	    gtk_widget_show(label);
  		gtk_container_add(GTK_CONTAINER(vbox_caches), label);
		sprintf(buffer, "for %d days", stream->cache_ttl / 86400);
		label = gtk_label_new(buffer);
	    gtk_widget_show(label);
  		gtk_container_add(GTK_CONTAINER(vbox_caches), label);
    }


    frame = gtk_frame_new("Shared Secrets");
    gtk_container_border_width (GTK_CONTAINER (frame), 5);
    gtk_container_add (GTK_CONTAINER (vbox), frame);
    gtk_widget_show(frame);
    inner_vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add (GTK_CONTAINER (frame), inner_vbox);
    gtk_widget_show(inner_vbox);

	int i;
	GtkWidget *align = gtk_fixed_new();
	GtkWidget *img;

    hbox = gtk_hbox_new(FALSE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
	gtk_container_add(GTK_CONTAINER(hbox), align);
    gtk_widget_show(align);

    label = gtk_label_new("RS1");    
    gtk_widget_show(label);
	gtk_fixed_put(GTK_FIXED(align), label, X_OFFSET, 0);
    label = gtk_label_new("RS2");    
    gtk_widget_show(label);
	gtk_fixed_put(GTK_FIXED(align), label, X_OFFSET + 65, 0);
    label = gtk_label_new("AUX");    
    gtk_widget_show(label);
	gtk_fixed_put(GTK_FIXED(align), label, X_OFFSET + 130, 0);
    label = gtk_label_new("PBX");    
    gtk_widget_show(label);
	gtk_fixed_put(GTK_FIXED(align), label, X_OFFSET + 195, 0);
	

	align = gtk_fixed_new();
    hbox = gtk_hbox_new(FALSE, 1);
    gtk_container_add(GTK_CONTAINER(inner_vbox), hbox);
    gtk_widget_show(hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), 1);
	gtk_container_add(GTK_CONTAINER(hbox), align);
    gtk_widget_show(align);

	int offset = 0;
    for (i = 1; i < 6; i++)
	{
		if (i == 3)
		  continue;
		if (!memcmp(stream->pkt, ZRTP_PRESHARED, 4))
		{
			if (i == 1)	
				img = gtk_image_new_from_file(SEC_GREEN_PIC);
			else
				img = gtk_image_new_from_file(SEC_GREY_PIC);
		}
		else
		{
			int mask = 1 << i;
			int colorIndex = (session->cached & mask) ? 2 : 0;
			colorIndex |= (session->matches & mask) ? 1 : 0;

			switch (colorIndex)
			{
				case 2:
					img = gtk_image_new_from_file(SEC_RED_PIC);
					break;
				case 3:
					img = gtk_image_new_from_file(SEC_GREEN_PIC);
					break;
				default:
					img = gtk_image_new_from_file(SEC_GREY_PIC);
					break;
			}
		}	
  		gtk_widget_show(img);
		gtk_fixed_put(GTK_FIXED(align), img, X_OFFSET + 65*offset, 0);
		offset++;
	}

	g_signal_connect(window_, "key-press-event", (GCallback)form_key_pressed, NULL);

    return 0;
}
