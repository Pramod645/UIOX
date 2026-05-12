#include "../include/tty_pass.h"

/*
 * tty_getpass() — Figure 18.17 (APUE §18.10)
 *
 * Password reading in canonical mode.
 *
 * Step-by-step:
 *
 * 1. fopen(ctermid(NULL), "r+")
 *    Opens the controlling terminal for both reading and writing.
 *    Using ctermid() instead of hard-coding "/dev/tty" aids
 *    portability to non-UNIX systems.
 *    "r+" mode: O_RDWR, no truncation.
 *
 * 2. setbuf(fp, NULL)
 *    Disables stdio buffering on the stream.
 *    Without this, fputs() and getc() might interact incorrectly
 *    since they share the same underlying fd.
 *
 * 3. Block SIGINT and SIGTSTP:
 *    sigprocmask(SIG_BLOCK, &sig, &osig)
 *    If SIGINT is delivered while echo is disabled, the program
 *    terminates and the terminal is left with no echo — unusable.
 *    Similarly for SIGTSTP (suspend): shell would return with
 *    echo still disabled.
 *    Blocking them holds them pending until we restore the mask.
 *
 * 4. Disable echo:
 *    ts.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL)
 *    ECHO   — character echo
 *    ECHOE  — visual erase (backspace-space-backspace)
 *    ECHOK  — echo NL after kill character
 *    ECHONL — echo NL even when ECHO off
 *    Terminal remains in CANONICAL mode: ERASE and KILL
 *    characters still work for editing the password.
 *    TCSAFLUSH: pending input discarded before applying.
 *
 * 5. Read up to MAX_PASS_LEN (8) characters.
 *    Loop ends on EOF (^D pressed twice) or newline.
 *    Additional characters beyond MAX_PASS_LEN are silently
 *    discarded to prevent buffer overflow.
 *
 * 6. putc('\n', fp)
 *    Since echo is off, the terminal did not advance the cursor.
 *    We manually echo a newline so the next prompt appears on
 *    a new line.
 *
 * 7. Restore terminal state (TCSAFLUSH) and signal mask.
 *
 * 8. fclose(fp) and return static buffer.
 */
char *tty_getpass(const char *prompt)
{
    static char      buf[MAX_PASS_LEN + 1]; /* +1 for null */
    char            *ptr;
    sigset_t         sig, osig;
    struct termios   ts, ots;
    FILE            *fp;
    int              c;

    /* Step 1: open controlling terminal */
    fp = fopen(ctermid(NULL), "r+");
    if (fp == NULL)
        return NULL;

    /* Step 2: unbuffered I/O */
    setbuf(fp, NULL);

    /* Step 3: block SIGINT and SIGTSTP */
    sigemptyset(&sig);
    sigaddset(&sig, SIGINT);    /* interrupt (^C) */
    sigaddset(&sig, SIGTSTP);   /* suspend  (^Z) */
    sigprocmask(SIG_BLOCK, &sig, &osig);

    /* Step 4: disable echo, stay in canonical mode */
    tcgetattr(fileno(fp), &ts);
    ots = ts;                   /* save original */
    ts.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    tcsetattr(fileno(fp), TCSAFLUSH, &ts);

    /* Step 5: display prompt and read password */
    fputs(prompt, fp);

    ptr = buf;
    while ((c = getc(fp)) != EOF && c != '\n') {
        if (ptr < &buf[MAX_PASS_LEN])
            *ptr++ = (char)c;
        /* extra characters are silently dropped */
    }
    *ptr = '\0';    /* null-terminate */

    /* Step 6: echo a newline (cursor did not advance) */
    putc('\n', fp);

    /* Step 7: restore terminal and signal mask */
    tcsetattr(fileno(fp), TCSAFLUSH, &ots);
    sigprocmask(SIG_SETMASK, &osig, NULL);

    /* Step 8: close /dev/tty */
    fclose(fp);

    return buf;
}
