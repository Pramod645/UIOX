
#ifndef __STDNORETURN__H
#define __STDNORETURN__H
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


#ifndef UIX_STDNORETURN_H
#define UIX_STDNORETURN_H

#define uix_noreturn   __attribute__((noreturn))
#define UIX_NORETURN   __attribute__((noreturn))

#endif /* UIX_STDNORETURN_H */


#endif /* End of __STDNORETURN__H */
/* ***This is End of file, there is no more line should be added after this line*** */