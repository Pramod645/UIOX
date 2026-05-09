
#ifndef __UIX_SETJMP__H
#define __UIX_SETJMP__H
/*
setjmp.h
*/
/* This is for only STDLIB */

#include "uix_features.h"//??

#include "uix_types.h"

#define UIX_JMP_BUF_SIZE 200   // Size of jump buffer — stores registers: rbx,rbp,r12-r15,rsp,rip,mxcsr

typedef struct { unsigned char data[UIX_JMP_BUF_SIZE]; } uix_jmp_buf[1];

typedef struct {
    unsigned char data[UIX_JMP_BUF_SIZE];
    uix_uint64_t  sigmask;
    int           saved_mask;
} uix_sigjmp_buf[1];  // Extended jump buffer also saves signal mask

int  uix_setjmp    (uix_jmp_buf env);  // setjmp() — saves CPU register state, returns 0 first call
void uix_longjmp   (uix_jmp_buf env, int val) __attribute__((noreturn));  // longjmp() — restores register state, setjmp returns val
int  uix_sigsetjmp (uix_sigjmp_buf env, int savemask); // sigsetjmp() — like setjmp but optionally saves signal mask
void uix_siglongjmp(uix_sigjmp_buf env, int val) __attribute__((noreturn));  // siglongjmp() — restores signal mask if saved


#endif /* End of __UIX_SETJMP__H */
/* ***This is End of file, there is no more line should be added after this line*** */
