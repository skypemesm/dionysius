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
#include <netinet/in.h>

#include <gdk/gdkkeysyms.h>

#include <zrtp.h>

#include "files.h"
#include "zfoneg_config.h"
#include "zfoneg_ctrl.h"
#include "zfoneg_cb.h"
#include "zfoneg_listbox.h"
#include "zfoneg_commands.h"
#include "zfoneg_utils.h"


#define PROTOCOL_MAX_SIZE	4
#define PORT_MAX_SIZE		10
#define DESCR_MAX_SIZE		100

#define	IP_FLAG_AUTO		"auto"
#define	IP_FLAG_MANUAL		"manual"

#define X_OFFSET	50
#define X_OFFSET2	90

// enumeration to distinguish checkboxes
enum checks
{
    CHK_GEN_AUTO,
    CHK_GEN_STAY,
    CHK_GEN_FOREVER,
    CHK_CRY_AES128,
    CHK_CRY_AES256,
    CHK_CRY_ATL32,
    CHK_CRY_ATL80,
    CHK_CRY_DH4096,
    CHK_CRY_DH3072,
	CHK_CRY_MULT,	
    CHK_CRY_SHA256,
    CHK_CRY_BASE32,
    CHK_CRY_BASE256,
	CHK_CRY_EC256,
	CHK_CRY_EC384,
	CHK_CRY_EC521,
    CHK_TEST_DISCLOSE,
    CHK_TEST_GIZMO,
    CHK_SNF_SPEC,
    CHK_SNF_UDP,
    CHK_SNF_TCP,
};

// enumeration to distinguish panes
enum panes
{
    PANE_GENERAL,
    PANE_CRYPTO,
    PANE_TEST,
    PANE_SNIFF
};

enum
{
    TEST_IP_ADD,
    TEST_IP_DEL
};

// Widgets
static GtkWidget	*pane_vbox;
static GtkWidget	*generalPane, *cryptoPane, *currentPane, *testPane, *sniffPane;
static GtkWidget	*chkAutoSecure, *chkAllowClear;
static GtkWidget	*chkAES128, *chkAES256, *chkATL32, *chkATL80, *chkPreshared, *chkMulti;
static GtkWidget	*chkDH3072, *chkSHA256, *chkBase32, *chkBase256, *chkDisclose;
static GtkWidget	*chkECDH256, *chkECDH384, *chkECDH521;
static GtkWidget	*chkListenSpec, *chkListenUDP, *chkListenTCP;
static GtkWidget	*entryPort, *comboProto, *entryDescr;
static GtkWidget 	*status_bar, *spinner, *lbSecrets1, *lbSecrets2, *chkForever;
static GtkWidget	*window_, *sniff_list;
static GtkWidget	*list_ip;
static GtkWidget 	*entryIP;
static GtkWidget	*chkPrintDebug, *chkHearCtx;
static GtkWidget	*chkAlert;

// variable for statusbar
static gint		context_id; 

static int		sniff_count = 0;
static int		sniff_selected;
static int		ip_count = 0;
static int		ip_selected, ip_modified;

//------------------------------------------------------------------------------
//! set status bar text
static void setStatusBarText(char *text)
{
    gtk_statusbar_pop( GTK_STATUSBAR(status_bar), context_id);		    
    gtk_statusbar_push( GTK_STATUSBAR(status_bar), context_id, text);		    
}

//------------------------------------------------------------------------------
//! enables or disables spinner and labels for setting secret expiration
static void enable_secrets_expire(int is_enabled)
{
    if ( is_enabled )
    {
		gtk_widget_set_sensitive(spinner, TRUE);
		gtk_widget_set_sensitive(lbSecrets1, TRUE);
		gtk_widget_set_sensitive(lbSecrets2, TRUE);
    }
    else
    {
		gtk_widget_set_sensitive(spinner, FALSE);
		gtk_widget_set_sensitive(lbSecrets1, FALSE);
		gtk_widget_set_sensitive(lbSecrets2, FALSE);
    }
}

//------------------------------------------------------------------------------
void clearStatusBar()
{
    setStatusBarText("Do not modify these settings unless you know what you are doing");
}


//------------------------------------------------------------------------------
//! handles checkboxes' events
void checked( GtkWidget *widget, gpointer data )
{
	static int ignore_checked = 0;

	if (ignore_checked)
		return;

    clearStatusBar();
    switch ((long)data)
    {
	// this checkboxes have to be checked all the time
	case CHK_CRY_AES128:
	case CHK_CRY_ATL32:
	case CHK_CRY_MULT:
	case CHK_CRY_DH3072:
	case CHK_CRY_SHA256:
	case CHK_CRY_BASE32:
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
	    break;
	}
	case CHK_GEN_FOREVER:
	{
	    enable_secrets_expire(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	    break;
	}
	case CHK_CRY_AES256:
	{
		if (params.is_ec)
		{
	  		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) )
		  		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH384), TRUE);
			else
			{
		  		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH256), TRUE);
		  		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH384), FALSE);
		  		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH521), FALSE);
			}
		}
		break;
	}
    case CHK_CRY_DH4096:
	{
		ignore_checked = 1;
	    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) )
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES256), TRUE);
		ignore_checked = 0;
		break;
	}
	case CHK_CRY_EC384:
	{
		ignore_checked = 1;
	    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) )
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES256), TRUE);
		else if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkECDH521)))
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES256), FALSE);
		ignore_checked = 0;
		break;
	}
	case CHK_CRY_EC521:
	{
		ignore_checked = 1;
	    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) )
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES256), TRUE);
		else if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkECDH384)))
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES256), FALSE);
		ignore_checked = 0;
		break;
	}
    }
}

//! handles form destroying 
void on_destroy()
{
    // delete references to panes
    gtk_widget_unref(generalPane);
    gtk_widget_unref(cryptoPane);
    gtk_widget_unref(sniffPane);
    if (testPane) gtk_widget_unref(testPane);
    
    window_ = NULL;
}

//------------------------------------------------------------------------------
void pref_close()
{
    gtk_widget_destroy(window_);
}

static int get_proto_type(gchar *value)
{
    if (!strcmp(value, "UDP"))
		return PROTO_UDP;
    if (!strcmp(value, "TCP")) 
		return PROTO_TCP;
	
    return -1;
}

