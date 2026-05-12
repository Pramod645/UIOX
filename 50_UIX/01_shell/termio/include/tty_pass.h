#ifndef TTY_PASS_H
#define TTY_PASS_H

/*
 * tty_pass.h — Password reading in canonical mode.
 *
 * Section 18.10 / Figure 18.17
 *
 * getpass() reads a password from the controlling terminal:
 *
 * Key design decisions:
 *   1. Opens controlling terminal via ctermid() — not stdin.
 *      This allows the function to work even if stdin is
 *      redirected.
 *   2. Sets stream to unbuffered (setbuf(fp, NULL)) to avoid
 *      buffering interactions between fputs and getc.
 *   3. Blocks SIGINT and SIGTSTP while reading:
 *      - Without this, typing INTR (^C) would abort the
 *        program and leave the terminal with echoing disabled.
 *      - Without this, typing SUSP (^Z) would suspend the
 *        program and return to the shell with no echo.
 *      - Signals are held while echo is off; restored after.
 *   4. Disables echo flags: ECHO, ECHOE, ECHOK, ECHONL.
 *      Uses TCSAFLUSH so pending input is discarded.
 *   5. Stays in CANONICAL mode — line editing (ERASE, KILL)
 *      still works while entering password.
 *   6. Stores at most MAX_PASS_LEN (8) characters.
 *   7. Echoes a newline after password entry.
 *   8. Restores termios and signal mask before returning.
 *   9. Caller should zero the returned buffer when done.
 */

#include "tty.h"

/*
 * tty_getpass()
 *
 * Displays prompt on the controlling terminal, disables echo,
 * reads up to MAX_PASS_LEN characters, re-enables echo.
 *
 * @param prompt  String displayed before reading input.
 *
 * Returns pointer to static buffer containing password,
 * or NULL if controlling terminal cannot be opened.
 *
 * Security: caller must zero the buffer when done:
 *   char *pw = tty_getpass("Password: ");
 *   // use pw ...
 *   while (*pw) *pw++ = 0;  // zero out cleartext
 */
char *tty_getpass(const char *prompt);

#endif /* TTY_PASS_H */
