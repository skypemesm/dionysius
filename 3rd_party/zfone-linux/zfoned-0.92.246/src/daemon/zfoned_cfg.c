/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 * 
 * Nikolay Popok <chaser@soft-industry.com>
 * Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>
#include <stdarg.h>

#include <zrtp.h>
#include "zfone.h"

#define _ZTU_ "zfoned cfg"

typedef int bool;


#define	false	0
#define true	1


#define CPM_EOF			0x1A	/*!< ^Z = CPM EOF char */
#define MAX_ERRORS		3		/*!< Max.no.errors before we give up */
#define LINEBUF_SIZE	256		/*!< Size of input buffer */

#define TMP_FILE	"/tmp/zfone.cfg.tmp" /*!< tmp file for configuration file rewriting */


/* The types of params */
typedef enum { PT_BOOL, PT_NUMERIC, PT_STRING, PT_ENUMERATION } INPUT_TYPE;

static char *log_mode_values[] =
{
    "ERROR",
    "SECURITY",
    "DEBUG",
    NULL
};
#define log_mode_values_size 3

static char *sip_proto_values[] =
{
    "UDP",
    "TCP",
    NULL
};
#define sip_proto_values_size 2

static char *sip_detection_values[] =
{
    "SPEC",
    "ALL_UDP",
    "ALL_TCP",
    NULL
};
#define sip_detection_values_size 3

static char *hash_values[] =
{
	"S256",
	NULL
};
#define hash_values_size	1

static char *cipher_values[] =
{
	"AES1",
	"AES3",
	NULL
};
#define cipher_values_size	2

static char *atl_values[] = 
{
	"HS32",
	"HS80",
	NULL
};
#define atl_values_size	2

static char *sas_values[] =
{
	"B32",
	"B256",
	NULL
};
#define sas_values_size	2

static char *pk_values[] =
{
	"PRSH",
	"MULT",
	"DH2K",
	"EC256",
	"DH3K",
	"EC384",
	"EC521",
	"DH4K",
	NULL
};
#define pk_values_size	8

static char *license_values[] =
{
	"PASSIVE",
	"ACTIVE",
	"UNLIMITEDLIC",
	NULL
};
#define license_values_size	3

static char *detect_values[] =
{
    "SIP",
    "RTP",
    NULL
};
#define detect_values_size 2


typedef enum sip_proto_type
{
    SIP_PROTO_UDP = 1,
    SIP_PROTO_TCP = 2
} sip_proto_type_t;

// string names of params
static char *intrinsics[] = 
{
    "AUTOSECURE", "ALLOWCLEAR",
    "ATL", "HASH", "SAS", "PKTYPE", "CIPHER", 
    "LOG_FILE", "LOG_MODE", "MAX_SIZE", "UPDATE_TIMEOUT", "STORING_PERIOD",
    "CACHE_TTL", "DETECTION", "SIP_PORT", "SIP_PROTO", "SIP_DESCR", 
    "SIP_DETECTION", "DEBUG", "PRINT_DEBUG", "ALERT", "IS_EC", "LICENSE", "HEAR_CTX",
    NULL
};

// array, which contains params types
static INPUT_TYPE intrinsicType[] = 
{
    PT_BOOL, PT_BOOL,
    PT_ENUMERATION, PT_ENUMERATION, PT_ENUMERATION, PT_ENUMERATION, PT_ENUMERATION,
    PT_STRING, PT_ENUMERATION, PT_NUMERIC, PT_NUMERIC, PT_NUMERIC,
    PT_NUMERIC, PT_ENUMERATION, PT_NUMERIC, PT_ENUMERATION, PT_STRING,
    PT_ENUMERATION, PT_BOOL, PT_BOOL, PT_BOOL, PT_BOOL, PT_ENUMERATION, PT_BOOL
};

enum 
{
    AUTOSECURE = 0, ALLOWCLEAR, 
    ATL, HASH, SAS, PKT, CIPHER,
    LOG_FILE, LOG_MODE, MAX_SIZE, UPDATE_TIMEOUT, STORING_PERIOD,
    CACHE_TTL, DETECTION, SIP_PORT, SIP_PROTO, SIP_DESCR,
    SIP_DETECTION, IS_DEBUG, PRINT_DEBUG, ALERT, IS_EC, LICENSE, HEAR_CTX_PAR, PARAMETER_COUNT
};

int load_from_fd(struct zfone_configurator* config, FILE *configFilePtr);
int store_to_fd( FILE *configFilePtr, FILE *tmpFilePtr, struct zfone_configurator* config);
void print_error(const char* format, ...);

/*============================================================================*/
/*     Interface implementation						      */
/*============================================================================*/

void zrtp_set_dafault_logs(struct zfone_configurator* config)
{
    strncpy(config->loggers[0].file_name, "all", sizeof(config->loggers[0].file_name)-1);
    config->loggers[0].log_mode = 3;
    strncpy(config->loggers[1].file_name, "errors", sizeof(config->loggers[1].file_name)-1);
    config->loggers[1].log_mode = 1;
    config->loggers_count = 2;
}

//------------------------------------------------------------------------------
static int create(struct zfone_configurator* config, void* fname)
{
    // check and save config file name
    if ( strlen((char*)fname) >= sizeof(config->file_name)-1 )
    {
		print_error("ZFONED create configurator: ERROR! file name too long.\n");
		return -2;
    }
    
    strncpy(config->file_name, (char*)fname, sizeof(config->file_name)-1);
    config->params.license_mode = ZRTP_LICENSE_MODE_ACTIVE;
	
    // clear all crypto components preferences
    return ZRTP_CONFIG_OK;
}

//------------------------------------------------------------------------------
static void destroy(struct zfone_configurator* config)
{
}

