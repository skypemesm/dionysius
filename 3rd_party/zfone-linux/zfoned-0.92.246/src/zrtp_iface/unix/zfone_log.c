/*
 * Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
 * Contact: http://philzimmermann.com
 *
 * Nikolay Popok <chaser@soft-industry.com>
 */

#include <string.h>

#include <unistd.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>

#include <zrtp.h>
#include "zfone.h"

//#include "zrtp_iface/zfone_log.h"

#define TMP_FILE_NAME	"zfone_tmp.log"
#define LOG_TMP_SIZE 	strlen(TMP_FILE_NAME)
#define LOG_PATH_SIZE	128

typedef struct logger
{
    unsigned int loggers_count;
    /*!< log-elements list */
    logger_config_elem_t elems[LOG_ELEMS_COUNT];
    
    FILE 	 *tmp_log_file;
    char	 log_path[LOG_PATH_SIZE];
    unsigned int max_logfile_size;
} logger_t;


int find_files(char *name, char *out, unsigned int size);
int check_files(int days, unsigned long _length);

static FILE *cross_print(FILE *file, const char *buffer, int level);

// one object of logger_t for lib. Its used in all logging actions
static logger_t	logger;

// levels names for printing
static char *levels_names[] =
{
    "ERROR",
    "SECURITY",
    "DEBUG",
    NULL
};

char delim_symbol[LOG_DELIM_COUNT] =
{
    '\\',
    '/',
    '=',
    '=',
    ' ',
    '-',
    '-',
    '*'
};

//------------------------------------------------------------------------------
int zrtp_init_loggers(char *path)
{
    char buffer[LOGGER_FILENAME_SIZE];

    if (strlen(path) >= LOG_PATH_SIZE)
		return 0;

    strncpy(logger.log_path, path, sizeof(logger.log_path)-1);
    
    // copy path to tmp file
    strncpy(buffer, path, sizeof(buffer)-1);
    strncat(buffer, TMP_FILE_NAME, sizeof(buffer)-1);
    if ( !(logger.tmp_log_file = fopen(buffer, "w")) )
		return 0;
    
    return 1;
}

//------------------------------------------------------------------------------
void zrtp_log_truncate()
{
    unsigned int i;

    for (i=0; i<logger.loggers_count; i++)
    {
		if (logger.elems[i].file)
	    	truncate(logger.elems[i].file_name, 0);
    }
}

//------------------------------------------------------------------------------
/* adding new logger file */
log_error_t zrtp_logger_add_file(char *file, int level)
{
    int index, size;
    char *ptr;

    if ( logger.loggers_count >= LOG_ELEMS_COUNT )
		return LOG_ERROR_MAX_COUNT_EXCEED;

    index = logger.loggers_count++;
    ptr = logger.elems[index].file_name;
	size = sizeof(logger.elems[index].file_name) - 1;
    strncpy(ptr, logger.log_path, size);
    /* "zfone_" */
    strncat(ptr, FILES_PREFIX, size);
    /* now coping file name */
    strncat(ptr, file, size);
    strncat(ptr, FILES_END, size);

 	if (level > 3 || level < 1)
		level = 3;
    logger.elems[index].log_mode = level;

    /* open log file */
    if ( !(logger.elems[index].file = fopen(ptr, "a")) )
    {
		return LOG_ERROR_FILE_OPEN;
    }

    logger.elems[index].logged_messages = 0;
    
	return LOG_ERROR_NONE;
}

//------------------------------------------------------------------------------
/* prints message to file in format "current_time: level: message" */
static FILE *cross_print(FILE *file, const char *buffer, int level)
{
#ifdef WIN32
    SYSTEMTIME st;
#else
    struct tm t;                                                                
    struct timeval tv;                                                          
#endif

    if (!file)
	return NULL;

#ifdef WIN32
    GetLocalTime(&st);
					                                                                                
    if (!fprintf(file, "%02i:%02i:%02i %s: %s"
	,st.wHour
	,st.wMinute
	,st.wSecond
	,levels_names[level-1]
	,buffer)
    )
    {
	// TODO: file recreation
    }
#else
    gettimeofday(&tv, 0);                                                       
    localtime_r((const time_t*)&tv.tv_sec, &t);                                 
					                                                                                
    if (!fprintf(file, "%02i:%02i:%02i %s: %s"
	,t.tm_hour
	,t.tm_min
	,t.tm_sec
	,levels_names[level-1]
	,buffer
	))
    {
	// TODO: file recreation
    }
#endif
    fflush(file);
    return file;
}

void check_log_by_size(logger_t *l, int index)
{
    logger_config_elem_t *elem = &l->elems[index];

    if ( !elem || !elem->file )
	return;

    if ( ftell(elem->file) > (long)l->max_logfile_size * 1024 )
    {
		truncate(elem->file_name, 0);
    }

    return;
}

