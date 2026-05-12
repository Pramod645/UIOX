#ifndef PTY_OPEN_H
#define PTY_OPEN_H

/*
 * pty_open.h — PTY master and slave open functions.
 *
 * Section 19.3 / Figure 19.9
 *
 * The two-function design:
 *
 *   ptym_open()  — opens the PTY master, runs grantpt/unlockpt,
 *                  returns slave name and master fd.
 *
 *   ptys_open()  — opens the PTY slave by name.
 *                  On Solaris, also pushes STREAMS modules.
 *
 * These are kept separate because the child must call setsid()
 * BEFORE opening the slave.  Opening the slave after setsid()
 * causes it to become the controlling terminal of the new session
 * on Linux, macOS, and Solaris (System V behavior).
 *
 * Sequence:
 *   parent:  fdm = ptym_open(pts_name, sizeof(pts_name))
 *   fork()
 *   child:   setsid()
 *            fds = ptys_open(pts_name)     ← slave now controlling terminal
 *
 * On FreeBSD: setsid() does not acquire controlling terminal
 *   automatically; must call ioctl(fds, TIOCSCTTY, 0) after ptys_open.
 */

#include "pty.h"

/*
 * ptym_open()
 *
 * Opens the next available PTY master device using the portable
 * POSIX XSI sequence:
 *   1. posix_openpt(O_RDWR) — find and open a free master
 *   2. grantpt(fdm)          — set slave uid=caller, gid=tty, mode=0620
 *   3. unlockpt(fdm)         — clear lock on slave
 *   4. ptsname(fdm)          — get slave pathname
 *
 * @param pts_name    Caller-allocated buffer to receive slave pathname.
 * @param pts_namesz  Size of pts_name buffer in bytes.
 *
 * Returns master fd on success, -1 on error (errno set).
 * pts_name is always null-terminated even on truncation.
 */
int ptym_open(char *pts_name, int pts_namesz);

/*
 * ptys_open()
 *
 * Opens the PTY slave device by pathname.
 *
 * On Solaris, after opening the slave, checks whether the
 * line discipline modules are already present (via autopush).
 * If not, pushes:
 *   ptem    — PTY emulation module
 *   ldterm  — terminal line discipline
 *   ttcompat — V7/4BSD/Xenix ioctl compatibility
 *
 * On other platforms, a simple open() is sufficient.
 *
 * @param pts_name  Slave device pathname (from ptym_open).
 *
 * Returns slave fd on success, -1 on error.
 */
int ptys_open(char *pts_name);

#endif /* PTY_OPEN_H */
