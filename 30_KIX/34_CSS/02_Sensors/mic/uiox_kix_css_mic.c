#include "uiox_kix_css_mic.h"

#define MIC_EFAULT  ((int64_t)-14)
#define MIC_EINVAL  ((int64_t)-22)
#define MIC_ENODEV  ((int64_t)-19)

/* gate — bounds are 50_scpcs's, character for character */
#define MIC_USER_BASE   ((uiox_uint64_t)0x00001000ull)
#define MIC_USER_TOP    ((uiox_uint64_t)0xC0000000ull)

/* one read's ceiling, checked before the size arithmetic */
#define MIC_MAX_SAMPLES ((uiox_uint64_t)65536u)

static int mic_check_user_ptr(uiox_uintptr_t p, uiox_uint64_t len,
                              uiox_uint64_t align)
{
    uiox_uint64_t a   = (uiox_uint64_t)p;
    uiox_uint64_t end;

    if (len == 0u) return 0;
    if (a  < MIC_USER_BASE) return MIC_EFAULT;

    end = a + len;
    if (end < a) return MIC_EFAULT;
    if (end > MIC_USER_TOP) return MIC_EFAULT;

    if (align && (a & (align - 1ull)) != 0u) return MIC_EFAULT;

    return 0;
}

int64_t uiox_kix_css_mic_read(uiox_uintptr_t buf, uiox_uint64_t n_samples)
{
    uiox_mic_subsys_t *sys = uiox_mic_subsys_current();
    uiox_uint64_t      bytes;

    if (!sys)                                     return MIC_ENODEV;
    if (sys->state == UIOX_MIC_STATE_STOPPED)     return MIC_ENODEV;
    if (!buf || !n_samples)                       return MIC_EINVAL;

    /* bound FIRST: a huge n_samples would make the size test itself the
     * overflow, and the check would pass on the arithmetic rather than
     * on the buffer */
    if (n_samples > MIC_MAX_SAMPLES)              return MIC_EINVAL;
    bytes = n_samples * 2u;                       /* int16_t samples   */

    if (mic_check_user_ptr(buf, bytes, 2u))       return MIC_EFAULT;

    /* subsys_read drains mif and runs the DSP over the result, so the
     * PCM the caller receives is post-AGC, post-VAD.  That is the
     * layer's design; this row does not reinterpret it. */
    return (int64_t)uiox_mic_subsys_read(sys, (int16_t *)(uintptr_t)buf,
                                         (uiox_uint32_t)n_samples);
}

int64_t uiox_kix_css_mic_gain(uiox_uint64_t gain_db)
{
    uiox_mic_subsys_t *sys = uiox_mic_subsys_current();

    if (!sys)                                     return MIC_ENODEV;
    if (sys->state == UIOX_MIC_STATE_STOPPED)     return MIC_ENODEV;
    if (gain_db > 255u)                           return MIC_EINVAL;

    return (int64_t)uiox_mic_subsys_set_gain(sys, (uiox_uint8_t)gain_db);
}

int64_t uiox_kix_css_mic_mute(uiox_uint64_t on)
{
    uiox_mic_subsys_t *sys = uiox_mic_subsys_current();

    if (!sys)                                     return MIC_ENODEV;
    if (sys->state == UIOX_MIC_STATE_STOPPED)     return MIC_ENODEV;

    return (int64_t)uiox_mic_subsys_set_mute(sys, on ? true : false);
}

int64_t uiox_kix_css_mic_state(void)
{
    uiox_mic_subsys_t *sys = uiox_mic_subsys_current();

    if (!sys) return MIC_ENODEV;

    return (int64_t)sys->state;   /* UIOX_MIC_STATE_* */
}
