/*
 * 50_UIX/00_libs/00_uixlibs/dev/nvme/uiox_dev_nvme.c
 *
 * NVMe on the BLOCK shape.  Every syscall belongs to the shape; the
 * commands go through uiox_dev_block_ioctl, the shape's own escape
 * hatch.  No 30_KIX header.
 */
#include "uiox_dev_nvme.h"

/* ── lifecycle ────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_nvme_open(uiox_dev_nvme_t *n, int unit, int flags)
{
    uiox_dev_err_t rc;

    if (!n) return UIOX_DEV_EBADNAME;

    /* The shape builds "/dev/<name><unit>" and calls sys_open.  unit 0
     * gives "/dev/nvme", the bare-prefix form devparse.c expects. */
    rc = uiox_dev_block_open(&n->h, UIOX_DEV_NVME_DEVPATH, flags);
    if (rc != UIOX_DEV_OK) return rc;

    n->h.unit = unit;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_nvme_close(uiox_dev_nvme_t *n)
{
    if (!n) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_close(&n->h);
}

/* ── block I/O ──────────────────────────────────────────────────────── */

uix_ssize_t uiox_dev_nvme_read(uiox_dev_nvme_t *n, void *buf, uix_size_t len)
{
    if (!n) return (uix_ssize_t)-1;
    return uiox_dev_block_read(&n->h, buf, len);
}

uix_ssize_t uiox_dev_nvme_write(uiox_dev_nvme_t *n, const void *buf, uix_size_t len)
{
    if (!n) return (uix_ssize_t)-1;
    return uiox_dev_block_write(&n->h, buf, len);
}

uix_ssize_t uiox_dev_nvme_pread(uiox_dev_nvme_t *n, void *buf, uix_size_t len,
                                unsigned long long offset)
{
    if (!n) return (uix_ssize_t)-1;
    return uiox_dev_block_pread(&n->h, buf, len, offset);
}

uix_ssize_t uiox_dev_nvme_pwrite(uiox_dev_nvme_t *n, const void *buf, uix_size_t len,
                                 unsigned long long offset)
{
    if (!n) return (uix_ssize_t)-1;
    return uiox_dev_block_pwrite(&n->h, buf, len, offset);
}

/* ── identity and geometry ───────────────────────────────────────────
 * GET_IDENT writes the whole struct — model, serial, firmware, nsid and
 * both capacity fields together — so one call answers everything the
 * identity holds. */

uiox_dev_err_t uiox_dev_nvme_get_ident(uiox_dev_nvme_t *n,
                                       uiox_dev_nvme_ident_t *out)
{
    unsigned int i;
    if (!n || !out) return UIOX_DEV_EBADNAME;
    for (i = 0u; i < sizeof *out; i++) ((unsigned char *)out)[i] = 0u;
    return uiox_dev_block_ioctl(&n->h, UIOX_DEV_NVME_IOC_GET_IDENT, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_nvme_get_bytes(uiox_dev_nvme_t *n,
                                       unsigned long long *out_bytes)
{
    unsigned long long v = 0ull;
    if (!n || !out_bytes) return UIOX_DEV_EBADNAME;
    if (uiox_dev_block_ioctl(&n->h, UIOX_DEV_NVME_IOC_GET_BYTES, &v) != 0)
        return UIOX_DEV_EIO;
    *out_bytes = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_nvme_get_blocks(uiox_dev_nvme_t *n,
                                        unsigned int *out_blocks)
{
    unsigned int v = 0u;
    if (!n || !out_blocks) return UIOX_DEV_EBADNAME;
    if (uiox_dev_block_ioctl(&n->h, UIOX_DEV_NVME_IOC_GET_BLOCKS, &v) != 0)
        return UIOX_DEV_EIO;
    *out_blocks = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_nvme_get_ns(uiox_dev_nvme_t *n, unsigned int *out_nsid)
{
    unsigned int v = 0u;
    if (!n || !out_nsid) return UIOX_DEV_EBADNAME;
    if (uiox_dev_block_ioctl(&n->h, UIOX_DEV_NVME_IOC_GET_NS, &v) != 0)
        return UIOX_DEV_EIO;
    *out_nsid = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_nvme_get_stats(uiox_dev_nvme_t *n, void *out)
{
    if (!n || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&n->h, UIOX_DEV_NVME_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

/* ── the two controller-level writes ─────────────────────────────────
 * Both take arg == NULL — the driver's own comment says so — so the
 * shape's escape hatch is called with no argument at all. */

uiox_dev_err_t uiox_dev_nvme_flush(uiox_dev_nvme_t *n)
{
    if (!n) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&n->h, UIOX_DEV_NVME_IOC_FLUSH, 0)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_nvme_refresh(uiox_dev_nvme_t *n)
{
    if (!n) return UIOX_DEV_EBADNAME;
    return uiox_dev_block_ioctl(&n->h, UIOX_DEV_NVME_IOC_REFRESH, 0)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
