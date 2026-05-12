#ifndef TTY_WINSIZE_H
#define TTY_WINSIZE_H

/*
 * tty_winsize.h — Terminal window size support.
 *
 * Section 18.12 / Figure 18.22
 *
 * The kernel maintains a winsize structure for every terminal
 * and pseudo terminal:
 *
 *   struct winsize {
 *       unsigned short ws_row;    // rows, in characters
 *       unsigned short ws_col;    // columns, in characters
 *       unsigned short ws_xpixel; // horizontal size, pixels
 *       unsigned short ws_ypixel; // vertical size, pixels
 *   };
 *
 * Rules:
 *   TIOCGWINSZ ioctl: read current window size.
 *   TIOCSWINSZ ioctl: set new window size; if changed, kernel
 *                     sends SIGWINCH to foreground process group.
 *   Default SIGWINCH action: ignore (see Figure 10.1).
 *
 * Applications (vi, less, emacs) catch SIGWINCH and call
 * TIOCGWINSZ to get the new dimensions, then redraw.
 */

#include "tty.h"

#ifndef TIOCGWINSZ
#include <sys/ioctl.h>
#endif

/*
 * tty_get_winsize()
 *
 * Fetches the current window size of terminal fd using
 * ioctl(fd, TIOCGWINSZ, &size).
 *
 * @param fd   Terminal file descriptor.
 * @param rows Out: number of rows.
 * @param cols Out: number of columns.
 *
 * Returns 0 on success, -1 on error.
 */
int tty_get_winsize(int fd, unsigned short *rows,
                    unsigned short *cols);

/*
 * tty_set_winsize()
 *
 * Sets the window size of terminal fd using
 * ioctl(fd, TIOCSWINSZ, &size).
 *
 * If the new size differs from the current size, the kernel
 * sends SIGWINCH to the foreground process group.
 *
 * @param fd   Terminal file descriptor.
 * @param rows New number of rows.
 * @param cols New number of columns.
 *
 * Returns 0 on success, -1 on error.
 */
int tty_set_winsize(int fd, unsigned short rows,
                    unsigned short cols);

/*
 * tty_print_winsize()
 *
 * Prints "N rows, M columns" to stdout for terminal fd.
 * Used in the SIGWINCH handler example (Figure 18.22).
 */
void tty_print_winsize(int fd);

/*
 * tty_watch_winsize()
 *
 * Installs a SIGWINCH handler, prints initial window size,
 * then sleeps forever (pauses on each signal).
 *
 * Demonstrates SIGWINCH handling from Figure 18.22.
 * Terminate with Ctrl-C (SIGINT).
 */
void tty_watch_winsize(void);

#endif /* TTY_WINSIZE_H */