//------------------------------------------------------------------------------
static int load(struct zfone_configurator* config)
{
    /* try to open configuration file */
    FILE *conf_fd = NULL;
    int status = ZRTP_CONFIG_ERROR;

    conf_fd = fopen(config->file_name, "r");
    if ( !conf_fd )
    {
		print_error("ZFONED zfone_configurator->load: Can't open file %s for reading.", config->file_name);
		return ZRTP_CONFIG_NO;
    }
    
    /* parse configuration file */
    status = load_from_fd(config, conf_fd);
    
    /* close configuration file */
    fclose(conf_fd);

	// for release
	//config->params.is_debug = 0;
    return status;
}

//------------------------------------------------------------------------------
static void load_defaults(struct zfone_configurator* config)
{
	int is_debugging = config->params.is_debug;
	int license = config->params.license_mode;
	zfone_sip_port_t* port = NULL;
    memset(&config->params, 0, sizeof(config->params));

    // Restore defaults Zfone profile flags
    config->params.zrtp.autosecure	= true;
    config->params.zrtp.allowclear	= true;
    
    config->params.zrtp.disclose_bit	 = 0;
    config->params.zrtp.cache_ttl		 = SECRET_NEVER_EXPIRES;
    
    // Set Zfone crypto components priority
    config->params.zrtp.sas_schemes[0]   = ZRTP_SAS_BASE256;
    config->params.zrtp.sas_schemes[1]   = ZRTP_SAS_BASE32;
	config->params.zrtp.pk_schemes[0]   = ZRTP_PKTYPE_MULT;	
#if (defined(ZRTP_USE_ENTERPRISE) && (ZRTP_USE_ENTERPRISE == 1))
	config->params.zrtp.pk_schemes[1]	= ZRTP_PKTYPE_EC256P;
	config->params.zrtp.pk_schemes[2]   = ZRTP_PKTYPE_DH3072;
	config->params.is_ec = 1;
#else
	config->params.zrtp.pk_schemes[1]  = ZRTP_PKTYPE_DH3072;
	config->params.is_ec = 0;
#endif
	config->params.zrtp.cipher_types[0]  = ZRTP_CIPHER_AES128;    
    config->params.zrtp.auth_tag_lens[0] = ZRTP_ATL_HS32;
    config->params.zrtp.hash_schemes[0]  = ZRTP_HASH_SHA256;
    
    // Restoring default Zfone sniffing options
    config->params.sniff.sip_scan_mode 	 = ZRTP_BIT_SIP_SCAN_PORTS;
    config->params.sniff.sip_scan_mode 	|= ZRTP_BIT_SIP_SCAN_UDP;
    config->params.sniff.rtp_detection_mode = ZRTP_BIT_RTP_DETECT_SIP;
    
    // mark known SIP ports 5060 and several ports after it to handle situation when
    // VoIP clients can't get acces to 5060 and use next one after 5060.
    port = &config->params.sniff.sip_ports[0];
    port->port = VOIP_SIP_PORT;
    port->proto = voip_proto_UDP;
    snprintf(port->desc, sizeof(port->desc), "%s", "RFC 3261 default port");
    
    // mark special SIP ports for Gizmo
    port = &config->params.sniff.sip_ports[1];
    port->port = VOIP_SIP_PORT_GIZMO1;
    port->proto = voip_proto_UDP;
    snprintf(port->desc, sizeof(port->desc), "%s", "Gizmo client first default");
    
    port = &config->params.sniff.sip_ports[2];
    port->port = VOIP_SIP_PORT_GIZMO2;
    port->proto = voip_proto_UDP;
    snprintf(port->desc, sizeof(port->desc), "%s", "Gizmo client first default");

	// for release	
	config->params.is_debug = is_debugging;
	config->params.license_mode = license;

    config->max_size = 3000;
    
	config->params.print_debug = 1;
	config->params.alert = 1;
	config->params.hear_ctx = 0;
	
    set_log_files_size(config->max_size);
}


//------------------------------------------------------------------------------
static int storage(const struct zfone_configurator* config)
{
	FILE* conf_fd = NULL;
	FILE* tmp_fd = NULL;
	int status = ZRTP_CONFIG_ERROR;

    char buffer[CONFIG_FILE_NAME_SIZE + 4];
    strncpy(buffer, config->file_name, sizeof(buffer)-1);
    strncat(buffer, ".bak", sizeof(buffer)-1);
    rename(config->file_name, buffer);

    // Try to open file. Create it if it doesn't exist
	conf_fd = fopen(buffer, "r");
    if ( !conf_fd )
    {
		ZRTP_LOG(1, (_ZTU_, "ZFONED zfone_configurator->storage: Can't"
						" config file %s for reading - create it.\n", config->file_name));
		conf_fd = fopen(config->file_name, "w");
		if ( !conf_fd )
		{
	    	ZRTP_LOG(1, (_ZTU_, "ZFONED zfone_configurator->storage: Can't"
						    " create conf file %s.\n", config->file_name ));
	    	return ZRTP_CONFIG_ERROR;
		}
    }

	tmp_fd = fopen(TMP_FILE, "w");
    if ( !tmp_fd )
    {
		fclose(conf_fd);
		ZRTP_LOG(1, (_ZTU_, "ZFONED zfone_configurator->storage: Can't open"
						" tmp file %s for writing.\n", TMP_FILE));
		return ZRTP_CONFIG_ERROR;
    }
    
    // storing options
    status = store_to_fd( conf_fd, tmp_fd, (struct zfone_configurator*) config );
    
    // close configuration file
    fclose(conf_fd);
    fclose(tmp_fd);
        
    rename(TMP_FILE, config->file_name);
    
    return status;
}

