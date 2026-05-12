
#ifndef __UIX_STDNORETURN__H
#define __UIX_STDNORETURN__H
/*
stddef.h
*/
/* This is for only STDLIB */

//#include "features.h"


#define uix_noreturn   __attribute__((noreturn)) //C11 _Noreturn — tells compiler function never returns, enables optimization. Applied to exit(), abort(), pthread_exit()
#define UIX_NORETURN   __attribute__((noreturn))



#endif /* End of __UIX_STDNORETURN__H */
/* ***This is End of file, there is no more line should be added after this line*** */
