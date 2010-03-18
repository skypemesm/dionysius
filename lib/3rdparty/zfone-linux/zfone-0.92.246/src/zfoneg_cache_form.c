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

enum
{
	NAME_COLUMN,
	CREATED_COLUMN,
	ACCESSED_COLUMN,
	VERIFIED_COLUMN,
	INDEX_COLUMN,
	ROW_COLOR_COLUMN,
	ROW_COLOR_SET_COLUMN,
	N_COLUMNS
};

static GtkWidget *tree;

static GtkWidget *cache_window;
//static GtkListStore *store;

//! form destroying event handler 
static void destroy()
{
    cache_window = NULL;
}

char *color_name[] = 
{
	"white", 	// !expired, !mitm
	"yellow", 	// !expired, mitm
	"gray",		// expired,  !mitm
	"brown",	// expired,  mitm	
};

void fill_store(GtkListStore *store)
{	
	int i, length;	
	GtkTreeIter iter;
	char buffer_a[30], buffer_c[30], *ptr;

	gtk_list_store_clear(store);
	memset(&iter, 0, sizeof(iter));

	for (i = 0; i < global_cache_cmd.count && i < ZRTP_MAX_CACHE_INFO_RECORD; i++)
	{
		int color_index = 0;
		gtk_list_store_append(store, &iter);
		memset(buffer_c, 0, sizeof(buffer_a));
		memset(buffer_a, 0, sizeof(buffer_a));
		uint32_t uint_tmp = global_cache_cmd.list[i].time_created;
		time_t time_tmp = uint_tmp;
		ptr = ctime((time_t*)&time_tmp);
		if (ptr)
		{
			length = strlen(ptr);
			if ( ptr[length-1] == '\n' ) length--;
			memcpy(buffer_c, ptr, length);
		}
		else
			memcpy(buffer_c, " ", 2);

		time_tmp = global_cache_cmd.list[i].time_accessed;
		ptr = ctime((time_t*)&time_tmp);
		if (ptr)
		{
			length = strlen(ptr);
			if ( ptr[length-1] == '\n' ) length--;
			memcpy(buffer_a, ptr, length);
		}
		else
			memcpy(buffer_a, " ", 2);

		color_index = global_cache_cmd.list[i].is_mitm ? 1 : 0;
		color_index |= global_cache_cmd.list[i].is_expired ? 2 : 0;
		gtk_list_store_set(store, &iter, 
						   NAME_COLUMN,       global_cache_cmd.list[i].name,
						   CREATED_COLUMN,    buffer_c, 
						   ACCESSED_COLUMN,   buffer_a, 
						   VERIFIED_COLUMN,   global_cache_cmd.list[i].verified,
						   INDEX_COLUMN,      i, 
						   ROW_COLOR_COLUMN,  color_name[color_index],
						   ROW_COLOR_SET_COLUMN, /*global_cache_cmd.list[i].is_mitm*/0,
						   -1);
	}
}

//------------------------------------------------------------------------------
void name_edited_callback(GtkCellRendererText *cell, gchar *path_string, char *new_text, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	
	if (gtk_tree_model_get_iter_from_string(store, &iter, path_string))
	{
		int index;	

		gtk_tree_model_get(store, &iter, INDEX_COLUMN, &index, -1);
		if ( index < ZRTP_MAX_CACHE_INFO_RECORD )
		{
			strncpy(global_cache_cmd.list[index].name, new_text, ZFONE_CACHE_NAME_LENGTH - 1);
			global_cache_cmd.list[index].oper = 'm';
		}
	  
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, NAME_COLUMN, new_text, -1);
	}
}

//------------------------------------------------------------------------------
void verified_toggled_callback(GtkCellRendererToggle *cell, gchar *path_string, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	
	if (gtk_tree_model_get_iter_from_string(store, &iter, path_string))
	{
		int value, index;

		gtk_tree_model_get(store, &iter, VERIFIED_COLUMN, &value, INDEX_COLUMN, &index, -1);
		if ( index < ZRTP_MAX_CACHE_INFO_RECORD )
		{
			global_cache_cmd.list[index].verified = !value;
			global_cache_cmd.list[index].oper = 'm';
		}
			
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, VERIFIED_COLUMN, value ? 0 : 1, -1);
	}
}

void cache_close()
{
    gtk_widget_destroy(cache_window);
}

gboolean foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	gchar *name;
	int i = global_cache_cmd.count;

	if (i >= ZRTP_MAX_CACHE_INFO_RECORD	)
	{
		return TRUE;
	}

	gtk_tree_model_get(model, iter, 
					   NAME_COLUMN, &name, 
					   VERIFIED_COLUMN,   &global_cache_cmd.list[i].verified, 
					   -1);
	memcpy(global_cache_cmd.list[i].name, name, strlen(name)+1);
	g_free(name);
	global_cache_cmd.count++;

	return FALSE;
}

void cache_save()
{
//	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

//	memset(&global_cache_cmd, 0, sizeof(global_cache_cmd));	

//	gtk_tree_model_foreach(store, foreach_func, NULL);
    send_cmd(SET_CACHE_INFO, 0, 0, &global_cache_cmd, sizeof(zrtp_cache_info_t));
    gtk_widget_destroy(cache_window);
}

