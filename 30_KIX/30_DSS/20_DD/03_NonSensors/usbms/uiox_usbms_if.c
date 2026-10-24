/**
 * @file    uiox_usbms_if.c
 * @brief   UIOX USB mass-storage interface implementation.
 *
 * Two seams meet here:
 *   · uiox_usb_class.c's msc_* drives BOT and calls scsi_cmd.
 *   · this file's scsi.xfer drives the actual block transfer through the
 *     same URB path uiox_usb_cdc_write() uses, and supplies scsi_cmd.
 *
 * REFRESHING GEOMETRY: READ CAPACITY(10) must run once, through BOT,
 * before blk_ops can answer num_blocks.  That happens in _start() below:
 * the MSC driver's own CBW path is not used — the geometry query is a
 * direct submit here, because at _start() the class driver is not yet
 * receiving host traffic.
 */
#include "uiox_usbms_if.h"

/* ── the xfer hook uiox_usbms_scsi.c calls ────────────────────────── */

static int usbms_xfer(uint64_t lba, uint32_t sectors, uint8_t *buf,
                      uint32_t bytes, bool data_in, void *io_ctx)
{
    /* lba and sectors address the transfer in the CDB, which the caller
     * (uiox_usbms_scsi.c) has already built and sent.  This is the data
     * phase only — the URB carries the payload — so the two are not
     * needed here.  They stay in the signature because the xfer hook is
     * the SCSI layer's interface. */
    (void)lba; (void)sectors;


    uiox_usbms_if_t *bif = (uiox_usbms_if_t *)io_ctx;
    if (!bif || !bif->uif || !buf || !bytes) return -EINVAL;

    uiox_usb_urb_t *urb = uiox_usb_buf_alloc();
    if (!urb) return -ENOMEM;

    if (bytes > UIOX_USB_URB_DATA_MAX) return -EINVAL;
    if (!data_in) memcpy(urb->buf, buf, bytes);     /* OUT: host → device */

    urb->buf_len = bytes;
    urb->ep_addr = data_in ? bif->ep_in : bif->ep_out;
    urb->type    = UIOX_URB_BULK;

    int rc = uiox_usb_if_submit(bif->uif, urb);
    if (rc < 0) { bif->stats.transport_errors++; bif->stats.errors++; return rc; }

    /* The completion path (uiox_usb_if_complete → msc_ep_event →
     * uiox_usbms_scsi_cmd) delivers the result; this hook's job is the
     * submission.  A synchronous wait is not introduced here — it would
     * deadlock against the bottom half that must run to complete it. */
    bif->stats.scsi_cmds++;
    if (data_in) { bif->stats.blocks_read += sectors;
                   bif->stats.bytes_read  += bytes; }
    else         { bif->stats.blocks_written += sectors;
                   bif->stats.bytes_written  += bytes; }
    return 0;
}

/* ── the scsi_cmd hook msc_ep_event() calls ───────────────────────── */

static int usbms_scsi_hook(uint8_t lun, const uint8_t *cb, uint8_t cb_len,
                           uint8_t *data, uint32_t *data_len,
                           bool data_in, void *ctx)
{
    uiox_usbms_if_t *bif = (uiox_usbms_if_t *)ctx;
    if (!bif) return -EINVAL;

    int rc = uiox_usbms_scsi_cmd(lun, cb, cb_len, data, data_len,
                                 data_in, &bif->scsi);
    if (rc < 0) { bif->stats.sense_errors++; bif->stats.errors++; }
    return rc;
}

/* ── config / start / stop ────────────────────────────────────────── */

int uiox_usbms_if_config(uiox_usbms_if_t *bif, uiox_usb_if_t *uif,
                         uint8_t ep_in, uint8_t ep_out)
{
    if (!bif || !uif) return -EINVAL;
    memset(bif, 0, sizeof(*bif));

    bif->uif    = uif;
    bif->ep_in  = ep_in;
    bif->ep_out = ep_out;
    bif->primed = true;

    /* The class driver: BOT, already written in uiox_usb_class.c. */
    int rc = uiox_usb_msc_init(&bif->msc, ep_in, ep_out, 0u);
    if (rc < 0) return rc;

    /* Point it at this file's SCSI handler.  Without this the hook stays
     * NULL and msc_ep_event() frames a CSW with no command executed. */
    bif->msc.scsi_cmd = usbms_scsi_hook;
    bif->msc.scsi_ctx = bif;

    /* The SCSI layer's view: geometry + sense + the transfer hook. */
    bif->scsi.io_ctx        = bif;
    bif->scsi.xfer          = usbms_xfer;
    bif->scsi.geom.ready    = false;
    bif->scsi.geom.block_size = USBMS_SECTOR_SIZE;

    return 0;
}

