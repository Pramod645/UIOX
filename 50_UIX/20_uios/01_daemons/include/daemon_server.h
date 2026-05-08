#ifndef DAEMON_SERVER_H
#define DAEMON_SERVER_H

/*
 * daemon_server.h
 *
 * Server-process utilities for daemon processes —
 * based on APUE Section 13.7.
 *
 * When a daemon forks a child to service a client and then
 * exec's another program, open file descriptors inherited
 * from the parent can be a:
 *
 *   • Correctness problem — child inherits fds it doesn't need.
 *   • Security problem   — child program could abuse them.
 *
 * The solution is to set FD_CLOEXEC on all fds that the
 * exec'd program should not inherit.  FD_CLOEXEC causes the
 * kernel to close the fd automatically on execve().
 *
 * Additional utilities:
 *   • Determine the highest open fd (for bulk close/cloexec).
 *   • Fork a child to handle a client request.
 */

#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/*
 * set_cloexec()
 *
 * Sets the FD_CLOEXEC flag on file descriptor fd.
 * The fd is automatically closed when execve() is called.
 * (Figure 13.9 from the text.)
 *
 * Returns  0 on success.
 * Returns -1 on error (errno is set).
 */
int set_cloexec(int fd);

/*
 * close_on_exec_all()
 *
 * Sets FD_CLOEXEC on all file descriptors from fd_start
 * up to the system limit.  Used by a server before forking
 * a child that will exec another program.
 *
 * @param fd_start  First fd to mark (e.g. 3 to skip stdin/out/err).
 */
void close_on_exec_all(int fd_start);

/*
 * get_open_max()
 *
 * Returns the maximum number of open file descriptors for
 * the current process.  Uses getrlimit(RLIMIT_NOFILE).
 * Falls back to DAEMON_MAX_FD (1024) if the limit is
 * RLIM_INFINITY.
 */
int get_open_max(void);

/*
 * server_fork_client()
 *
 * Forks a child to service a client.  The child:
 *   1. Sets FD_CLOEXEC on all fds except conn_fd.
 *   2. Calls the provided service function with conn_fd.
 *   3. Exits when the service function returns.
 *
 * The parent closes conn_fd and returns immediately.
 *
 * @param conn_fd    Connected socket fd to pass to child.
 * @param service_fn Function the child calls to service client.
 */
void server_fork_client(int conn_fd,
                         void (*service_fn)(int conn_fd));

#endif /* DAEMON_SERVER_H */