//------------------------------------------------------------------------------
static int configure_global(struct zfone_configurator* config)
{
    zrtp_print_log_delim(3, LOG_START_SUBSECTION, "configuring Zfone");

    // Configuring firewall according to SIP detection policy
    zfone_network_add_rules((config->params.sniff.sip_scan_mode & ZRTP_BIT_SIP_SCAN_TCP), voip_proto_TCP);

    // configuring SIP sniffer
    manager.config_sip(&manager, config);

    zrtp_print_log_delim(3, LOG_END_SUBSECTION, "configuring Zfone");

    return ZRTP_CONFIG_OK;
}

//------------------------------------------------------------------------------
static int update_params( const char* params,
						  int params_length,
						  struct zfone_configurator* config)
{
    if (!params || !config)
    {
		ZRTP_LOG(1, (_ZTU_, "ZFONED configurator update_params: ERROR!"
									   " wrong params. Params or config is NULL.\n"));
		return ZRTP_CONFIG_ERROR;
    }
    
    if (params_length != sizeof(zfone_params_t))
    {
		ZRTP_LOG(1, (_ZTU_, "ZFONED configurator update_params: ERROR! wrong"
									   " params length %d insted %d\n",
										params_length, sizeof(zfone_params_t)));
		return ZRTP_CONFIG_ERROR;
    }
    
    memcpy(&config->params, params, params_length);
    return ZRTP_CONFIG_OK;
}

//------------------------------------------------------------------------------
int zfone_configurator_ctor(struct zfone_configurator* config)
{    
    if (!config)    
		return ZRTP_CONFIG_ERROR;    
    
    memset(config, 0, sizeof(struct zfone_configurator));

    config->create 			= create;
    config->destroy 		= destroy;
    config->load			= load;
    config->load_defaults	= load_defaults;
    config->storage			= storage;
    config->configure_global= configure_global;
    config->update_params	= update_params;

    return ZRTP_CONFIG_OK;
}

//==============================================================================
// Configurator internal realization
//==============================================================================


#define	DELIMITER_SYMBOL	';'
#define BET_DELIM_SYMBOL	','

static int 	line;		/* The line on which an error occurred */
static int 	errCount;	/* Total error count */
static bool hasError;	/* Whether this line has an error in it */
static char *errtag;   	/* Name of the parsed file. Used in error messages */
static bool	profile_dirty[ZRTP_MAX_COMP_COUNT]; /* indicates that profile was changed */
static bool	sip_detection_dirty; /* indicates that sip detection was changed */
static bool	detection_dirty; /* indicates that sessions detection was changed */
static bool	loggers_dirty;

static zfone_sip_port_t tmp_port_record;


/*! The types of error we check for */
enum { NO_ERROR, ILLEGAL_CHAR_ERROR, LINELENGTH_ERROR };

/* values for boolean params*/
static char *settings[] = { "ON", "OFF", "YES", "NO", "TRUE", "FALSE", NULL };

/*!
 * \brief pointer type for functions, which get params values from char buffer    
 * \param  key - buffer
 * \keyLength - buffer length
 * \ptr - extra param, which is interpreted individually in each function
 */
typedef int (*funcGetParamValue)( char *key, int keyLength, void *ptr );


/* For each enumeration there should be declared char* array with values */
//...
static int lookup( char *key, int keyLength, char *keyWords[] );
static int extractToken( char *buffer, int *endIndex, int *length );
static int getBit( char *key, int keyLength, void *ptr);
static int getBool(char *buffer, int length, void *ptr);
static int getNumeric(char *buffer, int length, void *ptr);
static int getString( char *buffer, int endIndex, void *ptr);

static funcGetParamValue	getFuncs[4];


//------------------------------------------------------------------------------
void print_error(const char* format, ...)
{
    char buffer[1024];
    va_list arg;
    va_start(arg, format);
    vsnprintf(buffer, 1024, format, arg);
	if (line)    
		ZRTP_LOG(3, (_ZTU_, "%s. Line: %d\n", buffer, line));    
    else    
		ZRTP_LOG(3, (_ZTU_, "%s.\n", buffer));
    va_end(arg);
}

//------------------------------------------------------------------------------
/* returns pointer to array of bit values */
static void *getEnumsValues(int aEnum)
{
    switch (aEnum)
    {
	case LOG_MODE:	
	    return log_mode_values;
	case SIP_PROTO:
	    return sip_proto_values;
	case SIP_DETECTION:
	    return sip_detection_values;
	case ATL:	
		return atl_values;
	case HASH:	
		return hash_values;
	case SAS:
		return sas_values;
	case PKT:
		return pk_values;
	case CIPHER:
		return cipher_values;
	case DETECTION:
		return detect_values;
	case LICENSE:
		return license_values;
	default:	
        print_error("ZFONED getEnumsValues: ERROR! UNMATCHED ENUMERATION!");
	    return NULL;
    }
}

//------------------------------------------------------------------------------
/* Search a list of keywords for a match */
static int lookup( char *key, int keyLength, char *keyWords[] )
{
    int index, position = 0, noMatches = 0;
	char optstr[LINEBUF_SIZE];

    if ( keyWords[0] == NULL )
        return ZRTP_CONFIG_ERROR;
    
    strncpy(optstr, key, keyLength);
    optstr[ keyLength ] = '\0';

    /* Make the search case insensitive */
    for( index = 0; index < keyLength; index++ )
		key[index] = toupper(key[index]);

    for(index = 0; keyWords[index] != NULL; index++)
    {
		if( !strncmp(key, keyWords[index], keyLength ))
        {
    	    if( (int) strlen(keyWords[index]) == keyLength )			
				return index;	/* exact match */			
			position = index;
			noMatches++;
		}
    }
	
    switch( noMatches )
    {
	case 0:
	    print_error("ZFONED lookup: ERROR! %s: unknown keyword: \"%s\"", errtag, optstr);
	    break;
	case 1:
	    return( position );	/* Match succeeded */
	default:
	    print_error("ZFONED lookup: ERROR! %s: \"%s\" is ambiguous", errtag, optstr );
    }
    return ZRTP_CONFIG_ERROR;
}


