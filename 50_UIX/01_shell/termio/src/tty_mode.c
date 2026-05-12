#include "../include/tty_mode.h"

/* ── Module state ───────────────────────────────────────────── */

/* Saved original terminal settings */
static struct termios save_termios;

/* fd that was saved (-1 = nothing saved) */
static int ttysavefd = -1;

/* Current terminal mode state */
static enum { RESET, RAW, CBREAK } ttystate = RESET;

/* ── tty_cbreak ──────────────────────────────────────────────
 *
 * Figure 18.20 ( §18.11) — cbreak mode.
 *
 * Cbreak mode changes:
 *   c_lflag &= ~(ECHO | ICANON)
 *     ECHO   — disable character echo
 *     ICANON — disable canonical (line-at-a-time) processing
 *              disables: ERASE, KILL, EOF, NL, EOL, EOL2 etc.
 *              ISIG remains ON so Ctrl-C/Z still generate signals
 *
 *   c_cc[VMIN]  = 1   — read returns after 1 byte minimum
 *   c_cc[VTIME] = 0   — no timer (Case B: blocks until 1 byte)
 *
 * Verification loop:
 *   tcsetattr can return 0 on partial success (§18.4).
 *   We always re-read and compare to detect partial failure.
 */
int tty_cbreak(int fd)
{
    int           err;
    struct termios buf;

    /* Refuse if already in a non-canonical mode */
    if (ttystate != RESET) {
        errno = EINVAL;
        return -1;
    }

    /* Fetch current terminal attributes */
    if (tcgetattr(fd, &buf) < 0)
        return -1;

    /* Save a copy for tty_reset() */
    save_termios = buf;

    /* ── Apply cbreak settings ──────────────────────────── */

    /* Turn off echo and canonical mode */
    buf.c_lflag &= ~(ECHO | ICANON);

    /*
     * Case B noncanonical: MIN=1, TIME=0.
     * read() will not return until at least 1 byte is available.
     * This can block indefinitely if no input arrives.
     */
    buf.c_cc[VMIN]  = 1;
    buf.c_cc[VTIME] = 0;

    /* Apply — TCSAFLUSH: drain output, discard pending input */
    if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
        return -1;

    /* ── Verify all changes took effect ─────────────────── */
    if (tcgetattr(fd, &buf) < 0) {
        err = errno;
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = err;
        return -1;
    }

    if ((buf.c_lflag & (ECHO | ICANON)) ||
         buf.c_cc[VMIN]  != 1           ||
         buf.c_cc[VTIME] != 0) {
        /* Only partial success — restore original */
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = EINVAL;
        return -1;
    }

    ttystate  = CBREAK;
    ttysavefd = fd;
    return 0;
}

/* ── tty_raw ─────────────────────────────────────────────────
 *
 * Figure 18.20 (APUE §18.11) — raw mode.
 *
 * Raw mode changes:
 *
 * c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG)
 *   ECHO   — disable echo
 *   ICANON — disable canonical processing
 *   IEXTEN — disable extended processing (DISCARD, LNEXT etc.)
 *   ISIG   — disable signal generation (INTR, QUIT, SUSP)
 *             In raw mode, Ctrl-C does NOT send SIGINT.
 *
 * c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON)
 *   BRKINT — no SIGINT on BREAK condition
 *   ICRNL  — no CR-to-NL translation on input
 *   INPCK  — disable input parity checking
 *   ISTRIP — don't strip 8th bit from input bytes
 *   IXON   — disable XON/XOFF output flow control
 *
 * c_cflag &= ~(CSIZE|PARENB); c_cflag |= CS8
 *   CSIZE  — mask for character size bits (zero them first)
 *   PARENB — no parity generation/checking
 *   CS8    — 8 bits per character
 *
 * c_oflag &= ~OPOST
 *   OPOST  — disable all output processing (NL→CRNL etc.)
 *             Note: without OPOST, printf("\n") does NOT move
 *             cursor to column 0 — output raw NL only.
 *
 * c_cc[VMIN]=1, c_cc[VTIME]=0 (Case B)
 */
int tty_raw(int fd)
{
    int           err;
    struct termios buf;

    if (ttystate != RESET) {
        errno = EINVAL;
        return -1;
    }

    if (tcgetattr(fd, &buf) < 0)
        return -1;

    save_termios = buf;

    /* ── Apply raw settings ─────────────────────────────── */

    /* Local flags: echo off, canonical off, extended off,
     * signal generation off                                */
    buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /* Input flags: no BREAK signal, no CR-NL, no parity,
     * keep 8th bit, no flow control                       */
    buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    /* Control flags: 8-bit chars, no parity */
    buf.c_cflag &= ~(CSIZE | PARENB);
    buf.c_cflag |=  CS8;

    /* Output flags: no output processing */
    buf.c_oflag &= ~OPOST;

    /* Case B: 1 byte at a time, no timer */
    buf.c_cc[VMIN]  = 1;
    buf.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
        return -1;

    /* ── Verify ─────────────────────────────────────────── */
    if (tcgetattr(fd, &buf) < 0) {
        err = errno;
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = err;
        return -1;
    }

    if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG))         ||
        (buf.c_iflag & (BRKINT | ICRNL | INPCK | ISTRIP | IXON))||
        ((buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8)          ||
        (buf.c_oflag & OPOST)                                     ||
         buf.c_cc[VMIN]  != 1                                     ||
         buf.c_cc[VTIME] != 0) {
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = EINVAL;
        return -1;
    }

    ttystate  = RAW;
    ttysavefd = fd;
    return 0;
}

