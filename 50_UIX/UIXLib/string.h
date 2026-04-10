
#ifndef __STRING__H
#define __STRING__H
/*
<string.h> is one of the most essential headers in the C Standard Library.  
It defines functions and macros for handling C-style strings (null‑terminated character arrays) and raw memory blocks.

*/
/* This is for only POXIS */

#include "features.h"

#include <stddef.h>   / for sizet /

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

// Copying functions /
void memcpy(void dest, const void src, sizet n);
void memmove(void dest, const void src, sizet n);
char strcpy(char dest, const char src);
char strncpy(char dest, const char src, sizet n);

// Concatenation /
char strcat(char dest, const char src);
char strncat(char dest, const char src, sizet n);

// Comparison /
int memcmp(const void s1, const void s2, sizet n);
int strcmp(const char s1, const char s2);
int strncmp(const char s1, const char s2, sizet n);

// Search /
void memchr(const void s, int c, sizet n);
char strchr(const char s, int c);
char strrchr(const char s, int c);
char strstr(const char haystack, const char needle);

// Length /
sizet strlen(const char s);
sizet strnlen(const char s, sizet maxlen);

// Memory set /
void memset(void s, int c, sizet n);

// Utility /
char strerror(int errnum);
void memmove(void dest, const void src, sizet n);

// Tokenization /
char strtok(char str, const char delim);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/

#endif /* End of __STRING__H */
/* ***This is End of file, there is no more line should be added after this line*** */