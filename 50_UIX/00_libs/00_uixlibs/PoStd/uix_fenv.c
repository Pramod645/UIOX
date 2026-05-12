/* src/uix_fenv.c */
#include "uix_fenv.h"

#if 0
int uix_feclearexcept(int excepts)
{
    (void)excepts;
    __asm__ __volatile__("fnclex" ::: "memory");
    unsigned int mxcsr;
    __asm__ __volatile__("stmxcsr %0" : "=m"(mxcsr));
    mxcsr &= ~(unsigned int)(excepts & UIX_FE_ALL_EXCEPT);
    __asm__ __volatile__("ldmxcsr %0" :: "m"(mxcsr));
    return 0;
}

int uix_fetestexcept(int excepts)
{
    unsigned short sw; unsigned int mxcsr;
    __asm__ __volatile__("fnstsw %0" : "=m"(sw));
    __asm__ __volatile__("stmxcsr %0" : "=m"(mxcsr));
    return (int)((sw | mxcsr) & excepts & UIX_FE_ALL_EXCEPT);
}

int uix_feraiseexcept(int excepts)
{
    if (excepts & UIX_FE_DIVBYZERO) { volatile double z=1.0/0.0; (void)z; }
    if (excepts & UIX_FE_INVALID)   { volatile double n=0.0/0.0; (void)n; }
    if (excepts & UIX_FE_OVERFLOW)  { volatile double o=1e308*1e308; (void)o; }
    return 0;
}

int uix_fegetexceptflag(uix_fexcept_t *flagp, int excepts)
    { if (!flagp) return -1; *flagp = (uix_fexcept_t)uix_fetestexcept(excepts); return 0; }
int uix_fesetexceptflag(const uix_fexcept_t *flagp, int excepts)
    { if (!flagp) return -1; (void)excepts; return 0; }

int uix_fegetround(void)
{
    unsigned short cw;
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    return (int)(cw & 0x0C00);
}

int uix_fesetround(int round)
{
    unsigned short cw;
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    cw = (unsigned short)((cw & ~0x0C00) | (round & 0x0C00));
    __asm__ __volatile__("fldcw %0" :: "m"(cw));
    return 0;
}

int uix_fegetenv(uix_fenv_t *envp)
{
    if (!envp) return -1;
    __asm__ __volatile__("fnstcw %0" : "=m"(envp->cw));
    __asm__ __volatile__("fnstsw %0" : "=m"(envp->sw));
    __asm__ __volatile__("stmxcsr %0" : "=m"(envp->mxcsr));
    return 0;
}

int uix_fesetenv(const uix_fenv_t *envp)
{
    if (!envp || envp == UIX_FE_DFL_ENV) {
        __asm__ __volatile__("fninit" ::: "memory");
        return 0;
    }
    __asm__ __volatile__("fldcw %0"   :: "m"(envp->cw));
    __asm__ __volatile__("ldmxcsr %0" :: "m"(envp->mxcsr));
    return 0;
}

int uix_feholdexcept(uix_fenv_t *envp)
{
    uix_fegetenv(envp);
    uix_feclearexcept(UIX_FE_ALL_EXCEPT);
    return 0;
}

int uix_feupdateenv(const uix_fenv_t *envp)
{
    int raised = uix_fetestexcept(UIX_FE_ALL_EXCEPT);
    uix_fesetenv(envp);
    if (raised) uix_feraiseexcept(raised);
    return 0;
}
#endif

/* ***This is End of file, there is no more line should be added after this line*** */