//------------------------------------------------------------------------------
/* Extract a token from a buffer */
static int extractToken( char *buffer, int *endIndex, int *length )
{
    int index = 0, tokenStart;
    char ch = 0;
	char prev_ch = 0;    
	int quote_flag = 0;

    /* Skip whitespace */
    for(ch = buffer[ index ]; ch && ( ch == ' ' || ch == '\t' ); ch = buffer[ index ] )    
		index++;
    
    if ( ch == BET_DELIM_SYMBOL )
		index++;

    for(ch = buffer[ index ]; ch && ( ch == ' ' || ch == '\t' ); ch = buffer[ index ] )    
		index++;

    tokenStart = index;
    
    /* Find end of setting */
    while( index < LINEBUF_SIZE && ( ch = buffer[ index ] ) != '\0')
    {
		if (!quote_flag && (ch == ' ' || ch == '\t' || ch == DELIMITER_SYMBOL || ch == BET_DELIM_SYMBOL))		
			break;
		
		if (ch == '\"')
		{
			if (quote_flag && prev_ch != '\\')
			{
				index++;
				break;
			}
			quote_flag++;
		}
		index++;
		prev_ch = ch;
    }

    *endIndex += index;
    *length = index - tokenStart;

    /* Return start position of token in buffer */
    return tokenStart;
}

//------------------------------------------------------------------------------
/* 
    gets bit for a bit mask
    ptr - pointer to array of char*, which contains values for bit mask
*/
static int getBit( char *key, int keyLength, void *ptr)
{
    int result = lookup( key, keyLength, ptr );
    if ( result == ZRTP_CONFIG_ERROR )
    {
		hasError = true;
		return ZRTP_CONFIG_ERROR;
    }

    return 1 << result;   
}

//------------------------------------------------------------------------------
/*
    returns boolean value from buffer
    ptr - pointer to param name, used in error message
*/
static int getBool(char *buffer, int length, void *ptr)
{    
    int settingIndex = lookup(buffer, length, settings);
    if( settingIndex != ZRTP_CONFIG_ERROR )    
		return ( settingIndex % 2 ) ? false : true;    

    hasError = true;
    return ZRTP_CONFIG_ERROR;
}

//------------------------------------------------------------------------------
/*
    get numeric value from buffer
    ptr - pointer to param name, used in error message
*/
static int getNumeric(char *buffer, int length, void *ptr)
{
    char *p = NULL;
    long longval = strtol(buffer, &p, 0);
    if (p == buffer+length && longval <= INT_MAX && longval >= INT_MIN) 
		return (int)longval;    

    hasError = true;
    return ZRTP_CONFIG_ERROR;
}

//------------------------------------------------------------------------------
/* get string value from buffer */
static int getString( char *buffer, int endIndex, void *ptr)
{
    bool noQuote = false;
    int stringIndex = 0, bufferIndex = 1;
    char ch = *buffer;

    char *str = (char *)ptr;
    /* Skip whitespace */
    while( ch && ( ch == ' ' || ch == '\t' ) )    
    	ch = buffer[ bufferIndex++ ];
    
    /* Check for non-string */
    if( ch != '\"' )
    {
		endIndex += bufferIndex;

		/* Check for special case of null string */
		if( !ch )
		{
    	    *str = '\0';
		    return ZRTP_CONFIG_OK;
		}

		/* Use nasty non-rigorous string format */
		noQuote = true;
    }

    /* Get first char of string */
    if( !noQuote )
		ch = buffer[ bufferIndex++ ];

    int prev_ch = 0;
    /* Get string into string */
    while( ch ) 
    {
		if (!noQuote && ch == '\"' && prev_ch != '\\')
		    break;
		/* Exit on '#' if using non-rigorous format */
		if( noQuote && (ch == '#' || ch == ' ' || ch == '\t' || ch == BET_DELIM_SYMBOL) )	
	  	    break;

		str[ stringIndex++ ] = ch;
		prev_ch = ch;
		ch = buffer[ bufferIndex++ ];
    }

    /* If using the non-rigorous format, stomp trailing spaces */
    if( noQuote )
    {
		while( stringIndex > 0 && str[ stringIndex - 1 ] == ' ' )		
	  	    stringIndex--;		
    }
    str[ stringIndex++ ] = '\0';
    endIndex += bufferIndex;

    /* Check for missing string terminator */
    if( ch != '\"' && !noQuote )
    {
		printf("ZFONED getString: ERROR! unterminated string: '\"%s'", str );
		hasError = true;
		return ZRTP_CONFIG_ERROR;
    }

    return ZRTP_CONFIG_OK;
}

void set_profile_settings(uint8_t *array, int enum_index)
{
	int i, value = 0;
	
	while (enum_index)
	{
		value++;
		enum_index >>= 1;
	}

	for (i = 0; i < ZRTP_MAX_COMP_COUNT; i++)
	{
		if ( array[i] == value )
			return;
		if ( !array[i] )
			break;
	}
	
	if ( i == ZRTP_MAX_COMP_COUNT )
		return;

	array[i] = value;
}

