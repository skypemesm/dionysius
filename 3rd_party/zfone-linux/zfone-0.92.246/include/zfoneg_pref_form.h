/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok mailto: <chaser@soft-industry.com>
 */

#ifndef __ZFONEG_PREF_FORM_H__
#define __ZFONEG_PREF_FORM_H__

#include "zfoneg_config.h"

int  create_pref_form(int no_calls);
void set_prefs(zfone_params_t *p);
void pref_form_no_active(int value);

#endif //__ZFONEG_PREF_FORM_H__
