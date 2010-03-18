/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok <chaser@soft-industry.com>
 */
 
#ifndef __ZFONE_LOG_H__
#define __ZFONE_LOG_H__

#include <zrtp.h>

#if ZRTP_PLATFORM != ZP_WIN32_KERNEL

	#define LINUX 	1
	#define WIN32K	0
	#define PLATFORM LINUX

	#include "sfiles.h"
	#include "zfone_types.h"

	#if ( PLATFORM == LINUX )
		#include <stdio.h>	/* for unix files operations */
	#endif

#endif //!KERNEL

/*!
 * \brief log delimiters types
 */
typedef enum log_delim
{
    LOG_START_SECTION		= 0,
    LOG_END_SECTION			= 1,
    LOG_START_SUBSECTION	= 2,
    LOG_END_SUBSECTION		= 3,
    LOG_SPACE				= 4,
    LOG_START_SELECT		= 5,
    LOG_END_SELECT			= 6,
    LOG_LABEL				= 7,
    LOG_DELIM_COUNT			= 8 // delimiters count
} log_delim_t;

#if ZRTP_PLATFORM != ZP_WIN32_KERNEL
	#include <string.h>

	#define	LOGGER_FILENAME_SIZE	ZFONE_MAX_FILENAME_SIZE
	#define	FILES_PREFIX		"zfone_"
	#define FILES_PREFIX_SIZE	6
	// .ddmmyy
	#define FILES_SUFFIX_SIZE	7		
	#define	FILES_END			".log"
	#define	FILES_END_SIZE		4
	#define FILES_NAME_SIZE		LOGGER_FILENAME_SIZE - FILES_PREFIX_SIZE - FILES_SUFFIX_SIZE - FILES_END_SIZE - FILES_DIR_SIZE - 1
//	#define FILES_DIR_SIZE		strlen(LOG_PATH)
#else
	#define FILES_NAME_SIZE		128
#endif //KERNEL

typedef enum log_error
{
    LOG_ERROR_PATH_TOO_LONG     = -6,
    LOG_ERROR_FILE_OPEN         = -5,
    LOG_ERROR_MAX_COUNT_EXCEED	= -4,
    LOG_ERROR_FILENAME_SIZE		= -3,
    LOG_ERROR_NO_LOG_FILES		= -2,
    LOG_ERROR_NOT_INITED		= -1,
    LOG_ERROR_NONE				=  0
} log_error_t;

/*!
 * \brief structure for log-element descrybing
 * Associate the log-file with log-levels lyst
 */
typedef struct logger_config_elem
{
#if  ZRTP_PLATFORM != ZP_WIN32_KERNEL
    char  file_name[LOGGER_FILENAME_SIZE];	/*!< file name ?!!!!*/
    FILE*	file;				/*!< unix file-pointer */
#elif ( PLATFORM == WIN32K )
    
#else
    #error "Platform not defined."
#endif
    int log_mode;
    unsigned int logged_messages;
}logger_config_elem_t;

/*!
 * \brief Max number of log-elements (files)
 * We don't want to use any dynamic memory allocation for log-elemnts storing, so change 
 * this value if more log-elements needed.
 */
#define LOG_ELEMS_COUNT 3

void zrtp_logger_reset();
int zrtp_init_loggers(char *path);

#if ZRTP_PLATFORM != ZP_WIN32_KERNEL
	log_error_t zrtp_logger_add_file(char *file_name, int level);
#endif

void zfone_log_write(int level, const char *buffer, int len);

void zrtp_print_log_delim(int level, log_delim_t delim, const char* name);
int  zrtp_print_log_delim_and_exit(int level, log_delim_t delim, const char* name, int status);
void zrtp_log_truncate();
void set_log_files_size(long size);

#if  ZRTP_PLATFORM != ZP_WIN32_KERNEL
    #define	LOG_BUFFER_SIZE		1024
#else
    #define	LOG_BUFFER_SIZE		512*1024
#endif

#define LOG_SCREEN_SIZE			80
#define LOG_CHECK_SIZE_COUNT	50

#if (ZRTP_PLATFORM == ZP_LINUX) || (ZRTP_PLATFORM == ZP_DARWIN)

#define PRINT_START_ERROR	zrtp_print_log_console

void zrtp_print_log_console(int level, char *format, ...);

#else

#define PRINT_START_ERROR	zrtp_print_log

#endif

#endif //__ZFONE_LOG_H__

