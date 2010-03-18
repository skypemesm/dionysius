/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#ifndef __ZCMD_CONV_H__
#define __ZCMD_CONV_H__

#include "zfoneg_commands.h"

/*!
    \defgroup Convertor Zfone commands data convertor
    \ingroup Commands
    \{
*/


/*!
    \brief Convert all needed cmd params to the network byte-order
    \param cmd - command for conversion
*/
void zfone_cmd_hton(zrtp_cmd_t* cmd);

/*!
    \brief Convert all needed cmd params to the host byte-order
    \param cmd - command for conversion
*/
void zfone_cmd_ntoh(zrtp_cmd_t* cmd);


/*! \} */

#endif //__ZCMD_CONV_H__
