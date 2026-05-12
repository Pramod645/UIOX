#include "../include/pty_loop.h"
#include <stdarg.h>

/* ── Terminal raw mode ──────────────────────────────────── */

/* Saved original terminal settings for tty_reset() */
static struct termios  _saved_termios;
static int             _saved_fd = -1;

/*
 * tty_raw() — put terminal fd into raw (non-canonical) mode.
 *
 * Raw mode settings:
 *   c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG)
 *     ECHO   — disable echo
 *     ICANON — disable line editing (backspace, kill, etc.)
 *     IEXTEN — disable implementation-defined processing
 *     ISIG   — disable signal generation (Ctrl-C, Ctrl-Z)
 *
 *   c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON)
 *     BRKINT — no SIGINT on break
 *     ICRNL  — no CR→NL translation on input
 *     INPCK  — no parity checking
 *     ISTRIP — don't strip 8th bit
 *     IXON   — disable XON/XOFF flow control
 *
 *   c_cflag &= ~(CSIZE|PARENB); c_cflag |= CS8
 *     8-bit characters, no parity
 *
 *   c_oflag &= ~OPOST
 *     Disable output processing (NL→CRNL etc.)
 *
 *   c_cc[VMIN]  = 1  — minimum 1 character per read
 *   c_cc[VTIME] = 0  — no timeout
 *
 * This ensures every keystroke goes directly to ptym without
 * local terminal processing, so the line discipline above the
 * PTY slave processes it instead.
 */
int tty_raw(int fd)
{
    struct termios raw;

    if (tcgetattr(fd, &_saved_termios) < 0)
        return -1;
    _saved_fd = fd;

    raw = _saved_termios;

    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |=  CS8;
    raw.c_oflag &= ~OPOST;

    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0)
        return -1;

    return 0;
}

/*
 * tty_reset() — restore terminal to saved cooked mode.
 */
int tty_reset(int fd)
{
    if (_saved_fd < 0)
        return 0;   /* nothing saved */
    return tcsetattr(fd, TCSAFLUSH, &_saved_termios);
}

/*
 * tty_atexit() — atexit handler that restores stdin terminal.
 * Registered with atexit() by pty_main when running interactively.
 */
void tty_atexit(void)
{
    tty_reset(STDIN_FILENO);
}

/*
 * set_noecho() — disable echo and ONLCR on slave PTY fd.
 *
 * Used by the -e option when pty drives a coprocess.
 *
 * Without -e: both the real terminal's line discipline and the
 *   slave PTY's line discipline echo input, resulting in
 *   double-echo — everything appears twice.
 *
 * With -e: echo is disabled in the slave's line discipline.
 *   Only the real terminal echoes, giving normal appearance.
 *
 * ONLCR turns NL output into CR+NL.  Disabling it prevents
 * every line from the coprocess ending with CR+NL+NL.
 */
void set_noecho(int fd)
{
    struct termios stermios;

    if (tcgetattr(fd, &stermios) < 0) {
        perror("tcgetattr");
        exit(1);
    }

    /* Disable echo flags */
    stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

    /* Disable NL → CR+NL output mapping */
    stermios.c_oflag &= ~ONLCR;

    if (tcsetattr(fd, TCSANOW, &stermios) < 0) {
        perror("tcsetattr");
        exit(1);
    }
}

/* ── I/O copy loop ──────────────────────────────────────── */

/*
 * Signal flag set by sig_term() when SIGTERM is received.
 * sig_atomic_t guarantees the write is atomic with respect
 * to signal delivery — no mutex needed.
 */
static volatile sig_atomic_t sigcaught;

/*
 * sig_term() — SIGTERM handler for the parent (ptym→stdout) process.
 *
 * The child sends SIGTERM to the parent when stdin reaches EOF.
 * This signal interrupts the parent's blocking read(ptym),
 * which returns -1 with errno=EINTR, causing the loop to break.
 *
 * We set sigcaught=1 so the parent knows a signal—not an
 * error or real EOF—caused the break, and therefore does not
 * need to send SIGTERM to the child (child already exited).
 */
static void sig_term(int signo)
{
    (void)signo;
    sigcaught = 1;
}

/*
 * signal_intr() — install signal handler that restarts on EINTR
 *   but does NOT set SA_RESTART (so blocking reads ARE interrupted).
 */
static void (*signal_intr(int signo, void (*func)(int)))(int)
{
    struct sigaction sa, osa;
    sa.sa_handler = func;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;           /* no SA_RESTART — we want EINTR */
    if (sigaction(signo, &sa, &osa) < 0)
        return SIG_ERR;
    return osa.sa_handler;
}

