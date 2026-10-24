/**
 * @file  uiox_uart_if.h
 * @brief UIOX console driver — termios state, the discipline, and the
 *        class's own vtable.
 *
 * PATH  30_KIX/30_DSS/20_DD/01_Com/uart/uiox_uart_if.h
 *
 * 20_DD performs what 10_DI declares.  The semantics are
 * 50_UIX/01_shell/termio/'s (APUE Ch.18) — that tree is the
 * specification, and it is libc-based, so it cannot be the kernel's
 * implementation.
 *
 * ── the class's own vtable lives HERE ───────────────────────────────
 * Same placement as uiox_gpu_ops_t in uiox_gpu_if.h: declared in the
 * class's 20_DD header, defined in the class's .c, and named nowhere
 * above.  uiox_devops.h is frozen at family granularity — family 5
 * (CMD) is uiox_devctl_ops_t, one ioctl slot — so a CMD class's byte
 * stream is the class's own business.
 *
 * dev->ops gets the FAMILY slot (uiox_devctl_ops_t).  The byte stream
 * is reached in-kernel through uiox_uart_if_read()/write()/pending() —
 * the path the boot-context shell uses — with dev->drv_priv and
 * hw->priv both pointing at the same uiox_uart_if_t.  read(2) on
 * /dev/ttyS0 arrives when a second CMD-with-stream class makes the
 * pattern worth generalising.
 *
 * KEYED BY (cls, unit): UIOX_DEVCLASS_UART = UIOX_DEVCLASS_(CMD, 15),
 * unit 0.  No major/minor anywhere in this path.
 *
 * @version 1.0.0  @date 2026-10-24
 */
#ifndef UIOX_UART_IF_H
#define UIOX_UART_IF_H

#include "uiox_uart_hw.h"
#include "uiox_termios.h"
#include "uiox_klibc.h"

typedef struct {
    uiox_uart_hw_t       *hw;
    struct uiox_termios   tio;        /* live termios                  */
    struct uiox_termios   saved;      /* for tty_reset() semantics     */
    struct uiox_winsize   win;
    bool                  primed;     /* config() done, start() not yet */
    bool                  saved_valid;
    uiox_tty_stats_t      stats;
    uint16_t              line_pos;   /* canonical assembly cursor     */
} uiox_uart_if_t;

/* ═════════════════════════════════════════════════════════════════════
 * The class's own vtable — private to the class, as uiox_gpu_ops_t is.
 *
 * It takes uiox_uart_if_t *, not uiox_dev_dev_t *: the family head is
 * generic, the class ops are the class's own.  Neither 10_scfs nor any
 * shared header names this type.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    int  (*read)    (uiox_uart_if_t *tif, uint8_t *buf, uint32_t count);
    int  (*write)   (uiox_uart_if_t *tif, const uint8_t *buf, uint32_t count);
    bool (*pending) (uiox_uart_if_t *tif);
} uiox_uart_class_ops_t;

/* Defined in uiox_uart_blk.c. */
extern const uiox_uart_class_ops_t s_uart_class_ops;

int  uiox_uart_if_config(uiox_uart_if_t *tif, uiox_uart_hw_t *hw);
int  uiox_uart_if_start (uiox_uart_if_t *tif);
void uiox_uart_if_stop  (uiox_uart_if_t *tif);

int  uiox_uart_if_read  (uiox_uart_if_t *tif, uint8_t *buf, uint32_t count);
int  uiox_uart_if_write (uiox_uart_if_t *tif, const uint8_t *buf, uint32_t count);
int  uiox_uart_if_ioctl (uiox_uart_if_t *tif, unsigned long cmd, void *arg);
bool uiox_uart_if_pending(uiox_uart_if_t *tif);

/* ISIG's signal delivery.  33_PCS owns it; weak by default so the
 * console links and refuses honestly before the scheduler exists,
 * rather than inventing a pcb. */
void uiox_tty_signal_from_char(uiox_uart_if_t *tif, uint8_t c);

#endif /* UIOX_UART_IF_H */
