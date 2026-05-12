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

//#include "uix_features.h" //?


int uix_isalpha (int c); // True if c is a-z or A-Z
int uix_isdigit (int c); // TTrue if c is 0-9
int uix_isalnum (int c); // True if alpha or digit
int uix_isspace (int c); // True if space, tab, newline, carriage return, form feed, vertical tab
int uix_isupper (int c); // True if A-Z
int uix_islower (int c); // True if a-z
int uix_ispunct (int c); // True if printable but not alnum or space
int uix_isprint (int c); // True if c is printable including space (0x20–0x7E)
int uix_isgraph (int c); // True if printable and not space
int uix_iscntrl (int c); // True if control character (0–31 or 127)
int uix_isxdigit(int c); // True if 0-9, a-f, or A-F
int uix_isblank (int c); // True if space or tab — C99/POSIX 2008 addition
int uix_toupper (int c); // Converts lowercase to uppercase
int uix_tolower (int c); // Converts uppercase to lowercase



#endif /* End of __UIX_CTYPE__H */
/* ***This is End of file, there is no more line should be added after this line*** */
