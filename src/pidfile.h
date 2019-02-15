/*
    pidfile.h - interact with pidfiles
*/

#if __cplusplus
extern "C" {
#endif

/* read_pid
 *
 * Reads the specified pidfile and returns the read pid.
 * 0 is returned if either there's no pidfile, it's empty
 * or no pid can be read.
 */
int read_pid (char *pidfile);

/* check_pid
 *
 * Reads the pid using read_pid and looks up the pid in the process
 * table (using /proc) to determine if the process already exists. If
 * so 1 is returned, otherwise 0.
 */
int check_pid (char *pidfile);

/* write_pid
 *
 * Writes the pid to the specified file. If that fails 0 is
 * returned, otherwise the pid.
 */
int write_pid (char *pidfile);

/* remove_pid
 *
 * Remove the the specified file. The result from unlink(2)
 * is returned
 */
int remove_pid (char *pidfile);

#if __cplusplus
}
#endif
