#ifndef TTY_MODE_H
#define TTY_MODE_H

/*
 * tty_mode.h — Terminal mode functions: raw, cbreak, reset.
 *
 * Section 18.11 / Figure 18.20
 *
 * Terminal mode state machine:
 *
 *   RESET ──tty_cbreak()──> CBREAK
 *   RESET ──tty_raw()────> RAW
 *   CBREAK ─tty_reset()──> RESET   (must call before switching)
 *   RAW ────tty_reset()──> RESET
 *
 * CBREAK mode (noncanonical, echo off, MIN=1, TIME=0):
 *   - Turns off ECHO and ICANON (canonical mode).
 *   - Signal-generating characters (INTR, QUIT, SUSP) still work.
 *   - One byte returned per read() (Case B: MIN=1, TIME=0).
 *
 * RAW mode (fully raw, noncanonical):
 *   - Turns off ECHO, ICANON, IEXTEN, ISIG.
 *   - Turns off BRKINT, ICRNL, INPCK, ISTRIP, IXON.
 *   - Sets CS8, clears CSIZE and PARENB.
 *   - Turns off OPOST (no output processing).
 *   - One byte returned per read() (Case B: MIN=1, TIME=0).
 *
 * Noncanonical MIN/TIME cases (Section 18.11):
 *   A: MIN>0, TIME>0 — interbyte timer; blocks until 1st byte
 *   B: MIN>0, TIME=0 — blocks until MIN bytes received
 *   C: MIN=0, TIME>0 — read timer; returns 0 on timeout
 *   D: MIN=0, TIME=0 — returns immediately with available data
 *
 * tcsetattr() partial-success note (Section 18.4):
 *   tcsetattr() returns 0 even on partial success.
 *   We always re-read with tcgetattr() to verify all changes
 *   actually took effect, restoring original state if not.
 */

#include "tty.h"

/*
 * tty_cbreak()
 *
 * Sets terminal fd to cbreak mode:
 *   - ECHO off, ICANON off
 *   - MIN=1, TIME=0 (Case B noncanonical)
 *   - Signals still generated (ISIG on)
 *   - Saves original termios in save_termios
 *
 * Returns 0 on success, -1 on error (errno set).
 * Error EINVAL if already in RAW or CBREAK state.
 */
int tty_cbreak(int fd);

/*
 * tty_raw()
 *
 * Sets terminal fd to raw mode:
 *   - ECHO, ICANON, IEXTEN, ISIG off (c_lflag)
 *   - BRKINT, ICRNL, INPCK, ISTRIP, IXON off (c_iflag)
 *   - CSIZE, PARENB cleared; CS8 set (c_cflag)
 *   - OPOST off (c_oflag)
 *   - MIN=1, TIME=0 (Case B noncanonical)
 *   - Saves original termios in save_termios
 *
 * Returns 0 on success, -1 on error.
 */
int tty_raw(int fd);

/*
 * tty_reset()
 *
 * Restores terminal fd to original cooked (canonical) mode
 * saved by tty_cbreak() or tty_raw().
 *
 * Uses TCSAFLUSH: waits for output to drain, then discards
 * unread input before applying saved termios.
 *
 * Returns 0 on success (or if already RESET), -1 on error.
 */
int tty_reset(int fd);

/*
 * tty_atexit()
 *
 * atexit(3) handler that calls tty_reset(ttysavefd).
 * Register with: atexit(tty_atexit)
 *
 * Ensures terminal is restored even if the program exits
 * abnormally — critical because a raw-mode terminal left
 * behind is unusable (no echo, no line editing).
 */
void tty_atexit(void);

/*
 * tty_termios()
 *
 * Returns pointer to the original termios structure saved
 * by the last call to tty_cbreak() or tty_raw().
 *
 * Used by the pty program (Chapter 19) to pass the original
 * terminal settings to the PTY slave.
 */
struct termios *tty_termios(void);

/*
 * tty_charsize()
 *
 * Reads the CSIZE field from fd's termios and prints the
 * character size (5, 6, 7, or 8 bits/byte).
 * Demonstrates use of the CSIZE mask (Figure 18.11).
 */
void tty_charsize(int fd);

/*
 * tty_set8bit()
 *
 * Sets the character size to CS8 (8 bits/byte) on fd.
 * First zeros the CSIZE mask bits, then sets CS8.
 * Demonstrates tcgetattr/tcsetattr pair (Figure 18.11).
 */
int tty_set8bit(int fd);

/*
 * tty_disable_special()
 *
 * Disables the INTR special character and changes EOF to
 * Control-B on fd.  (Figure 18.10 example.)
 *
 * Uses fpathconf(_PC_VDISABLE) to get the disable value.
 * VINTR = _POSIX_VDISABLE  → disables interrupt character.
 * VEOF  = 2 (^B)           → Control-B becomes EOF.
 */
int tty_disable_special(int fd);

#endif /* TTY_MODE_H */
