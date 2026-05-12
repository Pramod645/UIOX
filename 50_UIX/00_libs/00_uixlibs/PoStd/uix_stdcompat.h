
#ifndef __UIX_STDCOMPAT__H
#define __UIX_STDCOMPAT__H
/*
stddef.h
*/
/* This is for only STDLIB */

//#include "uix_features.h"


/* Compatibility shims for POSIX / C99 / C11 / C23 */
#include "uix_features.h"
#include "uix_types.h"

/* min/max */
#define UIX_MIN(a,b) ((a)<(b)?(a):(b)) //Safe minimum macro — glibc defines identical MIN() in <sys/param.h>
#define UIX_MAX(a,b) ((a)>(b)?(a):(b)) //Safe maximum macro
#define UIX_CLAMP(v,lo,hi) UIX_MIN(UIX_MAX((v),(lo)),(hi)) //Clamp value to range — common glibc extension

/* Array helpers */
#define UIX_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0])) //Number of elements in array — sizeof(a)/sizeof(a[0]) idiom

/* Stringify / concat */
#define UIX_STR(x)   #x
#define UIX_XSTR(x)  UIX_STR(x)
#define UIX_CAT(a,b) a##b

/* Unused variable suppressor */
#define UIX_UNUSED(x) ((void)(x))

/* Deprecated attribute */
#define UIX_DEPRECATED __attribute__((deprecated))

/* Visibility */
#define UIX_EXPORT __attribute__((visibility("default"))) //Symbol visibility — maps to glibc's __attribute__((visibility("default")))
#define UIX_HIDDEN __attribute__((visibility("hidden"))) // Internal symbol — maps to __attribute__((visibility("hidden")))




#endif /* End of __UIX_STDCOMPAT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
