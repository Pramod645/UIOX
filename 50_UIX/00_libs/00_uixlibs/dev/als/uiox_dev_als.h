/*
 * 50_UIX/00_libs/00_uixlibs/dev/als/uiox_dev_als.h
 *
 * ALS — the application's view of the sensor, at EL0.
 *
 * ── WHERE THE CONVERSION HAPPENS ────────────────────────────────────
 * The kernel's ALS ioctl vocabulary is raw counts and setters:
 *     GET_RAW  GET_WHITE  GET_IR  SET_GAIN  SET_ITIME  SET_THRESH  GET_STATS
 * There is no GET_LUX and no GET_CCT — deliberately not added, because
 * the kernel side stays as it is.
 *
 * So counts -> millilux and counts -> CCT are computed HERE, in the
 * device layer.  That is a real cost, not a free choice:
 *
 *   · the coefficient table below must match the IC the kernel is
 *     driving.  It is one entry per supported part, selected at open
 *     from the IC the caller names.
 *   · if the kernel ever retunes its own conversion, these numbers stop
 *     agreeing with it.  Nothing will fail; the readings will just
 *     differ.  Keep this table and the kernel's in step by hand.
 *
 * ── WHAT THIS LAYER DOES NOT DO ─────────────────────────────────────
 * It does not talk to I2C, hold a sample cache, manage gain policy or
 * own thresholds.  Those are the kernel's.  It converts and it names.
 *
 * v1.0.0  @date 2026-11-17
 */
#ifndef UIOX_DEV_ALS_H
#define UIOX_DEV_ALS_H

#include "../uiox_dev_reg.h"      /* the family shape — not a 30_KIX header */
#include "uiox_dev.h"          /* uiox_dev_handle_t, uiox_dev_err_t */
#include "../../sys/uix_types.h"

/* ── commands ────────────────────────────────────────────────────────
 * Mirrored from 30_KIX/30_DSS/20_DD/02_Sensors/als/uiox_als_if.h:72-78.
 * That header is a driver interface, so its types are not pulled in.
 * Values MUST stay identical; the driver's number space is not
 * validated above it. */
#define UIOX_DEV_ALS_IOC_GET_RAW    0x01u   /* out: uiox_dev_als_raw_t */
#define UIOX_DEV_ALS_IOC_GET_WHITE  0x02u   /* out: uint16_t           */
#define UIOX_DEV_ALS_IOC_GET_IR     0x03u   /* out: uint16_t           */
#define UIOX_DEV_ALS_IOC_SET_GAIN   0x04u   /* in:  unsigned int       */
#define UIOX_DEV_ALS_IOC_SET_ITIME  0x05u   /* in:  unsigned int       */
#define UIOX_DEV_ALS_IOC_SET_THRESH 0x06u   /* in:  {low,high}         */
#define UIOX_DEV_ALS_IOC_GET_STATS  0x07u   /* out: caller's stats buf */

/* Raw counts, in the order the kernel packs them. */
typedef struct { uix_uint16_t als, white, ir; } uiox_dev_als_raw_t;

/* Which part is behind /dev/als.  Determines the coefficient row used
 * by get_lux, and is passed to open rather than probed — the caller
 * knows what board it is on. */
typedef enum {
    UIOX_DEV_ALS_IC_VEML7700 = 0,
    UIOX_DEV_ALS_IC_OPT3001,
    UIOX_DEV_ALS_IC_BH1750,
    UIOX_DEV_ALS_IC_TSL2591
} uiox_dev_als_ic_t;

typedef struct {
    uiox_dev_handle_t h;
    uiox_dev_als_ic_t ic;
    unsigned int      gain_x100;     /* 100 = 1x,  800 = 8x  … */
    unsigned int      itime_ms;      /* 25 … 800               */
} uiox_dev_als_t;

/* ── lifecycle ───────────────────────────────────────────────────── */
uiox_dev_err_t uiox_dev_als_open (uiox_dev_als_t *a, int unit,
                                  uiox_dev_als_ic_t ic, int flags);
uiox_dev_err_t uiox_dev_als_close(uiox_dev_als_t *a);

/* ── data ────────────────────────────────────────────────────────── */
/* Raw counts straight from the driver. */
int uiox_dev_als_get_raw  (uiox_dev_als_t *a, uiox_dev_als_raw_t *r);
int uiox_dev_als_get_white(uiox_dev_als_t *a, uix_uint16_t *out);
int uiox_dev_als_get_ir   (uiox_dev_als_t *a, uix_uint16_t *out);

/* Converted here.  lux_milli is thousandths of a lux (336000 = 336.000);
 * cct_k is kelvin.  Both return 0 on success. */
int uiox_dev_als_get_lux(uiox_dev_als_t *a, unsigned int *lux_milli);
int uiox_dev_als_get_cct(uiox_dev_als_t *a, unsigned int *cct_k);

/* ── control ─────────────────────────────────────────────────────── */
int uiox_dev_als_set_gain  (uiox_dev_als_t *a, unsigned int gain_x100);
int uiox_dev_als_set_itime (uiox_dev_als_t *a, unsigned int itime_ms);
int uiox_dev_als_set_thresh(uiox_dev_als_t *a,
                            unsigned int low_milli, unsigned int high_milli);

/* ── stats ───────────────────────────────────────────────────────── */
int uiox_dev_als_get_stats(uiox_dev_als_t *a, void *out);

#endif /* UIOX_DEV_ALS_H */
