#include "uiox_dev_bms.h"

/* ── the staging struct, byte for byte ────────────────────────────────
 * Two things it must get right, and the first version got both wrong:
 *
 *   · soc_pct and soh_pct are ONE BYTE each in the kernel, not a word.
 *     Writing `unsigned int` there pushes every later field four bytes
 *     further along, so remain_mah onward read from the wrong place.
 *   · it needs all nine fields.  GET_TELEM fills the cell and temp
 *     arrays too, so a struct that stops at remain_mah is written past
 *     its end.
 *
 * The alternative to reproducing the layout would be including the
 * kernel header, which puts a 30_KIX path on the app's include line —
 * the boundary this layer exists to hold.  A mismatch here is silent:
 * the build succeeds and the numbers are wrong. */
struct bms_telem_kernel {
    unsigned int   ts_ms;
    unsigned int   pack_mv;
    int            current_ma;
    unsigned char  soc_pct;
    unsigned char  soh_pct;
    int            remain_mah;
    int            full_mah;
    unsigned int   fault_flags;
    unsigned int   cell_mv[UIOX_DEV_BMS_MAX_CELLS];
    short          temp_dc[UIOX_DEV_BMS_MAX_TEMPS];
};

uiox_dev_err_t uiox_dev_bms_open(uiox_dev_bms_t *b, int unit, int flags)
{
    uiox_dev_err_t rc;
    if (!b) return UIOX_DEV_EBADNAME;
    (void)unit;
    rc = uiox_dev_reg_open(&b->h, UIOX_DEV_BMS_DEVPATH, flags);
    if (rc != UIOX_DEV_OK) return rc;
    b->num_cells = 0u;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_bms_close(uiox_dev_bms_t *b)
{
    if (!b) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_close(&b->h);
}

uiox_dev_err_t uiox_dev_bms_snapshot(uiox_dev_bms_t *b,
                                     uiox_dev_bms_telem_t *out)
{
    struct bms_telem_kernel k;
    unsigned int i, n = 0u;

    if (!b || !out) return UIOX_DEV_EBADNAME;

    for (i = 0u; i < sizeof k; i++) ((unsigned char *)&k)[i] = 0u;
    if (uiox_dev_reg_get(&b->h, UIOX_DEV_BMS_IOC_GET_TELEM, &k) != 0)
        return UIOX_DEV_EIO;

    out->ts_ms       = k.ts_ms;
    out->pack_mv     = k.pack_mv;
    out->current_ma  = k.current_ma;
    out->soc_pct     = (unsigned int)k.soc_pct;
    out->soh_pct     = (unsigned int)k.soh_pct;
    out->remain_mah  = k.remain_mah;
    out->full_mah    = k.full_mah;
    out->fault_flags = k.fault_flags;

    /* The driver fills the whole array; a cell reading 0 mV is not a
     * cell, so the count stops at the last non-zero entry. */
    for (i = 0u; i < UIOX_DEV_BMS_MAX_CELLS; i++) {
        out->cell_mv[i] = k.cell_mv[i];
        if (k.cell_mv[i] != 0u) n = i + 1u;
    }
    for (i = 0u; i < UIOX_DEV_BMS_MAX_TEMPS; i++)
        out->temp_dc[i] = (int)k.temp_dc[i];

    b->num_cells = n;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_bms_pack_mv(uiox_dev_bms_t *b, unsigned int *mv)
{
    /* GET_PACK_MV is its own command — one trap, no staging. */
    unsigned int v = 0u;
    if (!b || !mv) return UIOX_DEV_EBADNAME;
    if (uiox_dev_reg_get(&b->h, UIOX_DEV_BMS_IOC_GET_PACK_MV, &v) != 0)
        return UIOX_DEV_EIO;
    *mv = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_bms_get_stats(uiox_dev_bms_t *b, void *out)
{
    if (!b || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_get(&b->h, UIOX_DEV_BMS_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
