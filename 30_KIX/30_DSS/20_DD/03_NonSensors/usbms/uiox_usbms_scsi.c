/**
 * @file    uiox_usbms_scsi.c
 * @brief   SCSI command handling for USB mass storage.
 *
 * uiox_usb_class.c's msc_ep_event() already does BOT: it receives the
 * CBW, checks the signature, calls this file's entry point, and frames
 * the CSW with the residue and status.  So this file answers commands
 * and nothing else — no USB, no endpoints, no URB.
 *
 * The three that matter for a block device:
 *   INQUIRY          — identify the unit
 *   READ CAPACITY(10)— geometry (last LBA + block size)
 *   READ(10)/WRITE(10) — the transfers blk_ops drives
 * plus TEST UNIT READY, which bcache's probe path may use.
 */
#include "uiox_usbms_scsi.h"

/* ── helpers ──────────────────────────────────────────────────────── */

static void msb_set16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8u); p[1] = (uint8_t)v; }
static void msb_set32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24u); p[1] = (uint8_t)(v >> 16u);
    p[2] = (uint8_t)(v >>  8u); p[3] = (uint8_t)v;
}
static uint32_t msb_get32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24u) | ((uint32_t)p[1] << 16u) |
           ((uint32_t)p[2] <<  8u) |  (uint32_t)p[3];
}

static int fail(uiox_usbms_ctx_t *c, uint8_t key, uint8_t asc, uint8_t ascq)
{
    if (c) { c->sense.key = key; c->sense.asc = asc; c->sense.ascq = ascq; }
    return -EIO;                 /* BOT turns this into CSW status 1 */
}

/* ── the commands ─────────────────────────────────────────────────── */

static int scsi_inquiry(uiox_usbms_ctx_t *c, uint8_t *data,
                        uint32_t *data_len)
{
    if (*data_len < 36u) return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);

    uint8_t *d = data;
    for (uint32_t i = 0; i < 36u; i++) d[i] = 0u;

    d[0] = 0x00u;                /* peripheral qualifier 0 */
    d[1] = 0x00u;                /* device type 0 = direct-access block */
    d[2] = 0x05u;                /* SPC-3 */
    d[3] = 0x02u;                /* response data format 2 */
    d[4] = 0x1Fu;                /* additional length = 31 */
    d[7] = 0x02u;                /* CMDQUE */

    /* vendor(8) product(16) revision(4), space-padded */
    const char *v = "UIOX    ";
    const char *p = "USB Mass Storage";
    const char *r = "1.0 ";
    for (uint32_t i = 0u; i < 8u;  i++) d[8u  + i] = (uint8_t)v[i];
    for (uint32_t i = 0u; i < 16u; i++) d[16u + i] = (uint8_t)p[i];
    for (uint32_t i = 0u; i < 4u;  i++) d[32u + i] = (uint8_t)r[i];

    *data_len = 36u;
    return 0;
}

static int scsi_read_capacity10(uiox_usbms_ctx_t *c, uint8_t *data,
                                uint32_t *data_len)
{
    if (*data_len < 8u) return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);
    if (!c->geom.ready) return fail(c, USBMS_SENSE_NOT_READY, 0x3Au, 0u);

    /* READ CAPACITY returns the LAST addressable LBA, not the count —
     * a one-off an implementation that returns the count gets wrong by
     * exactly one sector at the end of the device. */
    msb_set32(&data[0], c->geom.last_lba);
    msb_set32(&data[4], c->geom.block_size);
    *data_len = 8u;
    return 0;
}

static int scsi_read10(uiox_usbms_ctx_t *c, const uint8_t *cb,
                       uint8_t *data, uint32_t *data_len)
{
    if (!c->geom.ready) return fail(c, USBMS_SENSE_NOT_READY, 0x3Au, 0u);

    uint32_t lba    = msb_get32(&cb[2]);
    uint16_t blocks = (uint16_t)(((uint16_t)cb[7] << 8u) | cb[8]);

    if (blocks == 0u) { *data_len = 0u; return 0; }
    if (blocks > USBMS_MAX_TRANSFER_SECTORS)
        return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);   /* invalid field in CDB */
    if (lba > c->geom.last_lba ||
        (blocks - 1u) > (c->geom.last_lba - lba))
        return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x21u, 0u);   /* LBA out of range */

    uint32_t bytes = (uint32_t)blocks * USBMS_SECTOR_SIZE;
    if (*data_len < bytes) return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);

    int rc = c->xfer(lba, blocks, data, bytes, true, c->io_ctx);
    if (rc < 0) return fail(c, USBMS_SENSE_MEDIUM_ERROR, 0x11u, 0u);

    *data_len = bytes;
    return 0;
}

