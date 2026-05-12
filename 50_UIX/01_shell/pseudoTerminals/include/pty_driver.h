#ifndef PTY_DRIVER_H
#define PTY_DRIVER_H

/*
 * pty_driver.h — Driver subprocess support for the pty program.
 *
 * Section 19.6 / Figure 19.16
 *
 * Purpose:
 *   Allows pty to be driven by an external program (-d option)
 *   instead of interactively from the user's terminal.
 *
 * Architecture after do_driver():
 *
 *   driver program ──bidirectional pipe── pty parent ──PTY master──  slave
 *                                            ↑
 *                                        pty's stdin/stdout
 *                                        are now the pipe ends
 *
 * The driver communicates with pty via a full-duplex pipe (fd_pipe),
 * similar to a coprocess but on the "other side" of pty.
 *
 * The driver can still access the user's terminal directly via
 * /dev/tty, even though its stdin/stdout are the pipe.
 *
 * Sequence inside do_driver():
 *   1. fd_pipe(pipe)         — create full-duplex pipe pair
 *   2. fork()
 *   3. Child: dup2(pipe[0], stdin); dup2(pipe[0], stdout); execlp(driver)
 *   4. Parent: dup2(pipe[1], stdin); dup2(pipe[1], stdout)
 *              returns — pty now reads/writes the pipe
 *
 * After do_driver() returns, the parent's loop() call will pump
 * data between the driver (via the pipe, now stdin/stdout) and
 * the slave program (via ptym).
 */

#include "pty.h"

/*
 * do_driver()
 *
 * Connects pty's stdin and stdout to a driver subprocess via
 * a bidirectional pipe.  After this call, the caller's stdin
 * and stdout are the pipe ends; the driver process has the other ends.
 *
 * @param driver  Name of the driver program to exec (searched via PATH).
 */
void do_driver(char *driver);

/*
 * fd_pipe()
 *
 * Creates a full-duplex pipe pair.
 * On Linux: uses socketpair(AF_UNIX, SOCK_STREAM, 0, ...).
 * On others: same approach.
 *
 * @param fd  Array of two file descriptors.
 *            fd[0] and fd[1] are both readable and writable.
 *
 * Returns 0 on success, -1 on error.
 */
int fd_pipe(int fd[2]);

#endif /* PTY_DRIVER_H */