//------------------------------------------------------------------------------
//! save preferences
void pref_save()
{
    int i;
    gchar *text;

    // clear params as we will fill them
    clearParams();

    params.zrtp.autosecure = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkAutoSecure));
    params.zrtp.allowclear = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkAllowClear));
    params.hear_ctx = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkHearCtx));

    params.print_debug = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkPrintDebug));
    params.alert = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkAlert));

    params.sniff.sip_scan_mode = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkListenSpec)) * ZRTP_BIT_SIP_SCAN_PORTS  |
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkListenUDP))  * ZRTP_BIT_SIP_SCAN_UDP    |
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkListenTCP))  * ZRTP_BIT_SIP_SCAN_TCP; 


    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkAES256)) )
		addParam(ZRTP_CC_CIPHER, ZRTP_CIPHER_AES256);
    if (  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkAES128)) )
		addParam(ZRTP_CC_CIPHER, ZRTP_CIPHER_AES128);
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkATL80)) )
		addParam(ZRTP_CC_ATL, ZRTP_ATL_HS80);
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkATL32)) )
		addParam(ZRTP_CC_ATL, ZRTP_ATL_HS32);

	if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkPreshared)) )
		addParam(ZRTP_CC_PKT, ZRTP_PKTYPE_PRESH);
#if (defined(ZRTP_USE_ENTERPRISE) && (ZRTP_USE_ENTERPRISE == 1))
	if ( params.is_ec )
	{
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkECDH521)) )
		{
			addParam(ZRTP_CC_PKT, ZRTP_PKTYPE_EC521P);
		}
	  	if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkECDH384)) )
			addParam(ZRTP_CC_PKT, ZRTP_PKTYPE_EC384P);
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkECDH256)) )
			addParam(ZRTP_CC_PKT, ZRTP_PKTYPE_EC256P);
	}
#endif
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkDH3072)) )
		addParam(ZRTP_CC_PKT, ZRTP_PKTYPE_DH3072);
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkMulti)) )
		addParam(ZRTP_CC_PKT, ZRTP_PKTYPE_MULT);

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkSHA256)) )
		addParam(ZRTP_CC_HASH, ZRTP_HASH_SHA256);
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkBase256)) )
		addParam(ZRTP_CC_SAS, ZRTP_SAS_BASE256);
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkBase32)) )
		addParam(ZRTP_CC_SAS, ZRTP_SAS_BASE32);
		
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkForever)) )
    {
		params.zrtp.cache_ttl = SECRET_NEVER_EXPIRES;
    }
    else
    {
		params.zrtp.cache_ttl = (int)gtk_spin_button_get_value( GTK_SPIN_BUTTON(spinner)) * 86400; 
    }
    

    for (i = 0; i < ZRTP_MAX_SIP_PORTS_FOR_SCAN; i++)
    {
		params.sniff.sip_ports[i].port = 0;
    }
    
    for(i = 0; i < sniff_count; i++)
    {
		gtk_clist_get_text(GTK_CLIST(sniff_list), i, 0, &text);
		params.sniff.sip_ports[i].port = atoi(text);
		gtk_clist_get_text(GTK_CLIST(sniff_list), i, 1, &text);
		params.sniff.sip_ports[i].proto = get_proto_type(text);
		gtk_clist_get_text(GTK_CLIST(sniff_list), i, 2, &text);
		strncpy(params.sniff.sip_ports[i].desc, text, ZRTP_SIP_PORT_DESC_MAX_SIZE - 1);
		params.sniff.sip_ports[i].desc[ZRTP_SIP_PORT_DESC_MAX_SIZE - 1] = 0;
    }
    while(params.sniff.sip_ports[i].port != 0)
    {
		params.sniff.sip_ports[i++].port = 0;
    }

	if (ip_modified)
	{
	    int ip;
	    for (i = 0; i < ip_count; i++)	
	    {
			gtk_clist_get_text(GTK_CLIST(list_ip), i, 0, &text);
			ip = htonl(zfone_str2ip(text));
			if (ip)
			{
			    ips_holder.ips[i] = ip;
			    gtk_clist_get_text(GTK_CLIST(list_ip), i, 1, &text);
			    ips_holder.flags[i] = !strcmp(text, IP_FLAG_AUTO) ? IP_FLAG_AUTO_VALUE : IP_FLAG_MANUAL_VALUE;
			}
  	    }
	    ips_holder.ip_count = ip_count;
	    send_cmd(SET_IPS, 0, 0, &ips_holder, sizeof(ips_holder));
	}

    if (testPane)
    {
		params.zrtp.disclose_bit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkDisclose));
    }
    
    send_cmd(SET_PREF, 0, 0, &params, sizeof(params));
    pref_close();
    update_main_view(0);
}

//------------------------------------------------------------------------------
//! changes panes
void change_pane(GtkWidget *widget, gpointer data)
{
    switch ((long)data)
    {
	case PANE_GENERAL:
	{
	    if (currentPane != generalPane)
	    {
		gtk_container_remove(GTK_CONTAINER(pane_vbox), currentPane);
		gtk_container_add(GTK_CONTAINER(pane_vbox), generalPane);
		currentPane = generalPane;
	    }
	    break;
	}
	case PANE_CRYPTO:
	{
	    if (currentPane != cryptoPane)
	    {
		gtk_container_remove(GTK_CONTAINER(pane_vbox), currentPane);
		gtk_container_add(GTK_CONTAINER(pane_vbox), cryptoPane);
		currentPane = cryptoPane;
	    }
	    break;
	}
	case PANE_TEST:
	{
	    if (currentPane != testPane)
	    {
		gtk_container_remove(GTK_CONTAINER(pane_vbox), currentPane);
		gtk_container_add(GTK_CONTAINER(pane_vbox), testPane);
		currentPane = testPane;
	    }
	    break;
	}
	case PANE_SNIFF:
	{	
	    if (currentPane != sniffPane)
	    {
		gtk_container_remove(GTK_CONTAINER(pane_vbox), currentPane);
		gtk_container_add(GTK_CONTAINER(pane_vbox), sniffPane);
		currentPane = sniffPane;
	    }
	    break;
	}
    }
}

