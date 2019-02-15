/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * log.h - logging functions
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Symbolic levels for output levels. */
enum log_level {
	DEBUG, 
	BACKTRACE,
	TRACE,
	INFO, 
	WARNING, 
	ERROR, 
	FATAL,
	PANIC, 
	GLOBAL
};

/* Log flags for output file. */
enum log_flag {
	PID, 
	TID, 
	TIME
};

/* Structrue for logging. */
struct logfile{
    FILE *file;
    int output_level;
	char *file_path;
	int file_size;
	int max_size;
	int max_rollover;
	time_t start_time;
	time_t file_period;
    char* filename;
    pthread_mutex_t lock;
	int terminated;
};

/* Structrue for logging config. */
struct logconf{
	int sysloglevel;
	int dosyslog;
    char* logname;
    int max_rollover;
	int terminated;
};

/* Open the log file */
struct logfile* log_open(char *filename, int level);

/* Close the log file*/
void log_close(struct logfile*);

/* Print a panicky error message and terminate the program with a failure. */
void log_panic(struct logfile*, char *, ...);

/* Print a normal error message. */
void log_error(struct logfile*, char *, ...);

/* Print a warning message. */
void log_warning(struct logfile*, char *, ...);

/* Print an informational message. */
void log_info(struct logfile*, char *, ...);

/* Print a debug message. */
void log_debug(struct logfile*, char *, ...);

/* Print a buffer hex string. */
void log_hexdump (struct logfile *log, char *buf, int size);

/* Set minimum level for output messages to stderr. */
void log_set_level(struct logfile*, int level);

#endif
