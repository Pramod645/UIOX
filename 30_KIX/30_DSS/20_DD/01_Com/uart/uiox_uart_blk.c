/**
 * @file    uiox_uart_blk.c
 * @brief   The tty vtable for the console — the one place the
 *          filesystem's dispatch meets the driver.
 *
 * PATH    30_KIX/30_DSS/20_DD/01_Com/uart/uiox_uart_blk.c
 *
 * Same job as uiox_emmc_blk.c, one family over.  Everything above knows
 * nothing about uiox_uart_if_t: 10_scfs holds a (cls, unit) pair, the
 * registry returns a uiox_dev_dev_t.  This file unwraps dev->drv_priv
 * back to the driver's typed pointer and hands the call over.
 *
 * THE PATH IT COMPLETES
 *
 *   read(2) on /dev/ttyS0
 *    → 10_scfs read() → uiox_dev_lookup(UIOX_DEVCLASS_UART, 0)
 *      → ((uiox_tty_ops_t*)d->ops)->read(...)        [HERE]
 *        → uiox_uart_if_read(tif, buf, count)        [uiox_uart_if.c]
 *          → the line discipline                     [ICANON, ISIG, ECHO]
 *
 * NOTE — the ops layout.  uiox_kix_scfs_read.c reaches family ops at
 * `(const char *)dev->ops + sizeof(uiox_dev_ops_t)` for the EVENT
 * family, so this struct must carry the head FIRST, exactly as
 * s_emmc_blk_ops does.  The head member below mirrors uiox_blk_ops_t's;
 * if uiox_devops.h names it differently, take that name — it is the one
 * thing here that must be copied, not reasoned about.
 *
 * @version 1.0.0  @date 2026-10-21
 */
#include "uiox_uart_if.h"
#include "uiox_uart_ioctl.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"
#include "uiox_klibc.h"

/* ═════════════════════════════════════════════════════════════════════
 * The one unwrap.  ops is written once at bind time and drv_priv only by
 * the driver, which is what makes the cast safe: nothing outside the
 * driver ever puts a different kind of pointer in drv_priv.
 * ═════════════════════════════════════════════════════════════════════ */
static uiox_uart_if_t *tif_of(uiox_dev_dev_t *dev)
{
    if (!dev || !dev->drv_priv) return NULL;
    return (uiox_uart_if_t *)dev->drv_priv;
}

static int uart_read(uiox_dev_dev_t *dev, uint8_t *buf, uint32_t count)
{
    uiox_uart_if_t *t = tif_of(dev);
    if (!t) return -ENODEV;
    return uiox_uart_if_read(t, buf, count);
}

static int uart_write(uiox_dev_dev_t *dev, const uint8_t *buf, uint32_t count)
{
    uiox_uart_if_t *t = tif_of(dev);
    if (!t) return -ENODEV;
    return uiox_uart_if_write(t, buf, count);
}

static int uart_ioctl(uiox_dev_dev_t *dev, unsigned long cmd, void *arg)
{
    uiox_uart_if_t *t = tif_of(dev);
    if (!t) return -ENODEV;
    return uiox_uart_if_ioctl(t, cmd, arg);
}

static bool uart_pending(uiox_dev_dev_t *dev)
{
    uiox_uart_if_t *t = tif_of(dev);
    return t ? uiox_uart_if_pending(t) : false;
}

/* ── the vtable ─────────────────────────────────────────────────────
 * Non-static: uiox_uart_probe.c takes its address via `extern const`. */
const uiox_tty_ops_t s_uart_tty_ops = {
    //.head    = { .type = UIOX_DEVFAM_CMD },   /* whatever the head's members are */ aaded for teminal was not in early
    .read    = uart_read,
    .write   = uart_write,
    .ioctl   = uart_ioctl,
    .pending = uart_pending,
};