//------------------------------------------------------------------------------
//! sets GUI controls due to params values
void set_prefs(zfone_params_t *p)
{
    int i;

    if ( !window_ ) return;

	params.is_ec = p->is_ec;
	params.is_debug = p->is_debug;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAutoSecure), p->zrtp.autosecure);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAllowClear), p->zrtp.allowclear);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkHearCtx), p->hear_ctx);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkPrintDebug), p->print_debug);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAlert), p->alert);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkListenSpec), 
  		p->sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_PORTS);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkListenUDP),  
		p->sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_UDP);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkListenTCP),  
		p->sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_TCP);

    if ( p->zrtp.cache_ttl == SECRET_NEVER_EXPIRES )
    {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), SECRET_DEFAULT_EXPIRE);
		enable_secrets_expire(0);    
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkForever), 1);    
    }
    else
    {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), p->zrtp.cache_ttl / 86400);
		enable_secrets_expire(1);    
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkForever), 0);    
    }
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkBase32), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkBase256), 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkPreshared), 0);
//    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkMulti), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDH3072), 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH256), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH384), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH521), 0);

	if (!p->is_ec)
	{
		gtk_widget_set_sensitive(chkECDH256, FALSE);
		gtk_widget_set_sensitive(chkECDH384, FALSE);
		gtk_widget_set_sensitive(chkECDH521, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(chkECDH256, TRUE);
		gtk_widget_set_sensitive(chkECDH384, TRUE);
		gtk_widget_set_sensitive(chkECDH521, TRUE);
	}

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkATL32), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkATL80), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES128), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES256), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkSHA256), 0);
    

    for (i = 0; i < ZRTP_MAX_COMP_COUNT; i++)
    {
    	switch ( p->zrtp.sas_schemes[i] )
	{
	    case ZRTP_SAS_BASE32:
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkBase32), 1);
		break;
	    }
	    case ZRTP_SAS_BASE256:
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkBase256), 1);
		break;
	    }
	}
    }

    for (i = 0; i < ZRTP_MAX_COMP_COUNT; i++)
    {
    	switch ( p->zrtp.pk_schemes[i] )
		{
	    case ZRTP_PKTYPE_PRESH:
	    {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkPreshared), 1);
			break;
	    }
	    case ZRTP_PKTYPE_MULT:
	    {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkMulti), 1);
			break;
	    }
	    case ZRTP_PKTYPE_DH3072:
	    {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDH3072), 1);
			break;
	    }
#if (defined(ZRTP_USE_ENTERPRISE) && (ZRTP_USE_ENTERPRISE == 1))
	    case ZRTP_PKTYPE_EC256P:
	    {
			if (p->is_ec)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH256), 1);
			break;
	    }
	    case ZRTP_PKTYPE_EC384P:
	    {
			if (p->is_ec)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH384), 1);
			break;
	    }
	    case ZRTP_PKTYPE_EC521P:
	    {
			if (p->is_ec)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkECDH521), 1);
			break;
	    }
#endif
		}
    }

    for (i = 0; i < ZRTP_MAX_COMP_COUNT; i++)
    {
	switch ( p->zrtp.auth_tag_lens[i] )
	{
	    case ZRTP_ATL_HS32:
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkATL32), 1);
		break;
	    }
	    case ZRTP_ATL_HS80:
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkATL80), 1);
		break;
	    }
	}
    }

    for (i = 0; i < ZRTP_MAX_COMP_COUNT; i++)
    {
	switch ( p->zrtp.cipher_types[i] )
	{
	    case ZRTP_CIPHER_AES128:
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES128), 1);
		break;
	    }
	    case ZRTP_CIPHER_AES256:
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES256), 1);
		break;
	    }
	}
    }

    for (i = 0; i < ZRTP_MAX_COMP_COUNT; i++)
    {
	switch ( p->zrtp.hash_schemes[i] )
	{
	    case ZRTP_HASH_SHA256:
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkSHA256), 1);
		break;
	    }
	}
    }

    sniff_count = 0;
    gtk_clist_clear(GTK_CLIST(sniff_list));
    for (i = 0; i < ZRTP_MAX_SIP_PORTS_FOR_SCAN; i++)
    {
	char proto[4], port[10], descr[DESCR_MAX_SIZE];
	char *ptrs[3];
	
	if ( !p->sniff.sip_ports[i].port )
	{
	    continue;
	}
	sprintf(port, "%d", p->sniff.sip_ports[i].port);
	strncpy(descr, p->sniff.sip_ports[i].desc, DESCR_MAX_SIZE - 1);
	descr[DESCR_MAX_SIZE - 1] = 0;
	if (p->sniff.sip_ports[i].proto == PROTO_UDP)
	    strcpy(proto, "UDP");
	else
	    strcpy(proto, "TCP");
	ptrs[0] = port;
	ptrs[1] = proto;
	ptrs[2] = descr;
	gtk_clist_append(GTK_CLIST(sniff_list), ptrs);
	sniff_count++;
    }


    if (sniff_count > 0)
    {
	gtk_clist_select_row(GTK_CLIST(sniff_list), 0, 0);    
    }

    {
	char	*ptrs[2], ip_buffer[20];
	ptrs[0] = ip_buffer;

	gtk_clist_clear(GTK_CLIST(list_ip));
	ip_count = 0;
	for (i = 0;i < ips_holder.ip_count; i++)
	{
	    memset(ip_buffer, 0, 16);
	    zfone_ip2str(ip_buffer, ntohl(ips_holder.ips[i]));
	    if ( ips_holder.flags[i] == IP_FLAG_AUTO_VALUE )
		ptrs[1] = IP_FLAG_AUTO;
	    else
		ptrs[1] = IP_FLAG_MANUAL;
	    gtk_clist_append(GTK_CLIST(list_ip), ptrs);
	    ip_count++;
	}

	if (ip_count > 0)
	{
	    gtk_clist_select_row(GTK_CLIST(list_ip), 0, 0);    
	}
    }
    
    if (testPane)
    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDisclose),   p->zrtp.disclose_bit);
    }
}

void setValues()
{
    set_prefs(&params);
}

//------------------------------------------------------------------------------
//! set defaults values
void pref_default()
{
    send_cmd(SET_DEFAULTS, 0, 0, NULL, 0);
}

