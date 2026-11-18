#ifndef UIOX_DEV_BMS_H
#define UIOX_DEV_BMS_H

#include "../uiox_dev_reg.h"

/* Mirrored from 30_KIX/30_DSS/20_DD/03_NonSensors/bms/uiox_bms_if.h:148-151.
 * Four commands, all reads.  Values MUST stay identical. */
#define UIOX_DEV_BMS_IOC_GET_TELEM   0x01u
#define UIOX_DEV_BMS_IOC_GET_PACK_MV 0x02u
#define UIOX_DEV_BMS_IOC_GET_CELLS   0x03u
#define UIOX_DEV_BMS_IOC_GET_STATS   0x04u

#define UIOX_DEV_BMS_DEVPATH  "bms"

/* Mirrored from 30_KIX/30_DSS/10_DI/03_NonSensors/bms/uiox_bms_hw.h:86-87.
 * GET_TELEM fills both arrays whole, so neither may be smaller. */
#define UIOX_DEV_BMS_MAX_CELLS  16u
#define UIOX_DEV_BMS_MAX_TEMPS  4u

/* The kernel's uiox_bms_telem_t, reproduced for userspace.  All nine
 * fields, in the driver's order — GET_TELEM writes the whole struct, so
 * a shorter copy is a stack overrun, not merely a wrong reading. */
typedef struct {
    unsigned int   ts_ms;
    unsigned int   pack_mv;
    int            current_ma;
    unsigned int   soc_pct;                 /* kernel: uint8_t */
    unsigned int   soh_pct;                 /* kernel: uint8_t */
    int            remain_mah;
    int            full_mah;
    unsigned int   fault_flags;
    unsigned int   cell_mv[UIOX_DEV_BMS_MAX_CELLS];
    int            temp_dc[UIOX_DEV_BMS_MAX_TEMPS];
} uiox_dev_bms_telem_t;

typedef struct {
    uiox_dev_handle_t h;
    unsigned int      num_cells;
} uiox_dev_bms_t;

uiox_dev_err_t uiox_dev_bms_open (uiox_dev_bms_t *b, int unit, int flags);
uiox_dev_err_t uiox_dev_bms_close(uiox_dev_bms_t *b);

uiox_dev_err_t uiox_dev_bms_snapshot(uiox_dev_bms_t *b,
                                     uiox_dev_bms_telem_t *out);
uiox_dev_err_t uiox_dev_bms_pack_mv (uiox_dev_bms_t *b, unsigned int *mv);
uiox_dev_err_t uiox_dev_bms_get_stats(uiox_dev_bms_t *b, void *out);

#endif
