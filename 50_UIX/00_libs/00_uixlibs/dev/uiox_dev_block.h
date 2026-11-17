#ifndef UIOX_DEV_BLOCK_H
#define UIOX_DEV_BLOCK_H

#include "uiox_dev.h"
#include "../sys/uix_types.h"     /* uix_ssize_t, uix_size_t, uix_off_t */

/* ── shape 2: BLOCK ────────────────────────────────────────────────
 * Family 1 (emmc, sdmmc, nvme, sata, usbms): a byte-addressable device
 * you position within and transfer to.
 *
 * NO DEVICE COMMANDS APPEAR HERE.  A device's control numbers live with
 * the device — dev/emmc/, dev/nvme/ — and reach the driver through
 * uiox_dev_block_ioctl() below.  The shape knows the operation; the
 * device knows the command. */

uiox_dev_err_t uiox_dev_block_open (uiox_dev_handle_t *h, const char *name, int flags);
uiox_dev_err_t uiox_dev_block_close(uiox_dev_handle_t *h);

uix_ssize_t    uiox_dev_block_read (uiox_dev_handle_t *h, void *buf, uix_size_t n);
uix_ssize_t    uiox_dev_block_write(uiox_dev_handle_t *h, const void *buf, uix_size_t n);

/* Positioned transfer — the block shape's defining operation. */
uix_ssize_t    uiox_dev_block_pread (uiox_dev_handle_t *h, void *buf, uix_size_t n,
                                     unsigned long long offset);
uix_ssize_t    uiox_dev_block_pwrite(uiox_dev_handle_t *h, const void *buf, uix_size_t n,
                                     unsigned long long offset);

/* Escape hatch for a device's own commands.  `arg` is opaque: the device
 * file includes the kernel header for the real type and passes its
 * address, per the kernel convention that the third ioctl argument
 * points at the type named in the command's comment. */
int uiox_dev_block_ioctl(uiox_dev_handle_t *h, unsigned long request, void *arg);

#endif