static void add_new_record()
{
    char proto[PROTOCOL_MAX_SIZE], 
	 port[PORT_MAX_SIZE],
	 descr[DESCR_MAX_SIZE],
	 *value, *ptrs[3];
    int  test_port;
    
    value = (char *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(comboProto)->entry));
    strcpy(proto, value);

    value = (char *)gtk_entry_get_text(GTK_ENTRY(entryPort));
    strncpy(port, value, PORT_MAX_SIZE - 1);
    port[PORT_MAX_SIZE - 1] = 0;
    test_port = atoi(port);
    if (test_port <= 0 || test_port > 0xFFFF)
    {
		printf("PORT IS INCORRECT!\n");
		return;
    }
    
    value = (char *)gtk_entry_get_text(GTK_ENTRY(entryDescr));
    strncpy(descr, value, DESCR_MAX_SIZE - 1);
    descr[PORT_MAX_SIZE - 1] = 0;

    ptrs[0] = port;
    ptrs[1] = proto;
    ptrs[2] = descr;
    gtk_clist_append(GTK_CLIST(sniff_list), ptrs);
    sniff_count++;
    gtk_entry_set_text(GTK_ENTRY(entryPort), "");
    gtk_entry_set_text(GTK_ENTRY(entryDescr), "");
}

void remove_record()
{
    gtk_clist_remove(GTK_CLIST(sniff_list), sniff_selected);
    sniff_count--;
    if (sniff_selected >= sniff_count)
        sniff_selected--;
}

static void select_row_ip(GtkWidget *widget,
                             gint row,
                             gint column,
                             GdkEventButton *event,
                             gpointer data)
{
    ip_selected = row;
}

static void ip_event(GtkWidget *widget, gpointer data)
{
    ip_modified = 1;
    if ((long)data == TEST_IP_ADD)
    {
	char	*ptrs[2];
	ptrs[0] = (char *)gtk_entry_get_text(GTK_ENTRY(entryIP));
	ptrs[1] = IP_FLAG_MANUAL;
	gtk_clist_append(GTK_CLIST(list_ip), ptrs);
	ip_count++;
	gtk_entry_set_text(GTK_ENTRY(entryIP), "");
    }
    else if ((long)data == TEST_IP_DEL)
    {
	gchar *text;
	gtk_clist_get_text(GTK_CLIST(list_ip), ip_selected, 1, &text);
	if (!strcmp(text, IP_FLAG_AUTO)) return;

	gtk_clist_remove(GTK_CLIST(list_ip), ip_selected);
	ip_count--;
	if (ip_selected >= ip_count)
    	    ip_selected--;
    }
}

//! general tab creation 
void create_general_pane()
{
    GtkWidget *align;
    GtkWidget *main_vbox, *separator;

    // frames and boxes creation
    generalPane = gtk_frame_new (NULL);
    gtk_container_set_border_width(GTK_CONTAINER(generalPane), 3);
    gtk_widget_set_size_request (GTK_WIDGET(generalPane), 400, 200);
    gtk_widget_show (generalPane);
		
    align = gtk_alignment_new(0.2, 0, 1, 1);
    gtk_container_add(GTK_CONTAINER (generalPane), align);
    gtk_widget_show(align);
    main_vbox = gtk_vbox_new (FALSE, 1);
    gtk_box_set_homogeneous(GTK_BOX(main_vbox), TRUE);
    gtk_container_add (GTK_CONTAINER (align), main_vbox);
    gtk_widget_show (main_vbox);

    chkAutoSecure = gtk_check_button_new_with_label ("Automatically initiate secure mode on connect");
    gtk_signal_connect (GTK_OBJECT (chkAutoSecure), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_GEN_AUTO);
    gtk_widget_show(chkAutoSecure);
	put_to_offset(main_vbox, chkAutoSecure, X_OFFSET);
    
    chkAllowClear = gtk_check_button_new_with_label ("Allow to switching to plain RTP");
    gtk_signal_connect (GTK_OBJECT (chkAllowClear), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_GEN_STAY);
    gtk_widget_show (chkAllowClear);
	put_to_offset(main_vbox, chkAllowClear, X_OFFSET);

    separator = gtk_hseparator_new();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (main_vbox), separator);

    GtkWidget *tmp_hbox = gtk_hbox_new(FALSE, 1);
    gtk_container_set_border_width(GTK_CONTAINER(tmp_hbox), 3);
    gtk_widget_show (tmp_hbox);
	put_to_offset(main_vbox, tmp_hbox, X_OFFSET);

    // secret expiration labels and spinner
    lbSecrets1 = gtk_label_new ("Retain shared secrets for ");    
    gtk_container_add (GTK_CONTAINER (tmp_hbox), lbSecrets1);
    gtk_widget_show(lbSecrets1);

    GtkObject *adj = gtk_adjustment_new(30, 0, 365 * 5, 1, 30, 0);
    spinner = gtk_spin_button_new ((GtkAdjustment *)adj, 0, 0);
    gtk_spin_button_set_wrap( GTK_SPIN_BUTTON(spinner), TRUE );
    gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(spinner), TRUE );
    gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON(spinner), GTK_UPDATE_IF_VALID );
    gtk_container_add (GTK_CONTAINER (tmp_hbox), spinner);
    gtk_widget_show(spinner);

    lbSecrets2 = gtk_label_new (" days");    
    gtk_container_add (GTK_CONTAINER (tmp_hbox), lbSecrets2);
    gtk_widget_show(lbSecrets2);

    chkForever = gtk_check_button_new_with_label ("Retain shared secrets forever");
    gtk_signal_connect (GTK_OBJECT (chkForever), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_GEN_FOREVER);
    gtk_widget_show (chkForever);
    put_to_offset(main_vbox, chkForever, X_OFFSET);

    separator = gtk_hseparator_new();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (main_vbox), separator);

    chkPrintDebug = gtk_check_button_new_with_label ("Create debug log");
    gtk_widget_show(chkPrintDebug);
    put_to_offset(main_vbox, chkPrintDebug, X_OFFSET);

    chkHearCtx = gtk_check_button_new_with_label ("Hear ctx");
    gtk_widget_show(chkHearCtx);
    put_to_offset(main_vbox, chkHearCtx, X_OFFSET);

    chkAlert = gtk_check_button_new_with_label ("Alert on disappearance of outgoing traffic stream");
    gtk_widget_show(chkAlert);
    put_to_offset(main_vbox, chkAlert, X_OFFSET);

    gtk_widget_ref(generalPane);
}

