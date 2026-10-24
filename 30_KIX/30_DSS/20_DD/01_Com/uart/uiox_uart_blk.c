/**
 * @file    uiox_uart_blk.c
 * @brief   The seam for the console — the family-5 vtable.
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
 *   ioctl(fd, UIOX_TTY_IOC_GETATTR, &tio) on /dev/ttyS0
 *    → 10_scfs ioctl → uiox_dev_lookup(UIOX_DEVCLASS_UART, 0u)
 *      → dev->ops->ioctl   (family 5)              [HERE]
 *        → uiox_uart_if_ioctl(tif, cmd, arg)       [uiox_uart_if.c]
 *
 *   The byte stream does NOT come through here — read(2) on a CMD inode
 *   has no path in 10_scfs, and a CMD class's vtable is private (see
 *   uiox_uart_if.h).  The boot-context shell calls uiox_uart_if_read()
 *   in-kernel instead.
 *
 * ── why the family slot only ────────────────────────────────────────
 * uiox_devops.h is FROZEN: family 5 (CMD) is uiox_devctl_ops_t, ONE
 * slot.  uiox_gpu_if.c:174 binds .ops = &uiox_gpu_class_ops with no
 * offset and uiox_emmc_probe.c:120 binds .ops = &s_emmc_blk_ops the
 * same way — the commented-out `uiox_dev_ops_t head` in uiox_blk_ops_t
 * is not in the working type.  So dev->ops here is the family vtable,
 * bare, and nothing computes an offset into it.
 *
 * @version 1.0.0  @date 2026-10-24
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

/* ═════════════════════════════════════════════════════════════════════
 * Family-5 vtable — uiox_devctl_ops_t has ONE slot, so this is the whole
 * of what dev->ops points at.
 * ═════════════════════════════════════════════════════════════════════ */
static int uart_devctl_ioctl(uiox_dev_dev_t *dev, unsigned long cmd, void *arg)
{
    uiox_uart_if_t *t = tif_of(dev);
    if (!t) return -ENODEV;
    return uiox_uart_if_ioctl(t, cmd, arg);
}

const uiox_devctl_ops_t s_uart_devctl_ops = {
    .ioctl = uart_devctl_ioctl,
};

/* ═════════════════════════════════════════════════════════════════════
 * The class ops — declared in uiox_uart_if.h, defined here.
 *
 * They take uiox_uart_if_t * directly: the caller already holds it
 * (the boot-context shell has &s_uart_if), so there is no unwrap to do.
 * These are the in-kernel entry points; nothing above the class calls
 * them through dev->ops, because a CMD class's vtable is private.
 * ═════════════════════════════════════════════════════════════════════ */
static int uart_class_read(uiox_uart_if_t *tif, uint8_t *buf, uint32_t count)
{
    return uiox_uart_if_read(tif, buf, count);
}

static int uart_class_write(uiox_uart_if_t *tif, const uint8_t *buf,
                            uint32_t count)
{
    return uiox_uart_if_write(tif, buf, count);
}

static bool uart_class_pending(uiox_uart_if_t *tif)
{
    return uiox_uart_if_pending(tif);
}

const uiox_uart_class_ops_t s_uart_class_ops = {
    .read    = uart_class_read,
    .write   = uart_class_write,
    .pending = uart_class_pending,
};
