/**
 * @file    uiox_nvme_ioctl.h
 * @brief   The NVMe control vocabulary — what /dev/nvme0 accepts.
 *
 * PATH    30_KIX/30_DSS/10_DI/03_NonSensors/nvme/uiox_nvme_ioctl.h or 10_DI/03_NonSensors/nvme/uiox_nvme_ioctl.h?
 *
 * 10_DI declares the numbers and never performs them; the dispatch lives
 * in 20_DD (uiox_nvme_blk.c).  0xE2 is the class tag — kept clear of
 * SCFS_* and of eMMC's 0xE0.
 *
 * The offsets are ADDED, not OR-ed: `|` merges bits, so 0x02 and 0x22
 * collapse onto one value and two commands collide.  `+` keeps the bytes
 * disjoint — 0xE2 + a 4-bit group + a 4-bit index, mirroring the
 * (family << 12) | index packing the registry uses.
 *
 * Every command below has a driver entry point in uiox_nvme_if.h; the
 * ioctl body is a dispatch over functions that exist.
 */
#ifndef UIOX_NVME_IOCTL_H
#define UIOX_NVME_IOCTL_H

#include "uiox_nvme_hw.h"   /* uiox_nvme_ident_t */

#define UIOX_NVME_IOC_MAGIC   0xE2u

/* ── identity — read-only ─────────────────────────────────────────── */
/* arg: uiox_nvme_ident_t *   model/serial/fw_rev/nsid/capacity */
#define UIOX_NVME_IOC_GET_IDENT   (UIOX_NVME_IOC_MAGIC + 0x01u)

/* arg: uint64_t *            namespace size in bytes */
#define UIOX_NVME_IOC_GET_BYTES   (UIOX_NVME_IOC_MAGIC + 0x02u)

/* arg: uint32_t *            blocks at BCACHE_SECTOR_SIZE */
#define UIOX_NVME_IOC_GET_BLOCKS  (UIOX_NVME_IOC_MAGIC + 0x03u)

/* arg: uint32_t *            active namespace id */
#define UIOX_NVME_IOC_GET_NS      (UIOX_NVME_IOC_MAGIC + 0x04u)

/* arg: uiox_nvme_if_stats_t *  counters the driver keeps */
#define UIOX_NVME_IOC_GET_STATS   (UIOX_NVME_IOC_MAGIC + 0x05u)

/* ── cache — write ────────────────────────────────────────────────── */
/* arg: NULL                  flush the volatile write cache */
#define UIOX_NVME_IOC_FLUSH       (UIOX_NVME_IOC_MAGIC + 0x10u)

/* arg: NULL                  re-run identify, refresh hw->ident */
#define UIOX_NVME_IOC_REFRESH     (UIOX_NVME_IOC_MAGIC + 0x11u)

#endif /* UIOX_NVME_IOCTL_H */
