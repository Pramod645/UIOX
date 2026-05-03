#include "uix_math.h"
#include "uix_errno.h"

/* ── Taylor-series helpers ──────────────────────────────────── */
static double uix_fabs_impl(double x) { return x < 0.0 ? -x : x; }

/* ── Trigonometric ──────────────────────────────────────────── */
double uix_sin(double x)
{
    /* Range-reduce to [-pi, pi] */
    while (x >  UIX_M_PI) x -= 2.0 * UIX_M_PI;
    while (x < -UIX_M_PI) x += 2.0 * UIX_M_PI;
    double r = x, t = x;
    for (int n = 1; n <= 10; n++) {
        t *= -x * x / (double)((2*n)*(2*n+1));
        r += t;
    }
    return r;
}

double uix_cos(double x)
{
    return uix_sin(x + UIX_M_PI / 2.0);
}

double uix_tan(double x)
{
    double c = uix_cos(x);
    if (uix_fabs_impl(c) < 1e-15) return UIX_HUGE_VAL;
    return uix_sin(x) / c;
}

double uix_asin(double x)
{
    if (x < -1.0 || x > 1.0) { uix_errno = UIX_EDOM; return UIX_NAN; }
    /* asin(x) = atan2(x, sqrt(1-x*x)) */
    return uix_atan2(x, uix_sqrt(1.0 - x*x));
}

double uix_acos(double x)
{
    if (x < -1.0 || x > 1.0) { uix_errno = UIX_EDOM; return UIX_NAN; }
    return UIX_M_PI / 2.0 - uix_asin(x);
}

double uix_atan(double x)
{
    /* atan(x) = x - x^3/3 + x^5/5 - ... (|x|<=1) */
    if (uix_fabs_impl(x) > 1.0)
        return (x > 0 ? 1 : -1) * (UIX_M_PI/2.0 - uix_atan(1.0/uix_fabs_impl(x)));
    double r = 0.0, t = x;
    for (int n = 0; n < 20; n++) {
        r += t / (double)(2*n+1) * (n%2 == 0 ? 1.0 : -1.0);
        t *= x * x;
    }
    return r;
}

double uix_atan2(double y, double x)
{
    if (x > 0.0)  return uix_atan(y / x);
    if (x < 0.0)  return uix_atan(y / x) + (y >= 0.0 ? UIX_M_PI : -UIX_M_PI);
    if (y > 0.0)  return  UIX_M_PI / 2.0;
    if (y < 0.0)  return -UIX_M_PI / 2.0;
    return 0.0;
}

/* ── Hyperbolic ─────────────────────────────────────────────── */
double uix_sinh(double x)
{
    double e = uix_exp(x);
    return (e - 1.0/e) / 2.0;
}

double uix_cosh(double x)
{
    double e = uix_exp(x);
    return (e + 1.0/e) / 2.0;
}

double uix_tanh(double x)
{
    double e2 = uix_exp(2.0*x);
    return (e2 - 1.0) / (e2 + 1.0);
}

/* ── Exponential and logarithm ──────────────────────────────── */
double uix_exp(double x)
{
    /* e^x via Taylor series */
    double r = 1.0, t = 1.0;
    for (int n = 1; n <= 30; n++) {
        t *= x / (double)n;
        r += t;
        if (uix_fabs_impl(t) < 1e-15) break;
    }
    return r;
}

double uix_log(double x)
{
    if (x <= 0.0) { uix_errno = UIX_EDOM; return UIX_NAN; }
    /* log(x) = 2 * atanh((x-1)/(x+1)) */
    double y = (x - 1.0) / (x + 1.0);
    double r = 0.0, t = y;
    for (int n = 0; n < 30; n++) {
        r += t / (double)(2*n+1);
        t *= y * y;
    }
    return 2.0 * r;
}

double uix_log2(double x)  { return uix_log(x) / UIX_M_LN2;  }
double uix_log10(double x) { return uix_log(x) / UIX_M_LN10; }

double uix_pow(double base, double exp)
{
    if (exp == 0.0) return 1.0;
    if (base == 0.0) return 0.0;
    if (base < 0.0 && exp != (int)exp) {
        uix_errno = UIX_EDOM; return UIX_NAN;
    }
    return uix_exp(exp * uix_log(uix_fabs_impl(base))) *
           (base < 0.0 && (int)exp % 2 != 0 ? -1.0 : 1.0);
}

double uix_sqrt(double x)
{
    if (x < 0.0) { uix_errno = UIX_EDOM; return UIX_NAN; }
    if (x == 0.0) return 0.0;
    double r = x, prev;
    do { prev = r; r = (r + x/r) / 2.0; }
    while (uix_fabs_impl(r - prev) > 1e-12);
    return r;
}

double uix_cbrt(double x)
{
    if (x == 0.0) return 0.0;
    double r = x, prev;
    do { prev = r; r = (2.0*r + x/(r*r)) / 3.0; }
    while (uix_fabs_impl(r - prev) > 1e-12);
    return r;
}

/* ── Rounding ────────────────────────────────────────────────── */
double uix_ceil (double x) { double t=(double)(long long)x; return t<x?t+1.0:t; }
double uix_floor(double x) { double t=(double)(long long)x; return t>x?t-1.0:t; }
double uix_trunc(double x) { return (double)(long long)x; }
double uix_round(double x)
{
    return x >= 0.0 ? uix_floor(x + 0.5) : uix_ceil(x - 0.5);
}
double uix_fabs(double x)  { return uix_fabs_impl(x); }
double uix_fmod(double x, double y)
{
    if (y == 0.0) { uix_errno = UIX_EDOM; return UIX_NAN; }
    return x - uix_trunc(x/y) * y;
}

/* ── Float versions ─────────────────────────────────────────── */
float uix_sinf (float x) { return (float)uix_sin(x); }
float uix_cosf (float x) { return (float)uix_cos(x); }
float uix_tanf (float x) { return (float)uix_tan(x); }
float uix_sqrtf(float x) { return (float)uix_sqrt(x); }
float uix_fabsf(float x) { return x < 0.0f ? -x : x; }
float uix_powf (float b, float e) { return (float)uix_pow(b,e); }
float uix_ceilf (float x) { return (float)uix_ceil(x); }
float uix_floorf(float x) { return (float)uix_floor(x); }
float uix_roundf(float x) { return (float)uix_round(x); }

/* ── Classification ─────────────────────────────────────────── */
int uix_isnan    (double x) { return x != x; }
int uix_isinf    (double x) { return x == UIX_INFINITY || x == -UIX_INFINITY; }
int uix_isfinite (double x) { return !uix_isnan(x) && !uix_isinf(x); }
