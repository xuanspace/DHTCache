/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
* log.c - implement logging functions
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <execinfo.h>
#include <syslog.h>
#include <pthread.h>
#include<sys/stat.h>
#include<sys/types.h>
#include "log.h"

/*
 * limitation of currently open log files.
 */
#define MAX_LOGFILES 	128
#define LOGMSG_LEN  	2048

/*
 * Syslog support.
 */
static int sysloglevel;
static int dosyslog = 0;

inline int lock_file(struct logfile* log)
{
	return pthread_mutex_lock(&log->lock);
}

inline int unlock_file(struct logfile* log)
{
	return pthread_mutex_unlock(&log->lock);
}

struct logfile* log_alloc(char* filename)
{
    struct logfile *log = NULL;
	int size = sizeof(struct logfile);
    log = (struct logfile*)malloc(size);
	
	if(log != NULL){
		memset(log,0,size);
		log->file_path = strdup(".");
		log->filename = strdup(filename);
		log->file_size = 0;
		log->max_size = 500000;
		log->max_rollover = 3;
		pthread_mutex_init(&log->lock,NULL);
	}
	return log;
}

void log_close(struct logfile* log)
{
	if(log != NULL){
		/* destroy rwlock */
		pthread_mutex_destroy(&log->lock);
		if(log->file_path)
			free(log->file_path);
		if(log->filename)
			free(log->filename);
		free(log);
	}
}

void log_set_level(struct logfile* log,int level)
{
    /* change output level */
	log->output_level = level;
}

void log_set_syslog(const char *ident, int syslog_level)
{
    if (ident == NULL)
        dosyslog = 0;
    else {
        dosyslog = 1;
        sysloglevel = syslog_level;
        openlog(ident, LOG_PID, LOG_DAEMON);
    }
}

int rolloever_files(struct logfile *log)
{
	int i;
    char src[FILENAME_MAX+1] = {0};
    char dst[FILENAME_MAX+1] = {0};

    for(i = log->max_rollover- 1; i >= 1; --i){
		snprintf (src, FILENAME_MAX,
			"%s.%d",
			log->filename,
			i);
		snprintf (dst, FILENAME_MAX,
			"%s.%d",
			log->filename,
			i + 1);
		rename (src, dst);
    }
    return 0;
}

int check_file_size(struct logfile *log)
{
    if(log->file_size >= log->max_size)
    {
		fclose(log->file);

		// Backup file, switch file
		if (log->max_rollover > 0)
		{
			rolloever_files(log);
			char dst[FILENAME_MAX + 1] = {0};
			snprintf (dst, FILENAME_MAX,
				"%s.%d",
				log->filename,
				1);
			rename (log->filename, dst);
		}
		log->file = fopen (log->filename, "wb");
		if (log->file == NULL){
			syslog(LOG_INFO, "Open file failed : %s\n",
				strerror (errno));
		}
		log->file_size = 0;
		return 1;
    }
    return 0;
}

struct logfile* log_open(char *filename,int level)
{
    FILE *f = NULL;
	struct logfile* log = NULL;
	
	/* Check file name length*/
    if (strlen(filename) > FILENAME_MAX) {
		syslog(LOG_INFO,"Log filename too long: `%s'.", filename);
        return 0;
    }

	/* Create a logfile struct */
	log = log_alloc(filename);
	if(log == NULL){
		syslog(LOG_INFO,"Fail to alloc struct for %s", filename);
		return log;
	}
		
    /* Check and create dir */
    if (mkdir(log->file_path, 0755) == -1){
		if (errno != EEXIST)
			return log;
	}
	
	/*Get file size. If the file is exsit and its size is out of */
    struct stat bf;
    memset (&bf, 0, sizeof(bf));
    int create_new_file = 1;
    if (stat (filename, &bf) == 0)
    {
		log->file_size = bf.st_size;
		if (check_file_size(log))
	    create_new_file = 0;
    }
	
    /* if not previously opened, then open it now */
    if (create_new_file) {
        f = fopen(filename, "a");
        if (f == NULL) {
			syslog(LOG_INFO,"Couldn't open logfile `%s'.", filename);
            return log;
        }
    }

    log->file = f;
	log->start_time = time(NULL);
    log->output_level = level;

    return log;
}
 
int log_check(struct logfile *log,int level)
{
	// Check log file handle
	if (log->file == NULL || 
		level < log->output_level)
		return -1;
		
	// Check log file size/rollover
    check_file_size(log);
	
	// Check log file time peroid
    if (log->file_period) {
		;
    }
	return 0;
} 

