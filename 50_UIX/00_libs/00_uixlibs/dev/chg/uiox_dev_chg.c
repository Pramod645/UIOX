/*
 * 50_UIX/00_libs/00_uixlibs/dev/chg/uiox_dev_chg.c
 */
#include "uiox_dev_chg.h"

uiox_dev_err_t uiox_dev_chg_open(uiox_dev_chg_t *c, int unit, int flags)
{
    if (!c) return UIOX_DEV_EBADNAME;
    (void)unit;                       /* reg_open is unit-0 only */
    return uiox_dev_reg_open(&c->h, UIOX_DEV_CHG_DEVPATH, flags);
}

uiox_dev_err_t uiox_dev_chg_close(uiox_dev_chg_t *c)
{
    if (!c) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_close(&c->h);
}

uiox_dev_err_t uiox_dev_chg_get_status(uiox_dev_chg_t *c,
                                       uiox_dev_chg_status_t *out)
{
    if (!c || !out) return UIOX_DEV_EBADNAME;

    /* Zero first: the driver writes all three fields, but a partial
     * write would otherwise leave the caller reading stale stack. */
    out->chrg   = 0;
    out->src    = 0;
    out->faults = 0u;

    return uiox_dev_reg_get(&c->h, UIOX_DEV_CHG_IOC_GET_STATUS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_chg_set_ichg(uiox_dev_chg_t *c, unsigned int ma)
{
    unsigned int v = ma;
    if (!c) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_set(&c->h, UIOX_DEV_CHG_IOC_SET_ICHG, &v)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_chg_set_vchg(uiox_dev_chg_t *c, unsigned int mv)
{
    unsigned int v = mv;
    if (!c) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_set(&c->h, UIOX_DEV_CHG_IOC_SET_VCHG, &v)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_chg_set_iin(uiox_dev_chg_t *c, unsigned int ma)
{
    unsigned int v = ma;
    if (!c) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_set(&c->h, UIOX_DEV_CHG_IOC_SET_IIN, &v)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_chg_set_enable(uiox_dev_chg_t *c, int on)
{
    /* The driver's argument is a bool; an int-sized 0/1 travels in the
     * same word the trap carries, and the dispatch reads it as bool. */
    int v = on ? 1 : 0;
    if (!c) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_set(&c->h, UIOX_DEV_CHG_IOC_SET_ENABLE, &v)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_chg_get_stats(uiox_dev_chg_t *c, void *out)
{
    if (!c || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_get(&c->h, UIOX_DEV_CHG_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
