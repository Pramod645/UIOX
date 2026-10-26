/**
 * @file    uiox_sata_ioctl.h
 * @brief   The SATA control vocabulary — what /dev/sata0 accepts.
 *
 * PATH    30_KIX/30_DSS/10_DI/03_NonSensors/sata/uiox_sata_ioctl.h
 *
 * 10_DI declares the numbers and never performs them; the dispatch lives
 * in 20_DD (uiox_sata_blk.c).  0xE1 is this class's tag: 0xE0 is eMMC,
 * 0xE2 NVMe, 0xE3 SDMMC, and a class tag that collides would make two
 * devices answer one command number.
 *
 * The offsets are ADDED, not OR-ed: `|` merges bits, so 0x02 and 0x22
 * collapse onto one value and two commands collide.  `+` keeps the bytes
 * disjoint — 0xE1 + a 4-bit group + a 4-bit index, mirroring the
 * (family << 12) | index packing the registry uses.
 *
 * Every command below has a driver entry point in uiox_sata_if.h; the
 * ioctl body is a dispatch over functions that exist.
 */
#ifndef UIOX_SATA_IOCTL_H
#define UIOX_SATA_IOCTL_H

#include "uiox_sata_hw.h"   /* uiox_sata_ident_t */

#define UIOX_SATA_IOC_MAGIC   0xE1u

/* ── identity — read-only ─────────────────────────────────────────── */
/* arg: uiox_sata_ident_t *   model/serial/fw_rev/lba48/capacity */
#define UIOX_SATA_IOC_GET_IDENT   (UIOX_SATA_IOC_MAGIC + 0x01u)

/* arg: uint64_t *            device size in bytes */
#define UIOX_SATA_IOC_GET_BYTES   (UIOX_SATA_IOC_MAGIC + 0x02u)

/* arg: uint32_t *            blocks at BCACHE_SECTOR_SIZE */
#define UIOX_SATA_IOC_GET_BLOCKS  (UIOX_SATA_IOC_MAGIC + 0x03u)

/* arg: uint32_t *            1 = SSD (no rotating media), 0 = HDD */
#define UIOX_SATA_IOC_GET_MEDIA   (UIOX_SATA_IOC_MAGIC + 0x04u)

/* arg: uiox_sata_if_stats_t *  counters the driver keeps */
#define UIOX_SATA_IOC_GET_STATS   (UIOX_SATA_IOC_MAGIC + 0x05u)

/* ── cache — write ────────────────────────────────────────────────── */
/* arg: NULL                  flush the volatile write cache */
#define UIOX_SATA_IOC_FLUSH       (UIOX_SATA_IOC_MAGIC + 0x10u)

/* arg: NULL                  re-run IDENTIFY, refresh hw->ident */
#define UIOX_SATA_IOC_REFRESH     (UIOX_SATA_IOC_MAGIC + 0x11u)

/* arg: NULL                  STANDBY IMMEDIATE — spindown */
#define UIOX_SATA_IOC_STANDBY     (UIOX_SATA_IOC_MAGIC + 0x12u)

/* arg: uiox_sata_trim_range_t *  Data Set Management hint */
#define UIOX_SATA_IOC_TRIM        (UIOX_SATA_IOC_MAGIC + 0x13u)

#endif /* UIOX_SATA_IOCTL_H */