//------------------------------------------------------------------------------
static void setValue(int intrinsicIndex, int result, void *ptr, struct zfone_configurator* config)
{
    // checks every parameter
    switch( intrinsicIndex )
    {
	case AUTOSECURE:
	    config->params.zrtp.autosecure = result;
	    break;
	case ALLOWCLEAR:
	    config->params.zrtp.allowclear = result;
	    break;
	case MAX_SIZE:
	    config->max_size = result;
	    break;
	case UPDATE_TIMEOUT:
	    config->update_timeout = result;	    
	    break;
	case STORING_PERIOD:
	    config->storing_period = result;
	    break;
	case LOG_FILE:
	{
	    if ( !loggers_dirty )
	    {
			loggers_dirty = true;
			config->loggers_count = 0;
	    }
	    if (config->loggers_count >= LOG_ELEMS_COUNT)
	    {
			print_error("Error: too many loggers. Maximum count is %d", LOG_ELEMS_COUNT);
			break;    
	    }
	    if (strlen(ptr) >= FILES_NAME_SIZE)
	    {
			print_error("Error: too long logger file name, maximum size is %d", LOGGER_FILENAME_SIZE);
			break;
	    }
	    
	    strncpy( config->loggers[config->loggers_count].file_name,
				 ptr,
				 sizeof(config->loggers[config->loggers_count].file_name)-1);
		config->loggers_count++;
	    print_error("file = %s", ptr);
	    break;
	}
	case LOG_MODE:
	{
	    if ( !config->loggers_count )
	    {
			print_error("Error: use LOG_FILE before LOG_MODE");
			errCount++;
			break;
	    }
		config->loggers[config->loggers_count-1].log_mode = (result/2)+1;
	    print_error("mode = %d", result);
	    break;
	}

	case ATL:
		set_profile_settings(config->params.zrtp.auth_tag_lens, result);
	    break;
	case HASH:
	    set_profile_settings(config->params.zrtp.hash_schemes, result);
	    break;
	case SAS:
        set_profile_settings(config->params.zrtp.sas_schemes, result);
	    break;
	case PKT:
	    set_profile_settings(config->params.zrtp.pk_schemes, result);
	    break;
	case CIPHER:
	    set_profile_settings(config->params.zrtp.cipher_types, result);
	    break;
	case CACHE_TTL:
	{
	    config->params.zrtp.cache_ttl = result;
	    if (result != (int)SECRET_NEVER_EXPIRES)
			config->params.zrtp.cache_ttl *= 86400;	    
	    break;
	}

	case SIP_PORT:
	{
	    if (tmp_port_record.port)
		{
			if (!tmp_port_record.proto)
			{
				print_error("sip protocol was not defined in previous port declaration");
			}
			else
			{
				int i;
				for (i=0; i<ZRTP_MAX_SIP_PORTS_FOR_SCAN; i++)
				{
					if ( !config->params.sniff.sip_ports[i].port )
					{
						zfone_sip_port_t *port = &config->params.sniff.sip_ports[i];
						port->port  = tmp_port_record.port;
						port->proto = tmp_port_record.proto;
						tmp_port_record.port  = 0;
						tmp_port_record.proto = 0;
						print_error("added new sip record: port = %d, proto = %d", port->port, port->proto);
						break;
					}
				}
				if (i == ZRTP_MAX_SIP_PORTS_FOR_SCAN)
				print_error("Ports array is full");
			}
	    }

	    if (result <= 0 || result > 0xFFFF)
			print_error("sip port is out of range: %d", result);
	    else
			tmp_port_record.port = result;

		break;
	}
	case SIP_PROTO:
	{
	    if (result == SIP_PROTO_UDP)
			tmp_port_record.proto = voip_proto_UDP;
	    else if (result == SIP_PROTO_TCP)
			tmp_port_record.proto = voip_proto_TCP;
	    else
			print_error("unknown protocol");

		break;
	}
	case SIP_DESCR:
	{
	    int i;	    
	    if (tmp_port_record.port <= 0 )
			print_error("sip port is out of range: %d", tmp_port_record.port);
	    if ( !tmp_port_record.proto )
			print_error("protocol for sip is not specified");
	    
	    for (i = 0; i < ZRTP_MAX_SIP_PORTS_FOR_SCAN; i++)
	    {
			if ( !config->params.sniff.sip_ports[i].port )
			{
				zfone_sip_port_t *port = &config->params.sniff.sip_ports[i];
				port->port  = tmp_port_record.port;
				port->proto = tmp_port_record.proto;
				strncpy(port->desc, ptr, ZRTP_SIP_PORT_DESC_MAX_SIZE - 1);
				port->desc[ZRTP_SIP_PORT_DESC_MAX_SIZE - 1] = 0;
				tmp_port_record.port  = 0;
				tmp_port_record.proto = 0;
				print_error("added new sip record: port = %d, proto = %d, descr = %s", port->port, port->proto, port->desc);
				break;
			}
	    }

	    if (i == ZRTP_MAX_SIP_PORTS_FOR_SCAN)
			print_error("Ports array is full");
	    
	    break;
	}
	case SIP_DETECTION:
	{
	    if ( !sip_detection_dirty ) 
	    {
			config->params.sniff.sip_scan_mode = 0;
			sip_detection_dirty = true;
	    }
	    config->params.sniff.sip_scan_mode |= result;
	    break;
	}
	case IS_DEBUG:
	    config->params.is_debug = result;	    
	    break;
	case PRINT_DEBUG:
	    config->params.print_debug = result;	    
	    break;
	case ALERT:
	    config->params.alert = result;
	    break;
	case IS_EC:
	    config->params.is_ec = result;	    
	    break;
	case DETECTION:
	{
	    if ( !detection_dirty ) 
	    {
			config->params.sniff.rtp_detection_mode = 0;
			detection_dirty = true;
	    }
	    config->params.sniff.rtp_detection_mode |= result;
	    break;
	}
	case LICENSE:
		config->params.license_mode = result >> 1;
		break;
	case HEAR_CTX_PAR:
		config->params.hear_ctx = result;
		break;
    }
}

//------------------------------------------------------------------------------
/* Process an assignment */
static int processAssignment( int intrinsicIndex,
							  char *buffer,
							  int *endIndex,
							  struct zfone_configurator* config )
{
    char tmpStr[LINEBUF_SIZE];
    int 		length = 0;
    int 		type = 0;
	int 		result = 0;

