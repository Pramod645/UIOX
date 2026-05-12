
#ifndef __UIX_STDALIGN__H
#define __UIX_STDALIGN__H
/*
stddef.h
*/
/* This is for only STDLIB */

//#include "uix_features.h"//??


#ifndef UIX_STDALIGN_H
#define UIX_STDALIGN_H

#define uix_alignas(x)   __attribute__((aligned(x))) //C11 alignas() — guarantees structure/variable memory alignment
#define uix_alignof(x)   __alignof__(x)
#define UIX_ALIGNOF(x)   __alignof__(x)
#define UIX_ALIGNAS(x)   __attribute__((aligned(x))) // C11 alignof() — returns alignment requirement of a type




#endif /* End of __UIX_STDALIGN__H */
/* ***This is End of file, there is no more line should be added after this line*** */
