/**
 * @file  uiox_uart_if.h
 * @brief UIOX console driver — termios state and the line discipline.
 *
 * PATH  30_KIX/30_DSS/20_DD/01_Com/uart/uiox_uart_if.h
 *
 * 20_DD performs what 10_DI declares.  The semantics are
 * 50_UIX/01_shell/termio/'s (APUE Ch.18) — that tree is the
 * specification, and it is libc-based, so it cannot be the kernel's
 * implementation.
 *
 * @version 1.0.0  @date 2026-10-21
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