// crypto pane creation
void create_crypto_pane()
{
    GtkWidget *align, *label, *separator, *main_vbox, *hbox;

    // frames and boxes creation
    cryptoPane = gtk_frame_new (NULL);
    gtk_container_set_border_width (GTK_CONTAINER(cryptoPane), 3);
    gtk_widget_show (cryptoPane);
    gtk_widget_set_size_request (GTK_WIDGET(cryptoPane), 400, 440);

    align = gtk_alignment_new(0, 0, 1, 1);
    gtk_container_add(GTK_CONTAINER (cryptoPane), align);
    gtk_widget_show(align);
    main_vbox = gtk_vbox_new (FALSE, 1);
    gtk_box_set_homogeneous(GTK_BOX(main_vbox), FALSE);
    gtk_container_add (GTK_CONTAINER (align), main_vbox);
    gtk_widget_show (main_vbox);

// ---------------------------------------------------------------------------------
// Cipher type controls
    label = gtk_label_new ("Cipher types");    
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), label);
    gtk_widget_show(label);

    chkAES128 = gtk_check_button_new_with_label ("AES-128");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES128), 1);
    gtk_signal_connect (GTK_OBJECT (chkAES128), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_CRY_AES128);
    gtk_widget_set_sensitive(GTK_WIDGET(chkAES128), FALSE);
    gtk_widget_show(chkAES128);
	put_to_offset(main_vbox, chkAES128, X_OFFSET2);

    chkAES256 = gtk_check_button_new_with_label ("AES-256");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkAES256), 0);
    gtk_signal_connect (GTK_OBJECT (chkAES256), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_CRY_AES256);
    gtk_widget_show (chkAES256);
	put_to_offset(main_vbox, chkAES256, X_OFFSET2);

    separator = gtk_hseparator_new();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (main_vbox), separator);

// ---------------------------------------------------------------------------------
// Pkt controls
    label = gtk_label_new ("Public key exchange schemes");    
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_container_add (GTK_CONTAINER(main_vbox), label);
    gtk_widget_show(label);

    chkDH3072 = gtk_check_button_new_with_label ("DH-3072");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDH3072), 1);
    gtk_signal_connect (GTK_OBJECT (chkDH3072), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_CRY_DH3072);
    gtk_widget_set_sensitive(GTK_WIDGET(chkDH3072), FALSE);
    gtk_widget_show (chkDH3072);
	put_to_offset(main_vbox, chkDH3072, X_OFFSET2);

  	chkECDH256 = gtk_check_button_new_with_label("ECDH-256");
    gtk_signal_connect (GTK_OBJECT(chkECDH256), "clicked", GTK_SIGNAL_FUNC(checked), (gpointer)CHK_CRY_EC256);
	gtk_widget_show(chkECDH256);
	put_to_offset(main_vbox, chkECDH256, X_OFFSET2);

    chkECDH384 = gtk_check_button_new_with_label("ECDH-384");
    gtk_signal_connect (GTK_OBJECT(chkECDH384), "clicked", GTK_SIGNAL_FUNC(checked), (gpointer)CHK_CRY_EC384);
    gtk_widget_show(chkECDH384);
	put_to_offset(main_vbox, chkECDH384, X_OFFSET2);

    chkECDH521 = gtk_check_button_new_with_label("ECDH-521");
    gtk_signal_connect (GTK_OBJECT(chkECDH521), "clicked", GTK_SIGNAL_FUNC(checked), (gpointer)CHK_CRY_EC521);
    gtk_widget_show(chkECDH521);
	put_to_offset(main_vbox, chkECDH521, X_OFFSET2);

    if ( params.is_ec )
	{
		gtk_widget_set_sensitive(chkECDH256, 0);
		gtk_widget_set_sensitive(chkECDH384, 0);
		gtk_widget_set_sensitive(chkECDH521, 0);
	}
	

    chkMulti = gtk_check_button_new_with_label ("Multistream");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkMulti), 1);
    gtk_widget_set_sensitive(GTK_WIDGET(chkMulti), FALSE);
    gtk_widget_show (chkMulti);
	put_to_offset(main_vbox, chkMulti, X_OFFSET2);

    chkPreshared = gtk_check_button_new_with_label ("Preshared connections");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkPreshared), 1);
    gtk_widget_show (chkPreshared);
	put_to_offset(main_vbox, chkPreshared, X_OFFSET2);

    separator = gtk_hseparator_new();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (main_vbox), separator);

// ---------------------------------------------------------------------------------
// atl controls
    label = gtk_label_new ("SRTP auth tag");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), label);
    gtk_widget_show(label);

    hbox = gtk_hbox_new(FALSE, 1);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);
    gtk_container_add(GTK_CONTAINER (main_vbox), hbox);
    gtk_widget_show(hbox);

    chkATL32 = gtk_check_button_new_with_label ("HMAC-32");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkATL32), 1);
    gtk_signal_connect (GTK_OBJECT (chkATL32), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_CRY_ATL32);
    gtk_widget_set_sensitive(GTK_WIDGET(chkATL32), FALSE);
    gtk_widget_show (chkATL32);
	put_to_offset(main_vbox, chkATL32, X_OFFSET2);

    chkATL80 = gtk_check_button_new_with_label ("HMAC-80");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkATL80), 0);
    gtk_signal_connect (GTK_OBJECT (chkATL80), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_CRY_ATL80);
    gtk_widget_show (chkATL80);
	put_to_offset(main_vbox, chkATL80, X_OFFSET2);

    separator = gtk_hseparator_new();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (main_vbox), separator);


// ---------------------------------------------------------------------------------
// sas controls
    label = gtk_label_new ("SAS types");    
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), label);
    gtk_widget_show(label);

    chkBase32 = gtk_check_button_new_with_label ("Base-32");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkBase32), 1);
    gtk_signal_connect (GTK_OBJECT (chkBase32), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_CRY_BASE32);
    gtk_widget_set_sensitive(GTK_WIDGET(chkBase32), FALSE);
    gtk_widget_show (chkBase32);
	put_to_offset(main_vbox, chkBase32, X_OFFSET2);

    chkBase256 = gtk_check_button_new_with_label ("Base-256");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkBase256), 0);
    gtk_signal_connect (GTK_OBJECT (chkBase256), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_CRY_BASE256);
    gtk_widget_show (chkBase256);
	put_to_offset(main_vbox, chkBase256, X_OFFSET2);

    separator = gtk_hseparator_new();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (main_vbox), separator);