int uiox_usbms_if_start(uiox_usbms_if_t *bif)
{
    if (!bif || !bif->primed || !bif->uif || !bif->uif->hw) return -EINVAL;

    /* The device must be enumerated and configured before the class
     * driver can bind: bind opens the two bulk endpoints, which requires
     * a set configuration. */
    if (!bif->uif->hw->connected) return -ENODEV;

    int rc = bif->msc.base.bind(&bif->msc.base, bif->uif);
    if (rc < 0) return rc;

    /* ── geometry ─────────────────────────────────────────────────
     * READ CAPACITY(10) via the SCSI path, so both endpoints are live
     * and the answer flows through the same code a host READ would use.
     * Until this runs, geom.last_lba is 0 and num_blocks() would report
     * 1 block — a device that looks present and is 512 bytes long. */
    uint8_t  cap[8]  = { 0 };
    uint32_t cap_len = sizeof(cap);
    uint8_t  cdb[10] = { USBMS_SCSI_READ_CAPACITY10, 0,0,0,0,0,0,0,0,0 };

    rc = uiox_usbms_scsi_cmd(0u, cdb, sizeof(cdb), cap, &cap_len,
                             true, &bif->scsi);
    if (rc < 0) return rc;

    uint32_t last = ((uint32_t)cap[0] << 24u) | ((uint32_t)cap[1] << 16u) |
                    ((uint32_t)cap[2] <<  8u) |  (uint32_t)cap[3];
    uint32_t bsz  = ((uint32_t)cap[4] << 24u) | ((uint32_t)cap[5] << 16u) |
                    ((uint32_t)cap[6] <<  8u) |  (uint32_t)cap[7];

    bif->scsi.geom.last_lba   = last;
    bif->scsi.geom.block_size = bsz ? bsz : USBMS_SECTOR_SIZE;
    bif->scsi.geom.ready      = true;
    return 0;
}

void uiox_usbms_if_stop(uiox_usbms_if_t *bif)
{
    if (!bif) return;
    bif->msc.base.unbind(&bif->msc.base);
    bif->scsi.geom.ready = false;
}

/* ── the block transfers blk_ops drives ───────────────────────────── */

int uiox_usbms_if_read(uiox_usbms_if_t *bif, uint32_t lba,
                       uint8_t *buf, uint32_t count)
{
    if (!bif || !buf || !count) return -EINVAL;
    if (!bif->scsi.geom.ready) return -ENODEV;

    uint8_t  cdb[10] = { USBMS_SCSI_READ10, 0 };
    uint32_t bytes   = count * USBMS_SECTOR_SIZE;
    uint32_t len     = bytes;

    cdb[2] = (uint8_t)(lba >> 24u); cdb[3] = (uint8_t)(lba >> 16u);
    cdb[4] = (uint8_t)(lba >>  8u); cdb[5] = (uint8_t)lba;
    cdb[7] = (uint8_t)(count >> 8u); cdb[8] = (uint8_t)count;

    int rc = uiox_usbms_scsi_cmd(0u, cdb, sizeof(cdb), buf, &len,
                                 true, &bif->scsi);
    if (rc < 0) return rc;
    return (len == bytes) ? 0 : -EIO;
}

int uiox_usbms_if_write(uiox_usbms_if_t *bif, uint32_t lba,
                        const uint8_t *buf, uint32_t count)
{
    if (!bif || !buf || !count) return -EINVAL;
    if (!bif->scsi.geom.ready) return -ENODEV;
    if (bif->scsi.geom.write_protected) return -EACCES;

    uint8_t  cdb[10] = { USBMS_SCSI_WRITE10, 0 };
    uint32_t len     = count * USBMS_SECTOR_SIZE;

    cdb[2] = (uint8_t)(lba >> 24u); cdb[3] = (uint8_t)(lba >> 16u);
    cdb[4] = (uint8_t)(lba >>  8u); cdb[5] = (uint8_t)lba;
    cdb[7] = (uint8_t)(count >> 8u); cdb[8] = (uint8_t)count;

    /* The SCSI handler takes uint8_t *; WRITE(10) never writes to it. */
    return uiox_usbms_scsi_cmd(0u, cdb, sizeof(cdb), (uint8_t *)buf,
                               &len, false, &bif->scsi);
}

void uiox_usbms_if_stats_get(const uiox_usbms_if_t *bif,
                             uiox_usbms_if_stats_t *out)
{ if (!bif || !out) return; memcpy(out, &bif->stats, sizeof(*out)); }