static int scsi_write10(uiox_usbms_ctx_t *c, const uint8_t *cb,
                        uint8_t *data, uint32_t *data_len)
{
    if (c->geom.write_protected)
        return fail(c, USBMS_SENSE_WRITE_PROTECTED, 0x27u, 0u);
    if (!c->geom.ready) return fail(c, USBMS_SENSE_NOT_READY, 0x3Au, 0u);

    uint32_t lba    = msb_get32(&cb[2]);
    uint16_t blocks = (uint16_t)(((uint16_t)cb[7] << 8u) | cb[8]);

    if (blocks == 0u) { *data_len = 0u; return 0; }
    if (blocks > USBMS_MAX_TRANSFER_SECTORS)
        return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);
    if (lba > c->geom.last_lba ||
        (blocks - 1u) > (c->geom.last_lba - lba))
        return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x21u, 0u);

    uint32_t bytes = (uint32_t)blocks * USBMS_SECTOR_SIZE;
    if (*data_len < bytes) return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);

    int rc = c->xfer(lba, blocks, data, bytes, false, c->io_ctx);
    if (rc < 0) return fail(c, USBMS_SENSE_MEDIUM_ERROR, 0x0Cu, 0u);

    *data_len = 0u;              /* WRITE(10) returns no data */
    return 0;
}

/* ── the entry point BOT calls ────────────────────────────────────── */

int uiox_usbms_scsi_cmd(uint8_t lun, const uint8_t *cb, uint8_t cb_len,
                        uint8_t *data, uint32_t *data_len,
                        bool data_in, void *ctx)
{
    (void)lun;
    uiox_usbms_ctx_t *c = (uiox_usbms_ctx_t *)ctx;
    if (!c || !cb || !data_len || cb_len < 6u) return -EINVAL;

    switch (cb[0]) {

    case USBMS_SCSI_INQUIRY:
        if (!data_in) return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);
        return scsi_inquiry(c, data, data_len);

    case USBMS_SCSI_READ_CAPACITY10:
        if (!data_in) return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);
        return scsi_read_capacity10(c, data, data_len);

    case USBMS_SCSI_READ10:
        if (!data_in) return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);
        return scsi_read10(c, cb, data, data_len);

    case USBMS_SCSI_WRITE10:
        if (data_in)  return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);
        return scsi_write10(c, cb, data, data_len);

    case USBMS_SCSI_TEST_UNIT_READY:
        *data_len = 0u;
        return c->geom.ready ? 0
             : fail(c, USBMS_SENSE_NOT_READY, 0x3Au, 0u);

    case USBMS_SCSI_SYNC_CACHE10:
        *data_len = 0u;          /* writes are synchronous through BOT */
        return 0;

    case USBMS_SCSI_REQUEST_SENSE: {
        if (!data_in || *data_len < 18u)
            return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x24u, 0u);
        for (uint32_t i = 0u; i < 18u; i++) data[i] = 0u;
        data[0]  = 0x70u;                    /* current error, fixed format */
        data[2]  = c->sense.key;
        data[7]  = 0x0Au;                    /* additional sense length   */
        data[12] = c->sense.asc;
        data[13] = c->sense.ascq;
        c->sense.key = USBMS_SENSE_OK;       /* one-shot: cleared on read */
        *data_len = 18u;
        return 0;
    }

    default:
        /* A command this unit does not implement is an illegal request,
         * not silence — the host must learn the unit cannot do it. */
        return fail(c, USBMS_SENSE_ILLEGAL_REQUEST, 0x20u, 0u);  /* invalid command opcode */
    }
}
