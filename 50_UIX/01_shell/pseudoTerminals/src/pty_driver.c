#include "../include/pty_driver.h"
#include <sys/socket.h>

/*
 * fd_pipe() — create a full-duplex pipe using UNIX domain sockets.
 *
 * A regular pipe created with pipe() is half-duplex: fd[0] is
 * read-only, fd[1] is write-only.
 *
 * socketpair(AF_UNIX, SOCK_STREAM, 0, fd) creates two connected
 * sockets that are both readable and writable — a full-duplex pipe.
 *
 * This allows the driver and pty to use a single pair of fds for
 * bidirectional communication, rather than two separate pipes.
 */
int fd_pipe(int fd[2])
{
    return socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
}

/*
 * do_driver() — Figure 19.16 (APUE §19.6)
 *
 * Connects pty's stdin/stdout to a driver subprocess.
 *
 * Motivation:
 *   loop() pumps data between stdin/stdout and ptym.
 *   Normally stdin/stdout is the user's terminal.
 *   With -d, we replace them with a pipe to a driver program.
 *   The driver can send commands to the slave program and read
 *   its responses — like a programmable expect-lite.
 *
 * Process arrangement after do_driver():
 *
 *   driver process ─── fd_pipe ─── pty parent ─── PTY master ─── slave
 *        │                              │
 *        stdin=pipe[0]              stdin=pipe[1]
 *        stdout=pipe[0]             stdout=pipe[1]
 *
 * Steps:
 *   1. fd_pipe(pipe): create full-duplex socketpair.
 *      pipe[0] and pipe[1] are both connected ends.
 *
 *   2. fork().
 *
 *   3. Child (driver):
 *      close(pipe[1])                — not needed in child
 *      dup2(pipe[0], STDIN_FILENO)   — driver reads pty's output
 *      dup2(pipe[0], STDOUT_FILENO)  — driver writes become pty's input
 *      close(pipe[0]) if > 1        — clean up
 *      execlp(driver, driver, NULL) — run driver program
 *      stderr is left alone — driver can write diagnostics.
 *
 *   4. Parent (pty):
 *      close(pipe[0])                — not needed in parent
 *      dup2(pipe[1], STDIN_FILENO)   — loop reads driver's output
 *      dup2(pipe[1], STDOUT_FILENO)  — loop writes go to driver
 *      close(pipe[1]) if > 1        — clean up
 *      return — caller's loop() now pumps driver↔ptym
 *
 * Note: stderr is NOT redirected in either process.
 *   The driver can still read/write /dev/tty for user interaction.
 *   The pty parent still writes diagnostic messages to stderr.
 */
void do_driver(char *driver)
{
    pid_t child;
    int   pipe[2];

    /* Step 1: create full-duplex pipe */
    if (fd_pipe(pipe) < 0) {
        perror("do_driver: fd_pipe");
        exit(1);
    }

    /* Step 2: fork */
    child = fork();
    if (child < 0) {
        perror("do_driver: fork");
        exit(1);
    }

    if (child == 0) {
        /* ── Child: becomes the driver process ─────────── */

        /* Don't need pipe[1] in child */
        close(pipe[1]);

        /* stdin ← pipe[0]: child reads pty's output */
        if (dup2(pipe[0], STDIN_FILENO) != STDIN_FILENO) {
            perror("do_driver child: dup2 stdin");
            exit(1);
        }

        /* stdout → pipe[0]: child writes become pty's input */
        if (dup2(pipe[0], STDOUT_FILENO) != STDOUT_FILENO) {
            perror("do_driver child: dup2 stdout");
            exit(1);
        }

        /* Close original if not already on 0 or 1 */
        if (pipe[0] != STDIN_FILENO && pipe[0] != STDOUT_FILENO)
            close(pipe[0]);

        /* stderr left alone — driver can still write diagnostics */

        /* exec the driver program (searched via PATH) */
        execlp(driver, driver, (char *)0);
        perror("do_driver child: execlp");
        exit(1);
    }

    /* ── Parent: reconnect its own stdin/stdout to pipe ── */

    /* Don't need pipe[0] in parent */
    close(pipe[0]);

    /* stdin ← pipe[1]: parent reads driver's output */
    if (dup2(pipe[1], STDIN_FILENO) != STDIN_FILENO) {
        perror("do_driver parent: dup2 stdin");
        exit(1);
    }

    /* stdout → pipe[1]: parent writes go to driver's stdin */
    if (dup2(pipe[1], STDOUT_FILENO) != STDOUT_FILENO) {
        perror("do_driver parent: dup2 stdout");
        exit(1);
    }

    if (pipe[1] != STDIN_FILENO && pipe[1] != STDOUT_FILENO)
        close(pipe[1]);

    /*
     * Parent returns.
     * Now loop(fdm, ignoreeof) will copy:
     *   driver stdout → pty stdin → ptym → slave
     *   slave output  → ptym → pty stdout → driver stdin
     */
}
