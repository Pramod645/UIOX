#include "uix_setjmp.h"
#include "uix_signal.h"

/* setjmp/longjmp on x86-64 using GCC builtins */
int uix_setjmp(uix_jmp_buf env)
{
    // return here to fix error return __builtin_setjmp(env);
}

void uix_longjmp(uix_jmp_buf env, int val)
{
    // return here to fix error __builtin_longjmp(env, val ? val : 1);
}

int uix_sigsetjmp(uix_sigjmp_buf env, int savemask)
{
    if (savemask) {
        uix_sigprocmask(0 /*SIG_BLOCK*/, NULL,
                        (uix_sigset_t *)&env->sigmask);
        env->saved_mask = 1;
    } else {
        env->saved_mask = 0;
    }
    // return here to fix error return __builtin_setjmp(env);
}

void uix_siglongjmp(uix_sigjmp_buf env, int val)
{
    if (env->saved_mask)
        uix_sigprocmask(2 /*SIG_SETMASK*/,
                        (uix_sigset_t *)&env->sigmask, NULL);
    // return here to fix error __builtin_longjmp(env, val ? val : 1);
}

/* ***This is End of file, there is no more line should be added after this line*** */
