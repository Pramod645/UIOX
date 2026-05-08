#ifndef DAEMON_H
#define DAEMON_H

/*
 * daemon.h
 *
 * Daemon process initialization — based on APUE Chapter 13.
 *
 * Rules implemented by daemonize():
 *   1. umask(0)          — clear file creation mask
 *   2. fork + exit       — parent exits, child continues
 *   3. setsid()          — create new session, lose controlling TTY
 *   4. fork + exit       — guarantee not session leader (System V)
 *   5. chdir("/")        — prevent blocking mounted filesystems
 *   6. close all fds     — release inherited descriptors
 *   7. open /dev/null    — redirect stdin/stdout/stderr
 *   8. openlog()         — initialize syslog connection
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Default working directory after daemonization */
#define DAEMON_WORK_DIR     "/"

/* Default /dev/null path used to redirect standard fds */
#define DAEMON_DEV_NULL     "/dev/null"

/* Maximum file descriptors to close if rlimit is RLIM_INFINITY */
#define DAEMON_MAX_FD       1024

/*
 * daemonize()
 *
 * Initializes the calling process as a proper UNIX daemon.
 *
 * @param cmd  Name of the daemon (used as syslog ident string).
 *
 * This function never returns on error — it calls exit(1).
 * On success the calling process has been transformed into a
 * fully detached background daemon.
 */
void daemonize(const char *cmd);

#endif /* DAEMON_H */
