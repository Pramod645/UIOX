
#ifndef __UCHAR__H
#define __UCHAR__H
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

#ifndef UIX_UCHAR_H
#define UIX_UCHAR_H

#include "uix_types.h"
#include "uix_wchar.h"

typedef uix_uint16_t uix_char16_t;
typedef uix_uint32_t uix_char32_t;

uix_size_t uix_mbrtoc16(uix_char16_t *pc16, const char *s,
                         uix_size_t n, uix_mbstate_t *ps);
uix_size_t uix_c16rtomb(char *s, uix_char16_t c16, uix_mbstate_t *ps);
uix_size_t uix_mbrtoc32(uix_char32_t *pc32, const char *s,
                         uix_size_t n, uix_mbstate_t *ps);
uix_size_t uix_c32rtomb(char *s, uix_char32_t c32, uix_mbstate_t *ps);

#endif /* UIX_UCHAR_H */


#endif /* End of __UCHAR__H */
/* ***This is End of file, there is no more line should be added after this line*** */