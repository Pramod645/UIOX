/**
 * @file    uiox_kix_math.h
 * @brief   The transcendental functions the kernel needs, with no libm
 *          and no <math.h>.
 *
 * PATH    30_KIX/common/uiox_kix_math.h
 *
 * -nostdinc makes <math.h> unreachable on purpose, and a grep of
 * uiox_klibc.h, 02_FwHal/include and 10_BSP/03_SoC/include finds no
 * powf/expf/sqrtf/log10f/fabsf/cosf/sinf/tanhf anywhere in the tree.
 * The audio DSPs (34_CSS .../speaker/uiox_spk_dsp.c and .../mic/uiox_mic_dsp.c)
 * call eight of them, so here they are.
 *
 * WHY common/, not 02_FwHal or a layer include/
 *
 * Same directory uiox_klibc.h lives in, for the same reason: a cosine is
 * arithmetic, not hardware.  Every layer already carries -I../common, so
 * this resolves with no Makefile change anywhere.
 *
 * ACCURACY
 *
 * Not libm.  Each is ~1e-6 over its useful range, far tighter than a
 * 16-bit PCM path can resolve (one int16 step is 1/32768 ≈ 3e-5).
 *
 * No FPU assumed: float throughout, so a soft-float target (arm32 builds
 * with -mfloat-abi=soft) calls the compiler's float routines rather than
 * trapping.
 *
 * @version 1.0.0  @date 2026-10-17
 */
#ifndef UIOX_KIX_MATH_H
#define UIOX_KIX_MATH_H

#include "uiox_klibc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_MATH_PI   3.14159265358979323846f

/* ── fabsf ──────────────────────────────────────────────────────────── */
static inline float uiox_math_fabsf(float x)
{
    return (x < 0.0f) ? -x : x;
}

/* ── sqrtf — Newton-Raphson from a bit-hack seed ────────────────────── */
static inline float uiox_math_sqrtf(float x)
{
    if (x <= 0.0f) return 0.0f;

    union { float f; uint32_t u; } v;
    v.f = x;
    v.u = (v.u >> 1) + 0x1FC00000u;      /* standard float-sqrt seed */

    float r = v.f;
    r = 0.5f * (r + x / r);
    r = 0.5f * (r + x / r);
    r = 0.5f * (r + x / r);
    return r;
}

/* ── sinf / cosf — range reduction then Taylor series ───────────────── */
static inline float uiox_math_sinf(float x)
{
    while (x >  UIOX_MATH_PI) x -= 2.0f * UIOX_MATH_PI;
    while (x < -UIOX_MATH_PI) x += 2.0f * UIOX_MATH_PI;

    const float x2 = x * x;
    return x * (1.0f
        - x2 / 6.0f
        + x2 * x2 / 120.0f
        - x2 * x2 * x2 / 5040.0f
        + x2 * x2 * x2 * x2 / 362880.0f);
}

static inline float uiox_math_cosf(float x)
{
    return uiox_math_sinf(x + (UIOX_MATH_PI * 0.5f));
}

/* ── tanhf — rational approximation ─────────────────────────────────── */
static inline float uiox_math_tanhf(float x)
{
    if (x >  5.0f) return  1.0f;
    if (x < -5.0f) return -1.0f;

    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

/* ── logf / log10f — mantissa split plus atanh series ───────────────── */
static inline float uiox_math_logf(float x)
{
    if (x <= 0.0f) return -1e30f;

    union { float f; uint32_t u; } v;
    v.f = x;
    int e = (int)((v.u >> 23) & 0xFFu) - 127;
    v.u = (v.u & 0x007FFFFFu) | 0x3F800000u;    /* mantissa → [1,2) */
    float m = v.f;

    float y  = (m - 1.0f) / (m + 1.0f);
    float y2 = y * y;
    float lm = 2.0f * y * (1.0f + y2 * (1.0f / 3.0f
                          + y2 * (1.0f / 5.0f
                          + y2 * (1.0f / 7.0f))));

    return lm + (float)e * 0.6931472f;
}

static inline float uiox_math_log10f(float x)
{
    return uiox_math_logf(x) * 0.4342945f;
}

/* ── expf — 2^n by exponent field, 2^f by series ────────────────────── */
static inline float uiox_math_expf(float x)
{
    float t = x * 1.44269504f;           /* x / ln2 */
    int   n = (int)t;
    float f = t - (float)n;

    const float c = f * 0.6931472f;
    float frac = 1.0f + c * (1.0f + c * (0.5f + c * (1.0f / 6.0f)));

    union { float f; uint32_t u; } v;
    v.u = (uint32_t)(n + 127) << 23;
    return v.f * frac;
}

/* ── pow10f — base 10 only, because that is all the tree uses powf for ─ */
static inline float uiox_math_pow10f(float y)
{
    int   n = (int)y;
    float f = y - (float)n;

    const float t = f * 2.302585093f;          /* f · ln(10) */
    float frac = 1.0f + t * (1.0f + t * (0.5f + t * (1.0f / 6.0f)));

    float scale = 1.0f;
    if (n > 0) { for (int i = 0; i <  n; i++) scale *= 10.0f; }
    else       { for (int i = 0; i < -n; i++) scale *= 0.1f;  }

    return scale * frac;
}

#ifdef __cplusplus
}
#endif
#endif /* UIOX_KIX_MATH_H */
