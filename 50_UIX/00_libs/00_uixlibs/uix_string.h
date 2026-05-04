
#ifndef __UIX_STRING__H
#define __UIX_STRING__H
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

#ifndef UIX_STRING_H
#define UIX_STRING_H

#include "uix_types.h"

uix_size_t  uix_strlen (const char *str);
char       *uix_strcpy (char *dest, const char *src);
char       *uix_strncpy(char *dest, const char *src, uix_size_t n);
char       *uix_strcat (char *dest, const char *src);
char       *uix_strncat(char *dest, const char *src, uix_size_t n);
int         uix_strcmp (const char *s1, const char *s2);
int         uix_strncmp(const char *s1, const char *s2, uix_size_t n);
char       *uix_strchr (const char *str, int c);
char       *uix_strrchr(const char *str, int c);
char       *uix_strstr (const char *haystack, const char *needle);
char       *uix_strtok (char *str, const char *delim);
uix_size_t  uix_strspn (const char *str, const char *accept);
uix_size_t  uix_strcspn(const char *str, const char *reject);
char       *uix_strdup (const char *str);
char       *uix_strndup(const char *str, uix_size_t n);

void       *uix_memcpy (void *dest, const void *src, uix_size_t n);
void       *uix_memmove(void *dest, const void *src, uix_size_t n);
void       *uix_memset (void *ptr,  int value,        uix_size_t n);
int         uix_memcmp (const void *s1, const void *s2, uix_size_t n);
void       *uix_memchr (const void *ptr, int value, uix_size_t n);

#endif /* UIX_STRING_H */


#endif /* End of __UIX_STRING__H */
/* ***This is End of file, there is no more line should be added after this line*** */