#ifndef TTY_ID_H
#define TTY_ID_H

/*
 * tty_id.h — Terminal identification functions.
 *
 * Section 18.9
 *
 * Three functions for identifying terminal devices:
 *
 *   ctermid()  — name of controlling terminal
 *   isatty()   — is a file descriptor a terminal?
 *   ttyname()  — pathname of terminal device
 *
 * ctermid() (Figure 18.12):
 *   Returns "/dev/tty" on all four platforms described
 *   in the text.  Uses a static buffer if ptr==NULL.
 *
 * isatty() (Figure 18.13):
 *   Calls tcgetattr() — if it succeeds, fd is a terminal.
 *   Returns 1 (true) or 0 (false).
 *
 * ttyname() (Figure 18.15):
 *   Searches /dev recursively by inode+device number match.
 *   Uses struct stat with st_ino and st_dev fields.
 *   Skips: /dev/., /dev/.., /dev/fd, stdin/stdout/stderr.
 *   Uses a linked list (struct devdir) to track subdirs found
 *   during the search, then searches each subdirectory.
 */

#include "tty.h"

/* L_ctermid is defined in <stdio.h>; define fallback if needed */
#ifndef L_ctermid
#define L_ctermid 9
#endif

/*
 * tty_ctermid()
 *
 * POSIX.1 ctermid() implementation (Figure 18.12).
 * Returns pathname of the controlling terminal.
 *
 * If str != NULL: stores name in str (must be L_ctermid bytes).
 * If str == NULL: uses an internal static buffer.
 *
 * Returns starting address of the name string.
 * On all four platforms: returns "/dev/tty".
 */
char *tty_ctermid(char *str);

/*
 * tty_isatty()
 *
 * POSIX.1 isatty() implementation (Figure 18.13).
 *
 * Calls tcgetattr(fd, &ts).  If it succeeds (returns != -1),
 * fd refers to a terminal device.
 *
 * Returns 1 if fd is a terminal, 0 otherwise.
 */
int tty_isatty(int fd);

/*
 * tty_ttyname()
 *
 * POSIX.1 ttyname() implementation (Figure 18.15).
 *
 * Finds the pathname of the terminal device open on fd by:
 *   1. Calling isatty(fd) — returns NULL if not a terminal.
 *   2. Calling fstat(fd, &fdstat) — get inode + device number.
 *   3. Searching /dev recursively for a character special file
 *      with matching st_ino and st_dev.
 *
 * Returns pointer to static pathname buffer, or NULL on error.
 *
 * Note: result is in static storage — not reentrant.
 */
char *tty_ttyname(int fd);

#endif /* TTY_ID_H */
