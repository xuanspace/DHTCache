#include "includes.h"
#include "xmalloc.h"
#include "ssh.h"

static int log_debug = 0;
static int log_quiet = 0;

/* Name of the host we are running on. This is used in debug()
   and initialized in log_init() */
#ifdef HAVE_GETHOSTNAME
static char local_hostname[257];
#endif

void log_init(char *av0, int on_stderr, int debug, int quiet,
              SyslogFacility facility)
{
  log_debug = debug;
  log_quiet = quiet;

  /* Get our own hostname */
#ifdef HAVE_GETHOSTNAME
  if (gethostname(local_hostname, sizeof(local_hostname)) < 0)
    fatal("gethostname: %.100s", strerror(errno));
#endif
}

void log_msg(const char *fmt, ...)
{
  va_list args;

  if (log_quiet)
    return;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\r\n");
  va_end(args);
}

void log_severity(SyslogSeverity severity, const char *fmt, ...)
{
  va_list args;

  if (log_quiet)
    return;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\r\n");
  va_end(args);
}

void debug(const char *fmt, ...)
{
  va_list args;
  if (log_quiet || !log_debug)
    return;
#if defined(HAVE_GETHOSTNAME) && defined(LOCAL_HOSTNAME_IN_DEBUG)
  fprintf(stderr, "%s: ", local_hostname);
#endif
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\r\n");
  va_end(args);
}

void error(const char *fmt, ...)
{
  va_list args;
  if (log_quiet)
    return;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\r\n");
  va_end(args);
}

struct fatal_cleanup
{
  struct fatal_cleanup *next;
  void (*proc)(void *);
  void *context;
};

static struct fatal_cleanup *fatal_cleanups = NULL;

/* Registers a cleanup function to be called by fatal() before exiting. */

void fatal_add_cleanup(void (*proc)(void *), void *context)
{
  struct fatal_cleanup *cu;

  cu = xmalloc(sizeof(*cu));
  cu->proc = proc;
  cu->context = context;
  cu->next = fatal_cleanups;
  fatal_cleanups = cu;
}

/* Removes a cleanup frunction to be called at fatal(). */

void fatal_remove_cleanup(void (*proc)(void *context), void *context)
{
  struct fatal_cleanup **cup, *cu;

  for (cup = &fatal_cleanups; *cup; cup = &cu->next)
    {
      cu = *cup;
      if (cu->proc == proc && cu->context == context)
        {
          *cup = cu->next;
          xfree(cu);
          return;
        }
    }
  fatal("fatal_remove_cleanup: no such cleanup function: 0x%lx 0x%lx\n",
        (unsigned long)proc, (unsigned long)context);
}

/* Executes fatal() cleanups. */

static void do_fatal_cleanups(void)
{
  struct fatal_cleanup *cu, *next_cu;
  static int fatal_called = 0;

  if (!fatal_called)
    {
      fatal_called = 1;

      /* Call cleanup functions. */
      for (cu = fatal_cleanups; cu; cu = next_cu)
        {
          next_cu = cu->next;
          (*cu->proc)(cu->context);
        }
    }
}

/* Function to display an error message and exit.  This is in this file because
   this needs to restore terminal modes before exiting.  See log-client.c
   for other related functions. */

void fatal(const char *fmt, ...)
{
  va_list args;

  do_fatal_cleanups();

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(255);
}

void fatal_severity(SyslogSeverity severity, const char *fmt, ...)
{
  va_list args;

  do_fatal_cleanups();

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(255);
}
