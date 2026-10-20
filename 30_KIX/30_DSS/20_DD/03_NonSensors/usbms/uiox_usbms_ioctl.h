/**
 * @file    uiox_usbms_ioctl.h
 * @brief   The USB mass-storage control vocabulary — /dev/usbms0.
 *
 * 0xE4 is the class tag — clear of SCFS_*, eMMC's 0xE0, NVMe's 0xE2 and
 * SD's 0xE3.  Offsets are ADDED, not OR-ed: `|` merges bits, so 0x02 and
 * 0x22 collapse onto one value and two commands collide.
 */
#ifndef UIOX_USBMS_IOCTL_H
#define UIOX_USBMS_IOCTL_H

#include "uiox_usbms_if.h"

#define UIOX_USBMS_IOC_MAGIC   0xE4u

/* arg: uint64_t *            capacity in bytes */
#define UIOX_USBMS_IOC_GET_BYTES   (UIOX_USBMS_IOC_MAGIC + 0x01u)

/* arg: uint32_t *            blocks at BCACHE_SECTOR_SIZE */
#define UIOX_USBMS_IOC_GET_BLOCKS  (UIOX_USBMS_IOC_MAGIC + 0x02u)

/* arg: uiox_usbms_sense_t *  last SCSI sense key/asc/ascq */
#define UIOX_USBMS_IOC_GET_SENSE   (UIOX_USBMS_IOC_MAGIC + 0x03u)

/* arg: uiox_usbms_if_stats_t *  counters the driver keeps */
#define UIOX_USBMS_IOC_GET_STATS   (UIOX_USBMS_IOC_MAGIC + 0x04u)

#endif /* UIOX_USBMS_IOCTL_H */
