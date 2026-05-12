#ifndef PTY_H
#define PTY_H

/*
 * pty.h — Master header for the pseudo terminal (PTY) library.
 *
 * Pseudo Terminals
 *
 * Architecture overview:
 *
 *   User process (parent)
 *     │
 *     │  opens PTY master (ptym_open)
 *     │
 *     ├─── fork ───────────────────────────────────────────────┐
 *     │                                                         │
 *   Parent                                                    Child
 *   holds fdm                                                 setsid()
 *   reads/writes PTY master                                   opens PTY slave (ptys_open)
 *   <──────────── terminal line discipline ───────────────────>
 *                                                             slave becomes
 *                                                             controlling terminal
 *                                                             dup2 to stdin/stdout/stderr
 *                                                             exec target program
 *
 * Key POSIX XSI functions used:
 *   posix_openpt()  — open next available PTY master
 *   grantpt()       — set slave device permissions (owner=caller, group=tty, mode=0620)
 *   unlockpt()      — unlock slave so it can be opened
 *   ptsname()       — return name of slave device
 *
 * Platform notes:
 *   Linux/macOS/Solaris: open() of slave allocates controlling terminal
 *                        if caller is session leader without one.
 *   FreeBSD:             Must use TIOCSCTTY ioctl explicitly.
 *   Solaris:             Must push STREAMS modules: ptem, ldterm, ttcompat.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Buffer size for PTY I/O loop */
#define PTY_BUFFSIZE        512

/* Maximum PTY slave name length */
#define PTY_NAME_MAX        64

/* Default slave name buffer size */
#define PTY_SLAVE_NAME_SZ   64

/* I/O loop option flags */
#define PTY_OPT_IGNOREEOF   0x01  /* ignore EOF on stdin */
#define PTY_OPT_NOECHO      0x02  /* disable echo on slave */
#define PTY_OPT_INTERACTIVE 0x04  /* stdin is a terminal */
#define PTY_OPT_VERBOSE     0x08  /* print slave name to stderr */

/* Utility: write exactly n bytes, retrying on partial writes */
ssize_t pty_writen(int fd, const void *buf, size_t n);

/* Utility: print error message and exit */
void pty_err_sys(const char *fmt, ...);
void pty_err_quit(const char *fmt, ...);

#endif /* PTY_H */
