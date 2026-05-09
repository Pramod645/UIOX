
#ifndef __UIX_STDBOOL__H
#define __UIX_STDBOOL__H
/*
stddef.h
*/
/* This is for only STDLIB */

#include "uix_features.h"//??


#ifndef __cplusplus
typedef int uix_bool; // Boolean type — maps to C99 _Bool, stdbool.h
#define uix_true  1 // Boolean true — POSIX uses integer 1 for true
#define uix_false 0 // Boolean false
#endif

#define UIX_BOOL_WIDTH 1


#endif /* End of __UIX_STDBOOL__H */
/* ***This is End of file, there is no more line should be added after this line*** */
