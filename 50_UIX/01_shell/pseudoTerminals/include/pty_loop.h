#ifndef PTY_LOOP_H
#define PTY_LOOP_H

/*
 * pty_loop.h — I/O copy loop between PTY master and stdin/stdout.
 *
 * Section 19.5 / Figure 19.12
 *
 * loop() implements the bidirectional data pump between:
 *   stdin  → PTY master  (typed input goes to slave program)
 *   PTY master → stdout  (slave program output goes to terminal)
 *
 * Implementation uses two processes:
 *
 *   Parent (loop caller):
 *     Installs SIGTERM handler.
 *     Reads from ptym, writes to stdout.
 *     Breaks on EOF, error, or SIGTERM from child.
 *
 *   Child (forked inside loop):
 *     Reads from stdin, writes to ptym.
 *     On stdin EOF: if ignoreeof==0, sends SIGTERM to parent then exits.
 *                   if ignoreeof==1, exits without notifying parent.
 *
 * Termination protocol:
 *   Child EOF on stdin  → child sends SIGTERM to parent → parent exits.
 *   Parent EOF on ptym  → parent sends SIGTERM to child → parent exits.
 *   SIGTERM sets sigcaught flag to break parent's read loop.
 *
 * Alternative: single process using select() or poll() — Exercise 19.3.
 *
 * Terminal raw mode:
 *   The caller (pty main) sets the real terminal to raw mode before
 *   calling loop(), so that every keystroke goes directly to ptym
 *   without local processing.  An atexit handler restores cooked mode.
 */

#include "pty.h"

/*
 * loop()
 *
 * Copies stdin → ptym and ptym → stdout until one side closes.
 *
 * @param ptym       PTY master file descriptor.
 * @param ignoreeof  If 1, child does not notify parent on stdin EOF.
 *                   Used with -i option to watch background programs.
 */
void loop(int ptym, int ignoreeof);

/*
 * tty_raw()
 *
 * Sets the terminal referenced by fd to raw (non-canonical, no echo)
 * mode using tcsetattr().  Saves original settings for tty_reset().
 *
 * Returns 0 on success, -1 on error.
 */
int tty_raw(int fd);

/*
 * tty_reset()
 *
 * Restores terminal fd to the settings saved by the last tty_raw().
 * Returns 0 on success, -1 on error.
 */
int tty_reset(int fd);

/*
 * tty_atexit()
 *
 * atexit() handler that calls tty_reset(STDIN_FILENO).
 * Registered with atexit() by the pty main function when
 * running interactively, so the terminal is always restored.
 */
void tty_atexit(void);

/*
 * set_noecho()
 *
 * Turns off echo and NL→CRNL mapping on the slave PTY fd.
 * Used by the -e option to prevent double-echo when pty
 * drives a coprocess.
 *
 * Clears:
 *   c_lflag: ECHO, ECHOE, ECHOK, ECHONL
 *   c_oflag: ONLCR
 */
void set_noecho(int fd);

#endif /* PTY_LOOP_H */
