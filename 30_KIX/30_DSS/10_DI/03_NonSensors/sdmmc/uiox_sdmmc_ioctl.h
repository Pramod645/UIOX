/**
 * @file    uiox_sdmmc_ioctl.h
 * @brief   The SD control vocabulary — what /dev/sdmmc0 accepts.
 *
 * 10_DI declares the numbers, 20_DD (uiox_sdmmc_blk.c) dispatches.
 * 0xE3 is the class tag — clear of SCFS_* and of eMMC's 0xE0 / NVMe's
 * 0xE2.
 *
 * Offsets are ADDED, not OR-ed: `|` merges bits, so 0x02 and 0x22
 * collapse onto one value and two commands collide.  `+` keeps the bytes
 * disjoint — a 4-bit group and a 4-bit index.
 */
#ifndef UIOX_SDMMC_IOCTL_H
#define UIOX_SDMMC_IOCTL_H

#include "uiox_sdmmc_hw.h"   /* uiox_sdmmc_ident_t */

#define UIOX_SDMMC_IOC_MAGIC   0xE3u

/* ── identity — read-only ─────────────────────────────────────────── */
/* arg: uiox_sdmmc_ident_t *  card type, rca, capacity, presence */
#define UIOX_SDMMC_IOC_GET_IDENT   (UIOX_SDMMC_IOC_MAGIC + 0x01u)

/* arg: uint64_t *            capacity in bytes */
#define UIOX_SDMMC_IOC_GET_BYTES   (UIOX_SDMMC_IOC_MAGIC + 0x02u)

/* arg: uint32_t *            blocks at BCACHE_SECTOR_SIZE */
#define UIOX_SDMMC_IOC_GET_BLOCKS  (UIOX_SDMMC_IOC_MAGIC + 0x03u)

/* arg: uint32_t *            UIOX_SDMMC_CARD_* — SDSC/SDHC/SDXC */
#define UIOX_SDMMC_IOC_GET_CARD    (UIOX_SDMMC_IOC_MAGIC + 0x04u)

/* arg: uiox_sdmmc_if_stats_t *  counters the driver keeps */
#define UIOX_SDMMC_IOC_GET_STATS   (UIOX_SDMMC_IOC_MAGIC + 0x05u)

/* ── medium — write ───────────────────────────────────────────────── */
/* arg: NULL                  re-run detect: seats/removes at runtime */
#define UIOX_SDMMC_IOC_RESCAN      (UIOX_SDMMC_IOC_MAGIC + 0x10u)

/* arg: NULL                  re-run card_init (CMD0→ACMD41→CMD2→CMD3) */
#define UIOX_SDMMC_IOC_REINIT      (UIOX_SDMMC_IOC_MAGIC + 0x11u)

/* arg: uint8_t *             1 or 4 data lines */
#define UIOX_SDMMC_IOC_SET_WIDTH   (UIOX_SDMMC_IOC_MAGIC + 0x12u)

/* arg: uint32_t *            bus clock in Hz */
#define UIOX_SDMMC_IOC_SET_CLOCK   (UIOX_SDMMC_IOC_MAGIC + 0x13u)

#endif /* UIOX_SDMMC_IOCTL_H */