void cache_del()
{
	GtkTreeSelection	*selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		int index;
		gtk_tree_model_get(model, &iter, INDEX_COLUMN, &index, -1);
		if ( index < ZRTP_MAX_CACHE_INFO_RECORD )
		{
			global_cache_cmd.list[index].oper = 'd';
		}
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}
}


static gboolean cache_form_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_Escape)
	{
		cache_close();
		return TRUE;
	}
	
	return FALSE;
}


static gboolean cache_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_Delete)
	{
		cache_del();
		return TRUE;
	}
	if (event->keyval == GDK_Escape)
	{
		cache_close();
		return TRUE;
	}
	
	return FALSE;
}

void cache_request()
{
    send_cmd(GET_CACHE_INFO, 0, 0, NULL, 0);
	
	return;
}

//------------------------------------------------------------------------------
void create_cache_form()
{
    GtkWidget *vbox;
	GtkWidget *align;
	
    if ( cache_window ) 
	{
		fill_store( GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree))) );
		return;
	}
    
    // window creation
    cache_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(cache_window), GTK_WIN_POS_CENTER);
    gtk_window_set_policy(GTK_WINDOW(cache_window), FALSE, FALSE, FALSE);
    gtk_window_set_resizable(GTK_WINDOW(cache_window), FALSE);
    gtk_container_border_width (GTK_CONTAINER (cache_window), 0);
    gtk_window_set_title(GTK_WINDOW(cache_window), "Caches");
    gtk_widget_set_size_request (GTK_WIDGET(cache_window), 650, 300);
    gtk_signal_connect (GTK_OBJECT (cache_window), "destroy", GTK_SIGNAL_FUNC (destroy), NULL);
    gtk_window_set_icon(GTK_WINDOW(cache_window), get_app_icon());
    gtk_widget_show(cache_window);

    // main vbox
    vbox = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(cache_window), vbox);
    gtk_widget_show(vbox);
    gtk_container_border_width(GTK_CONTAINER(vbox), 4);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
	
	// --------------------------------------------------------------------------------------------------
	GtkTreeViewColumn *column;	
	GtkCellRenderer *renderer;
	
	GtkListStore *store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_BOOLEAN);
	fill_store(store);
	
	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_text_new(); 
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", (GCallback)name_edited_callback, NULL);
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", NAME_COLUMN, "cell-background", ROW_COLOR_COLUMN, "cell-background-set", ROW_COLOR_SET_COLUMN, NULL);
	gtk_tree_view_column_set_min_width(column, 200);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	renderer = gtk_cell_renderer_text_new(); 
	column = gtk_tree_view_column_new_with_attributes("Created", renderer, "text", CREATED_COLUMN, "cell-background", ROW_COLOR_COLUMN, "cell-background-set", ROW_COLOR_SET_COLUMN,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	renderer = gtk_cell_renderer_text_new(); 
	column = gtk_tree_view_column_new_with_attributes("Accessed", renderer, "text", ACCESSED_COLUMN, "cell-background", ROW_COLOR_COLUMN,  "cell-background-set", ROW_COLOR_SET_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	renderer = gtk_cell_renderer_toggle_new(); 
	g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
	g_signal_connect(renderer, "toggled", (GCallback)verified_toggled_callback, NULL);
	column   = gtk_tree_view_column_new_with_attributes("Verified", renderer, "active", VERIFIED_COLUMN,  "cell-background", ROW_COLOR_COLUMN,  "cell-background-set", ROW_COLOR_SET_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	g_signal_connect(tree, "key-press-event", (GCallback)cache_key_pressed, NULL);
	g_signal_connect(cache_window, "key-press-event", (GCallback)cache_form_key_pressed, NULL);

    gtk_widget_set_size_request (GTK_WIDGET(tree), 640, 260);
    gtk_container_add(GTK_CONTAINER (vbox), tree);
	gtk_widget_show(tree);
	
// ---------------------------------------------------------------------

    GtkWidget *del_button = gtk_button_new_with_label ("Delete");
    gtk_signal_connect (GTK_OBJECT (del_button), "clicked", GTK_SIGNAL_FUNC (cache_del), NULL);
    GtkWidget *ok_button = gtk_button_new_with_label ("Save");
    gtk_signal_connect (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (cache_save), NULL);
    GtkWidget *cancel_button = gtk_button_new_with_label ("Cancel");
    gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked", GTK_SIGNAL_FUNC (cache_close), NULL);
    GtkWidget *button_hbox = gtk_hbox_new(TRUE, 5);
    gtk_container_border_width(GTK_CONTAINER (button_hbox), 3);
    gtk_widget_set_size_request(del_button, 80, 25);
    gtk_widget_set_size_request(ok_button, 80, 25);
    gtk_widget_set_size_request(cancel_button, 80, 25);
    align = gtk_alignment_new(1.0, 0.5, 0, 0);

    gtk_container_add(GTK_CONTAINER (button_hbox), del_button);
    gtk_container_add(GTK_CONTAINER (button_hbox), ok_button);
    gtk_container_add(GTK_CONTAINER (button_hbox), cancel_button);
    gtk_container_add(GTK_CONTAINER (align), button_hbox);
    gtk_container_add(GTK_CONTAINER (vbox), align);
    gtk_widget_show(del_button);
    gtk_widget_show(ok_button);
    gtk_widget_show(cancel_button);
    gtk_widget_show(align);
    gtk_widget_show(button_hbox);

	return;
}
