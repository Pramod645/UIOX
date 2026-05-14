
#ifndef __UIX_STRING__H
#define __UIX_STRING__H
/*
<string.h> is one of the most essential headers in the C Standard Library.  
It defines functions and macros for handling C-style strings (null‑terminated character arrays) and raw memory blocks.

*/
/* This is for only POXIS and standerd*/

//#include "features.h"


#include "../sys/uix_types.h"

uix_size_t  uix_strlen (const char *str); // Counts bytes until null terminator. O(n).
char       *uix_strcpy (char *dest, const char *src); // Copies including null. POSIX warns: no bounds check — use strncpy
char       *uix_strncpy(char *dest, const char *src, uix_size_t n); // Copies up to n bytes, pads with nulls if source shorter
char       *uix_strcat (char *dest, const char *src); // Appends s to d. No bounds check
char       *uix_strncat(char *dest, const char *src, uix_size_t n); // Appends at most n bytes, always null-terminates
int         uix_strcmp (const char *s1, const char *s2); // Lexicographic compare, returns negative/zero/positive
int         uix_strncmp(const char *s1, const char *s2, uix_size_t n); // Compares at most n characters
char       *uix_strchr (const char *str, int c); // Returns pointer to first occurrence of c in s
char       *uix_strrchr(const char *str, int c); // Returns pointer to last occurrence
char       *uix_strstr (const char *haystack, const char *needle);// Finds first occurrence of substring
char       *uix_strtok (char *str, const char *delim); //Tokenizes string — NOT reentrant, uses static state
uix_size_t  uix_strspn (const char *str, const char *accept); // Length of initial segment of s using only chars in a
uix_size_t  uix_strcspn(const char *str, const char *reject); // Length of initial segment of s not containing chars in r
char       *uix_strdup (const char *str); // Allocates and copies string — POSIX.1-2008 extension
char       *uix_strndup(const char *str, uix_size_t n); // Like strdup but limited to n chars — POSIX.1-2008

void       *uix_memcpy (void *dest, const void *src, uix_size_t n); // Copies n bytes — undefined behavior if regions overlap
void       *uix_memmove(void *dest, const void *src, uix_size_t n); // Copies n bytes — safe even if regions overlap
void       *uix_memset (void *ptr,  int value,        uix_size_t n); // Fills n bytes of memory with value v
int         uix_memcmp (const void *s1, const void *s2, uix_size_t n); // Byte-by-byte comparison
void       *uix_memchr (const void *ptr, int value, uix_size_t n); // Scans n bytes for first occurrence of c


#endif /* End of __UIX_STRING__H */
/* ***This is End of file, there is no more line should be added after this line*** */
