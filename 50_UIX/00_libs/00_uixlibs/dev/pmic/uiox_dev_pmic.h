/*
 * 50_UIX/00_libs/00_uixlibs/dev/pmic/uiox_dev_pmic.h
 *
 * PMIC — the application's view, at EL0.  Read-only by choice.
 *
 * ── FAMILY 4 (REG) ──────────────────────────────────────────────────
 * Four commands exist; this layer exposes three:
 *
 *   GET_TELEM  0x01  uiox_pmic_telem_t     ← exposed
 *   GET_VSYS   0x02  uint32_t mV           ← exposed
 *   FIELD_WR   0x03  uiox_pmic_field_t     ← NOT exposed, see below
 *   GET_STATS  0x04  uiox_pmic_if_stats_t  ← exposed
 *
 * ── WHY FIELD_WR IS DELIBERATELY ABSENT ─────────────────────────────
 * uiox_pmic_if.h:129-134 explains that FIELD_WR exists because this
 * PMIC's register space is uint16_t while §8's bus path is 8-bit —
 * "the family vocabulary is not widened for one class."  It carries
 * {reg, mask, shift, val}: kernel register-layout facts.
 *
 * Exposing it would be the first time this layer handed a userspace
 * caller a kernel register address.  Every other class kept those on
 * the kernel side, and a register map mirrored into dev/ ages with the
 * kernel silently.  So rail and OV/UV configuration stay kernel-side.
 *
 * ── WHAT THAT COSTS ─────────────────────────────────────────────────
 * uiox_pmic_rail_on()/rail_off()/rail_voltage() take a rail NAME and
 * have no ioctl; uiox_pmic_adc_read() likewise.  An application can
 * observe the PMIC through telemetry and stats, and cannot command it.
 * If rail control from EL0 is ever wanted, it needs a named command
 * kernel-side — not FIELD_WR plus a mirrored register map.
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_PMIC_H
#define UIOX_DEV_PMIC_H

#include "../uiox_dev_reg.h"

/* Mirrored from 30_KIX/30_DSS/20_DD/03_NonSensors/pmic/uiox_pmic_if.h:135-138.
 * Values MUST stay identical; the driver's number space is not validated. */
#define UIOX_DEV_PMIC_IOC_GET_TELEM  0x01u
#define UIOX_DEV_PMIC_IOC_GET_VSYS   0x02u
#define UIOX_DEV_PMIC_IOC_GET_STATS  0x04u

#define UIOX_DEV_PMIC_DEVPATH  "pmic"

/* The kernel's uiox_pmic_telem_t (uiox_pmic_if.h:83-101), mirrored.
 * Note the two int8_t at the end — SIGNED narrow types, so the copy
 * in the .c widens them deliberately rather than casting through a
 * larger unsigned field. */
typedef struct {
    unsigned int  ts_ms;
    unsigned int  vsys_mv;
    unsigned int  vbat_mv;
    unsigned int  ibat_ma;
    unsigned int  vbus_mv;
    int           die_temp_c;    /* kernel: int8_t */
    int           ntc_temp_c;    /* kernel: int8_t */
} uiox_dev_pmic_telem_t;

typedef struct {
    uiox_dev_handle_t h;
} uiox_dev_pmic_t;

uiox_dev_err_t uiox_dev_pmic_open (uiox_dev_pmic_t *p, int unit, int flags);
uiox_dev_err_t uiox_dev_pmic_close(uiox_dev_pmic_t *p);

/* One GET_TELEM: rails, currents and both temperatures together. */
uiox_dev_err_t uiox_dev_pmic_get_telemetry(uiox_dev_pmic_t *p,
                                           uiox_dev_pmic_telem_t *out);

/* GET_VSYS — one field, its own command, one trap. */
uiox_dev_err_t uiox_dev_pmic_get_vsys(uiox_dev_pmic_t *p, unsigned int *mv);

/* Opaque: the caller includes the kernel header for
 * uiox_pmic_if_stats_t and passes its address. */
uiox_dev_err_t uiox_dev_pmic_get_stats(uiox_dev_pmic_t *p, void *out);

#endif
