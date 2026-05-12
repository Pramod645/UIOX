#ifndef TTY_H
#define TTY_H

/*
 * tty.h — Master header for Terminal I/O.
 *
 * Terminal I/O Architecture (Section 18.2):
 *
 *   User process
 *       |
 *   read/write functions
 *       |
 *   Terminal line discipline   ← canonical processing lives here
 *       |
 *   Terminal device driver
 *       |
 *   Actual device (RS-232, PTY, ...)
 *
 * The termios structure (defined in <termios.h>) controls all
 * terminal device characteristics:
 *
 *   struct termios {
 *       tcflag_t c_iflag;   // input flags
 *       tcflag_t c_oflag;   // output flags
 *       tcflag_t c_cflag;   // control flags
 *       tcflag_t c_lflag;   // local flags
 *       cc_t     c_cc[NCCS];// control characters
 *   };
 *
 * Two terminal modes:
 *   Canonical   — input processed as lines; line discipline
 *                 handles ERASE/KILL/EOF/NL etc.
 *   Noncanonical — raw bytes; MIN/TIME control when read()
 *                  returns (four cases A–D, Section 18.11).
 *
 * 13 POSIX terminal functions (Figure 18.7):
 *   tcgetattr    tcsetattr
 *   cfgetispeed  cfgetospeed  cfsetispeed  cfsetospeed
 *   tcdrain      tcflow       tcflush      tcsendbreak
 *   tcgetpgrp    tcsetpgrp    tcgetsid
 */

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

/* Maximum password length for getpass() (Section 18.10) */
#define MAX_PASS_LEN    8

/* POSIX path max fallback */
#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 256
#endif

#endif /* TTY_H */
