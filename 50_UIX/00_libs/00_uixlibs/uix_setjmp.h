
#ifndef __SETJUMP__H
#define __SETJUMP__H
/*
stddef.h
*/
/* This is for only STDLIB */

#include "features.h"

#if  (define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif



#ifdef cplusplus
}
#endif


#endif /* End  of STDLIB*/

#ifndef UIX_SETJMP_H
#define UIX_SETJMP_H

#include "uix_types.h"

#define UIX_JMP_BUF_SIZE 200

typedef struct { unsigned char data[UIX_JMP_BUF_SIZE]; } uix_jmp_buf[1];

typedef struct {
    unsigned char data[UIX_JMP_BUF_SIZE];
    uix_uint64_t  sigmask;
    int           saved_mask;
} uix_sigjmp_buf[1];

int  uix_setjmp    (uix_jmp_buf env);
void uix_longjmp   (uix_jmp_buf env, int val) __attribute__((noreturn));
int  uix_sigsetjmp (uix_sigjmp_buf env, int savemask);
void uix_siglongjmp(uix_sigjmp_buf env, int val) __attribute__((noreturn));

#endif /* UIX_SETJMP_H */


#endif /* End of __SETJUMP__H */
/* ***This is End of file, there is no more line should be added after this line*** */