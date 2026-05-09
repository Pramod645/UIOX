
#ifndef __UIX_UCHAR__H
#define __UIX_UCHAR__H
/*
stddef.h
*/
/* This is for only STDLIB */

#include "features.h"


#include "uix_types.h"
#include "uix_wchar.h"

typedef uix_uint16_t uix_char16_t; //UTF-16 code unit — C11 char16_t from <uchar.h>
typedef uix_uint32_t uix_char32_t; //UTF-32 code unit — C11 char32_t

uix_size_t uix_mbrtoc16(uix_char16_t *pc16, const char *s,
                         uix_size_t n, uix_mbstate_t *ps); //Multibyte to UTF-16 — C11 POSIX mbrtoc16()
uix_size_t uix_c16rtomb(char *s, uix_char16_t c16, uix_mbstate_t *ps);
uix_size_t uix_mbrtoc32(uix_char32_t *pc32, const char *s,
                         uix_size_t n, uix_mbstate_t *ps); //Multibyte to UTF-32 — C11 POSIX mbrtoc32()
uix_size_t uix_c32rtomb(char *s, uix_char32_t c32, uix_mbstate_t *ps); //UTF-32 to multibyte — C11 POSIX c32rtomb()



#endif /* End of __UIX_UCHAR__H */
/* ***This is End of file, there is no more line should be added after this line*** */
