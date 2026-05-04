#include "uix_math.h"
#include "uix_errno.h"

static double _abs(double x) { return x < 0.0 ? -x : x; }

double uix_fabs(double x)  { return _abs(x); }
double uix_trunc(double x) { return (double)(long long)x; }
double uix_ceil (double x) { double t = uix_trunc(x); return t < x ? t+1.0 : t; }
double uix_floor(double x) { double t = uix_trunc(x); return t > x ? t-1.0 : t; }
double uix_round(double x) { return x>=0.0 ? uix_floor(x+0.5) : uix_ceil(x-0.5); }
double uix_fmod (double x, double y)
{
    if (y==0.0) { uix_errno=UIX_EDOM; return UIX_NAN; }
    return x - uix_trunc(x/y)*y;
}

double uix_exp(double x)
{
    double r=1.0, t=1.0;
    for (int n=1; n<=40; n++) {
        t *= x/(double)n; r += t;
        if (_abs(t) < 1e-17) break;
    }
    return r;
}

double uix_log(double x)
{
    if (x<=0.0) { uix_errno=UIX_EDOM; return UIX_NAN; }
    double y=(x-1.0)/(x+1.0), r=0.0, t=y;
    for (int n=0; n<50; n++) { r += t/(double)(2*n+1); t*=y*y; }
    return 2.0*r;
}

double uix_log2 (double x) { return uix_log(x)/UIX_M_LN2;  }
double uix_log10(double x) { return uix_log(x)/UIX_M_LN10; }

double uix_sqrt(double x)
{
    if (x<0.0) { uix_errno=UIX_EDOM; return UIX_NAN; }
    if (x==0.0) return 0.0;
    double r=x, p;
    do { p=r; r=(r+x/r)/2.0; } while (_abs(r-p)>1e-14);
    return r;
}

double uix_cbrt(double x)
{
    if (x==0.0) return 0.0;
    double r=x, p;
    do { p=r; r=(2.0*r+x/(r*r))/3.0; } while (_abs(r-p)>1e-14);
    return r;
}

double uix_pow(double b, double e)
{
    if (e==0.0) return 1.0;
    if (b==0.0) return 0.0;
    if (b<0.0 && e!=(long long)e) { uix_errno=UIX_EDOM; return UIX_NAN; }
    double ab = _abs(b);
    double r  = uix_exp(e * uix_log(ab));
    return (b<0.0 && (long long)e%2!=0) ? -r : r;
}

double uix_sin(double x)
{
    while (x >  UIX_M_PI) x -= 2.0*UIX_M_PI;
    while (x < -UIX_M_PI) x += 2.0*UIX_M_PI;
    double r=x, t=x;
    for (int n=1; n<=15; n++) {
        t *= -x*x/(double)((2*n)*(2*n+1));
        r += t;
    }
    return r;
}

double uix_cos(double x)  { return uix_sin(x + UIX_M_PI/2.0); }
double uix_tan(double x)
{
    double c = uix_cos(x);
    if (_abs(c)<1e-15) return UIX_HUGE_VAL;
    return uix_sin(x)/c;
}

double uix_atan(double x)
{
    if (_abs(x)>1.0)
        return (x>0?1:-1)*(UIX_M_PI/2.0 - uix_atan(1.0/_abs(x)));
    double r=0.0, t=x;
    for (int n=0; n<30; n++) {
        r += t/(double)(2*n+1) * (n%2==0 ? 1.0 : -1.0);
        t *= x*x;
    }
    return r;
}

double uix_atan2(double y, double x)
{
    if (x>0.0) return uix_atan(y/x);
    if (x<0.0) return uix_atan(y/x)+(y>=0.0?UIX_M_PI:-UIX_M_PI);
    return y>0.0 ? UIX_M_PI/2.0 : (y<0.0 ? -UIX_M_PI/2.0 : 0.0);
}

double uix_asin(double x)
{
    if (x<-1.0||x>1.0) { uix_errno=UIX_EDOM; return UIX_NAN; }
    return uix_atan2(x, uix_sqrt(1.0-x*x));
}

double uix_acos(double x)
{
    if (x<-1.0||x>1.0) { uix_errno=UIX_EDOM; return UIX_NAN; }
    return UIX_M_PI/2.0 - uix_asin(x);
}

double uix_sinh(double x) { double e=uix_exp(x); return (e-1.0/e)/2.0; }
double uix_cosh(double x) { double e=uix_exp(x); return (e+1.0/e)/2.0; }
double uix_tanh(double x) { double e=uix_exp(2.0*x); return (e-1.0)/(e+1.0); }

int uix_isnan    (double x) { return x != x; }
int uix_isinf    (double x) { return x==UIX_INFINITY||x==-UIX_INFINITY; }
int uix_isfinite (double x) { return !uix_isnan(x)&&!uix_isinf(x); }

float uix_sinf (float x) { return (float)uix_sin(x); }
float uix_cosf (float x) { return (float)uix_cos(x); }
float uix_tanf (float x) { return (float)uix_tan(x); }
float uix_sqrtf(float x) { return (float)uix_sqrt(x); }
float uix_fabsf(float x) { return x<0.0f?-x:x; }
float uix_powf (float b, float e) { return (float)uix_pow(b,e); }
float uix_ceilf (float x) { return (float)uix_ceil(x); }
float uix_floorf(float x) { return (float)uix_floor(x); }
float uix_roundf(float x) { return (float)uix_round(x); }