//------------------------------------------------------------------------------
/* prints message to log files */
void zfone_log_write(int level, const char *buffer, int len)
{
    unsigned int i;
    int found = 0;

	if (level > 3)
		level = 3;
	if (level < 1)
		return;
	if (level == 3 && !zfone_cfg.params.print_debug)
		return;
	
    /* if there are no open log files, we'll use default one */
    if ( !logger.loggers_count && logger.tmp_log_file )
    {
		cross_print(logger.tmp_log_file, buffer, level);
		return;
    }

    /* otherwise find appropriate file and print into it */
    for (i = 0; i < logger.loggers_count; i++)
    {
		if (level <= logger.elems[i].log_mode) 
		{
		    if (logger.elems[i].logged_messages > LOG_CHECK_SIZE_COUNT)
		    {
				logger.elems[i].logged_messages = 0;
				check_log_by_size(&logger, i);
	  	    }
	  	    cross_print(logger.elems[i].file, buffer, level);
		    logger.elems[i].logged_messages++;
		    found++;
		}
    }
}

//------------------------------------------------------------------------------
/* closes all log files  */
void zrtp_logger_reset()
{
    unsigned int i;
    for (i = 0; i < logger.loggers_count; i++)
    {
		if ( logger.elems[i].file )
		{
			fclose(logger.elems[i].file);
			logger.elems[i].file = NULL;
			logger.elems[i].log_mode = 0;
		}
    }
    logger.loggers_count = 0;
    if (logger.tmp_log_file)
    {
		fclose(logger.tmp_log_file);
		logger.tmp_log_file = NULL;
    }
}

void zrtp_print_log_delim(int level, log_delim_t delim, const char* name)
{
    int	 name_len;
    int  delim_elem_count, offset;
    int  spaces = 0;
    int  screen_size;
    int  extra = 0;

    char buffer[LOG_SCREEN_SIZE + 1];
    
    if ( !name )
		name_len = 0;
    else
		name_len = strlen(name);

    if (name_len)
		spaces = 2;

    screen_size = LOG_SCREEN_SIZE - 11 - strlen(levels_names[level-1]);

    if (delim >= LOG_DELIM_COUNT)
		return;

    if (name_len > screen_size)
    {
		delim_elem_count = 3;
		name_len = screen_size - 3 * 2 - spaces;
    }
    else
    {
		delim_elem_count = (screen_size - name_len - spaces) / 2;
		if ((screen_size - name_len - spaces) % 2)
		{
			extra = 1;
		}
    }
    
    switch (delim)
    {
		case LOG_LABEL:
		{
			memset(buffer, delim_symbol[delim], screen_size);
			buffer[screen_size] = '\n';
			buffer[screen_size + 1] = 0;
			zfone_log_write(level, buffer, LOG_SCREEN_SIZE+1);   
			memset(buffer, delim_symbol[delim], delim_elem_count);
			if (spaces)
			{
				buffer[delim_elem_count]   = ' ';
				buffer[delim_elem_count+1] = 0;
			}
			else
				buffer[delim_elem_count] = 0;
			if ( name )
				strncat(buffer, name, name_len);

			offset = delim_elem_count + name_len;
			if (spaces)
			{
				buffer[++offset] = ' ';
				offset++;
			}
			memset(buffer + offset, delim_symbol[delim], delim_elem_count + extra);
			buffer[offset + delim_elem_count + extra] = '\n';
			buffer[offset + delim_elem_count + extra +1] = '\0';
			zfone_log_write(level, buffer, LOG_SCREEN_SIZE+1);
			memset(buffer, delim_symbol[delim], screen_size);
			buffer[screen_size] = '\n';
			buffer[screen_size + 1] = 0;
			zfone_log_write(level, buffer, LOG_SCREEN_SIZE+1);
			break;
		}
		case LOG_SPACE:
		{
			zfone_log_write(level, "\n", 1);
			zfone_log_write(level, "\n", 1);
			break;
		}
		default:
		{
			if ((delim == LOG_START_SECTION) || (delim == LOG_START_SUBSECTION))
				zfone_log_write(level, "\n", 1);
	    
			memset(buffer, delim_symbol[delim], delim_elem_count);
			if (spaces)
			{
				buffer[delim_elem_count]   = ' ';
				buffer[delim_elem_count+1] = 0;
			}
			else
				buffer[delim_elem_count] = 0;
	
			if ( name )
				strncat(buffer, name, name_len);
	
			offset = delim_elem_count + name_len;
			if (spaces)
			{
				buffer[++offset] = ' ';
				offset++;
			}
			memset(buffer + offset, delim_symbol[delim], delim_elem_count + extra);
			buffer[offset + delim_elem_count + extra] = '\n';
			buffer[offset + delim_elem_count + extra +1] = '\0';
			zfone_log_write(level, buffer, sizeof(buffer));
	    
			if ((delim == LOG_END_SECTION) || (delim == LOG_END_SUBSECTION))
				zfone_log_write(level, "\n", 1);
		}
    }
}

int zrtp_print_log_delim_and_exit(int level, log_delim_t delem, const char* name, int status)
{
    zrtp_print_log_delim(level, delem, name);
    return status;
}

void set_log_files_size(long size)
{
    logger.max_logfile_size = size;
}

#if (ZRTP_PLATFORM == ZP_LINUX) || (ZRTP_PLATFORM == ZP_DARWIN)

void zrtp_print_log_console(int level, char *format, ...)
{
    char buffer[LOG_BUFFER_SIZE];
    va_list arg;
	
    va_start(arg, format);
    vsnprintf(buffer, LOG_BUFFER_SIZE, format, arg);
    va_end( arg );
    
    printf("[ERROR]:%s", buffer);
	zfone_log_write(level, buffer, LOG_BUFFER_SIZE);
    return;
}

#endif
