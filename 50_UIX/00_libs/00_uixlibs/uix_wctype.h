
#ifndef __WCTYPE__H
#define __WCTYPE__H
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


#ifndef UIX_WCTYPE_H
#define UIX_WCTYPE_H

#include "uix_wchar.h"

typedef uix_uint32_t uix_wctype_t; //Wide character class descriptor — POSIX wctype_t
typedef uix_uint32_t uix_wctrans_t;

int uix_iswalpha (uix_wint_t c); //Tests if wide char is alphabetic — POSIX iswalpha()
int uix_iswdigit (uix_wint_t c); //Tests if wide char is digit — POSIX iswdigit()
int uix_iswalnum (uix_wint_t c);
int uix_iswspace (uix_wint_t c);
int uix_iswupper (uix_wint_t c);
int uix_iswlower (uix_wint_t c);
int uix_iswpunct (uix_wint_t c);
int uix_iswprint (uix_wint_t c);
int uix_iswgraph (uix_wint_t c);
int uix_iswcntrl (uix_wint_t c);
int uix_iswxdigit(uix_wint_t c);
int uix_iswblank (uix_wint_t c);

uix_wint_t   uix_towupper (uix_wint_t c); //Converts wide char to uppercase — POSIX towupper()
uix_wint_t   uix_towlower (uix_wint_t c);
uix_wctype_t uix_wctype   (const char *name); //Returns class handle for named class like "alpha" — POSIX wctype()
int          uix_iswctype (uix_wint_t c, uix_wctype_t type); //Tests wide char against class — POSIX iswctype()

#endif /* UIX_WCTYPE_H */




#endif /* End of __WCTYPE__H */
/* ***This is End of file, there is no more line should be added after this line*** */