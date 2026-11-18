/*
 * 50_UIX/00_libs/00_uixlibs/dev/rtc/uiox_dev_rtc.c
 */
#include "uiox_dev_rtc.h"

uiox_dev_err_t uiox_dev_rtc_open(uiox_dev_rtc_t *r, int unit, int flags)
{
    if (!r) return UIOX_DEV_EBADNAME;
    (void)unit;                       /* reg_open is unit-0 only */
    return uiox_dev_reg_open(&r->h, UIOX_DEV_RTC_DEVPATH, flags);
}

uiox_dev_err_t uiox_dev_rtc_close(uiox_dev_rtc_t *r)
{
    if (!r) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_close(&r->h);
}

uiox_dev_err_t uiox_dev_rtc_get_time(uiox_dev_rtc_t *r,
                                     uiox_dev_rtc_time_t *out)
{
    if (!r || !out) return UIOX_DEV_EBADNAME;
    /* The kernel struct is byte-for-byte this layout, so the buffer is
     * passed straight through — no staging copy needed, unlike BMS and
     * PMIC whose structs this file deliberately cannot name. */
    return uiox_dev_reg_get(&r->h, UIOX_DEV_RTC_IOC_GET_TIME, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_rtc_set_time(uiox_dev_rtc_t *r,
                                     const uiox_dev_rtc_time_t *in)
{
    if (!r || !in) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_set(&r->h, UIOX_DEV_RTC_IOC_SET_TIME, (void *)in)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_rtc_get_alarm(uiox_dev_rtc_t *r,
                                      uiox_dev_rtc_alarm_t *out)
{
    if (!r || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_get(&r->h, UIOX_DEV_RTC_IOC_GET_ALARM, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_rtc_set_alarm(uiox_dev_rtc_t *r,
                                      const uiox_dev_rtc_alarm_t *in)
{
    if (!r || !in) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_set(&r->h, UIOX_DEV_RTC_IOC_SET_ALARM, (void *)in)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_rtc_get_stats(uiox_dev_rtc_t *r, void *out)
{
    if (!r || !out) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_get(&r->h, UIOX_DEV_RTC_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}
