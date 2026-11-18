/*
 * 50_UIX/00_libs/00_uixlibs/dev/pmic/uiox_dev_pmic.c
 *
 * PMIC on the REG shape, read-only.  Reaches the kernel only through
 * uiox_dev_reg_get — uiox_dev_reg_set is unused here, because the one
 * write command (FIELD_WR) is deliberately not exposed.
 */
#include "uiox_dev_pmic.h"

/* The kernel's uiox_pmic_telem_t wrote through this layout.  The two
 * trailing members are int8_t — ONE BYTE SIGNED — so they must be
 * signed char here, not unsigned: a -20 C reading cast through an
 * unsigned byte would come back as 236. */
struct pmic_telem_kernel {
    unsigned int  ts_ms;
    unsigned int  vsys_mv;
    unsigned int  vbat_mv;
    unsigned int  ibat_ma;
    unsigned int  vbus_mv;
    signed char   die_temp_c;
    signed char   ntc_temp_c;
};

uiox_dev_err_t uiox_dev_pmic_open(uiox_dev_pmic_t *p, int unit, int flags)
{
    if (!p) return UIOX_DEV_EBADNAME;
    (void)unit;                       /* reg_open is unit-0 only */
    return uiox_dev_reg_open(&p->h, UIOX_DEV_PMIC_DEVPATH, flags);
}

uiox_dev_err_t uiox_dev_pmic_close(uiox_dev_pmic_t *p)
{
    if (!p) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_close(&p->h);
}

uiox_dev_err_t uiox_dev_pmic_get_telemetry(uiox_dev_pmic_t *p,
                                           uiox_dev_pmic_telem_t *out)
{
    struct pmic_telem_kernel k;
    unsigned int i;

    if (!p || !out) return UIOX_DEV_EBADNAME;

    for (i = 0u; i < sizeof k; i++) ((unsigned char *)&k)[i] = 0u;
    if (uiox_dev_reg_get(&p->h, UIOX_DEV_PMIC_IOC_GET_TELEM, &k) != 0)
        return UIOX_DEV_EIO;

    out->ts_ms      = k.ts_ms;
    out->vsys_mv    = k.vsys_mv;
    out->vbat_mv    = k.vbat_mv;
    out->ibat_ma    = k.ibat_ma;
    out->vbus_mv    = k.vbus_mv;
    /* Widened through int, so the sign survives.  A plain assignment
     * from signed char to int does this correctly; the cast is here to
     * make the intent visible rather than incidental. */
    out->die_temp_c = (int)k.die_temp_c;
    out->ntc_temp_c = (int)k.ntc_temp_c;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_pmic_get_vsys(uiox_dev_pmic_t *p, unsigned int *mv)
{
    unsigned int v = 0u;
    if (!p || !mv) return UIOX_DEV_EBADNAME;
    if (uiox_dev_reg_get(&p->h, UIOX_DEV_PMIC_IOC_GET_VSYS, &v) != 0)
        return UIOX_DEV_EIO;
    *mv = v;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_pmic_get_stats(uiox_dev_pmic_t *p, void *out)
{
    if (!p || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_get(&p->h, UIOX_DEV_PMIC_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