/* ── tty_reset ───────────────────────────────────────────────
 *
 * Restores the terminal to original cooked (canonical) mode.
 *
 * TCSAFLUSH: wait for pending output to drain, then discard
 * any unread input, then apply the saved settings.
 * This ensures we don't confuse the restored line discipline
 * with bytes that were entered in raw/cbreak mode.
 */
int tty_reset(int fd)
{
    if (ttystate == RESET)
        return 0;

    if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0)
        return -1;

    ttystate = RESET;
    return 0;
}

/* ── tty_atexit ──────────────────────────────────────────────
 *
 * Registered with atexit(3) to guarantee terminal restoration.
 * Called by exit() in reverse registration order.
 * If the program crashes (SIGSEGV etc.) this is NOT called —
 * use signal handlers to call tty_reset() for that case.
 */
void tty_atexit(void)
{
    if (ttysavefd >= 0)
        tty_reset(ttysavefd);
}

/* ── tty_termios ─────────────────────────────────────────────
 *
 * Returns pointer to saved original termios structure.
 * Used by pty_fork (Chapter 19) to initialize the PTY slave
 * with the same settings as the real terminal.
 */
struct termios *tty_termios(void)
{
    return &save_termios;
}

/* ── tty_charsize ────────────────────────────────────────────
 *
 * Figure 18.11 — extract character size using CSIZE mask.
 *
 * CSIZE is a mask (not a single bit) that identifies the
 * character-size bits in c_cflag.  To extract the value:
 *   term.c_cflag & CSIZE
 * gives one of CS5, CS6, CS7, CS8.
 */
void tty_charsize(int fd)
{
    struct termios term;

    if (tcgetattr(fd, &term) < 0) {
        perror("tcgetattr");
        return;
    }

    switch (term.c_cflag & CSIZE) {
    case CS5: printf("5 bits/byte\n"); break;
    case CS6: printf("6 bits/byte\n"); break;
    case CS7: printf("7 bits/byte\n"); break;
    case CS8: printf("8 bits/byte\n"); break;
    default:  printf("unknown bits/byte\n"); break;
    }
}

/* ── tty_set8bit ─────────────────────────────────────────────
 *
 * Figure 18.11 — set character size to CS8.
 *
 * Pattern for mask fields:
 *   1. Zero the mask bits:  term.c_cflag &= ~CSIZE;
 *   2. Set the desired value: term.c_cflag |= CS8;
 *
 * TCSANOW: apply change immediately without waiting.
 */
int tty_set8bit(int fd)
{
    struct termios term;

    if (tcgetattr(fd, &term) < 0)
        return -1;

    term.c_cflag &= ~CSIZE;  /* zero the character-size bits */
    term.c_cflag |=  CS8;    /* set 8 bits/byte              */

    return tcsetattr(fd, TCSANOW, &term);
}

/* ── tty_disable_special ─────────────────────────────────────
 *
 * Figure 18.10 — modify special characters.
 *
 * _POSIX_VDISABLE: the value used to disable a special char.
 *   Linux/Solaris: 0
 *   FreeBSD/macOS: 0xff
 * Use fpathconf(_PC_VDISABLE) for portable value.
 *
 * c_cc[VINTR] = vdisable → typing ^C no longer sends SIGINT.
 * c_cc[VEOF]  = 2        → ^B (Control-B, ASCII 2) is EOF.
 *
 * Note: disabling the key is different from ignoring the signal.
 * kill() can still send SIGINT to the process.
 *
 * TCSAFLUSH: discard pending input so previously typed
 * characters aren't reinterpreted with new settings.
 */
int tty_disable_special(int fd)
{
    struct termios term;
    long           vdisable;

    if (!isatty(fd)) {
        fprintf(stderr, "tty_disable_special: not a terminal\n");
        return -1;
    }

    /* Get the platform's vdisable value */
    vdisable = fpathconf(fd, _PC_VDISABLE);
    if (vdisable < 0) {
        perror("fpathconf(_PC_VDISABLE)");
        return -1;
    }

    /* Fetch current terminal state */
    if (tcgetattr(fd, &term) < 0)
        return -1;

    /* Disable INTR character (^C → no longer SIGINT) */
    term.c_cc[VINTR] = (cc_t)vdisable;

    /* Change EOF to Control-B (ASCII 2) */
    term.c_cc[VEOF]  = 2;

    return tcsetattr(fd, TCSAFLUSH, &term);
}
