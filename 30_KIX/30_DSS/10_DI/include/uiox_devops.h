/**
 * @file    uiox_devops.h
 * @brief   The two ops vtables every 30_DSS layer dispatches through.
 *
 * PATH    30_KIX/30_DSS/10_DI/include/uiox_devops.h
 *
 * uiox_devclass.h holds `const void *ops` and explains why: the class
 * value packs its family, so a caller answers "which vtable is this?"
 * with UIOX_DEVCLASS_FAMILY(cls) and a shift — no lookup table, no
 * per-class header.  This file is where those vtables are declared.
 *
 * Two rules this header keeps:
 *
 *   1. It forward-declares uiox_dev_dev_t rather than including
 *      uiox_devclass.h, so the two headers stay self-sufficient — the
 *      same rule 32_FS/Makefile states for the 10_unfs <-> 02_jrnal
 *      pair: a mutual dependency lives in the .c files, never here.
 *
 *   2. It includes uiox_klibc.h only — no FwHal header.  10_DI declares
 *      the ops and never performs them; 02_FwHal is the other side.
 *
 * Both vtables take uiox_dev_dev_t * and reach the driver's state
 * through dev->drv_priv — the only handle bcache and the filesystem
 * have.  The driver's typed struct (uiox_emmc_if_t, …) lives behind
 * that pointer, cast once at the top of each entry point.
 *
 * @version 1.0.0  @date 2026-10-16
 */
#ifndef UIOX_DEVOPS_H
#define UIOX_DEVOPS_H

#include "uiox_klibc.h"     /* uint32_t, uint8_t, bool under -nostdinc */
/* Option 1 — give the vtable members a matching tag by asking
 * devclass.h to do it. Cleanest: no forward decl at all, just include
 * the header. It has no dependency on this one, so no cycle. */
#include "uiox_devclass.h"


#ifdef __cplusplus
extern "C" {
#endif

//struct uiox_dev_dev;
//typedef struct uiox_dev_dev uiox_dev_dev_t;

/* ═════════════════════════════════════════════════════════════════════
 * family 1 — BLOCK
 *
 * Addressed storage.  00_bcache is the only client of read/write: it
 * keys buffers on (dev, blkno), so a block index locates the bytes and
 * the cache can hold them.  Nothing here is pooled.
 *
 * UNIT: `blkno` and `count` are in BCACHE_SECTOR_SIZE units, not bytes.
 * A read of `count` blocks fills `count * BCACHE_SECTOR_SIZE` bytes at
 * `buf`.
 *
 * RETURN: 0 on success, negative errno on failure.  Every other vtable
 * in this tree uses the same convention, so uiox_fw_rc() is not needed
 * at this level — FwHal's error space is mapped down in the adapter, at
 * the one place that talks to it.
 *
 * num_blocks returning 0 means UNKNOWN, and bcache_internal.h is
 * explicit: callers skip the bound check rather than treating every
 * block as past the end.  A driver that has not yet read EXT_CSD
 * returns 0 and stays correct.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    int      (*read)       (uiox_dev_dev_t *dev, uint32_t blkno,
                            uint32_t count, uint8_t *buf);
    int      (*write)      (uiox_dev_dev_t *dev, uint32_t blkno,
                            uint32_t count, const uint8_t *buf);

    /* Push write-back state to the medium.  bcache calls this at
     * unmount; for eMMC it is CMD with FLUSH, for NVMe a flush command.
     * May be NULL — a driver with no write-back has nothing to flush. */
    int      (*flush)      (uiox_dev_dev_t *dev);

    /* Device size in BCACHE_SECTOR_SIZE units.  0 = unknown. */
    uint32_t (*num_blocks) (uiox_dev_dev_t *dev);

    /* The filesystem's ioctl path.  Reached from
     * 10_scfs/uiox_kix_scfs_ioctl.c after uiox_dev_lookup(), with the
     * (cls, unit) the inode carried.  May be NULL — ioctl then returns
     * -ENOTTY, the correct answer for a device with no control surface. */
    int      (*ioctl)      (uiox_dev_dev_t *dev, unsigned long cmd, void *arg);
} uiox_blk_ops_t;

/* ═════════════════════════════════════════════════════════════════════
 * families 4 and 5 — REG and CMD
 *
 * No buffer at all.  A REG read returns a value (a lux, a temperature, a
 * rail voltage); a CMD write sets a mode.  uiox_devclass.h is blunt
 * about it: "Their _buf.c/_buf.h pairs under 00_drvbuff are dead files —
 * a lux value has no stream."
 *
 * So the vtable has one slot.  What differs between the seven REG
 * classes and the three CMD classes is the command vocabulary, not the
 * shape of the call — and that vocabulary is per-class, which is why it
 * is not expressed here.
 *
 * The three CMD classes are GPU, HDMI and MONITOR.  UART is also CMD, at
 * index 15, deliberately clear of every family's range so a probe loop
 * cannot mistake the console for hardware to bind.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    int (*ioctl) (uiox_dev_dev_t *dev, unsigned long cmd, void *arg);
} uiox_devctl_ops_t;

/* ═════════════════════════════════════════════════════════════════════
 * Families 2, 3 and 6 — STREAM, EVENT and BUS — are deliberately absent.
 *
 * STREAM and EVENT are pooled by 00_drvbuff, and their frames move
 * through tx_submit / rx_poll on the class's own vtable in 10_DI, not
 * through a family-level one: a NIC's ring and a camera's frame queue
 * have nothing in common but the word "buffer".
 *
 * BUS creates other devices instead of transferring payload — its output
 * is a hotplug event.  A device that attaches takes a class from one of
 * the five families above, so tb4 and usbhc appear to the filesystem
 * only through what they enumerate.
 * ═════════════════════════════════════════════════════════════════════ */

#ifdef __cplusplus
}
#endif

#endif /* UIOX_DEVOPS_H */