// ---------------------------------------------------------------------------------
// hash controls
    label = gtk_label_new ("Hash algorithms");    
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), label);
    gtk_widget_show(label);

    chkSHA256 = gtk_check_button_new_with_label ("SHA-256");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkSHA256), 1);
    gtk_signal_connect (GTK_OBJECT (chkSHA256), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_CRY_SHA256);
    gtk_widget_set_sensitive(GTK_WIDGET(chkSHA256), FALSE);
    gtk_widget_show (chkSHA256);
    put_to_offset(main_vbox, chkSHA256, X_OFFSET2);

    gtk_widget_ref(cryptoPane);
}


void create_test_pane()
{
    GtkWidget *align, *label, *main_vbox;
    // frames and boxes creation
    testPane = gtk_frame_new (NULL);
    gtk_container_set_border_width(GTK_CONTAINER (testPane), 3);
    gtk_widget_show (testPane);
    gtk_widget_set_size_request (GTK_WIDGET(testPane), 400, 150);

    align = gtk_alignment_new(0, 0, 1, 1);
    gtk_container_add(GTK_CONTAINER(testPane), align);
    gtk_widget_show(align);
    main_vbox = gtk_vbox_new (FALSE, 1);
    gtk_box_set_homogeneous(GTK_BOX(main_vbox), FALSE);
    gtk_container_add (GTK_CONTAINER (align), main_vbox);
    gtk_widget_show (main_vbox);

    label = gtk_label_new ("Test options");    
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), label);
    gtk_widget_show(label);

    align = gtk_alignment_new(0.67, 0, 0, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), align);
    GtkWidget *vbox = gtk_vbox_new(TRUE, 0);
    gtk_container_add (GTK_CONTAINER (align), vbox);
    gtk_widget_show(align);    
    gtk_widget_show(vbox);    

    chkDisclose = gtk_check_button_new_with_label ("Set evil bit");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkDisclose), 1);
    gtk_signal_connect (GTK_OBJECT (chkDisclose), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_TEST_DISCLOSE);
    gtk_widget_show (chkDisclose);
    gtk_container_add (GTK_CONTAINER (vbox), chkDisclose);

/*
    scroll = gtk_scrolled_window_new(0, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), list_ip);
    gtk_widget_set_size_request(scroll, 390, 120);
    gtk_container_add (GTK_CONTAINER (vbox), scroll);
    gtk_widget_show(scroll);
*/
    gtk_widget_ref(testPane);
}

//------------------------------------------------------------------------------
void create_sniff_pane()
{
    GtkWidget *align, *main_vbox, *hbox, *add_button, *scroll, *rem_button, *label, *separator, *vbox, *testHbox;
    GList *combo_items = NULL;

    char *list_titles[] = 
    {
  		"port",
		"proto",
		"description"
    };

    // frames and boxes creation
    sniffPane = gtk_frame_new (NULL);
    gtk_container_set_border_width(GTK_CONTAINER (sniffPane), 3);
    gtk_widget_show (sniffPane);
//    gtk_box_set_homogeneous(GTK_BOX(sniffPane), FALSE);
    gtk_widget_set_size_request (GTK_WIDGET(sniffPane), 400, 420);

//    align = gtk_alignment_new(0, 0, 1, 1);
//    gtk_container_add(GTK_CONTAINER(sniffPane), align);
//    gtk_widget_show(align);
    main_vbox = gtk_vbox_new(FALSE, 1);
    gtk_box_set_homogeneous(GTK_BOX(main_vbox), FALSE);
    gtk_container_add (GTK_CONTAINER (sniffPane), main_vbox);
    gtk_widget_show (main_vbox);

    label = gtk_label_new ("SIP detection");    
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_container_add (GTK_CONTAINER(main_vbox), label);
    gtk_widget_show(label);

    sniff_list = gtk_clist_new_with_titles(3, list_titles);
    gtk_widget_set_size_request(sniff_list, 400, 120);
//    gtk_container_add (GTK_CONTAINER (main_vbox), sniff_list);
    gtk_widget_show(sniff_list);

    scroll = gtk_scrolled_window_new(0, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll, 400, 120);
    gtk_container_add (GTK_CONTAINER (main_vbox), scroll);
    gtk_widget_show(scroll);
    gtk_container_add (GTK_CONTAINER (scroll), sniff_list);

    gtk_clist_set_column_width(GTK_CLIST(sniff_list), 0, 40);
    gtk_clist_set_column_width(GTK_CLIST(sniff_list), 1, 40);

    hbox = gtk_hbox_new(FALSE, 1);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);
    gtk_container_add(GTK_CONTAINER (main_vbox), hbox);
    gtk_widget_show(hbox);
    
    align = gtk_alignment_new(0, 0, 0, 0);
    gtk_widget_set_size_request(align, 40, 20);
    gtk_container_add(GTK_CONTAINER(hbox), align);
    gtk_widget_show(align);
    entryPort = gtk_entry_new();    
    gtk_widget_set_size_request(entryPort, 80, 20);
    gtk_container_add(GTK_CONTAINER(align), entryPort);
    gtk_widget_show(entryPort);

    align = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(hbox), align);
    gtk_widget_show(align);
    gtk_widget_set_size_request(align, 60, 20);
//    combo_items = g_list_append(combo_items, "ANY");
    combo_items = g_list_append(combo_items, "UDP");
    combo_items = g_list_append(combo_items, "TCP");
    comboProto = gtk_combo_new();
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(comboProto)->entry), "ANY");
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(comboProto)->entry), 0);
    gtk_combo_set_popdown_strings( GTK_COMBO(comboProto), combo_items);
    gtk_widget_set_size_request(comboProto, 60, 20);
    gtk_container_add(GTK_CONTAINER(align), comboProto);
    gtk_widget_show(comboProto);
    
    align = gtk_alignment_new(0, 0, 1, 0);
    gtk_container_add(GTK_CONTAINER(hbox), align);
    gtk_widget_show(align);
//    gtk_widget_set_size_request(align, 235, 20);
    entryDescr = gtk_entry_new();    
    gtk_widget_set_size_request(entryDescr, 235, 20);
    gtk_container_add(GTK_CONTAINER (align), entryDescr);
    gtk_widget_show(entryDescr);

    align = gtk_alignment_new(0, 0, 0, 0);
    gtk_widget_show(align);
