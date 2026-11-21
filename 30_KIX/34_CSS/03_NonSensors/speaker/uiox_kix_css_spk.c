#include "uiox_kix_css_spk.h"

#define SPK_EFAULT  ((int64_t)-14)
#define SPK_EINVAL  ((int64_t)-22)
#define SPK_ENODEV  ((int64_t)-19)

#define SPK_USER_BASE   ((uiox_uint64_t)0x00001000ull)
#define SPK_USER_TOP    ((uiox_uint64_t)0xC0000000ull)

/* one write's ceiling, checked before the size arithmetic */
#define SPK_MAX_PAIRS   ((uiox_uint64_t)32768u)

static int spk_check_user_ptr(uiox_uintptr_t p, uiox_uint64_t len,
                              uiox_uint64_t align)
{
    uiox_uint64_t a   = (uiox_uint64_t)p;
    uiox_uint64_t end;

    if (len == 0u) return 0;
    if (a  < SPK_USER_BASE) return SPK_EFAULT;

    end = a + len;
    if (end < a) return SPK_EFAULT;          /* the sum wrapped */
    if (end > SPK_USER_TOP) return SPK_EFAULT;

    if (align && (a & (align - 1ull)) != 0u) return SPK_EFAULT;

    return 0;
}

int64_t uiox_kix_css_spk_write(uiox_uintptr_t samples, uiox_uint64_t n_stereo)
{
    uiox_spk_subsys_t *sys = uiox_spk_subsys_current();
    uiox_uint64_t      bytes;

    if (!sys)                                      return SPK_ENODEV;
    if (sys->state == UIOX_SPK_STATE_STOPPED)      return SPK_ENODEV;
    if (!samples || !n_stereo)                     return SPK_EINVAL;

    /* bound FIRST: a huge n_stereo would make the size test itself the
     * overflow, and the check would pass on the arithmetic rather than
     * on the buffer */
    if (n_stereo > SPK_MAX_PAIRS)                  return SPK_EINVAL;
    bytes = n_stereo * 4u;                     /* 2 x int16_t per pair */

    if (spk_check_user_ptr(samples, bytes, 2u))    return SPK_EFAULT;

    return (int64_t)uiox_spk_subsys_write(sys,
                                          (const int16_t *)(uintptr_t)samples,
                                          (uiox_uint32_t)n_stereo);
}

int64_t uiox_kix_css_spk_volume(uiox_uint64_t vol_pct)
{
    uiox_spk_subsys_t *sys = uiox_spk_subsys_current();

    if (!sys)                                      return SPK_ENODEV;
    if (sys->state == UIOX_SPK_STATE_STOPPED)      return SPK_ENODEV;
    if (vol_pct > 100u)                            return SPK_EINVAL;

    return (int64_t)uiox_spk_subsys_set_vol(sys, (uiox_uint8_t)vol_pct);
}

int64_t uiox_kix_css_spk_mute(uiox_uint64_t on)
{
    uiox_spk_subsys_t *sys = uiox_spk_subsys_current();

    if (!sys)                                      return SPK_ENODEV;
    if (sys->state == UIOX_SPK_STATE_STOPPED)      return SPK_ENODEV;

    return (int64_t)uiox_spk_subsys_mute(sys, on ? true : false);
}

int64_t uiox_kix_css_spk_state(void)
{
    uiox_spk_subsys_t *sys = uiox_spk_subsys_current();

    if (!sys) return SPK_ENODEV;

    return (int64_t)sys->state;   /* UIOX_SPK_STATE_* */
}
