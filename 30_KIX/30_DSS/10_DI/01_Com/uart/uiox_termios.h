/**
 * @file    uiox_termios.h
 * @brief   Kernel termios — the tty's own vocabulary.
 *
 * PATH    30_KIX/30_DSS/10_DI/01_Com/uart/uiox_termios.h
 *
 * ── why it lives HERE and not at 30_DSS/include/ ───────────────────
 * uiox_devclass.h sits at 30_DSS/include/ because it is the REGISTRY's
 * vocabulary — families, class tuples.  Termios is the TTY's vocabulary,
 * and the tree's habit puts a class's constants in the class directory:
 * uiox_emmc_ioctl.h is in the emmc class directory, not shared.
 *
 * The practical reason: the FS Makefile already does
 *     DI_CLASS_DIRS := $(wildcard $(MFDIR)../30_DSS/10_DI
 *     COMMON_INCLUDES += $(addprefix -I,$(DI_CLASS_DIRS))
 * so a new class directory is on the include path with no Makefile edit.
 * 10_scfs reaches this header the same way — it is the ioctl boundary.
 *
 * ── the values are glibc's, deliberately ───────────────────────────
 * 50_UIX/01_shell/termio/ implements these semantics against a host libc
 * (<termios.h>, tcsetattr, TCSAFLUSH) and cannot link under -nostdinc.
 * If a userspace program ever shares this struct across the SCFS ioctl
 * boundary, both sides must agree — and glibc is what that reference was
 * written against.  Changing a value changes the ABI.
 *
 * @version 1.0.0  @date 2026-10-21
 */
#ifndef UIOX_TERMIOS_H
#define UIOX_TERMIOS_H

#include "uiox_klibc.h"

typedef uint32_t uiox_tcflag_t;
typedef uint8_t  uiox_cc_t;
typedef uint32_t uiox_speed_t;

#define UIOX_NCCS   19

struct uiox_termios {
    uiox_tcflag_t c_iflag;
    uiox_tcflag_t c_oflag;
    uiox_tcflag_t c_cflag;
    uiox_tcflag_t c_lflag;
    uiox_cc_t     c_line;
    uiox_cc_t     c_cc[UIOX_NCCS];
    uiox_speed_t  c_ispeed;
    uiox_speed_t  c_ospeed;
};

/* ── c_cc indices ──────────────────────────────────────────────────── */
#define UIOX_VINTR     0
#define UIOX_VQUIT     1
#define UIOX_VERASE    2
#define UIOX_VKILL     3
#define UIOX_VEOF      4
#define UIOX_VTIME     5
#define UIOX_VMIN      6
#define UIOX_VSWTC      7
#define UIOX_VSTART     8
#define UIOX_VSTOP      9
#define UIOX_VSUSP     10
#define UIOX_VEOL      11
#define UIOX_VREPRINT  12
#define UIOX_VDISCARD  13
#define UIOX_VWERASE   14
#define UIOX_VLNEXT    15
#define UIOX_VEOL2     16

/* ── c_iflag ───────────────────────────────────────────────────────── */
#define UIOX_IGNBRK   0x0001u
#define UIOX_BRKINT   0x0002u
#define UIOX_IGNPAR   0x0004u
#define UIOX_PARMRK   0x0008u
#define UIOX_INPCK    0x0010u
#define UIOX_ISTRIP   0x0020u
#define UIOX_INLCR    0x0040u
#define UIOX_IGNCR    0x0080u
#define UIOX_ICRNL    0x0100u
#define UIOX_IXON     0x0400u
#define UIOX_IXOFF    0x1000u

/* ── c_oflag ───────────────────────────────────────────────────────── */
#define UIOX_OPOST    0x0001u
#define UIOX_ONLCR    0x0004u

/* ── c_cflag ───────────────────────────────────────────────────────── */
#define UIOX_CSIZE    0x0030u
#define UIOX_CS5      0x0000u
#define UIOX_CS6      0x0010u
#define UIOX_CS7      0x0020u
#define UIOX_CS8      0x0030u
#define UIOX_CSTOPB   0x0040u
#define UIOX_CREAD    0x0080u
#define UIOX_PARENB   0x0100u
#define UIOX_CLOCAL   0x0800u

/* ── c_lflag ───────────────────────────────────────────────────────── */
#define UIOX_ISIG     0x0001u
#define UIOX_ICANON   0x0002u
#define UIOX_ECHO     0x0008u
#define UIOX_ECHOE    0x0010u
#define UIOX_ECHOK    0x0020u
#define UIOX_ECHONL   0x0040u
#define UIOX_IEXTEN   0x8000u

/* ── tcsetattr actions ─────────────────────────────────────────────── */
#define UIOX_TCSANOW    0
#define UIOX_TCSADRAIN  1
#define UIOX_TCSAFLUSH  2

struct uiox_winsize {
    uint16_t ws_row;
    uint16_t ws_col;
    uint16_t ws_xpixel;
    uint16_t ws_ypixel;
};

typedef struct {
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint32_t rx_overruns;
    uint32_t canon_lines;
    uint32_t erases;
    uint32_t kills;
    uint32_t intr_signals;
    uint32_t swinsz_changes;
} uiox_tty_stats_t;

/* POSIX default initial state — what a fresh console starts as, and what
 * tty_reset() in 50_UIX/01_shell/termio/ restores to. */
void uiox_tty_defaults(struct uiox_termios *t);

#endif /* UIOX_TERMIOS_H */
