/*
    pidfile.c - interact with pidfiles    
*/


#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

/* read_pid
 *
 * Reads the specified pidfile and returns the read pid.
 * 0 is returned if either there's no pidfile, it's empty
 * or no pid can be read.
 */
int read_pid (char *pidfile)
{
  FILE *f;
  int pid;

  if (!(f=fopen(pidfile,"r")))
    return 0;
  fscanf(f,"%d", &pid);
  fclose(f);
  return pid;
}

/* check_pid
 *
 * Reads the pid using read_pid and looks up the pid in the process
 * table (using /proc) to determine if the process already exists. If
 * so 1 is returned, otherwise 0.
 */
int check_pid (char *pidfile)
{
  int pid = read_pid(pidfile);

  /* Amazing ! _I_ am already holding the pid file... */
  if ((!pid) || (pid == getpid ()))
    return 0;

  /*
   * The 'standard' method of doing this is to try and do a 'fake' kill
   * of the process.  If an ESRCH error is returned the process cannot
   * be found -- GW
   */
  /* But... errno is usually changed only on error.. */
  errno = 0;
  if (kill(pid, 0) && errno == ESRCH)
	  return(0);

  return pid;
}

/* write_pid
 *
 * Writes the pid to the specified file. If that fails 0 is
 * returned, otherwise the pid.
 */
int write_pid (char *pidfile)
{
  FILE *f;
  int fd;
  int pid;

  if ( ((fd = open(pidfile, O_RDWR|O_CREAT, 0644)) == -1)
       || ((f = fdopen(fd, "r+")) == NULL) ) {
      fprintf(stderr, "Can't open or create %s.\n", pidfile ? pidfile : "(null)");
      return 0;
  }
  
#ifdef HAVE_FLOCK
  if (flock(fd, LOCK_EX|LOCK_NB) == -1) {
      fscanf(f, "%d", &pid);
      fclose(f);
      printf("Can't lock, lock is held by pid %d.\n", pid);
      return 0;
  }
#endif

  pid = getpid();
  if (!fprintf(f,"%d\n", pid)) {
      printf("Can't write pid , %s.\n", strerror(errno));
      close(fd);
      return 0;
  }
  fflush(f);

#ifdef HAVE_FLOCK
  if (flock(fd, LOCK_UN) == -1) {
      printf("Can't unlock pidfile %s, %s.\n", pidfile, strerror(errno));
      close(fd);
      return 0;
  }
#endif
  close(fd);

  return pid;
}

/* remove_pid
 *
 * Remove the the specified file. The result from unlink(2)
 * is returned
 */
int remove_pid (char *pidfile)
{
  return unlink (pidfile);
}
  