	if ( intrinsicIndex >= PARAMETER_COUNT )
	{
	    print_error("ZFONED processAssignment: ERROR! %s wrong parameter.\n", errtag);
		hasError = true;
		errCount++;
		return  ZRTP_CONFIG_ERROR;
	}
    
    type = intrinsicType[intrinsicIndex];
    buffer += extractToken( buffer, endIndex, &length );

    /* Check for an assignment operator */
    if( *buffer != '=' )
    {
		if( line )
	    	print_error("ZFONED processAssignment: ERROR! %s: expected '=' in line %d", errtag, line);
		else
		    print_error("ZFONED processAssignment: ERROR! %s: expected '=' after \"%s\"", errtag, intrinsics[intrinsicIndex]);

		hasError = true;
		errCount++;
		return ZRTP_CONFIG_ERROR;
    }
    buffer++;	/* Skip '=' */

    /* loop through tokens */
    while (*buffer)
    {
		void *ptr = NULL;
		buffer += extractToken( buffer, endIndex, &length );
		
		/* assign ptr for function, which will process buffer for result */
		switch ( type )
		{
		// just param name for error messaging
		case PT_BOOL:
		case PT_NUMERIC:
			ptr = intrinsics[intrinsicIndex];
			break;
		// result string will be return in tmpStr
		case PT_STRING:
			ptr = tmpStr;
			break;
		// ptr has to point to the array with possible values
		case PT_ENUMERATION:
			ptr = getEnumsValues(intrinsicIndex);
			if (ptr == NULL)
			{
				print_error("ZFONED processAssignment: ERROR! processing current line is stoped");
				return ZRTP_CONFIG_ERROR;
			}
			break;
		default:
			print_error("ZFONED processAssignment: ERROR! %s wrong parameter type.\n", errtag);
			hasError = true;
			errCount++;
			return ZRTP_CONFIG_ERROR;
		}
	
		// call function for result
		result = getFuncs[type](buffer, length, ptr);
		
		if ( hasError )
		{
			errCount++;
			return ZRTP_CONFIG_ERROR; 
		}
		
		// sets value to parameter
		setValue(intrinsicIndex, result, ptr, config);	
	
		buffer += length;
    }

    return ZRTP_CONFIG_OK;
}

int getLine(FILE *configFilePtr, char *inBuffer, int *lineBufCount, int *errType, int *errPos)
{
    int ch = 0, theChar = 0;
	int quote_flag = 0, comment_flag = 0;
	char prev_ch = 0;
        
    // Skip whitespace
    while( ( ( ch = getc( configFilePtr ) ) == ' ' || ch == '\t' ) && ch != EOF );
    // Get a line into the inBuffer
    *lineBufCount = 0;
    *errType = NO_ERROR;
    
    
    if ( ch == '#' )
		comment_flag = 1;

    while( ch != '\r' && ch != '\n' && ch != CPM_EOF && ch != EOF )
    {
		//Check for an illegal character in the data
		if( ( ch < ' ' || ch > '~' ) &&
			  ch != '\r' && ch != '\n' &&
			  ch != ' ' && ch != '\t' && ch != CPM_EOF &&
			  ch != EOF )
		{
			if( *errType == NO_ERROR )
				*errPos = *lineBufCount; // Save pos of first illegal char			
			*errType = ILLEGAL_CHAR_ERROR;
		}
		else if ( ch == '\"' && prev_ch != '\\')
		{
			quote_flag ^= 1;
		}
		else if ( ch == DELIMITER_SYMBOL && !quote_flag && !comment_flag)
		{
			break;
		}
	
		// Make sure the path is of the correct length.  Note that the code is
		// ordered so that a LINELENGTH_ERROR takes precedence over an ILLEGAL_CHAR_ERROR
		if( *lineBufCount >= LINEBUF_SIZE - 1)
			*errType = LINELENGTH_ERROR;
		else
			inBuffer[ (*lineBufCount)++ ] = ch;
		
		prev_ch = ch;
		if( ( ch = getc( configFilePtr ) ) == '#' )
		{
			// Skip comment section and trailing  whitespace
			while( ch != '\r' && ch != '\n' && ch != CPM_EOF && ch != EOF )			
				ch = getc( configFilePtr );			
			break;
		}
    }

    if (ch == '\r' || ch == '\n' || ch == CPM_EOF || ch == EOF )
		line++;
    // Skip trailing whitespace and add der terminador
    while( *lineBufCount && ( (theChar = inBuffer[ *lineBufCount - 1 ]) == ' ' || theChar == '\t') )
        (*lineBufCount)--;
    
    inBuffer[ *lineBufCount ] = '\0';
    return ch;
}

int getWord(char *inBuffer, int *chr)
{
    int index, ch = *chr;
    for( index = 0; index < LINEBUF_SIZE && ( ch = inBuffer[ index ] ) != '\0' 
    	 && ch != ' ' && ch != '\t' && ch != '='; index++ );

    *chr = ch;
    return index;
}

