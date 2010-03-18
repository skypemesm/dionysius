/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok mailto: <chaser@soft-industry.com>
 */

#ifndef __ZFONEG_CHECK_FORM_H__
#define __ZFONEG_CHECK_FORM_H__

// create update form
int create_check_form(char *current_version, char *new_version);
// update version info
void update_check_info(zrtp_cmd_t *cmd, GtkWidget *mainWindow);


#endif //__ZFONEG_CHECK_FORM_H__
