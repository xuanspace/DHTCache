/*
 * Kad Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * DHT Cache main
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 * $Id: main.c 4655 2009-07-27 11:25:59Z wxlin $
 * 
 */

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include "option.h"

void help(int error) 
{
	fprintf(error ? stderr : stdout,
		"usage: kadcache [-bhmpvVw] [-c [file]] | [file...]\n"
	);
}

void version() 
{
	printf("KadCache 1.1.0 Copyright 2008, 2009 wxlin.\n");
}

int daemon(int nochdir, int noclose)
{
    int fd; 

    switch (fork()) {
		case -1:
			return (-1);
		case 0: 
			break;  
		default:
			_exit(0);
    }

    if (setsid() == -1)
        return (-1);

    if (!nochdir)
        (void)chdir(¡±/¡±);

    if (!noclose && (fd = open(¡±/dev/null¡±, O_RDWR, 0)) != -1) {
        (void)dup2(fd, STDIN_FILENO);
        (void)dup2(fd, STDOUT_FILENO);
        (void)dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO)
            (void)close(fd);
    }
    return (0);
}

int set_maximum_open_files(uint32 max_file_limit)
{
	int retval;
	struct rlimit rlimit;
	ulong old_cur;

	retval = getrlimit(RLIMIT_NOFILE,&rlimit)
	if (retval == 0) {
		old_cur = rlimit.rlim_cur;
		if (rlimit.rlim_cur >= max_file_limit)
			return rlimit.rlim_cur;
			
		rlimit.rlim_cur = rlimit.rlim_max = max_file_limit;
		retval = setrlimit(RLIMIT_NOFILE,&rlimit);
		if (retval != 0) {
			LOG("Warning: setrlimit couldn't set max open files %ld",rlimit.rlim_max);
			max_file_limit = old_cur;
		}
		else{
			getrlimit(RLIMIT_NOFILE,&rlimit);
			if (rlimit.rlim_cur != max_file_limit)
				LOG("Warning: setrlimit ok, but max open file limits is %ld", rlimit.rlim_cur);
			max_file_limit = rlimit.rlim_cur;
		}
	}	
	return max_file_limit;
}

int core_dump_disabled(void)
{
    struct rlimit rlimit;

    if (getrlimit(RLIMIT_CORE, &rlimit){
		LOG("Warning: getrlimit couldn't get core size.\n");
	}
	else{
		if (rlimit.rlim_cur < RLIM_INFINITY) {
			rlimit.rlim_cur = rlimit.rlim_max = RLIM_INFINITY;
			if(setrlimit(RLIMIT_CORE, &rlimit))
				LOG("Warning: setrlimit core size infinity failure.\n");
			else
				return 0;
		}	
	}
	return -1;
}

void get_options(int argc,char **argv,struct options *opt)
{
	/* read options */
	memset(&opt, 0, sizeof(struct options));
	while((ret = getopt(argc, argv, "bchmpvVw")) >= 0) {
		switch(ret) {
		case 'b':
			break;
		case 'c':
			digest = 1;
			break;
		case 'm':
			break;
		case 'p':
			break;
		case 'v':
			opt.verbose = 1;
			break;
		case 'w':
			opt.warn = 1;
			break;
		case 'V':
			version();
			quit = 1;
			break;
		case 'h':
			help(0);
			quit = 1;
			break;
		case '?':
			help(1);
			return 2;
		default:
			/* shouldn't happen */
			return -1;
		}
	}
}

int bootstrap()
{
	while(1){
		usleep(10000);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	int quit = 0;
	struct options opt;

	/* signal handle*/
	init_signals(void)
	
	/* open syslog*/
	openlog("kadcache", LOG_CONS | LOG_PID, 0);
	syslog(LOG_INFO | LOG_PID, "kadcache start\n");

	/* read options */	
	get_options(argc,argv);
	
	/* boot server*/
	bootstrap();
	
	/* close syslog*/
	closelog();
	
	/* quit main*/
	if(quit)
		return 0;

	return 0;
}