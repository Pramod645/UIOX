
#ifndef __STDCOMPAT__H
#define __STDCOMPAT__H
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


#ifndef UIX_STDCOMPAT_H
#define UIX_STDCOMPAT_H

/* Compatibility shims for POSIX / C99 / C11 / C23 */
#include "uix_features.h"
#include "uix_types.h"

/* min/max */
#define UIX_MIN(a,b) ((a)<(b)?(a):(b))
#define UIX_MAX(a,b) ((a)>(b)?(a):(b))
#define UIX_CLAMP(v,lo,hi) UIX_MIN(UIX_MAX((v),(lo)),(hi))

/* Array helpers */
#define UIX_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

/* Stringify / concat */
#define UIX_STR(x)   #x
#define UIX_XSTR(x)  UIX_STR(x)
#define UIX_CAT(a,b) a##b

/* Unused variable suppressor */
#define UIX_UNUSED(x) ((void)(x))

/* Deprecated attribute */
#define UIX_DEPRECATED __attribute__((deprecated))

/* Visibility */
#define UIX_EXPORT __attribute__((visibility("default")))
#define UIX_HIDDEN __attribute__((visibility("hidden")))

#endif /* UIX_STDCOMPAT_H */



#endif /* End of __STDCOMPAT__H */
/* ***This is End of file, there is no more line should be added after this line*** */