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

#include "files.h"
#include "zfoneg_ctrl.h"
#include "zfoneg_cb.h"
#include "zfoneg_listbox.h"
#include "zfoneg_commands.h"
#include "zfoneg_check_form.h"

#define CHK_AUTO	1
#define CHK_STAY	2
#define	CHK_ACTIVE	3

GtkWidget *checkWindow;
GtkWidget *label_current;
GtkWidget *label_new;
GtkWidget *label_state;

int	red_button;

//------------------------------------------------------------------------------
//! parse command and visualize version 
void  update_check_info(zrtp_cmd_t *cmd, GtkWidget *mainWindow)
{
    zrtp_cmd_send_version_t* sv = (zrtp_cmd_send_version_t*) (&cmd->data);
    
    // calculate new version
    int maj_ver_new = sv->new_version / 1000;
    int min_ver_new = (sv->new_version - (maj_ver_new * 1000));
    int maj_ver_cur = sv->curr_version / 1000;
    int min_ver_cur = (sv->curr_version - (maj_ver_cur * 1000));
	char zrtp_version[ZFONEG_VERSION_SIZE];

	memset(zrtp_version, 0, ZFONEG_VERSION_SIZE);
	memcpy(zrtp_version, sv->zrtp_version, ZFONEG_VERSION_SIZE);
    sprintf(version, "ver. %u.%02u build %u. ZRTP version %s", maj_ver_cur, min_ver_cur, sv->curr_build, zrtp_version);

    // if check window is not present on the screen
    if ( !checkWindow )
    {
	// if version is expired, than show update form
	if ( sv->is_expired == 1 || 
	   ( sv->curr_version < sv->new_version || 
	   ( sv->curr_version == sv->new_version && sv->curr_build < sv->new_build )))
	{
	    create_check_form(version, NULL);
	}
	else
	    return;
    }
    // else if window is present but version was not asked by user - just return
    else if ( !sv->is_manually )
    {
	return;
    }
    
    gtk_label_set_text(GTK_LABEL(label_current), version);
    if ( sv->is_expired == 2 && sv->is_manually )
    {
	// connection error
	gtk_label_set_text(GTK_LABEL(label_new), "Unknown");
	gtk_label_set_text(GTK_LABEL(label_state), "Can't connect to the server");
    }
    else
    {
	char new_version[128];
	sprintf(new_version, "ver. %u.%02u build %u", maj_ver_new, min_ver_new, sv->new_build);
	if ( !sv->new_version )
	{
	    gtk_label_set_text(GTK_LABEL(label_new), "Unknown");
	}
	else
	{
	    gtk_label_set_text(GTK_LABEL(label_new), new_version);
	}
	if ( sv->is_expired == 1 )
	{
	    char url_message[128];
	    int length;
	    strcpy(url_message,  "Your version is expired! Check at\n");
	    length = strlen(url_message);
	    memcpy(url_message + length, sv->url, sv->url_length);
	    url_message[length + sv->url_length] = 0;
	    gtk_label_set_text(GTK_LABEL(label_state), url_message);
	    red_button = 1;
	}
	else if ( sv->is_expired != 2 && 
	        ( sv->curr_version < sv->new_version || 
		( sv->curr_version == sv->new_version && sv->curr_build < sv->new_build )))
	{
	    char url_message[128];
	    int length;
	    strcpy(url_message,  "New version is available at\n");
	    length = strlen(url_message);
	    memcpy(url_message + length, sv->url, sv->url_length);
	    url_message[length + sv->url_length] = 0;
	    gtk_label_set_text(GTK_LABEL(label_state), url_message);
	}
	else
	{
	    gtk_label_set_text(GTK_LABEL(label_state), "No new updates are available");
	}
    }
}

//------------------------------------------------------------------------------
void check_destroy()
{
    if ( red_button )
    {
		raise(SIGTERM);
    }
    checkWindow = NULL;
}

//------------------------------------------------------------------------------
void close_check()
{
    gtk_widget_destroy(checkWindow);
}

