#ifndef UIOX_DEV_H
#define UIOX_DEV_H

/* ── open flags ──────────────────────────────────────────────────────
 * Mirrored from the kernel, which sys_open forwards raw into the trap:
 *   30_KIX/32_FS/10_scfs/include/uiox_kix_scfs.h:274-275
 * No O_* exists in 40_SCIX, so these must stay numerically identical. */
#define UIOX_O_RDONLY   0x000
#define UIOX_O_WRONLY   0x001
#define UIOX_O_RDWR     0x002

/* ── mmap flags ──────────────────────────────────────────────────────
 * Mirrored from uiox_kix_scfs.h:326-327 (SCFS_MAP_SHARED / _PRIVATE).
 * sys_mmap forwards flags raw, so these must match.  The shape files
 * pass them through without interpreting them. */
#define UIOX_DEV_MAP_SHARED   0x01
#define UIOX_DEV_MAP_PRIVATE  0x02

/* ── mmap protection ─────────────────────────────────────────────────
 * Mirrored from uiox_kix_scfs.h:329-330 (SCFS_PROT_READ / _WRITE).
 * No exec protection exists in the kernel's vocabulary, so there is
 * none here either. */
#define UIOX_DEV_PROT_READ    0x1
#define UIOX_DEV_PROT_WRITE   0x2

/* ── node path ───────────────────────────────────────────────────────
 * "/dev/" + prefix + unit + NUL.  The longest stem in s_devname[] is
 * "usbhid" (6), so a 5-digit unit still fits inside 32. */
#define UIOX_DEV_PATH_MAX  32

typedef struct { int fd; int unit; } uiox_dev_handle_t;

typedef enum {
    UIOX_DEV_OK       =  0,
    UIOX_DEV_EBADNAME = -1,   /* no such class — devparse.c refuses, so we do */
    UIOX_DEV_ENODEV   = -2,
    UIOX_DEV_EIO      = -3
} uiox_dev_err_t;

/* One place the node name is formatted, so all six shapes agree with
 * 30_KIX/30_DSS/src/uiox_devparse.c: a bare prefix is unit 0, a unit
 * appends a decimal, a name naming no class fails rather than defaults. */
uiox_dev_err_t uiox_dev_make_path(const char *prefix, int unit,
                                  char *out, unsigned long cap);
#endif
