/*
 * 50_UIX/00_libs/00_uixlibs/dev/thermal/uiox_dev_therm.h
 *
 * THERM — the application's view of the thermal stack, at EL0.
 *
 * ── FAMILY 4 (REG) ──────────────────────────────────────────────────
 * A REG class is reached only by ioctl: 10_scfs's read branch returns
 * SCFS_ENOTTY for a non-EVENT family.  Four commands, and GET_TELEM is
 * the workhorse — it returns the whole snapshot in one call.
 *
 * ── WHY NOT GET_TEMP ────────────────────────────────────────────────
 * uiox_therm_if.c:221-227 implements GET_TEMP as *temp_dc[0] — it takes
 * NO channel argument and silently answers for channel 0.  A caller
 * asking for channel 3 would get channel 0 with no error, so this layer
 * does not use it.  Everything goes through GET_TELEM instead, which
 * carries num_channels, temp_dc[] and alert[] together.
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_THERM_H
#define UIOX_DEV_THERM_H

#include "../uiox_dev_reg.h"
#include "../uiox_dev_reg.h"      /* the family shape — not a 30_KIX header */
#include "uiox_dev.h"          /* uiox_dev_handle_t, uiox_dev_err_t */
#include "../../sys/uix_types.h"


/* Mirrored from 30_KIX/30_DSS/20_DD/02_Sensors/thermal/uiox_therm_if.h:111-114.
 * Values MUST stay identical; the driver's number space is not validated. */
#define UIOX_DEV_THERM_IOC_GET_TEMP   0x01u
#define UIOX_DEV_THERM_IOC_GET_TELEM  0x02u
#define UIOX_DEV_THERM_IOC_SET_ALERT  0x03u
#define UIOX_DEV_THERM_IOC_GET_STATS  0x04u

/* The node stem — devparse.c has no "therm" entry, so /dev/therm would
 * be refused with SCFS_ENODEV.  The class is "thermal". */
#define UIOX_DEV_THERM_DEVPATH  "thermal"

/* Bounded copy of the driver's snapshot.  The kernel's own cap is
 * UIOX_THERM_MAX_CHANNELS; this must not exceed it. */
#ifndef UIOX_DEV_THERM_MAX_CH
#define UIOX_DEV_THERM_MAX_CH  8u
#endif

typedef struct {
    unsigned int  ts_ms;
    unsigned int  num_channels;
    short         temp_dc [UIOX_DEV_THERM_MAX_CH];   /* deci-°C */
    int           alert   [UIOX_DEV_THERM_MAX_CH];   /* 0 / 1   */
} uiox_dev_therm_telem_t;

typedef struct {
    uiox_dev_handle_t h;
    unsigned int      num_channels;   /* from the first snapshot */
} uiox_dev_therm_t;

/* Alert window, in deci-°C.  The kernel's SET_ALERT takes exactly this
 * pair (uiox_therm_alert_t at uiox_therm_if.h:118). */
typedef struct { short t_high_dc, t_hyst_dc; } uiox_dev_therm_alert_t;

uiox_dev_err_t uiox_dev_therm_open (uiox_dev_therm_t *t, int unit, int flags);
uiox_dev_err_t uiox_dev_therm_close(uiox_dev_therm_t *t);

/* One GET_TELEM call, copied into the caller's struct.  This is the
 * layer's primary operation: temps, alerts and the channel count all
 * arrive together, so a caller that wants three facts makes one call. */
uiox_dev_err_t uiox_dev_therm_snapshot(uiox_dev_therm_t *t,
                                       uiox_dev_therm_telem_t *out);

/* Convenience over the snapshot.  Each of these is one GET_TELEM, so a
 * loop calling read_ch per channel is N calls — snapshot() instead when
 * more than one channel is wanted. */
uiox_dev_err_t uiox_dev_therm_read_ch(uiox_dev_therm_t *t,
                                      unsigned int ch, short *temp_dc);
uiox_dev_err_t uiox_dev_therm_alert_active(uiox_dev_therm_t *t,
                                           unsigned int ch, int *active);

uiox_dev_err_t uiox_dev_therm_set_alert(uiox_dev_therm_t *t,
                                        short t_high_dc, short t_hyst_dc);

/* Opaque: the caller includes the kernel header for
 * uiox_therm_if_stats_t and passes its address. */
uiox_dev_err_t uiox_dev_therm_get_stats(uiox_dev_therm_t *t, void *out);

#endif /* UIOX_DEV_THERM_H */
