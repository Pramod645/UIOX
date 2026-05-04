#ifndef __UIX_CTYPE__H
#define __UIX_CTYPE__H
/*
ctype.h is one of the classic C standard headers.It provides character classification and conversion functions, 
which operate on values of type unsigned char (or EOF), commonly used when analyzing or transforming text.

Here a simplified conceptual model header showing what the standard defines.  

The real header in your system’s C library (glibc, musl, BSD libc, etc.) uses internal lookup tables and 
macros for performance.  
This version is educational—it represents the public interface and semantics clearly.

ctype.h — Character classification and conversion 
*/
/* This is for both POXIS and Standerd Library */

#include "features.h"

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

/* Classification functions: each returns nonzero if true, 0 otherwise */
int isalnum(int c);  /* Alphanumeric: isalpha() or isdigit() */
int isalpha(int c);  /* Alphabetic: upper or lower */
int iscntrl(int c);  /* Control character */
int isdigit(int c);  /* Decimal digit [0–9] */
int isgraph(int c);  /* Any visible character (not space) */
int islower(int c);  /* Lowercase letter */
int isprint(int c);  /* Printable (including space) */
int ispunct(int c);  /* Punctuation character */
int isspace(int c);  /* Whitespace (space, tab, newline, etc.) */
int isupper(int c);  /* Uppercase letter */
int isxdigit(int c); /* Hexadecimal digit [0–9a–fA–F] */

/* Additional POSIX function */
int isblank(int c);  /* Space or tab */

/* Character conversions */
int tolower(int c);
int toupper(int c);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB */

#ifndef UIX_CTYPE_H
#define UIX_CTYPE_H

int uix_isalpha (int c);
int uix_isdigit (int c);
int uix_isalnum (int c);
int uix_isspace (int c);
int uix_isupper (int c);
int uix_islower (int c);
int uix_ispunct (int c);
int uix_isprint (int c);
int uix_isgraph (int c);
int uix_iscntrl (int c);
int uix_isxdigit(int c);
int uix_isblank (int c);
int uix_toupper (int c);
int uix_tolower (int c);

#endif /* UIX_CTYPE_H */




#endif /* End of __UIX_CTYPE__H */
/* ***This is End of file, there is no more line should be added after this line*** */