int log_prefix(char *buf, int size, int timestamp)
{
	int len;
	time_t now;
	struct tm* tm;
	
	time(&now);
	tm = localtime(&now);
	if (timestamp) {
		len = strftime(buf, size, "[%b %d %H:%M:%S] ", tm);
	}
	return len;
}
 
void log_output(struct logfile *log,char* buf,size_t size)
{
	buf[size] = '\0';
	lock_file(log);
	if(fwrite(buf,size,1,log->file) <= 0){
		fclose(log->file);
	}else{
		fflush(log->file);
		log->file_size += size;
	}
	unlock_file(log);
}

int log_write(struct logfile *log,int level, char *fmt,...)
{
	int size,len;
	va_list args;
	char buf[LOGMSG_LEN+1];
	char *pbuf = buf;
	
	log_check(log,level);
	size = LOGMSG_LEN;
	len = log_prefix(buf,size,1);
	pbuf += len; size -= len;
	va_start(args, fmt);
	len = vsnprintf(pbuf, size, fmt, args);
	va_end(args);
	pbuf += len; size = pbuf-buf;
	log_output(log,buf,size);
	return size;
}

int format_backtrace(char* buf,int len)
{
	int n = 0;
    size_t size, i;
    char **strings;
    void *stack_frames[50];

    size = backtrace(stack_frames, sizeof(stack_frames)/sizeof(void*));
    strings = backtrace_symbols(stack_frames, size);
    if (strings) {
        n += sprintf(buf, "backtrace:\n");
        for (i = 0; i < size; i++)
            n += sprintf(buf, "    #%d %s\n", i,strings[i]);
        free (strings);
    }
	return n;
}

int format_hexdump ( char *buf,int size,char *obuf,int osize)
{
    unsigned char c;
    char textver[16 + 1];

    // We can fit 16 bytes output in text mode per line, 4 chars per byte.
    int maxlen = (osize/(68+20)) * 16;

    if (size > maxlen)
        size = maxlen;

    int i;
    int lines = size/16;
    for (i = 0; i < lines; i++){
        int j;
		sprintf(obuf,"%20c",' ');
		obuf += 20;
        for (j = 0 ; j < 16; j++){
            c = (unsigned char) buf[(i << 4) + j];    // or, buf[i*16+j]
            sprintf(obuf,"%02x ",c);
            obuf += 3;
            if (j == 7){
                sprintf(obuf," ");
                ++obuf;
            }
            textver[j] = isprint (c) ? c : '.';
        }

        textver[j] = 0;
        sprintf (obuf,"  %s\n",textver);
        while (*obuf != '\0')
            ++obuf;
    }

    if (size % 16){
        for (i = 0 ; i < size % 16; i++){
            c = (unsigned char) buf[size - size % 16 + i];
            sprintf (obuf,"%02x ",c);
            obuf += 3;
            if (i == 7){
                sprintf (obuf," ");
                ++obuf;
            }
            textver[i] = isprint (c) ? c : '.';
        }

        for (i = size % 16; i < 16; i++){
            sprintf (obuf,"   ");
            obuf += 3;
            if (i == 7){
                sprintf (obuf," ");
                ++obuf;
            }
            textver[i] = ' ';
        }

        textver[i] = 0;
        sprintf (obuf,"  %s\n",textver);
    }
    return size;
}

#define LOG_WRITE(log,buf,size)				\
	pbuf += len; size -= len;				\
	va_start(args, fmt);					\
	len = vsnprintf(pbuf,size,fmt, args);	\
	va_end(args);							\
	pbuf += len; size = pbuf-buf;			\
	log_output(log,buf,len);
	
	
void log_backtrace(struct logfile *log, char *fmt, ...)
{
	int size,len;
	va_list args; 
	char buf[LOGMSG_LEN+1];
	char *pbuf = buf;	
	
	log_check(log,BACKTRACE);
	size = LOGMSG_LEN-1;
	len = log_prefix(buf,size,1);
	
	pbuf += len; size -= len;
	va_start(args, fmt);
	len = vsnprintf(pbuf,size,fmt, args);
	va_end(args);
	
	pbuf += len; size -= len;
	len = format_backtrace(pbuf, size);
	pbuf += len; size = pbuf-buf;
	log_output(log,buf,size);
}

