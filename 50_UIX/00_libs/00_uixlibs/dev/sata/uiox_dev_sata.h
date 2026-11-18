/*
 * 50_UIX/00_libs/00_uixlibs/dev/sata/uiox_dev_sata.h
 *
 * SATA — the application's view of the AHCI controller, at EL0.
 *
 * ── FAMILY 1 (BLOCK) ────────────────────────────────────────────────
 * The third BLOCK device, after eMMC and NVMe.  Its I/O is the
 * simplest of the three:
 *
 *     uiox_sata_read (dev, uint64_t lba, uint8_t *buf, uint32_t sectors)
 *
 * — no partition or namespace selector, so it maps onto uiox_dev_block
 * more directly than either sibling did.
 *
 * ── NINE COMMANDS, THE MOST OF ANY BLOCK CLASS ──────────────────────
 *   GET_IDENT   0xE2  uiox_sata_ident_t      model/serial/fw/capacity
 *   GET_BYTES   0xE3  uint64_t               device size in bytes
 *   GET_BLOCKS  0xE4  uint32_t               blocks at BCACHE_SECTOR_SIZE
 *   GET_MEDIA   0xE5  uint32_t               1 = SSD, 0 = HDD
 *   GET_STATS   0xE6  uiox_sata_if_stats_t   driver counters
 *   FLUSH       0xF1  NULL                   flush the volatile write cache
 *   REFRESH     0xF2  NULL                   re-run IDENTIFY
 *   STANDBY     0xF3  NULL                   STANDBY IMMEDIATE — spindown
 *   TRIM        0xF4                         discard
 *
 * Three of those — STANDBY, TRIM, GET_MEDIA — have no equivalent in
 * eMMC's or NVMe's vocabularies.
 *
 * ── NO_DEV IS A NORMAL STATE ────────────────────────────────────────
 * uiox_sata_state_t has UIOX_SATA_STATE_NO_DEV, which eMMC and NVMe
 * did not: a SATA port with nothing attached is expected, not an error.
 * An app should treat it as "no device here" rather than as a fault.
 *
 * ── WHAT IT CANNOT DO ───────────────────────────────────────────────
 *   smart_read(buf)   the device layer exposes it; there is no ioctl
 *                     for it in the nine
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_SATA_H
#define UIOX_DEV_SATA_H

#include "../uiox_dev_block.h"     /* the BLOCK shape — takes uiox_dev_handle_t */
#include "../sys/uix_types.h"

/* Mirrored from 30_KIX/30_DSS/10_DI/03_NonSensors/sata/uiox_sata_ioctl.h.
 * MAGIC 0xE1, offsets ADD (0xE1 + 0x01) rather than OR.  The family's
 * numbering is contiguous: eMMC 0xE0, SATA 0xE1, NVMe 0xE2. */
#define UIOX_DEV_SATA_IOC_GET_IDENT   0xE2u
#define UIOX_DEV_SATA_IOC_GET_BYTES   0xE3u
#define UIOX_DEV_SATA_IOC_GET_BLOCKS  0xE4u
#define UIOX_DEV_SATA_IOC_GET_MEDIA   0xE5u
#define UIOX_DEV_SATA_IOC_GET_STATS   0xE6u
#define UIOX_DEV_SATA_IOC_FLUSH       0xF1u
#define UIOX_DEV_SATA_IOC_REFRESH     0xF2u
#define UIOX_DEV_SATA_IOC_STANDBY     0xF3u
#define UIOX_DEV_SATA_IOC_TRIM        0xF4u

#define UIOX_DEV_SATA_DEVPATH  "sata"

/* Mirrored from 30_KIX/30_DSS/10_DI/03_NonSensors/sata/uiox_sata_hw.h:
 *   :299 SECTOR_SIZE 512 · :305 ATA_ID_WORDS 256 · :321 MODEL_LEN 48
 *   :322 MAX_PORTS 32
 * ATA_ID_WORDS and the three string caps are ARRAY BOUNDS INSIDE the
 * mirrored struct below, so a wrong value shifts every field after it. */
