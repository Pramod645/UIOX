/**
 * @file    uiox_usbms_scsi.h
 * @brief   The SCSI command layer a USB mass-storage device needs.
 *
 * The BOT half already exists: uiox_usb_class.c's msc_ep_event() frames a
 * CBW, calls the scsi_cmd hook with the command block, and frames the
 * CSW around whatever that hook produced.  This file is the hook's
 * implementation — the commands a block device must answer.
 *
 * Opcodes are the SCSI ones, not invented here: SPC-4 for INQUIRY,
 * SBC-4 for READ CAPACITY / READ / WRITE / TEST UNIT READY.
 */
#ifndef UIOX_USBMS_SCSI_H
#define UIOX_USBMS_SCSI_H

#include "uiox_klibc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── SCSI opcodes (SBC-4 / SPC-4) ─────────────────────────────────── */
#define USBMS_SCSI_TEST_UNIT_READY  0x00u
#define USBMS_SCSI_REQUEST_SENSE    0x03u
#define USBMS_SCSI_INQUIRY          0x12u
#define USBMS_SCSI_MODE_SENSE_6     0x1Au
#define USBMS_SCSI_START_STOP       0x1Bu
#define USBMS_SCSI_READ_CAPACITY10  0x25u
#define USBMS_SCSI_READ10           0x28u
#define USBMS_SCSI_WRITE10          0x2Au
#define USBMS_SCSI_SYNC_CACHE10     0x35u

/* ── sense keys ───────────────────────────────────────────────────── */
#define USBMS_SENSE_OK              0x00u
#define USBMS_SENSE_NOT_READY       0x02u
#define USBMS_SENSE_MEDIUM_ERROR    0x03u
#define USBMS_SENSE_HARDWARE_ERROR  0x04u
#define USBMS_SENSE_ILLEGAL_REQUEST 0x05u
#define USBMS_SENSE_WRITE_PROTECTED 0x07u

#define USBMS_SECTOR_SIZE           512u
#define USBMS_MAX_TRANSFER_SECTORS  128u   /* one CBW's worth per call */

/* The device's own geometry, learned from READ CAPACITY(10) at bind. */
typedef struct {
    uint32_t last_lba;          /**< last addressable 512-byte sector */
    uint32_t block_size;        /**< reported sector size, bytes      */
    bool     write_protected;
    bool     ready;
} uiox_usbms_geom_t;

/* One sense reply, filled by the handler on failure. */
typedef struct {
    uint8_t  key;
    uint8_t  asc;
    uint8_t  ascq;
} uiox_usbms_sense_t;

/**
 * @brief  Answer one SCSI command block.
 *
 * The signature is uiox_usb_msc_t.scsi_cmd's, verbatim — this is its
 * implementation, so BOT's framing needs no change.
 *
 * @param  lun        logical unit from the CBW
 * @param  cb         the command block (CBWCB), cb_len bytes meaningful
 * @param  cb_len     bCBWCBLength
 * @param  data       the BOT data buffer: filled when data_in, read when not
 * @param  data_len   in: capacity of `data`; out: bytes produced
 * @param  data_in    true = device→host
 * @param  ctx        the uiox_usbms_ctx_t below
 * @return 0 on success, negative errno on failure (BOT maps it to CSW status)
 */
int uiox_usbms_scsi_cmd(uint8_t lun, const uint8_t *cb, uint8_t cb_len,
                        uint8_t *data, uint32_t *data_len,
                        bool data_in, void *ctx);

/* The transport the SCSI layer needs — implemented in liox_usbms_if.c,
 * so the SCSI code never names uiox_usb_if_t or a URB directly.  That
 * keeps the command semantics testable without a controller. */
typedef struct {
    uiox_usbms_geom_t  geom;
    uiox_usbms_sense_t sense;
    /* Transfer one block range to/from the medium through BOT. */
    int (*xfer)(uint64_t lba, uint32_t sectors,
                uint8_t *buf, uint32_t bytes, bool data_in, void *io_ctx);
    void  *io_ctx;
} uiox_usbms_ctx_t;

#ifdef __cplusplus
}
#endif
#endif /* UIOX_USBMS_SCSI_H */