//------------------------------------------------------------------------------
int load_from_fd( struct zfone_configurator* config, FILE *configFilePtr )
{	
    int ch = 0;
    int errType, errPos = 0, lineBufCount, intrinsicIndex;
    int index;
	int result;
    char inBuffer[ LINEBUF_SIZE ];

    print_error("loggers_count before test = %d\n", config->loggers_count);

    for (index = 0; index < ZRTP_MAX_COMP_COUNT; index++)
		profile_dirty[index] = false;

    sip_detection_dirty = false;
    detection_dirty = false;
    loggers_dirty = false;

    line = 0;
    errCount = 0;
    errtag = basename( (char*)config->file_name );

    getFuncs[PT_BOOL] = getBool;
    getFuncs[PT_NUMERIC] = getNumeric;
    getFuncs[PT_STRING] = getString;
    getFuncs[PT_ENUMERATION] = getBit;
	
    // Process each line in the configFile
    while( ch != EOF )
    {
        hasError = false;
	ch = getLine(configFilePtr, inBuffer, &lineBufCount, &errType, &errPos);

	// Process the line unless its a blank or comment line
	if( lineBufCount && *inBuffer != '#' )
	{
	    switch( errType )
	    {
	    case LINELENGTH_ERROR:
	        print_error("ZFONED load: ERROR! %s: line '%.30s...' too long", errtag, inBuffer );
		    errCount++;
		    break;

		case ILLEGAL_CHAR_ERROR:
		    print_error("ZFONED load: ERROR! > %s  ", inBuffer );
		    print_error("ZFONED load: ERROR! %*s^", errPos, "" );
		    print_error("ZFONED load: ERROR! %s: bad character in command on line %d", errtag, line );
		    errCount++;
		    break;

		default:
		    index = getWord(inBuffer, &ch);

		    /* Try and find the intrinsic.  We
		     don't treat unknown intrinsics as
		     an error to allow older versions to
		     be used with new config files */
		    intrinsicIndex = lookup(inBuffer, index, intrinsics );
				
		    if( intrinsicIndex == ZRTP_CONFIG_ERROR ) break;
		    /* lets process buffer and assign got value to parameter with index intrinsicIndex */
		    processAssignment( intrinsicIndex, inBuffer + index, &index, config );
		    break;
	    }
	}

	// Handle special-case of ^Z if configFile came off an  MSDOS system
	if( ch == CPM_EOF )
	    ch = EOF;

	// Exit if there are too many errors
	if( errCount >= MAX_ERRORS )
	    break;
    }

    print_error("loggers_count test = %d\n", config->loggers_count);
    
    for (index = 0; index < config->loggers_count; index++)
    {
		if ( (result = zrtp_logger_add_file(config->loggers[index].file_name, config->loggers[index].log_mode)) < 0 )	
			print_error("Error: couldnt init log file %s, error code %d\n", config->loggers[index].file_name, result);
    }
    set_log_files_size(config->max_size);


    // Exit if there were errors
    if( errCount )
    {
        printf("ZFONED load: ERROR! %s: %s%d error(s) detected\n\n",
			   config->file_name, ( errCount >= MAX_ERRORS ) ? "Maximum level of " : "", errCount );
		return ZRTP_CONFIG_ERROR;
    }

    return ZRTP_CONFIG_OK;
}

static void writeEnum( FILE *file, int value, char **stringValues, int length )
{
    int counter = 0, first = 1;
    while (value)
    {
		if (value & 1)
		{
			if (!first)
				fprintf(file, ",");
			else
				first = 0;
			if ( counter < length )
				fprintf(file, " %s", stringValues[counter]);
		}
		value >>= 1;
		counter++;
    }
}

static void write_profile(FILE *file, uint8_t *array, char **stringValues, int length)
{
    int i, first = 1;

	for (i=0; i<ZRTP_MAX_COMP_COUNT; i++)
    {
		if ( !array[i] )
			break;
		
	    if (!first)
			fprintf(file, ",");
	    else
			first = 0;

		// TODO: check string values
	    if ( array[i] <= length )
			fprintf(file, " %s", stringValues[array[i] - 1]);
    }
}

static int processWrite( FILE *file, int intrinsicIndex, struct zfone_configurator* config )
{
    // checks every parameter
    switch( intrinsicIndex )
    {
	case AUTOSECURE:
	    fprintf(file, "%s\t= %s\n", intrinsics[intrinsicIndex], settings[!config->params.zrtp.autosecure]);
	    break;
	case ALLOWCLEAR:
		fprintf(file, "%s\t= %s\n", intrinsics[intrinsicIndex], settings[!config->params.zrtp.allowclear]);
	    break;
	case MAX_SIZE:
	    fprintf(file, "%s = %d\n", intrinsics[intrinsicIndex], config->max_size);
	    break;
	case UPDATE_TIMEOUT:
	    fprintf(file, "%s = %d\n", intrinsics[intrinsicIndex], config->update_timeout);
	    break;
	case STORING_PERIOD:
	    fprintf(file, "%s = %d\n", intrinsics[intrinsicIndex], config->storing_period);
	    break;
	
	case LOG_FILE:
	case LOG_MODE:
	case SIP_PROTO:
	case SIP_PORT:
	case SIP_DESCR:	
	    break;

	case ATL:
	    fprintf(file, "%s = ", intrinsics[intrinsicIndex]);
	    write_profile(file, config->params.zrtp.auth_tag_lens, atl_values, atl_values_size);
		fprintf(file, "\n");
	    break;
	
	case HASH:
	    fprintf(file, "%s = ", intrinsics[intrinsicIndex]);
	    write_profile(file, config->params.zrtp.hash_schemes, hash_values, hash_values_size);
		fprintf(file, "\n");
	    break;
	
	case PKT:
	    fprintf(file, "%s = ", intrinsics[intrinsicIndex]);
	    write_profile(file, config->params.zrtp.pk_schemes, pk_values, pk_values_size);
		fprintf(file, "\n");
	    break;
	
	case CIPHER:
	    fprintf(file, "%s = ", intrinsics[intrinsicIndex]);
	    write_profile(file, config->params.zrtp.cipher_types, cipher_values, cipher_values_size);
		fprintf(file, "\n");
	    break;
	
	case SAS:
	    fprintf(file, "%s = ", intrinsics[intrinsicIndex]);
	    write_profile(file, config->params.zrtp.sas_schemes, sas_values, sas_values_size);
		fprintf(file, "\n");
	    break;
	
	case CACHE_TTL:
	{
		int cache_ttl_value = config->params.zrtp.cache_ttl;
		if ( cache_ttl_value != (int)SECRET_NEVER_EXPIRES )
		cache_ttl_value /= 86400;
		fprintf(file, "%s = %d\n", intrinsics[intrinsicIndex], cache_ttl_value);
		break;
	}	
	
	case SIP_DETECTION:
	    fprintf(file, "SIP_DETECTION = ");
	    writeEnum(file, config->params.sniff.sip_scan_mode, sip_detection_values, sip_detection_values_size);
	    fprintf(file, "\n");
		break;
	
	case IS_DEBUG:
	    fprintf(file, "%s = %s\n", intrinsics[intrinsicIndex], settings[!config->params.is_debug]);
	    break;
	case PRINT_DEBUG:
	    fprintf(file, "%s = %s\n", intrinsics[intrinsicIndex], settings[!config->params.print_debug]);
	    break;
	case ALERT:
	    fprintf(file, "%s = %s\n", intrinsics[intrinsicIndex], settings[!config->params.alert]);
	    break;
	case IS_EC:
		fprintf(file, "%s = %s\n", intrinsics[intrinsicIndex], settings[!config->params.is_ec]);
	    break;

	case DETECTION:
	    fprintf(file, "DETECTION = ");
	    writeEnum(file, config->params.sniff.rtp_detection_mode, detect_values, detect_values_size);
	    fprintf(file, "\n");
	    break;

	case LICENSE:
		fprintf(file, "%s = ", intrinsics[LICENSE]);
		writeEnum(file, 1 << config->params.license_mode, license_values, license_values_size);
		fprintf(file, "\n");
		break;

	case HEAR_CTX_PAR:
		fprintf(file, "%s\t= %s\n", intrinsics[intrinsicIndex], settings[!config->params.hear_ctx]);
		break;
			
	default:
	    ZRTP_LOG(1, (_ZTU_, "ZFONED processWrite: ERROR! unmatched parameter with index %d\n", intrinsicIndex));
	    break;	
    }

    return ZRTP_CONFIG_OK;
}