#define UIOX_DEV_SATA_SECTOR_SIZE  512u
#define UIOX_DEV_SATA_ATA_ID_WORDS 256u
#define UIOX_DEV_SATA_MODEL_LEN     48u
#define UIOX_DEV_SATA_MAX_PORTS     32u

/* The kernel's uiox_sata_ident_t (uiox_sata_hw.h:327-339), mirrored.
 * ELEVEN members, and the first four are arrays: the raw 512-byte
 * IDENTIFY buffer, then three NUL-terminated strings.  The string
 * lengths are 41/21/9 — the same as NVMe's — NOT UIOX_SATA_MODEL_LEN
 * (48), which sizes the hardware descriptor's own model field. */
typedef struct {
    char               model[UIOX_DEV_SATA_ATA_ID_WORDS];  /* raw words */
    char               model_str[41];
    char               serial_str[21];
    char               fw_str[9];
    unsigned long long lba48_sectors;
    unsigned long long capacity_bytes;
    unsigned int       rpm;              /* 0 = SSD              */
    unsigned char      ncq_depth;        /* 1–32                 */
    unsigned char      is_ssd;
    unsigned char      trim_supported;
    unsigned char      smart_supported;
} uiox_dev_sata_ident_t;

/* Device types, for printing. */
#define UIOX_DEV_SATA_DEV_NONE  0
#define UIOX_DEV_SATA_DEV_ATA   1
#define UIOX_DEV_SATA_DEV_ATAPI 2
#define UIOX_DEV_SATA_DEV_SEMB  3
#define UIOX_DEV_SATA_DEV_PM    4

/* Controller types, for printing. */
#define UIOX_DEV_SATA_CTRL_AHCI   0
#define UIOX_DEV_SATA_CTRL_LEGACY 1
#define UIOX_DEV_SATA_CTRL_ASPM   2

typedef struct {
    uiox_dev_handle_t h;      /* the shape's own handle — no wrapper type */
} uiox_dev_sata_t;

uiox_dev_err_t uiox_dev_sata_open (uiox_dev_sata_t *s, int unit, int flags);
uiox_dev_err_t uiox_dev_sata_close(uiox_dev_sata_t *s);

/* ── block I/O — delegated to the shape ──────────────────────────── */
uix_ssize_t uiox_dev_sata_read  (uiox_dev_sata_t *s, void *buf, uix_size_t len);
uix_ssize_t uiox_dev_sata_write (uiox_dev_sata_t *s, const void *buf, uix_size_t len);
uix_ssize_t uiox_dev_sata_pread (uiox_dev_sata_t *s, void *buf, uix_size_t len,
                                 unsigned long long offset);
uix_ssize_t uiox_dev_sata_pwrite(uiox_dev_sata_t *s, const void *buf, uix_size_t len,
                                 unsigned long long offset);

/* ── identity and geometry ───────────────────────────────────────── */
uiox_dev_err_t uiox_dev_sata_get_ident (uiox_dev_sata_t *s,
                                        uiox_dev_sata_ident_t *out);
uiox_dev_err_t uiox_dev_sata_get_bytes (uiox_dev_sata_t *s,
                                        unsigned long long *out_bytes);
uiox_dev_err_t uiox_dev_sata_get_blocks(uiox_dev_sata_t *s,
                                        unsigned int *out_blocks);
/* 1 = SSD (no rotating media), 0 = HDD. */
uiox_dev_err_t uiox_dev_sata_get_media (uiox_dev_sata_t *s, int *is_ssd);

/* Opaque: the caller includes the kernel header for
 * uiox_sata_if_stats_t and passes its address. */
uiox_dev_err_t uiox_dev_sata_get_stats (uiox_dev_sata_t *s, void *out);

/* ── the four writes, all arg: NULL ──────────────────────────────── */
uiox_dev_err_t uiox_dev_sata_flush  (uiox_dev_sata_t *s);
uiox_dev_err_t uiox_dev_sata_refresh(uiox_dev_sata_t *s);
uiox_dev_err_t uiox_dev_sata_standby(uiox_dev_sata_t *s);   /* spindown */
uiox_dev_err_t uiox_dev_sata_trim   (uiox_dev_sata_t *s, unsigned long long lba,
                                     unsigned int sectors);

#endif /* UIOX_DEV_SATA_H */
