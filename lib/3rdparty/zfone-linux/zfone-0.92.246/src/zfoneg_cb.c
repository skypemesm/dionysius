/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <string.h>

#include <zrtp_types.h>

#include "zfoneg_tcpconn.h"

#include "zfoneg_commands.h"
#include "zfoneg_cb.h"
#include "zfoneg_listbox.h"
#include "zfoneg_config.h"
#include "files.h"
#include "zfoneg_pref_form.h"
#include "zfoneg_cache_form.h"

extern list_box_t		_lb;
extern zfone_params_t 		params;
extern void set_tooltip(GtkWidget *widget, char *message);

extern tcp_conn_t	tconn;
zrtp_cache_info_t global_cache_cmd;


//------------------------------------------------------------------------------
void cb_started(zrtp_cmd_t *cmd)
{
    //update_main_view_with_state(STARTED);
    set_both_indicators(IDLE_PATH_PIC);
    enable_menus();
}

//------------------------------------------------------------------------------
void cb_stopped(zrtp_cmd_t *cmd)
{
	ipf_failed();
}

//------------------------------------------------------------------------------
void cb_crashed(zrtp_cmd_t *cmd)
{
	ipf_failed();
    show_crash_dialog();
}

//------------------------------------------------------------------------------
void cb_packet(zrtp_cmd_t *cmd)
{
    zrtp_cmd_zrtp_packet_t *cmd_packet = (zrtp_cmd_zrtp_packet_t *)(&cmd->data);
    list_box_item_t* item = listbox_find_item_with_session(&_lb, cmd->session_id);
    int stream_id = cmd->stream_id - 1;

    if ( !item )
    {
		printf("!!!!! ERROR !!!!! zrtp_packet: session is not found\n");
		return;
    }    

    if (stream_id < 0 || stream_id >= ZFONEG_STREAMS_COUNT)
    {
		printf("!!!!! ERROR !!!!! zrtp_packet: stream_id is buggy %u\n", cmd->stream_id);
		return;
    }

    if (cmd_packet->direction > 1)
    {
		printf("!!!!! ERROR !!!!! zrtp_packet: direction is buggy %u\n", cmd->stream_id);
		return;
    }

    set_rtp_activity(item, cmd->stream_id, cmd_packet->direction, cmd_packet->packet_type);
    update_main_view(0);
}

