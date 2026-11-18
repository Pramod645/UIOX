/*
 * 50_UIX/00_libs/00_uixlibs/dev/chg/uiox_dev_chg.h
 *
 * CHG — the application's view of the charger, at EL0.
 *
 * ── FAMILY 4 (REG) ──────────────────────────────────────────────────
 * Six commands, and four of them WRITE — the first class in this layer
 * where userspace can drive the hardware rather than only observe it.
 *
 *   GET_STATUS  0x01  out: uiox_chg_status_t
 *   SET_ICHG    0x02  in:  uint32_t mA
 *   SET_VCHG    0x03  in:  uint32_t mV
 *   SET_IIN     0x04  in:  uint32_t mA
 *   SET_ENABLE  0x05  in:  bool
 *   GET_STATS   0x06  out: uiox_chg_if_stats_t
 *
 * ── GET_STATUS POLLS THEN READS ─────────────────────────────────────
 * uiox_chg_if.c:242-252 polls once and then copies the descriptor's
 * cache (last_chrg/last_src/last_faults).  So one call returns current
 * status — no separate tick needed before reading.
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_CHG_H
#define UIOX_DEV_CHG_H

#include "../uiox_dev_reg.h"

/* Mirrored from 30_KIX/30_DSS/20_DD/03_NonSensors/chg/uiox_chg_if.h:73-78.
 * Values MUST stay identical; the driver's number space is not validated. */
#define UIOX_DEV_CHG_IOC_GET_STATUS 0x01u
#define UIOX_DEV_CHG_IOC_SET_ICHG   0x02u
#define UIOX_DEV_CHG_IOC_SET_VCHG   0x03u
#define UIOX_DEV_CHG_IOC_SET_IIN    0x04u
#define UIOX_DEV_CHG_IOC_SET_ENABLE 0x05u
#define UIOX_DEV_CHG_IOC_GET_STATS  0x06u

#define UIOX_DEV_CHG_DEVPATH  "chg"

/* The kernel's uiox_chg_status_t, mirrored.  Both enums are plain
 * enums with no explicit values (uiox_chg_hw.h:40-55, :152-163), so
 * each is int-sized and faults sits at offset 8.  Six fields would
 * change that; three does not. */
typedef struct {
    int           chrg;      /* UIOX_CHG_CHRG_*  */
    int           src;       /* UIOX_CHG_HW_SRC_* */
    unsigned int  faults;    /* UIOX_CHG_FAULT_* bitmap */
} uiox_dev_chg_status_t;

typedef struct {
    uiox_dev_handle_t h;
} uiox_dev_chg_t;

uiox_dev_err_t uiox_dev_chg_open (uiox_dev_chg_t *c, int unit, int flags);
uiox_dev_err_t uiox_dev_chg_close(uiox_dev_chg_t *c);

/* Status — one poll-and-read. */
uiox_dev_err_t uiox_dev_chg_get_status(uiox_dev_chg_t *c,
                                       uiox_dev_chg_status_t *out);

/* Control — the four writes.  Values are the kernel's units: mA, mV,
 * and a boolean as an integer, since the trap carries words. */
uiox_dev_err_t uiox_dev_chg_set_ichg  (uiox_dev_chg_t *c, unsigned int ma);
uiox_dev_err_t uiox_dev_chg_set_vchg  (uiox_dev_chg_t *c, unsigned int mv);
uiox_dev_err_t uiox_dev_chg_set_iin   (uiox_dev_chg_t *c, unsigned int ma);
uiox_dev_err_t uiox_dev_chg_set_enable(uiox_dev_chg_t *c, int on);

/* Opaque: the caller includes the kernel header for
 * uiox_chg_if_stats_t and passes its address. */
uiox_dev_err_t uiox_dev_chg_get_stats(uiox_dev_chg_t *c, void *out);

#endif