//    gtk_widget_set_size_request(align, 20, 20);
    gtk_container_add(GTK_CONTAINER(hbox), align);
    add_button = gtk_button_new_with_label("+");
    gtk_signal_connect (GTK_OBJECT(add_button), "clicked", GTK_SIGNAL_FUNC(add_new_record), NULL);
    gtk_widget_set_size_request(add_button, 20, 20);
    gtk_container_add(GTK_CONTAINER(align), add_button);
    gtk_widget_show(add_button);

    align = gtk_alignment_new(0, 0, 0, 0);
    gtk_widget_show(align);
//    gtk_widget_set_size_request(align, 20, 20);
    gtk_container_add(GTK_CONTAINER(hbox), align);
    rem_button = gtk_button_new_with_label("-");
    gtk_signal_connect (GTK_OBJECT(rem_button), "clicked", GTK_SIGNAL_FUNC(remove_record), NULL);
    gtk_widget_set_size_request(rem_button, 20, 20);
    gtk_container_add(GTK_CONTAINER(align), rem_button);
    gtk_widget_show(rem_button);

    vbox = gtk_vbox_new(FALSE, 1);
    gtk_widget_show (vbox);
	put_to_offset(main_vbox, vbox, X_OFFSET);
	
    chkListenSpec = gtk_check_button_new_with_label ("listen to SIP on specified ports");
    gtk_signal_connect (GTK_OBJECT (chkListenSpec), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_SNF_SPEC);
    gtk_widget_show (chkListenSpec);
    gtk_container_add (GTK_CONTAINER (vbox), chkListenSpec);

//    align = gtk_alignment_new(0.22, 0, 0, 0);
//    gtk_container_add (GTK_CONTAINER (main_vbox), align);
//    gtk_widget_show(align);
    chkListenUDP = gtk_check_button_new_with_label ("sniff all UDP packets");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkListenUDP), 1);
    gtk_signal_connect (GTK_OBJECT (chkListenUDP), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_SNF_UDP);
    gtk_widget_show (chkListenUDP);
    gtk_container_add (GTK_CONTAINER (vbox), chkListenUDP);

//    align = gtk_alignment_new(0.195, 0, 0, 0);
//    gtk_container_add (GTK_CONTAINER (main_vbox), align);
//    gtk_widget_show(align);
    chkListenTCP = gtk_check_button_new_with_label ("sniff TCP packets");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkListenTCP), 1);
    gtk_signal_connect (GTK_OBJECT (chkListenTCP), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_SNF_TCP);
    gtk_widget_show (chkListenTCP);
    gtk_container_add (GTK_CONTAINER (vbox), chkListenTCP);

    separator = gtk_hseparator_new();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (main_vbox), separator);

    label = gtk_label_new ("RTP detection");    
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_container_add (GTK_CONTAINER(main_vbox), label);
    gtk_widget_show(label);

    align = gtk_alignment_new(0.19, 0, 0, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), align);
    gtk_widget_show(align);

/*    chkGizmo = gtk_check_button_new_with_label ("Proxy mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkGizmo), 1);
    gtk_signal_connect (GTK_OBJECT (chkGizmo), "clicked", GTK_SIGNAL_FUNC (checked), (gpointer) CHK_TEST_GIZMO);
    gtk_widget_show (chkGizmo);
    gtk_container_add (GTK_CONTAINER (align), chkGizmo);*/

    vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add (GTK_CONTAINER (align), vbox);
    gtk_widget_show (vbox);
    
    scroll = gtk_scrolled_window_new(0, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll, 380, 120);
    gtk_container_add (GTK_CONTAINER (main_vbox), scroll);
    gtk_widget_show(scroll);

    list_ip = gtk_clist_new_with_titles(2, list_titles);
    gtk_widget_set_size_request(list_ip, 380, 120);
    gtk_container_add (GTK_CONTAINER (scroll), list_ip);
    gtk_widget_show(list_ip);
    gtk_clist_set_column_width(GTK_CLIST(list_ip), 0, 200);
    gtk_signal_connect(GTK_OBJECT(list_ip),
                   "select_row",
                   GTK_SIGNAL_FUNC(select_row_ip),
                   NULL);

    testHbox = gtk_hbox_new(FALSE, 1);
    gtk_container_add (GTK_CONTAINER (main_vbox), testHbox);
    gtk_widget_show(testHbox);    

    align = gtk_alignment_new(0, 0, 0, 0);
    gtk_widget_show(align);
    gtk_container_add(GTK_CONTAINER(testHbox), align);
    entryIP = gtk_entry_new();    
    gtk_widget_set_size_request(entryIP, 335, 20);
    gtk_container_add(GTK_CONTAINER(align), entryIP);
    gtk_widget_show(entryIP);

    align = gtk_alignment_new(0, 0, 0, 0);
    gtk_widget_show(align);
    gtk_container_add(GTK_CONTAINER(testHbox), align);
    add_button = gtk_button_new_with_label("+");
    gtk_signal_connect (GTK_OBJECT(add_button), "clicked", GTK_SIGNAL_FUNC(ip_event), (void*)TEST_IP_ADD);
    gtk_widget_set_size_request(add_button, 20, 20);
    gtk_container_add(GTK_CONTAINER(align), add_button);
    gtk_widget_show(add_button);

    align = gtk_alignment_new(0, 0, 0, 0);
    gtk_widget_show(align);
    gtk_container_add(GTK_CONTAINER(testHbox), align);
    rem_button = gtk_button_new_with_label("-");
    gtk_signal_connect (GTK_OBJECT(rem_button), "clicked", GTK_SIGNAL_FUNC(ip_event), (void*)TEST_IP_DEL);
    gtk_widget_set_size_request(rem_button, 20, 20);
    gtk_container_add(GTK_CONTAINER(align), rem_button);
    gtk_widget_show(rem_button);

    align = gtk_alignment_new(0.37, 0, 0, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), align);
    gtk_widget_show(align);
    vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add (GTK_CONTAINER (align), vbox);
    gtk_widget_show (vbox);

    gtk_widget_ref(sniffPane);
}

static gboolean key_press_callback( GtkWidget      *widget, 
                                       GdkEventKey    *event,
                                       gpointer       data)
{
    if (!strcmp(gdk_keyval_name(event->keyval), "Delete") && sniff_count)
    {
	remove_record();
	return TRUE;
    }
    return FALSE;
}				       

