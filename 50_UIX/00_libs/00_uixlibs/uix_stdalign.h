
#ifndef __STDALIGN__H
#define __STDALIGN__H
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


#ifndef UIX_STDALIGN_H
#define UIX_STDALIGN_H

#define uix_alignas(x)   __attribute__((aligned(x)))
#define uix_alignof(x)   __alignof__(x)
#define UIX_ALIGNOF(x)   __alignof__(x)
#define UIX_ALIGNAS(x)   __attribute__((aligned(x)))

#endif /* UIX_STDALIGN_H */



#endif /* End of __STDALIGN__H */
/* ***This is End of file, there is no more line should be added after this line*** */