void writeConfigs( FILE *file, struct zfone_configurator* config )
{
    int index;
    int count = config->loggers_count;
    for (index=0; index < count; index++)
    {
		fprintf(file, "LOG_FILE = \"%s\"", config->loggers[index].file_name);
		if ( config->loggers[index].log_mode )
		{
			int level = config->loggers[index].log_mode;
			if (level > 3 || level < 1)
				level = 3;
			fprintf(file, "\t%c LOG_MODE = %s", DELIMITER_SYMBOL, log_mode_values[level-1]);
		}
		fprintf(file, "\n");
    }
}

void writeSipPorts( FILE *file, struct zfone_configurator* config )
{
    int i;
    zfone_sip_port_t *ports = config->params.sniff.sip_ports;
    
    for (i = 0; i < ZRTP_MAX_SIP_PORTS_FOR_SCAN; i++)
    {
		if (!ports[i].port) continue;
		
		fprintf(file, "SIP_PORT = %d", ports[i].port);
		fprintf(file, "%c\tSIP_PROTO = %s", DELIMITER_SYMBOL, 
			ports[i].proto == voip_proto_UDP ? "UDP" : "TCP");
		if ( strlen(ports[i].desc) )
			fprintf(file, "%c\tSIP_DESCR = \"%s\"", DELIMITER_SYMBOL, ports[i].desc);
		fprintf(file, "\n");
    }
}

int store_to_fd( FILE *configFilePtr, FILE *tmpFilePtr, struct zfone_configurator* config )
{
    int ch = 0;
    int errType, errPos = 0, lineBufCount, intrinsicIndex;
    int index;
    int	parameter_array[PARAMETER_COUNT];
    char inBuffer[ LINEBUF_SIZE ];

    line = 1;
    errCount = 0;
    errtag = basename( (char*) config->file_name );

    for (index = 0; index < PARAMETER_COUNT; index++)    
		parameter_array[index] = 0;
    
    /* Process each line in the configFile */
    while( ch != EOF )
	{
		hasError = false;
		ch = getLine(configFilePtr, inBuffer, &lineBufCount, &errType, &errPos);
	
		/* Process the line unless its a blank or comment line */
		if( lineBufCount && *inBuffer != '#' )
		{
			switch( errType )
			{
			case LINELENGTH_ERROR:
			case ILLEGAL_CHAR_ERROR:
				errCount++;
			break;
	
			default:
				index = getWord(inBuffer, &ch);
	
				/* Try and find the intrinsic.  We
				   don't treat unknown intrinsics as
				   an error to allow older versions to
				   be used with new config files */
				intrinsicIndex = lookup(inBuffer, index, intrinsics );
					
				if( intrinsicIndex == ZRTP_CONFIG_ERROR ) break;
	
				/* lets process buffer and assign got value to parameter with index intrinsicIndex */
				if ( !parameter_array[intrinsicIndex] )
				{
					if ( ZRTP_CONFIG_OK == processWrite( tmpFilePtr, intrinsicIndex, config ) )
						parameter_array[intrinsicIndex] = 1;
				}
				break;
			}
		}
		else if ( lineBufCount || (ch != EOF && ch != CPM_EOF) )
		{
			fprintf(tmpFilePtr, "%s\n", inBuffer);
		}
	
		// Handle special-case of ^Z if configFile came off an
		// MSDOS system
		if( ch == CPM_EOF )
			ch = EOF;
	
		line++;
    }

    for (index = 0; index < PARAMETER_COUNT; index++)
    {
        if (!parameter_array[index] && index != LICENSE)
			processWrite( tmpFilePtr, index, config );
    }

    writeConfigs( tmpFilePtr, config );
    writeSipPorts( tmpFilePtr, config );

    return ZRTP_CONFIG_OK;
}
