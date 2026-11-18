/*
 * 50_UIX/00_libs/00_uixlibs/dev/emmc/uiox_dev_emmc.c
 *
 * EMMC on the BLOCK shape.  Three things the first attempt got wrong:
 *
 *   1. There is no uiox_dev_block_t.  The shape takes uiox_dev_handle_t *,
 *      so this layer holds one of those and passes &e->h at every call.
 *   2. open/close delegate to uiox_dev_block_open/_close, which own the
 *      sys_open/sys_close — so this file includes no syscall header.
 *   3. The commands go through uiox_dev_block_ioctl(), the shape's escape
 *      hatch, rather than uiox_dev_reg_get/_set.  A block device reaching
 *      through the REG accessors worked only by accident of both taking a
 *      handle pointer; the shape's own entry point is the honest one.
 */
#include "uiox_dev_emmc.h"

/* ── lifecycle ────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_emmc_open(uiox_dev_emmc_t *e, int unit, int flags)
{
    uiox_dev_err_t rc;

    if (!e) return UIOX_DEV_EBADNAME;

    /* The shape's open builds "/dev/<name><unit>" and calls sys_open.
     * unit 0 gives "/dev/emmc", which is what devparse.c expects for a
     * bare prefix. */
    rc = uiox_dev_block_open(&e->h, UIOX_DEV_EMMC_DEVPATH, flags);
    if (rc != UIOX_DEV_OK) return rc;

    e->h.unit = unit;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_emmc_close(uiox_dev_emmc_t *e)
{
    if (!e) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_close(&e->h);
}

/* ── block I/O ──────────────────────────────────────────────────────── */

uix_ssize_t uiox_dev_emmc_read(uiox_dev_emmc_t *e, void *buf, uix_size_t n)
{
    if (!e) return (uix_ssize_t)-1;
    return uiox_dev_block_read(&e->h, buf, n);
}

uix_ssize_t uiox_dev_emmc_write(uiox_dev_emmc_t *e, const void *buf, uix_size_t n)
{
    if (!e) return (uix_ssize_t)-1;
    return uiox_dev_block_write(&e->h, buf, n);
}

uix_ssize_t uiox_dev_emmc_pread(uiox_dev_emmc_t *e, void *buf, uix_size_t n,
                                unsigned long long offset)
{
    if (!e) return (uix_ssize_t)-1;
    return uiox_dev_block_pread(&e->h, buf, n, offset);
}

uix_ssize_t uiox_dev_emmc_pwrite(uiox_dev_emmc_t *e, const void *buf, uix_size_t n,
                                 unsigned long long offset)
{
    if (!e) return (uix_ssize_t)-1;
    return uiox_dev_block_pwrite(&e->h, buf, n, offset);
}

/* ── geometry and identity ───────────────────────────────────────────
 * GET_IDENT writes the whole struct — identity, health counters and the
 * partition table together — so one call answers everything. */

uiox_dev_err_t uiox_dev_emmc_get_ident(uiox_dev_emmc_t *e,
                                       uiox_dev_emmc_ident_t *out)
{
    unsigned int i;
    if (!e || !out) return UIOX_DEV_EBADNAME;
    for (i = 0u; i < sizeof *out; i++) ((unsigned char *)out)[i] = 0u;
    return uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_GET_IDENT, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_emmc_get_bytes(uiox_dev_emmc_t *e,
                                       unsigned long long *out_bytes)
{
    unsigned long long v = 0ull;
    if (!e || !out_bytes) return UIOX_DEV_EBADNAME;
    if (uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_GET_BYTES, &v) != 0)
        return UIOX_DEV_EIO;
    *out_bytes = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_emmc_get_blocks(uiox_dev_emmc_t *e,
                                        unsigned int *out_blocks)
{
    unsigned int v = 0u;
    if (!e || !out_blocks) return UIOX_DEV_EBADNAME;
    if (uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_GET_BLOCKS, &v) != 0)
        return UIOX_DEV_EIO;
    *out_blocks = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_emmc_get_part(uiox_dev_emmc_t *e,
                                      uiox_dev_emmc_part_info_t *out)
{
    unsigned int i;
    if (!e || !out) return UIOX_DEV_EBADNAME;
    for (i = 0u; i < sizeof *out; i++) ((unsigned char *)out)[i] = 0u;
    return uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_GET_PART, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_emmc_read_ext_csd(uiox_dev_emmc_t *e, void *out512)
{
    unsigned int i;
    if (!e || !out512) return UIOX_DEV_EBADNAME;
    for (i = 0u; i < UIOX_DEV_EMMC_EXT_CSD_LEN; i++)
        ((unsigned char *)out512)[i] = 0u;
    return uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_READ_EXT_CSD, out512)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_emmc_get_stats(uiox_dev_emmc_t *e, void *out)
{
    if (!e || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

/* ── configuration ───────────────────────────────────────────────────
 * Each argument is one byte or a word in the kernel struct, so a wider
 * value would be silently truncated.  Bounds are checked here. */

uiox_dev_err_t uiox_dev_emmc_set_part(uiox_dev_emmc_t *e, int part)
{
    int v;
    if (!e) return UIOX_DEV_EBADNAME;
    if (part < 0 || (unsigned int)part >= UIOX_DEV_EMMC_MAX_PARTS)
        return UIOX_DEV_EBADNAME;
    v = part;
    return uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_SET_PART, &v)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_emmc_set_width(uiox_dev_emmc_t *e, unsigned int lines)
{
    unsigned char v;
    if (!e) return UIOX_DEV_EBADNAME;
    if (lines != 1u && lines != 4u && lines != 8u) return UIOX_DEV_EBADNAME;
    v = (unsigned char)lines;
    return uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_SET_WIDTH, &v)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_emmc_set_speed(uiox_dev_emmc_t *e, int speed)
{
    int v;
    if (!e) return UIOX_DEV_EBADNAME;
    if (speed < 0 || speed > UIOX_DEV_EMMC_SPEED_HS400ES)
        return UIOX_DEV_EBADNAME;
    v = speed;
    return uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_SET_SPEED, &v)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_emmc_set_clock(uiox_dev_emmc_t *e, unsigned int hz)
{
    unsigned int v = hz;
    if (!e) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&e->h, UIOX_DEV_EMMC_IOC_SET_CLOCK, &v)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
