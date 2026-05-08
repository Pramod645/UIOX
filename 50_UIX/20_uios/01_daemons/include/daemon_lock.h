#ifndef DAEMON_LOCK_H
#define DAEMON_LOCK_H

/*
 * daemon_lock.h
 *
 * Single-instance daemon enforcement — based on APUE Section 13.5.
 *
 * Only one copy of a daemon should run at a time.  This is
 * enforced using a PID lock file:
 *
 *   • The daemon creates /var/run/<name>.pid
 *   • It places a write lock (F_WRLCK) on the entire file.
 *   • Only one write lock is allowed at a time — successive
 *     attempts fail with EACCES or EAGAIN.
 *   • The lock is automatically released when the process exits,
 *     so no cleanup is needed.
 *   • The daemon's PID is written into the file so administrators
 *     can identify it.
 *
 * Conventions (Section 13.6):
 *   Lock file : /var/run/<name>.pid
 *   Config    : /etc/<name>.conf
 *   Init      : /etc/init.d/<name>  or  /etc/rc*
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* Default lock file path (APUE Figure 13.6) */
#define DAEMON_LOCKFILE     "/var/run/daemon.pid"

/* Lock file permissions: rw-r--r-- */
#define DAEMON_LOCKMODE     (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/*
 * lockfile()
 *
 * Places a POSIX write lock (F_WRLCK) on the entire file
 * referred to by fd, using F_SETLK (non-blocking).
 *
 * Returns  0 on success (lock acquired).
 * Returns -1 on failure (errno = EACCES or EAGAIN means
 *             another instance is running).
 */
int lockfile(int fd);

/*
 * already_running()
 *
 * Checks whether another instance of this daemon is running
 * by opening the lock file and attempting to lock it.
 *
 * Returns 0  — this is the only running instance.
 *              PID written to lock file.
 * Returns 1  — another instance is already running.
 *
 * Exits (via syslog + exit(1)) on unexpected errors.
 */
int already_running(void);

/*
 * already_running_named()
 *
 * Like already_running() but uses a caller-supplied lock
 * file path instead of the default DAEMON_LOCKFILE.
 *
 * @param lockpath  Path to the PID lock file.
 */
int already_running_named(const char *lockpath);

#endif /* DAEMON_LOCK_H */
