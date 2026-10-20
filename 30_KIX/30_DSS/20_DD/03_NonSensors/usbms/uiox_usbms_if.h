/**
 * @file    uiox_usbms_if.h
 * @brief   UIOX USB mass-storage interface — binds MSC to a device.
 *
 * The class driver itself is uiox_usb_class.c's msc_* block: BOT is
 * already written there, and uiox_usb_msc_init() fills a
 * uiox_usb_class_drv_t.  What this file adds is the SCSI half (the
 * scsi_cmd hook msc_ep_event calls) and the medium geometry it needs.
 */
#ifndef UIOX_USBMS_IF_H
#define UIOX_USBMS_IF_H

#include "uiox_usb_class.h"     /* uiox_usb_msc_t, uiox_usb_if_t */
#include "uiox_usbms_scsi.h"    /* uiox_usbms_ctx_t, geom, sense */
#include "uiox_klibc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t blocks_read;
    uint64_t blocks_written;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint32_t scsi_cmds;
    uint32_t sense_errors;
    uint32_t transport_errors;
    uint32_t errors;
} uiox_usbms_if_stats_t;

typedef struct {
    uiox_usb_msc_t       msc;      /**< the BOT class driver          */
    uiox_usb_if_t       *uif;      /**< the interface it is bound to  */
    uiox_usbms_ctx_t     scsi;     /**< geometry + sense + xfer hook  */
    uiox_usbms_if_stats_t stats;
    uint8_t              ep_in, ep_out;
    bool                 primed;
} uiox_usbms_if_t;

int  uiox_usbms_if_config   (uiox_usbms_if_t *bif, uiox_usb_if_t *uif,
                             uint8_t ep_in, uint8_t ep_out);
int  uiox_usbms_if_start    (uiox_usbms_if_t *bif);
void uiox_usbms_if_stop     (uiox_usbms_if_t *bif);

int  uiox_usbms_if_read     (uiox_usbms_if_t *bif, uint32_t lba,
                             uint8_t *buf, uint32_t count);
int  uiox_usbms_if_write    (uiox_usbms_if_t *bif, uint32_t lba,
                             const uint8_t *buf, uint32_t count);

void uiox_usbms_if_stats_get(const uiox_usbms_if_t *bif,
                             uiox_usbms_if_stats_t *out);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_USBMS_IF_H */
