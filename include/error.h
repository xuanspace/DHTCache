/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
 *  Error codes returned.
 *
 * Author(s): wxlin    <weixuan.lin@sierraatlantic.com>
 *
 * $Id: error.h,v 1.3 2008-12-23 07:16:33 wxlin Exp $
 *
 */

#define ERR_OK         0
#define ERR_SYNTAX     1       /* syntax or usage error */
#define ERR_PROTOCOL   2       /* protocol incompatibility */
#define ERR_FILESELECT 3       /* errors selecting input/output files, dirs */
#define ERR_UNSUPPORTED 4      /* requested action not supported */
#define ERR_STARTCLIENT 5      /* error starting client-server protocol */

#define ERR_SOCKETIO   10      /* error in socket IO */
#define ERR_FILEIO     11      /* error in file IO */
#define ERR_STREAMIO   12      /* error in rsync protocol data stream */
#define ERR_MESSAGEIO  13      /* errors with program diagnostics */
#define ERR_IPC        14      /* error in IPC code */
#define ERR_CRASHED    15      /* sibling crashed */
#define ERR_TERMINATED 16      /* sibling terminated abnormally */

#define ERR_SIGNAL1    19      /* status returned when sent SIGUSR1 */
#define ERR_SIGNAL     20      /* status returned when sent SIGINT, SIGTERM, SIGHUP */
#define ERR_WAITCHILD  21      /* some error returned by waitpid() */
#define ERR_MALLOC     22      /* error allocating core memory buffers */
#define ERR_PARTIAL    23      /* partial transfer */
#define ERR_VANISHED   24      /* file(s) vanished on sender side */
#define ERR_DEL_LIMIT  25      /* skipped some deletes due to --max-delete */

#define ERR_TIMEOUT    30      /* timeout in data send/receive */
#define ERR_CONTIMEOUT 35      /* timeout waiting for daemon connection */

#define ERR_CMD_FAILED 124
#define ERR_CMD_KILLED 125
#define ERR_CMD_RUN 126
#define ERR_CMD_NOTFOUND 127
