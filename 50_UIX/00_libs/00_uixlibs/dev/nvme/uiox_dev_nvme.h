/*
 * 50_UIX/00_libs/00_uixlibs/dev/nvme/uiox_dev_nvme.h
 *
 * NVMe — the application's view of the SSD, at EL0.
 *
 * ── FAMILY 1 (BLOCK) ────────────────────────────────────────────────
 * The second BLOCK device, after eMMC.  Same shape, one difference:
 * NVMe addresses by (namespace, LBA) rather than (partition, LBA).
 *
 * ── THE NAMESPACE IS NOT SELECTABLE FROM HERE ───────────────────────
 * GET_NS returns the ACTIVE namespace id — its argument is uint32_t *,
 * per uiox_nvme_ioctl.h:37 — and no command switches namespaces.  So
 * this layer is a block device over whichever namespace the controller
 * has active, and get_ns() reports which that is.  Selecting another
 * would need a command that does not exist.
 *
 * ── WHAT NVMe HAS THAT eMMC DID NOT ────────────────────────────────
 *   FLUSH    0xF2  controller-level, arg NULL — eMMC's vocabulary had
 *                  no flush command at all
 *   REFRESH  0xF3  re-run identify
 *
 * ── WHAT IT CANNOT DO ───────────────────────────────────────────────
 *   smart_log / format_ns   the device layer exposes both, but the
 *                           vocabulary has no command for either
 *   ns_info(nsid)           no command returns a namespace descriptor;
 *                           the ident struct carries nsid and capacity
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_NVME_H
#define UIOX_DEV_NVME_H

#include "../uiox_dev_block.h"     /* the BLOCK shape — takes uiox_dev_handle_t */
#include "../sys/uix_types.h"

/* Mirrored from 30_KIX/30_DSS/10_DI/03_NonSensors/nvme/uiox_nvme_ioctl.h.
 * The MAGIC is 0xE2 and offsets ADD (0xE2 + 0x01), never OR. */
#define UIOX_DEV_NVME_IOC_GET_IDENT   0xE3u
#define UIOX_DEV_NVME_IOC_GET_BYTES   0xE4u
#define UIOX_DEV_NVME_IOC_GET_BLOCKS  0xE5u
#define UIOX_DEV_NVME_IOC_GET_NS      0xE6u
#define UIOX_DEV_NVME_IOC_GET_STATS   0xE7u
#define UIOX_DEV_NVME_IOC_FLUSH       0xF2u
#define UIOX_DEV_NVME_IOC_REFRESH     0xF3u

#define UIOX_DEV_NVME_DEVPATH  "nvme"

/* Mirrored from uiox_nvme_hw.h:57-59.  Each length INCLUDES the NUL —
 * the comment says "the FwHal field width" — and matches the device
 * layer's %.40s / %.20s / %.8s prints.  These are ARRAY BOUNDS INSIDE
 * the mirrored struct, so a wrong value shifts nsid and both capacity
 * fields by whole strings. */
#define UIOX_DEV_NVME_MODEL_LEN   41u
#define UIOX_DEV_NVME_SERIAL_LEN  21u
#define UIOX_DEV_NVME_FWREV_LEN    9u

/* The kernel's uiox_nvme_ident_t (uiox_nvme_hw.h:98-108), mirrored.
 * Three char arrays, then nsid, then two uint64_t, then two bools.  The
 * compiler pads after fw_rev[9] to align nsid to 4 and again before
 * capacity_lba to align to 8 — the declaration order is what makes the
 * mirror correct, not any packing. */
typedef struct {
    char               model[UIOX_DEV_NVME_MODEL_LEN];
    char               serial[UIOX_DEV_NVME_SERIAL_LEN];
    char               fw_rev[UIOX_DEV_NVME_FWREV_LEN];
    unsigned int       nsid;             /* active namespace        */
    unsigned long long capacity_lba;     /* 512-byte units          */
    unsigned long long capacity_bytes;
    unsigned char      volatile_wc;      /* volatile write cache    */
    unsigned char      trim_supported;   /* DSM / TRIM available    */
} uiox_dev_nvme_ident_t;

typedef struct {
    uiox_dev_handle_t h;      /* the shape's own handle — no wrapper type */
} uiox_dev_nvme_t;

uiox_dev_err_t uiox_dev_nvme_open (uiox_dev_nvme_t *n, int unit, int flags);
uiox_dev_err_t uiox_dev_nvme_close(uiox_dev_nvme_t *n);

/* ── block I/O — delegated to the shape ──────────────────────────── */
uix_ssize_t uiox_dev_nvme_read  (uiox_dev_nvme_t *n, void *buf, uix_size_t len);
uix_ssize_t uiox_dev_nvme_write (uiox_dev_nvme_t *n, const void *buf, uix_size_t len);
uix_ssize_t uiox_dev_nvme_pread (uiox_dev_nvme_t *n, void *buf, uix_size_t len,
                                 unsigned long long offset);
uix_ssize_t uiox_dev_nvme_pwrite(uiox_dev_nvme_t *n, const void *buf, uix_size_t len,
                                 unsigned long long offset);

/* ── identity and geometry ───────────────────────────────────────── */
uiox_dev_err_t uiox_dev_nvme_get_ident (uiox_dev_nvme_t *n,
                                        uiox_dev_nvme_ident_t *out);
uiox_dev_err_t uiox_dev_nvme_get_bytes (uiox_dev_nvme_t *n,
                                        unsigned long long *out_bytes);
uiox_dev_err_t uiox_dev_nvme_get_blocks(uiox_dev_nvme_t *n,
                                        unsigned int *out_blocks);
uiox_dev_err_t uiox_dev_nvme_get_ns    (uiox_dev_nvme_t *n,
                                        unsigned int *out_nsid);

/* Opaque: the caller includes the kernel header for
 * uiox_nvme_if_stats_t and passes its address. */
uiox_dev_err_t uiox_dev_nvme_get_stats (uiox_dev_nvme_t *n, void *out);

/* ── the two controller-level writes ─────────────────────────────── */
uiox_dev_err_t uiox_dev_nvme_flush  (uiox_dev_nvme_t *n);   /* arg: NULL */
uiox_dev_err_t uiox_dev_nvme_refresh(uiox_dev_nvme_t *n);   /* arg: NULL */

#endif /* UIOX_DEV_NVME_H */