//------------------------------------------------------------------------------
void cb_error(zrtp_cmd_t *cmd)
{
    zrtp_cmd_error_t* error_cmd = (zrtp_cmd_error_t*)(&cmd->data);
    char message[512];

    switch ( error_cmd->error_code )
    {
	case zrtp_error_timeout:
	{
		strcpy(message, "Protocol timeout expired.");
		break;
	}
	case zrtp_error_invalid_packet:
	{
		strcpy(message, "Malformed ZRTP protocol packet.");
		break;
	}
	case zrtp_error_software:
	{
		strcpy(message, "Critical software error in ZFone.");
		break;
	}
	case zrtp_error_version:
	{
		strcpy(message, "ZRTP peer has incompatible version of ZRTP protocol.");
		break;
	}
	case zrtp_error_hello_mistmatch:
	{
		strcpy(message, "Cannot agree on a common set of crypto algorithms with the remote peer.");
		break;
	}
	case zrtp_error_hash_unsp:
	{
		strcpy(message, "Unsupported Hash algorithm requested by other party.");
		break;
	}
	case zrtp_error_cipher_unsp:
	{
		strcpy(message, "Unsupported block cipher algorithm requested by other party.");
		break;
	}
	case zrtp_error_pktype_unsp:
	{
		strcpy(message, "Unsupported public key exchange scheme requested by other party.");
		break;
	}
	case zrtp_error_auth_unsp:
	{
		strcpy(message, "Unsupported SRTP auth tag scheme requested by other party.");
		break;
	}
	case zrtp_error_sas_unsp:
	{
		strcpy(message, "Unsupported SAS rendering scheme requested by other party.");
		break;
	}
	case zrtp_error_possible_mitm1:
	{
		strcpy(message, "Illegal value for DH public parameter.");
		break;
	}
	case zrtp_error_possible_mitm2:
	{
		strcpy(message, "Hash commitment failed to match.");
		break;
	}
	case zrtp_error_possible_mitm3:
	{
		strcpy(message, "Possible MiTM attack - Untrusted MiTM attempted to relay SAS.");
		break;
	}
	case zrtp_error_auth_decrypt:
	{
		strcpy(message, "Authentification error: bad Confirm packet HMAC");
		break;
	}
	case zrtp_error_nonse_reuse:
	{
		strcpy(message, "Nonce reuse by the remote peer.");
		break;
	}
	case zrtp_error_equal_zid:
	{
		strcpy(message, "Error: Both parties are using the same ZID cache index.");
		break;
	}
	case zrtp_error_goclear_unsp:
	{
		strcpy(message, "'Stay secure' option is enabled but GoClear packet received");
		break;
	}
	case zrtp_error_wrong_meshash:
	{
		strcpy(message, "Possible denial of service attack, wrong message hash pre-image.");
		break;
	}
	case ZFONE_ERROR_WRONG_CONFIG:
	{
		strcpy(message, "User preferences are incorrect. Default preferences will be used. You can change them by using the preference panel.");
		break;
	}
	case ZFONE_ERROR_NO_CONFIG:
	{
		strcpy(message, "Config file was not found. Default preferences will be used. You can change them by using the preference panel.");
		break;
	}
	case ZFONE_ZRTP_INIT_ERROR:
	{
		strcpy(message, "Initialization failed.");
		break;
	}
	case ZFONE_NO_ZID:
	{
		strcpy(message, "ZID was not found.");
		break;
	}
	case ZFONE_ERROR_SAS:
	{
		strcpy(message, "We expected the other party to have a shared "
			"secret cached from a previous call, but they "
			"don't have it. This may mean your partner simply "
			"lost his cache of shared secrets, but it could also "
			"mean someone is trying to wiretap you. To resolve "
			"this question you must check the authentication "
			"string with your partner. If it doesn't match, "
			"it indicates the presence of a wiretapper.");
		break;
	}
	default:
	{
		sprintf(message, "Unsupported type of error was received: %d", error_cmd->error_code);
		break;
	}
    }

    show_error(message);
}

//------------------------------------------------------------------------------
void cb_set_preferences(zrtp_cmd_t *cmd)
{
    zfone_params_t *p = (zfone_params_t *) cmd->data; 
    memcpy(&params, p, sizeof(zfone_params_t));
    enable_prefs();
}

void cb_set_defaults(zrtp_cmd_t *cmd)
{
    zfone_params_t *p = (zfone_params_t *) cmd->data; 
    set_prefs(p);
}

void cb_set_ips(zrtp_cmd_t *cmd)
{
    int i;
    zrtp_cmd_set_ips_t *c = (zrtp_cmd_set_ips_t *) cmd->data;
    memcpy(&ips_holder, c, sizeof(zrtp_cmd_set_ips_t));
    update_main_view(1);
    for (i = 0; i < ips_holder.ip_count; i++)  
    {
		if (ips_holder.flags[i] == IP_FLAG_AUTO_VALUE)
		    break;
    }
    if (ips_holder.ip_count && i >= ips_holder.ip_count)    
    {
		show_warning("No automatically detected interfaces. Zfone may miss calls");
    }
}

void cb_update_list(zrtp_cmd_t *cmd)
{
    zrtp_cmd_update_streams_t *update_cmd = (zrtp_cmd_update_streams_t *) cmd->data;
    renew_listbox_items(&_lb, update_cmd->list, update_cmd->count);
    update_main_view(1);
}

void cb_set_cache_info(zrtp_cmd_t *cmd)
{
	memcpy(&global_cache_cmd, (zrtp_cache_info_t *) cmd->data, sizeof(zrtp_cache_info_t));
    create_cache_form();
}
