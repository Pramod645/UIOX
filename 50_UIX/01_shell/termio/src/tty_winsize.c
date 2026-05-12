#include "../include/tty_winsize.h"

/* ── tty_get_winsize ─────────────────────────────────────────
 *
 * Reads current window size via TIOCGWINSZ ioctl.
 *
 * The winsize structure has four fields:
 *   ws_row    — rows in characters
 *   ws_col    — columns in characters
 *   ws_xpixel — horizontal pixels (often 0)
 *   ws_ypixel — vertical pixels   (often 0)
 *
 * Returns 0 on success, -1 on error.
 */
int tty_get_winsize(int fd, unsigned short *rows,
                    unsigned short *cols)
{
    struct winsize size;

    if (ioctl(fd, TIOCGWINSZ, &size) < 0)
        return -1;

    if (rows) *rows = size.ws_row;
    if (cols) *cols = size.ws_col;
    return 0;
}

/* ── tty_set_winsize ─────────────────────────────────────────
 *
 * Sets window size via TIOCSWINSZ ioctl.
 *
 * If the new size (rows × cols) differs from the current
 * size, the kernel automatically sends SIGWINCH to the
 * foreground process group of fd.
 *
 * The application can then call tty_get_winsize() in its
 * SIGWINCH handler to get the new dimensions and redraw.
 *
 * Returns 0 on success, -1 on error.
 */
int tty_set_winsize(int fd, unsigned short rows,
                    unsigned short cols)
{
    struct winsize size;

    /* Read current values first; preserve pixel dimensions */
    if (ioctl(fd, TIOCGWINSZ, &size) < 0)
        return -1;

    size.ws_row = rows;
    size.ws_col = cols;
    /* ws_xpixel and ws_ypixel are left unchanged */

    return ioctl(fd, TIOCSWINSZ, &size);
}

/* ── tty_print_winsize ───────────────────────────────────────
 *
 * Prints "N rows, M columns\n" to stdout.
 * Called on startup and in the SIGWINCH handler.
 */
void tty_print_winsize(int fd)
{
    struct winsize size;

    if (ioctl(fd, TIOCGWINSZ, &size) < 0) {
        perror("TIOCGWINSZ");
        return;
    }
    printf("%d rows, %d columns\n", size.ws_row, size.ws_col);
}

/* ── SIGWINCH handler ────────────────────────────────────────
 *
 * Signal handler for SIGWINCH (window size changed).
 *
 * Default SIGWINCH action is to ignore the signal (Figure 10.1).
 * Applications that need to track window size changes must
 * install this handler.
 *
 * Note: printf() is not async-signal-safe per POSIX, but is
 * commonly used in examples for simplicity.  A production
 * implementation would set a flag and print from main loop.
 */
static void sig_winch(int signo)
{
    (void)signo;
    printf("SIGWINCH received\n");
    tty_print_winsize(STDIN_FILENO);
}

/* ── tty_watch_winsize ───────────────────────────────────────
 *
 * Figure 18.22 (APUE §18.12) — watch for window size changes.
 *
 * 1. Verify stdin is a terminal (exit(1) if not).
 * 2. Install SIGWINCH handler.
 * 3. Print initial window size.
 * 4. Loop forever calling pause() — wakes on any signal.
 *    Each SIGWINCH wakes us and the handler prints new size.
 *    Terminate with Ctrl-C (SIGINT).
 */
void tty_watch_winsize(void)
{
    if (isatty(STDIN_FILENO) == 0) {
        fprintf(stderr, "stdin is not a terminal\n");
        exit(1);
    }

    if (signal(SIGWINCH, sig_winch) == SIG_ERR) {
        perror("signal(SIGWINCH)");
        exit(1);
    }

    /* Print current size before any changes */
    tty_print_winsize(STDIN_FILENO);

    /* Sleep forever, waking on SIGWINCH */
    for (;;)
        pause();
}