static void select_row_callback(GtkWidget *widget,
                             gint row,
                             gint column,
                             GdkEventButton *event,
                             gpointer data)
{
    sniff_selected = row;
}

void pref_form_no_active(int value)
{
    if (!window_) return;
}

static gboolean form_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_Escape)
	{
		pref_close();
		return TRUE;
	}
	
	return FALSE;
}


//------------------------------------------------------------------------------
// creates preference form
int create_pref_form(int no_calls)
{
    GtkWidget *vbox, *toolbar, *icon, *align;
    GtkWidget *gen_button, *cry_button, *ok_button, *test_button, *sniff_button;

    // if window is already created we'll just return
    if (window_)
		return 0;

    sniff_count = 0;
    ip_modified = 0;
    ip_count = 0;

    // window creation
    window_ = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
    gtk_window_set_policy(GTK_WINDOW(window_), FALSE, FALSE, FALSE);
    gtk_window_set_resizable(GTK_WINDOW(window_), FALSE);
    gtk_signal_connect (GTK_OBJECT (window_), "destroy", GTK_SIGNAL_FUNC (on_destroy), NULL);
    gtk_container_border_width (GTK_CONTAINER (window_), 0);
    gtk_window_set_title(GTK_WINDOW(window_), "Preferences");
    gtk_window_set_icon(GTK_WINDOW(window_), get_app_icon());

    // window vbox
    vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add(GTK_CONTAINER(window_), vbox);
    gtk_widget_show(vbox);
    

    // create toolbar, which will be added to window vbox
    toolbar = gtk_toolbar_new();
    gtk_container_set_border_width (GTK_CONTAINER (toolbar), 0);
    gtk_container_add (GTK_CONTAINER (vbox), toolbar);
    gtk_widget_show (toolbar);

    // pane_vbox will hold panes
    pane_vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add(GTK_CONTAINER(vbox), pane_vbox);
    gtk_widget_show(pane_vbox);

    icon = gtk_image_new_from_file(GEN_PANE_PIC);
    gen_button = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_RADIOBUTTON,
	NULL,
	"General",
	"general options", "private",
	icon, GTK_SIGNAL_FUNC(change_pane), (gpointer)PANE_GENERAL);
    // we dont need focused toolbar buttons
    GTK_WIDGET_UNSET_FLAGS(gen_button, GTK_CAN_FOCUS);
    
    icon = gtk_image_new_from_file(CRY_PANE_PIC);
    cry_button = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_RADIOBUTTON,
	gen_button,
	"Crypto",
	"crypto options", "private",
	icon, GTK_SIGNAL_FUNC(change_pane), (gpointer)PANE_CRYPTO);
    GTK_WIDGET_UNSET_FLAGS(cry_button, GTK_CAN_FOCUS);

    icon = gtk_image_new_from_file(SNF_PANE_PIC);
    sniff_button = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_RADIOBUTTON,
	cry_button,
	"Sniff",
	"test options", "private",
	icon, GTK_SIGNAL_FUNC(change_pane), (gpointer)PANE_SNIFF);
    GTK_WIDGET_UNSET_FLAGS(sniff_button, GTK_CAN_FOCUS);

    if ( params.is_debug )
    {
		icon = gtk_image_new_from_file(CRY_PANE_PIC);
        test_button = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_RADIOBUTTON,
											    sniff_button,
											    "Testing",
											    "test options", "private",
										  	    icon, GTK_SIGNAL_FUNC(change_pane), (gpointer)PANE_TEST);
		GTK_WIDGET_UNSET_FLAGS(test_button, GTK_CAN_FOCUS);
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gen_button), TRUE);

    create_general_pane();
    create_crypto_pane();
    create_sniff_pane();
    testPane = NULL;
    if ( params.is_debug ) 
    {
		create_test_pane();
    }
    // make references to panes as they may be destroyed becaused they wont be attached to window
    // all the time
    currentPane = generalPane;
    gtk_container_add(GTK_CONTAINER (pane_vbox), currentPane);

    // Common controls creation
    // OK
    ok_button = gtk_button_new_with_label ("Save");
    gtk_signal_connect (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (pref_save), NULL);
    // CANCEL
    GtkWidget *cancel_button = gtk_button_new_with_label ("Cancel");
    gtk_signal_connect(GTK_OBJECT (cancel_button), "clicked", GTK_SIGNAL_FUNC (pref_close), NULL);
	g_signal_connect(window_, "key-press-event", (GCallback)form_key_pressed, NULL);
    // DEFAULT
    GtkWidget *default_button = gtk_button_new_with_label ("Default");
    gtk_signal_connect (GTK_OBJECT (default_button), "clicked", GTK_SIGNAL_FUNC (pref_default), NULL);
    // buttons hbox
    GtkWidget *button_hbox = gtk_hbox_new(TRUE, 5);
    gtk_container_border_width(GTK_CONTAINER (button_hbox), 3);
    gtk_widget_set_size_request(ok_button, 80, 25);
    gtk_widget_set_size_request(cancel_button, 80, 25);
    gtk_widget_set_size_request(default_button, 80, 25);
    align = gtk_alignment_new(1.0, 0.5, 0, 0);

    gtk_container_add(GTK_CONTAINER (button_hbox), default_button);
    gtk_container_add(GTK_CONTAINER (button_hbox), ok_button);
    gtk_container_add(GTK_CONTAINER (button_hbox), cancel_button);
    gtk_container_add(GTK_CONTAINER (align), button_hbox);
    gtk_container_add(GTK_CONTAINER (vbox), align);
    gtk_widget_show(default_button);
    gtk_widget_show(ok_button);
    gtk_widget_show(cancel_button);
    gtk_widget_show(align);
    gtk_widget_show(button_hbox);

    // status bar creation
    status_bar = gtk_statusbar_new();      
    gtk_box_pack_start (GTK_BOX (vbox), status_bar, TRUE, TRUE, 0);
    gtk_widget_show (status_bar);

    context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar), "preferences statusbar");

    g_signal_connect (G_OBJECT (sniff_list), "key_press_event",
                  G_CALLBACK (key_press_callback), NULL);

    gtk_signal_connect(GTK_OBJECT(sniff_list),
                   "select_row",
                   GTK_SIGNAL_FUNC(select_row_callback),
                   NULL);

    setValues();
	pref_form_no_active(no_calls);

    gtk_widget_show(window_);
    clearStatusBar();

    return 0;
}

