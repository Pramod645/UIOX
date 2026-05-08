
#ifndef __STDBOOL__H
#define __STDBOOL__H
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

#ifndef UIX_STDBOOL_H
#define UIX_STDBOOL_H

#ifndef __cplusplus
typedef int uix_bool; // Boolean type — maps to C99 _Bool, stdbool.h
#define uix_true  1 // Boolean true — POSIX uses integer 1 for true
#define uix_false 0 // Boolean false
#endif

#define UIX_BOOL_WIDTH 1

#endif /* UIX_STDBOOL_H */


#endif /* End of __STDBOOL__H */
/* ***This is End of file, there is no more line should be added after this line*** */