/*
 * pty_writen() — write exactly n bytes, retrying on short writes.
 *
 * write() may return fewer bytes than requested if:
 *   • The fd has a buffer smaller than n.
 *   • A signal interrupts the write (EINTR).
 * Retry ensures all n bytes are written.
 */
ssize_t pty_writen(int fd, const void *buf, size_t n)
{
    size_t      nleft = n;
    ssize_t     nwritten;
    const char *ptr = (const char *)buf;

    while (nleft > 0) {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;   /* retry */
            else
                return -1;      /* real error */
        }
        nleft -= (size_t)nwritten;
        ptr   += nwritten;
    }
    return (ssize_t)n;
}

/*
 * loop() — Figure 19.12 (APUE §19.5)
 *
 * Bidirectional I/O copy loop using two processes.
 *
 * Child process (copy stdin → ptym):
 *   Reads from STDIN_FILENO in a tight loop.
 *   Each read block is written in full to ptym via pty_writen().
 *   On EOF (nread == 0):
 *     - If ignoreeof == 0: sends SIGTERM to parent, then exits.
 *       This is the normal case: user closes input → pty terminates.
 *     - If ignoreeof == 1: exits silently.
 *       Used with -i flag for background long-running programs.
 *       Parent continues copying ptym→stdout until slave exits.
 *
 * Parent process (copy ptym → stdout):
 *   Installs SIGTERM handler (sig_term) to catch notification
 *   from child when stdin EOF is reached.
 *   Reads from ptym in a tight loop.
 *   On EOF or error: breaks.
 *
 *   There are three ways to exit the parent's loop:
 *     1. sig_term() sets sigcaught=1; read() returns EINTR.
 *     2. read(ptym) returns 0 (slave closed — slave process exited).
 *     3. read(ptym) returns -1 with non-EINTR error.
 *
 *   If sigcaught==0: parent must notify child (case 2 or 3),
 *     sends SIGTERM to child.
 *   If sigcaught==1: child already exited, no notification needed.
 *
 * Termination:
 *   Parent returns to caller (pty_main), which calls exit().
 *   atexit(tty_atexit) restores terminal to cooked mode.
 */
void loop(int ptym, int ignoreeof)
{
    pid_t  child;
    int    nread;
    char   buf[PTY_BUFFSIZE];

    /* ── Fork child to handle stdin → ptym direction ───── */
    child = fork();
    if (child < 0) {
        perror("loop: fork");
        exit(1);
    }

    if (child == 0) {
        /* ── Child: copy stdin → PTY master ────────────── */
        for (;;) {
            nread = read(STDIN_FILENO, buf, PTY_BUFFSIZE);
            if (nread < 0) {
                perror("loop child: read from stdin");
                break;
            } else if (nread == 0) {
                break;  /* EOF on stdin */
            }

            if (pty_writen(ptym, buf, (size_t)nread) != nread) {
                perror("loop child: write to ptym");
                break;
            }
        }

        /*
         * Notify parent only if ignoreeof is 0.
         * With -i, child exits silently and parent continues
         * copying PTY output until the slave program exits.
         */
        if (ignoreeof == 0)
            kill(getppid(), SIGTERM);

        exit(0);   /* child cannot return; it would re-enter caller */
    }

    /* ── Parent: copy PTY master → stdout ──────────────── */

    /*
     * Install SIGTERM handler.
     * sig_term() sets sigcaught=1 and returns, which causes
     * the blocked read(ptym) to return -1/EINTR, breaking the loop.
     * We do NOT set SA_RESTART so the read is truly interrupted.
     */
    if (signal_intr(SIGTERM, sig_term) == SIG_ERR) {
        perror("loop parent: signal_intr SIGTERM");
        exit(1);
    }

    sigcaught = 0;

    for (;;) {
        nread = read(ptym, buf, PTY_BUFFSIZE);
        if (nread <= 0)
            break;  /* EINTR (sigcaught), EOF, or error */

        if (pty_writen(STDOUT_FILENO, buf, (size_t)nread) != nread) {
            perror("loop parent: write to stdout");
            break;
        }
    }

    /*
     * If sigcaught==0: child did not send SIGTERM.
     * Parent received EOF on ptym (slave exited) or got an error.
     * Must send SIGTERM to child so it cleans up.
     *
     * If sigcaught==1: child sent SIGTERM because stdin EOF.
     * Child has already exited; no signal needed.
     */
    if (sigcaught == 0)
        kill(child, SIGTERM);

    /* Parent returns to main() */
}
