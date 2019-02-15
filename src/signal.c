/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 * signal.c - signal handling
 *
  * Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
  *
 * $Id: signal.c,v 1.3 2008-12-23 07:16:33 wxlin Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/resource.h>

#if defined(linux) && defined(__SIGRTMIN)
/* the signal used by linuxthreads as exit signal for clone() threads */
# define SIG_PTHREAD_CANCEL (__SIGRTMIN+1)
#endif

/* SIGHUP handler */
void do_sighup( int signum )
{
	fprintf( stderr, "sigalrm sighup handlers\n" );
}

/* SIGTERM handler */
void do_sigterm( int signum )
{
	fprintf( stderr, "sigalrm sigterm handlers\n" );
}

/* SIGINT handler */
void do_sigint( int signum )
{
	fprintf( stderr, "sigint signal handlers\n" );
}

/* SIGALRM handler */
void do_sigalrm( int signum )
{
	fprintf( stderr, "sigalrm signal handlers\n" );
}

/* SIGCHLD handler */
void do_sigchld( int signum )
{
	fprintf( stderr, "sigchld signal handlers\n" );
}

/* SIGSEGV handler */
void do_sigsegv( int signum )
{
    fprintf( stderr, "crashed, please enable coredumps (ulimit -c unlimited) and restart.\n");
    abort();
}

/* SIGIO handler */
#ifdef HAVE_SIGINFO_T_SI_FD
void do_sigio( int signum, siginfo_t *si, void *x )
{
    do_signal( handler_sigio );
    do_change_notify( si->si_fd );
}
#endif

void init_signals(void)
{
    struct sigaction action;
    sigset_t blocked_sigset;

    sigemptyset( &blocked_sigset );
    sigaddset( &blocked_sigset, SIGCHLD );
    sigaddset( &blocked_sigset, SIGHUP );
    sigaddset( &blocked_sigset, SIGINT );
    sigaddset( &blocked_sigset, SIGALRM );
    sigaddset( &blocked_sigset, SIGIO );
    sigaddset( &blocked_sigset, SIGQUIT );
    sigaddset( &blocked_sigset, SIGTERM );
	
#ifdef SIG_PTHREAD_CANCEL
    sigaddset( &blocked_sigset, SIG_PTHREAD_CANCEL );
#endif
    action.sa_mask = blocked_sigset;
    action.sa_flags = 0;
    action.sa_handler = do_sigchld;
    sigaction( SIGCHLD, &action, NULL );
	
#ifdef SIG_PTHREAD_CANCEL
    sigaction( SIG_PTHREAD_CANCEL, &action, NULL );
#endif
    action.sa_handler = do_sighup;
    sigaction( SIGHUP, &action, NULL );
    action.sa_handler = do_sigint;
    sigaction( SIGINT, &action, NULL );
    action.sa_handler = do_sigalrm;
    sigaction( SIGALRM, &action, NULL );
    action.sa_handler = do_sigterm;
    sigaction( SIGQUIT, &action, NULL );
    sigaction( SIGTERM, &action, NULL );
	
    //if (core_dump_disabled())
    {
        action.sa_handler = do_sigsegv;
        sigaction( SIGSEGV, &action, NULL );
    }
	
    action.sa_handler = SIG_IGN;
    sigaction( SIGXFSZ, &action, NULL );	
#ifdef HAVE_SIGINFO_T_SI_FD
    action.sa_sigaction = do_sigio;
    action.sa_flags = SA_SIGINFO;
    sigaction( SIGIO, &action, NULL );
#endif
    return;

    fprintf( stderr, "failed to initialize signal handlers\n" );
    exit(1);
}
