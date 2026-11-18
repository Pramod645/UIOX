/*
 * 50_UIX/00_libs/00_uixlibs/dev/sata/uiox_dev_sata.c
 *
 * SATA on the BLOCK shape.  Every syscall belongs to the shape; the
 * commands go through uiox_dev_block_ioctl, its own escape hatch.  No
 * 30_KIX header.
 */
#include "uiox_dev_sata.h"

/* ── lifecycle ────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_sata_open(uiox_dev_sata_t *s, int unit, int flags)
{
    uiox_dev_err_t rc;

    if (!s) return UIOX_DEV_EBADNAME;

    /* The shape builds "/dev/<name><unit>" and calls sys_open.  unit 0
     * gives "/dev/sata", the bare-prefix form devparse.c expects. */
    rc = uiox_dev_block_open(&s->h, UIOX_DEV_SATA_DEVPATH, flags);
    if (rc != UIOX_DEV_OK) return rc;

    s->h.unit = unit;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_sata_close(uiox_dev_sata_t *s)
{
    if (!s) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_close(&s->h);
}

/* ── block I/O ──────────────────────────────────────────────────────── */

uix_ssize_t uiox_dev_sata_read(uiox_dev_sata_t *s, void *buf, uix_size_t len)
{
    if (!s) return (uix_ssize_t)-1;
    return uiox_dev_block_read(&s->h, buf, len);
}

uix_ssize_t uiox_dev_sata_write(uiox_dev_sata_t *s, const void *buf, uix_size_t len)
{
    if (!s) return (uix_ssize_t)-1;
    return uiox_dev_block_write(&s->h, buf, len);
}

uix_ssize_t uiox_dev_sata_pread(uiox_dev_sata_t *s, void *buf, uix_size_t len,
                                unsigned long long offset)
{
    if (!s) return (uix_ssize_t)-1;
    return uiox_dev_block_pread(&s->h, buf, len, offset);
}

uix_ssize_t uiox_dev_sata_pwrite(uiox_dev_sata_t *s, const void *buf, uix_size_t len,
                                 unsigned long long offset)
{
    if (!s) return (uix_ssize_t)-1;
    return uiox_dev_block_pwrite(&s->h, buf, len, offset);
}

/* ── identity and geometry ───────────────────────────────────────────
 * GET_IDENT writes the whole struct — the raw IDENTIFY buffer, the
 * three strings, capacity and the media flags together — so one call
 * answers everything except the current controller state. */

uiox_dev_err_t uiox_dev_sata_get_ident(uiox_dev_sata_t *s,
                                       uiox_dev_sata_ident_t *out)
{
    unsigned int i;
    if (!s || !out) return UIOX_DEV_EBADNAME;
    for (i = 0u; i < sizeof *out; i++) ((unsigned char *)out)[i] = 0u;
    return uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_GET_IDENT, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_sata_get_bytes(uiox_dev_sata_t *s,
                                       unsigned long long *out_bytes)
{
    unsigned long long v = 0ull;
    if (!s || !out_bytes) return UIOX_DEV_EBADNAME;
    if (uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_GET_BYTES, &v) != 0)
        return UIOX_DEV_EIO;
    *out_bytes = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_sata_get_blocks(uiox_dev_sata_t *s,
                                        unsigned int *out_blocks)
{
    unsigned int v = 0u;
    if (!s || !out_blocks) return UIOX_DEV_EBADNAME;
    if (uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_GET_BLOCKS, &v) != 0)
        return UIOX_DEV_EIO;
    *out_blocks = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_sata_get_media(uiox_dev_sata_t *s, int *is_ssd)
{
    unsigned int v = 0u;
    if (!s || !is_ssd) return UIOX_DEV_EBADNAME;
    if (uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_GET_MEDIA, &v) != 0)
        return UIOX_DEV_EIO;
    /* The driver returns 1 for SSD and 0 for HDD — a word, not a byte. */
    *is_ssd = v ? 1 : 0;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_sata_get_stats(uiox_dev_sata_t *s, void *out)
{
    if (!s || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

/* ── the writes ──────────────────────────────────────────────────────
 * FLUSH, REFRESH and STANDBY all take arg == NULL — the driver's own
 * arg: comments say so — so the escape hatch is called with no argument
 * at all.  TRIM is the one write that carries a range. */

uiox_dev_err_t uiox_dev_sata_flush(uiox_dev_sata_t *s)
{
    if (!s) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_FLUSH, 0)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_sata_refresh(uiox_dev_sata_t *s)
{
    if (!s) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_REFRESH, 0)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_sata_standby(uiox_dev_sata_t *s)
{
    if (!s) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_STANDBY, 0)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_sata_trim(uiox_dev_sata_t *s, unsigned long long lba,
                                  unsigned int sectors)
{
    /* The ioctl's arg comment for TRIM was cut off in the header window,
     * so the argument shape is not confirmed.  A {lba, sectors} pair is
     * the conventional form and matches the device layer's own
     * uiox_sata_trim(dev, lba, sectors); if the driver expects a
     * different struct this needs adjusting. */
    struct { unsigned long long lba; unsigned int sectors; } r;
    if (!s) return UIOX_DEV_EBADNAME;
    r.lba     = lba;
    r.sectors = sectors;
    return uiox_dev_block_ioctl(&s->h, UIOX_DEV_SATA_IOC_TRIM, &r)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
