#include "../include/daemon_server.h"

/*
 * get_open_max()
 *
 * Returns the maximum number of open file descriptors.
 * Uses getrlimit(RLIMIT_NOFILE).  If rlim_max is RLIM_INFINITY
 * (no system limit configured), falls back to 1024 — the same
 * cap used in daemonize() (APUE Figure 13.1).
 */
int get_open_max(void)
{
    struct rlimit rl;

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        syslog(LOG_ERR, "getrlimit(RLIMIT_NOFILE): %s",
               strerror(errno));
        return 1024;
    }

    if (rl.rlim_max == RLIM_INFINITY)
        return 1024;

    return (int)rl.rlim_max;
}

/*
 * set_cloexec()
 *
 * Sets the FD_CLOEXEC (close-on-exec) flag on fd.
 * (APUE Figure 13.9.)
 *
 * FD_CLOEXEC tells the kernel to close the descriptor
 * automatically when the process calls execve().  This
 * prevents child processes that exec another program from
 * inheriting file descriptors they should not have access to.
 *
 * Algorithm:
 *   1. F_GETFD  — read current flags.
 *   2. OR in FD_CLOEXEC.
 *   3. F_SETFD  — write flags back.
 */
int set_cloexec(int fd)
{
    int val;

    /* Read current file descriptor flags */
    if ((val = fcntl(fd, F_GETFD, 0)) < 0)
        return -1;

    /* Enable close-on-exec */
    val |= FD_CLOEXEC;

    /* Write flags back */
    return fcntl(fd, F_SETFD, val);
}

/*
 * close_on_exec_all()
 *
 * Sets FD_CLOEXEC on every open file descriptor from
 * fd_start up to the system open-fd limit.
 *
 * Typically called by a server just before forking a child
 * that will exec another program, passing fd_start = 3
 * (to leave stdin/stdout/stderr untouched).
 *
 * A descriptor that is already closed simply returns -1
 * from fcntl(F_GETFD); we silently skip it.
 */
void close_on_exec_all(int fd_start)
{
    int max = get_open_max();
    int fd;

    for (fd = fd_start; fd < max; fd++) {
        /*
         * set_cloexec() returns -1 if fd is not open;
         * that is normal — just skip closed descriptors.
         */
        (void)set_cloexec(fd);
    }
}

/*
 * server_fork_client()
 *
 * Forks a child process to handle one client connection.
 *
 * Parent:
 *   • Closes conn_fd (child has its own copy).
 *   • Returns immediately to accept new connections.
 *
 * Child:
 *   • Sets FD_CLOEXEC on every fd except conn_fd and
 *     stdin/stdout/stderr, so an exec'd service program
 *     cannot access the server's internal file descriptors.
 *   • Calls service_fn(conn_fd) to handle the client.
 *   • Exits when service_fn returns.
 *
 * This follows the client-server model described in §13.7.
 */
void server_fork_client(int conn_fd,
                         void (*service_fn)(int conn_fd))
{
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "server_fork_client: fork: %s",
               strerror(errno));
        return;
    }

    if (pid == 0) {
        /* ── Child process ─────────────────────────────── */

        /*
         * Mark all fds except 0,1,2 and conn_fd as
         * close-on-exec.  The server's listen socket,
         * config file descriptors, log file, etc. will all
         * be closed if the child exec's another program.
         */
        close_on_exec_all(3);
        /* Make sure conn_fd itself is NOT close-on-exec */
        {
            int val = fcntl(conn_fd, F_GETFD, 0);
            if (val >= 0) {
                val &= ~FD_CLOEXEC;
                fcntl(conn_fd, F_SETFD, val);
            }
        }

        /* Service the client */
        service_fn(conn_fd);
        close(conn_fd);
        exit(0);

    } else {
        /* ── Parent process ────────────────────────────── */
        /*
         * Close parent's copy of the connected socket.
         * The child still has it open.
         */
        close(conn_fd);
    }
}