//------------------------------------------------------------------------------
int create_check_form(char *current_version, char *new_version)
{
    if ( checkWindow )
	return 0;
	
    red_button = 0;
    // window creation
    checkWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(checkWindow), GTK_WIN_POS_CENTER);
    gtk_window_set_policy(GTK_WINDOW(checkWindow), FALSE, FALSE, FALSE);
    gtk_window_set_resizable(GTK_WINDOW(checkWindow), FALSE);
    
    gtk_container_border_width (GTK_CONTAINER (checkWindow), 0);
    gtk_window_set_title(GTK_WINDOW(checkWindow), "Checking version");
    gtk_window_set_icon(GTK_WINDOW(checkWindow), get_app_icon());

    gtk_signal_connect (GTK_OBJECT (checkWindow), "destroy", GTK_SIGNAL_FUNC (check_destroy), NULL);
    
    // current version controls
    if ( !current_version || !*current_version ) 
		label_current = gtk_label_new ("Checking...");
    else
		label_current = gtk_label_new (current_version);    

    GtkWidget *label_current_frame  = gtk_frame_new( "Your version" );
    gtk_widget_set_size_request (label_current_frame, 230, 50);
    gtk_container_set_border_width (GTK_CONTAINER (label_current_frame), 3);


    // new vesion controls
    if ( !new_version || !*new_version )
		label_new = gtk_label_new ("Checking...");
    else
		label_new = gtk_label_new (new_version);
    GtkWidget *label_new_frame  = gtk_frame_new( "Version on update server" );
    gtk_widget_set_size_request (label_new_frame, 230, 50);
    gtk_container_set_border_width(GTK_CONTAINER(label_new_frame), 3);

    // progress
    GtkWidget *label_state_frame  = gtk_frame_new( "Checking progress" );
    gtk_widget_set_size_request (label_state_frame, 230, 70);
    gtk_container_set_border_width (GTK_CONTAINER (label_state_frame), 3);
    label_state = gtk_label_new ("Sending request...");

    GtkWidget *main_vbox = gtk_vbox_new (FALSE, 1);
    gtk_widget_set_size_request (GTK_WIDGET(main_vbox), 300, 200);
    gtk_container_add (GTK_CONTAINER (checkWindow), main_vbox);

    GtkWidget *align_current = gtk_alignment_new(0.9, 0.5, 0, 0);
    GtkWidget *align_new     = gtk_alignment_new(0.9, 0.5, 0, 0);
    GtkWidget *align_state   = gtk_alignment_new(0.9, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align_current) , label_current);
    gtk_container_add (GTK_CONTAINER (label_current_frame) , align_current);
    gtk_container_add (GTK_CONTAINER (align_new) , label_new);
    gtk_container_add (GTK_CONTAINER (label_new_frame) , align_new);
    gtk_container_add (GTK_CONTAINER (align_state) , label_state);
    gtk_container_add (GTK_CONTAINER (label_state_frame) , align_state);
    gtk_container_add (GTK_CONTAINER (main_vbox), label_current_frame);
    gtk_container_add (GTK_CONTAINER (main_vbox), label_new_frame);
    gtk_container_add (GTK_CONTAINER (main_vbox), label_state_frame);

    GtkWidget *button = gtk_button_new_with_label ("Close");
    gtk_widget_set_size_request (button, 60, 35);
    gtk_container_set_border_width (GTK_CONTAINER (button), 3);
    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (close_check), NULL);

    GtkWidget *align_button   = gtk_alignment_new(1.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align_button) , button);
    gtk_container_add (GTK_CONTAINER (main_vbox), align_button);

    gtk_widget_show(main_vbox);
    gtk_widget_show(align_current);
    gtk_widget_show(align_new);
    gtk_widget_show(align_state);
    gtk_widget_show(align_button);
    gtk_widget_show(label_current);
    gtk_widget_show(label_current_frame);
    gtk_widget_show(label_new);
    gtk_widget_show(label_new_frame);
    gtk_widget_show(label_state);
    gtk_widget_show(label_state_frame);
    gtk_widget_show(button);
    
    gtk_widget_show(checkWindow);

    return 1;
}
