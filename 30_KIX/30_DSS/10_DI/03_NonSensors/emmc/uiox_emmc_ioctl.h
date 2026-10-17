/**
 * @file    uiox_emmc_ioctl.h
 * @brief   The eMMC control vocabulary — what /dev/emmc0 accepts.
 *
 * PATH    30_KIX/30_DSS/10_DI/03_NonSensors/emmc/uiox_emmc_ioctl.h
 *
 * 10_DI declares the ops and never performs them, so the numbers live
 * here and the dispatch lives in 20_DD (uiox_emmc_blk.c).  Userspace
 * includes this header for the constants; the kernel includes it for
 * the same reason.
 *
 * Every command below already has a driver entry point in
 * uiox_emmc_if.h — the ioctl body is a dispatch table over functions
 * that exist, not a new API.
 *
 * Convention: the third ioctl argument is a pointer to the type named
 * in the comment.  Read-only commands fill it; write commands read it.
 *
 * @version 1.0.0  @date 2026-10-17
 */
#ifndef UIOX_EMMC_IOCTL_H
#define UIOX_EMMC_IOCTL_H

#include "uiox_emmc_hw.h"   /* uiox_emmc_hw_part_t, uiox_emmc_ident_t */

/* 0xE0 is the class tag — "emmc" — kept clear of the SCFS_* range.
 * The offsets are ADDED, not OR-ed: `|` merges bits, so 0x02 and 0x22
 * collapse onto the same value and two commands collide.  `+` keeps the
 * bytes disjoint — a command number is 0xE0 + a 4-bit group + a 4-bit
 * index, mirroring the (family << 12) | index packing the registry uses. */
#define UIOX_EMMC_IOC_MAGIC   0xE0u

/* ── identity — read-only ──────────────────────────────────────────── */
/* arg: uiox_emmc_ident_t *   CID/CSD/EXT_CSD, capacity, life estimate */
#define UIOX_EMMC_IOC_GET_IDENT   (UIOX_EMMC_IOC_MAGIC + 0x01u)

/* arg: uint64_t *            EXT_CSD SEC_COUNT x 512 bytes */
#define UIOX_EMMC_IOC_GET_BYTES   (UIOX_EMMC_IOC_MAGIC + 0x02u)

/* arg: uint32_t *            blocks at BCACHE_SECTOR_SIZE, the same
 *                            number num_blocks() reports */
#define UIOX_EMMC_IOC_GET_BLOCKS  (UIOX_EMMC_IOC_MAGIC + 0x03u)

/* arg: uiox_emmc_part_info_t *   the named partition's size/state */
#define UIOX_EMMC_IOC_GET_PART    (UIOX_EMMC_IOC_MAGIC + 0x04u)

/* ── mode — write ──────────────────────────────────────────────────── */
/* arg: uiox_emmc_hw_part_t *   switch active partition */
#define UIOX_EMMC_IOC_SET_PART    (UIOX_EMMC_IOC_MAGIC + 0x10u)

/* arg: uint8_t *               1, 4 or 8 data lines */
#define UIOX_EMMC_IOC_SET_WIDTH   (UIOX_EMMC_IOC_MAGIC + 0x11u)

/* arg: uiox_emmc_hw_speed_t *  IDENT / DS / HS52 / HS200 / HS400 */
#define UIOX_EMMC_IOC_SET_SPEED   (UIOX_EMMC_IOC_MAGIC + 0x12u)

/* arg: uint32_t *              bus clock in Hz */
#define UIOX_EMMC_IOC_SET_CLOCK   (UIOX_EMMC_IOC_MAGIC + 0x13u)

/* ── health — read-only ────────────────────────────────────────────── */
/* arg: uint8_t[512] *          the raw EXT_CSD block, for a tool that
 *                              wants the bytes rather than the decode */
#define UIOX_EMMC_IOC_READ_EXT_CSD (UIOX_EMMC_IOC_MAGIC + 0x05u)

/* arg: uiox_emmc_if_stats_t *  counters the driver keeps */
#define UIOX_EMMC_IOC_GET_STATS   (UIOX_EMMC_IOC_MAGIC + 0x06u)


#endif /* UIOX_EMMC_IOCTL_H */
