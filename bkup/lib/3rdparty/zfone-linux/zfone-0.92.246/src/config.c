/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok mailto: <chaser@soft-industry.com>
 */

#include <stdio.h>
#include <string.h>

#include <zrtp_types.h>

#include "zfoneg_config.h"

//------------------------------------------------------------------------------
int addParam(zrtp_crypto_comp_t type, int aValue)
{
	int i;
	uint8_t *cp;

	// if unknown param type
	if (type > ZRTP_COMP_COUNT)
	{
		return -1;
	}
    
	switch (type)
	{
    	    case ZRTP_CC_HASH:
		cp = params.zrtp.hash_schemes; break;
	    case ZRTP_CC_SAS:
		cp = params.zrtp.sas_schemes; break;
    	    case ZRTP_CC_CIPHER:
		cp = params.zrtp.cipher_types; break;
            case ZRTP_CC_PKT:
		cp = params.zrtp.pk_schemes; break;
	    case ZRTP_CC_ATL:
		cp = params.zrtp.auth_tag_lens; break;
	}

	// check for duplicate
	for (i = 0; i < ZRTP_MAX_COMP_COUNT; i++)
	{
	    if ( cp[i] == aValue )
	    {
		return 0;
	    }
	    if ( !cp[i] )
	    {
		break;
	    }
	}
	
	if (i == ZRTP_MAX_COMP_COUNT)
	    return 0;
	    
	cp[i] = aValue;
	return 1;
}

/*
//------------------------------------------------------------------------------
int insertParam(zrtp_crypto_comp_t type, char* aValue, int aPriority)
{
	// check valid type and priority
	if (type >= ZRTP_COMP_COUNT || aPriority >= ZRTP_MAX_COMP_COUNT || aPriority < 0)
		return -1;

	// get profile element
	zrtp_profile_elem_t *cp = params_elems[type-1];
	int	index, i;

	// find duplicate
	for (index = 0; index < cp->count; index++)
	{
		if ( !memcmp(cp->order[index], aValue, ZRTP_COMP_TYPE_SIZE) )
		{
			break;
		}
	}
	
	// it is already at its place
	if (index == aPriority)
		return 1;
		
	// adding new value to the end
	if (aPriority >= cp->count)
	{
		memcpy(cp->order[cp->count++], aValue, ZRTP_COMP_TYPE_SIZE);
		return 1;
	}
	
	if (aPriority < index)
	{
		// there were no such element
		if (index == cp->count)
		{
			if (cp->count >= ZRTP_MAX_COMP_COUNT)
				return -1;
			// the last index
			i = cp->count - 1;
			cp->count++;
		}
		else
		{
			i = index - 1;
		}
		// move items with indexes after priority to the right
		for (; aPriority <= i; i--)
		{
			memcpy(cp->order[i+1], cp->order[i], ZRTP_COMP_TYPE_SIZE);
		}
	}
	else 
	{
		// move items with indexes before priority to the left
		for (i = index; i < aPriority; i++)
		{
			memcpy(cp->order[i], cp->order[i+1], ZRTP_COMP_TYPE_SIZE);
		}
	}
	
	// copy value to its place
	memcpy(cp->order[aPriority], aValue, ZRTP_COMP_TYPE_SIZE);
	return 1;
}
*/
//------------------------------------------------------------------------------
void clearParams()
{
	int is_debug = params.is_debug;
	int is_ec = params.is_ec;
	int license = params.license_mode;
	memset(&params, 0, sizeof(params));
	params.is_debug = is_debug;
	params.is_ec = is_ec;
	 params.license_mode = license;
}
