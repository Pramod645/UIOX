/**
 * @file    uiox_uart_ioctl.h
 * @brief   The tty control vocabulary — what /dev/ttyS0 accepts.
 *
 * PATH    30_KIX/30_DSS/10_DI/01_Com/uart/uiox_uart_ioctl.h
 *
 * Convention copied from uiox_emmc_ioctl.h: a class tag plus disjoint
 * offsets, ADDED not OR-ed — `|` merges bits, so 0x02 and 0x22 collapse
 * onto one value and two commands collide.  `+` keeps the bytes apart,
 * mirroring the (family << 12) | index packing the registry uses.
 *
 * The semantics are POSIX's, as demonstrated by 50_UIX/01_shell/termio/
 * (APUE Ch.18): GETATTR is tcgetattr, SETATTR is tcsetattr, and the
 * action argument carries TCSANOW/TCSADRAIN/TCSAFLUSH.
 *
 * @version 1.0.0  @date 2026-10-21
 */
#ifndef UIOX_UART_IOCTL_H
#define UIOX_UART_IOCTL_H

#include "uiox_termios.h"

#define UIOX_TTY_IOC_MAGIC    0xC0u

/* arg: struct uiox_termios * — this IS the isatty() test in termio's
 * tty_id.h: "calls tcgetattr(fd, &ts); if it succeeds, fd is a tty". */
#define UIOX_TTY_IOC_GETATTR  (UIOX_TTY_IOC_MAGIC + 0x01u)

/* arg: struct uiox_termios * — tcsetattr.  The action
 * (TCSANOW/DRAIN/FLUSH) rides in the low bits of the command, so a
 * caller picks it without a second argument struct. */
#define UIOX_TTY_IOC_SETATTR  (UIOX_TTY_IOC_MAGIC + 0x02u)

/* arg: int * — 0 = input, 1 = output, 2 = both.  tcflush(). */
#define UIOX_TTY_IOC_FLUSH    (UIOX_TTY_IOC_MAGIC + 0x03u)

/* arg: struct uiox_winsize * — TIOCGWINSZ / TIOCSWINSZ.
 * termio's tty_winsize.h drives SIGWINCH off the setter. */
#define UIOX_TTY_IOC_GWINSZ   (UIOX_TTY_IOC_MAGIC + 0x04u)
#define UIOX_TTY_IOC_SWINSZ   (UIOX_TTY_IOC_MAGIC + 0x05u)

/* arg: NULL — tcdrain(). */
#define UIOX_TTY_IOC_DRAIN    (UIOX_TTY_IOC_MAGIC + 0x06u)

/* arg: uiox_tty_stats_t * */
#define UIOX_TTY_IOC_GETSTATS (UIOX_TTY_IOC_MAGIC + 0x07u)

/* arg: uint32_t * Hz */
#define UIOX_TTY_IOC_SETBAUD  (UIOX_TTY_IOC_MAGIC + 0x08u)

/* Setattr action selectors — the low 2 bits of the argument pointer's
 * meaning, kept here so both sides name them the same way. */
#define UIOX_TTY_ACT_NOW      0
#define UIOX_TTY_ACT_DRAIN    1
#define UIOX_TTY_ACT_FLUSH    2

#endif /* UIOX_UART_IOCTL_H */