void log_panic(struct logfile *log, char *fmt, ...)
{
	int size,len;
	va_list args; 
	char buf[LOGMSG_LEN+1];
	char *pbuf = buf;	
	
	log_check(log,ERROR);
	size = LOGMSG_LEN;
	len = log_prefix(pbuf,size,1);
	pbuf += len; size -= len;	
	len = snprintf(pbuf, size, "(%s:%u,%s) error %d,%s. ",
		__FILE__,__LINE__,__FUNCTION__,errno, strerror(errno));
		
	va_start(args, fmt);
	len = vsnprintf(pbuf,size,fmt, args);
	va_end(args);
	pbuf += len; size = pbuf-buf;
	log_output(log,buf,size);	
}

void log_error(struct logfile *log, char *fmt, ...)
{
	int size,len;
	va_list args; 
	char buf[LOGMSG_LEN+1];
	char *pbuf = buf;	
	
	log_check(log,ERROR);
	size = LOGMSG_LEN;
	len = log_prefix(pbuf,size,1);
	pbuf += len; size -= len;	
	len = snprintf(pbuf, size, "(%s:%u,%s) error %d,%s. ",
		__FILE__,__LINE__,__FUNCTION__,errno, strerror(errno));
		
	va_start(args, fmt);
	len = vsnprintf(pbuf,size,fmt, args);
	va_end(args);
	pbuf += len; size = pbuf-buf;
	log_output(log,buf,size);	
}

void log_warning(struct logfile *log, char *fmt, ...)
{
	int size,len;
	va_list args; 
	char buf[LOGMSG_LEN+1];
	char *pbuf = buf;	
	
	log_check(log,WARNING);
	size = LOGMSG_LEN;
	len = log_prefix(pbuf,size,1);
	pbuf += len; size -= len;	
		
	va_start(args, fmt);
	len = vsnprintf(pbuf,size,fmt, args);
	va_end(args);
	pbuf += len; size = pbuf-buf;
	log_output(log,buf,size);
}

void log_info(struct logfile *log, char *fmt, ...)
{
	int size,len;
	va_list args; 
	char buf[LOGMSG_LEN+1];
	char *pbuf = buf;	
	
	size = LOGMSG_LEN;
	log_check(log,INFO);
	len = log_prefix(pbuf,size,1);
	pbuf += len; size -= len;

	va_start(args, fmt);
	len = vsnprintf(pbuf,size,fmt, args);
	va_end(args);
	pbuf += len; size = pbuf-buf;
	log_output(log,buf,size);
}

void log_debug(struct logfile *log, char *fmt, ...)
{
	int size,len;
	va_list args; 
	char buf[LOGMSG_LEN+1];
	char *pbuf = buf;	
	
	size = LOGMSG_LEN;
	log_check(log,DEBUG);
	len = log_prefix(pbuf,size,1);
	pbuf += len; size -= len;
	len = snprintf(pbuf, size, "(%s:%u,%s) ",
			__FILE__,__LINE__,__FUNCTION__);			
	pbuf += len; size -= len;
	
	va_start(args, fmt);
	len = vsnprintf(pbuf,size,fmt, args);
	va_end(args);
	pbuf += len; size = pbuf - buf;
	log_output(log,buf,size);
}

void log_hexdump (struct logfile *log, char *buf, int bytes)
{
	int size,len;
	char obuf[LOGMSG_LEN+1];
	char *pbuf = obuf;	
	
	size = LOGMSG_LEN;
	log_check(log,DEBUG);
	len = log_prefix(pbuf,size,1);
	pbuf += len; size -= len;
	
    len = snprintf(pbuf,size,"buffer[0x%x] - HEXDUMP %d bytes\n",(size_t)buf,size);
    pbuf += len; size -= len;	
    len = format_hexdump(buf, bytes, pbuf, size);
	pbuf += len; size = pbuf-obuf;
	obuf[size-1] = '\n';
	log_output(log,obuf,size);
}

void log_syslog(char *format, va_list args, int level)
{
    char buf[4096]; /* Trying to log more than 4K could be bad */
    int translog;

    if (level >= sysloglevel && dosyslog) {
        if (args == NULL) {
            strncpy(buf, format, sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
        } else {
            vsnprintf(buf, sizeof(buf), format, args);
        }

        switch(level) {
        case DEBUG:
            translog = LOG_DEBUG;
            break;
        case INFO:
            translog = LOG_INFO;
            break;
        case WARNING:
            translog = LOG_WARNING;
            break;
        case ERROR:
            translog = LOG_ERR;
            break;
        case PANIC:
            translog = LOG_ALERT;
            break;
        default:
            translog = LOG_INFO;
            break;
        }
        syslog(translog, "%s", buf);
    }
}
