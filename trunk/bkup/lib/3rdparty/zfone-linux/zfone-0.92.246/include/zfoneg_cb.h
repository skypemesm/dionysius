/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Max Yegorov mailto: egm@soft.cn.ua, m.yegorov@gmail.com
 */

#ifndef __ZFONEG_CB_H__
#define __ZFONEG_CB_H__

#include <gtk/gtk.h>
#include "zfoneg_commands.h"

/*!
    \defgroup ControlClient Control Client
    \{
*/

extern void ipf_ok(void);
extern void ipf_failed(void);

/* Callbacks to pass to state machine.
 */
extern void register_callback(unsigned int callback_type, voip_ctrl_callback* cb);

// commands callbacks
extern void gui_cb_is_in_state_gothello(zrtp_cmd_t *c);
extern void gui_cb_is_in_state_gothelloack(zrtp_cmd_t *c);
extern void gui_cb_is_in_state_clear(zrtp_cmd_t *c);
extern void gui_cb_is_in_state_initiatingsecure(zrtp_cmd_t *c);
extern void gui_cb_is_in_state_pendingsecure(zrtp_cmd_t *c);
extern void gui_cb_is_in_state_pendingclear(zrtp_cmd_t *c);
extern void gui_cb_is_in_state_initiatingclear(zrtp_cmd_t *c);
extern void gui_cb_is_in_state_secure(zrtp_cmd_t *c);
extern void cb_create(zrtp_cmd_t *cmd); 			// New call is added
extern void cb_destroy(zrtp_cmd_t *cmd);			// Hung up
extern void cb_set_verified(zrtp_cmd_t *cmd);
extern void gui_cb_send_version(zrtp_cmd_t *cmd);

extern void cb_started(zrtp_cmd_t *cmd);
extern void cb_stopped(zrtp_cmd_t *cmd);
extern void cb_crashed(zrtp_cmd_t *cmd);
extern void cb_packet(zrtp_cmd_t *cmd);
extern void cb_error(zrtp_cmd_t *cmd);
extern void cb_check_sas(zrtp_cmd_t *cmd);
extern void cb_set_preferences(zrtp_cmd_t *cmd);
extern void cb_no_config(zrtp_cmd_t *cmd);
extern void cb_looking_4_zrtp(zrtp_cmd_t *cmd);
extern void cb_no_zrtp(zrtp_cmd_t *cmd);
extern void cb_wrong_config(zrtp_cmd_t *cmd);
extern void cb_set_defaults(zrtp_cmd_t *cmd);
extern void cb_set_ips(zrtp_cmd_t *cmd);
extern void cb_update_list(zrtp_cmd_t *cmd);
extern void cb_set_cache_info(zrtp_cmd_t *cmd);

char	version[512];


/* Staff for manipulating state visuals */
enum
{
    IMG_IsNotSecure = 0,
    IMG_PressClear,
    IMG_Securing,
    IMG_Waiting,
    IMG_Idle,
    IMG_NoIPFilter,
    IMG_IsSecure,
//    IMG_IsSecure256,
    IMG_NoZrtp,
    IMG_LookingZrtp,
    IMG_Error
//    IMG_IsSecure128_Disclose,
//    IMG_IsSecure256_Disclose
};
/* Array for mapping zFone state to GUI picture +2 states for AES128 and AES256*/
extern int state2pic[ZRTP_NR_CMDS+4];
//#define SECURE_AES128		ZRTP_NR_CMDS
//#define SECURE_AES256		ZRTP_NR_CMDS+1
//#define SECURE_AES128_DISCLOSE	ZRTP_NR_CMDS+2
//#define SECURE_AES256_DISCLOSE	ZRTP_NR_CMDS+3

//extern void show_state(int state);
extern unsigned short   port;

void switch_verified_off(void);
//! ignore verifying checkbox events
void ignore_verify(int on);
//! set tooltip for widget
void set_tooltip(GtkWidget *widget, char *message);
//void set_name(const char* name);
//! we need one icon for several windows, function checks if icon was loaded and returns it
GdkPixbuf* get_app_icon();

//! Force displaying of image. E.g. no calls but we need to display 'Idle'
extern void update_main_view(int make_top);
extern void set_indicator(char *picture, zfone_media_type_t stream_type);
extern void set_both_indicators(char *picture);
//! show alert informing about crash
extern void show_crash_dialog(void);
//! show warning alert
extern void show_warning(const char* message);
extern void show_error(const char* message);
//! enable pref and connection settings menus
extern void enable_menus();
//! disable pref and connection settings menus
extern void disable_menus();
//! clear GUI
extern void clear_main_view();

extern void rtp_ligts(int direction, int type);

extern void enable_prefs();
#endif //__ZFONEG_CB_H__

