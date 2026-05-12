
#ifndef __UIX_WCHAR__H
#define __UIX_WCHAR__H
/*
stddef.h
*/
/* This is for only STDLIB */

//#include "features.h"


#include "uix_types.h"
#include "uix_stddef.h"

typedef uix_int32_t  uix_wchar_t; //Wide char — POSIX wchar_t, 32-bit on Linux (UCS-4)
typedef uix_int32_t  uix_wint_t; //Wide int — holds any wide char plus WEOF
typedef struct { int state; } uix_mbstate_t;

#define UIX_WEOF        ((uix_wint_t)-1) //End-of-file for wide streams — maps to WEOF in <wchar.h>
#define UIX_WCHAR_MIN   (-2147483647-1)
#define UIX_WCHAR_MAX   2147483647

uix_size_t  uix_wcslen  (const uix_wchar_t *s); //Wide string length — POSIX wcslen() from <wchar.h>
uix_wchar_t *uix_wcscpy (uix_wchar_t *d, const uix_wchar_t *s); //Wide string copy — POSIX wcscpy()
uix_wchar_t *uix_wcsncpy(uix_wchar_t *d, const uix_wchar_t *s, uix_size_t n);
uix_wchar_t *uix_wcscat (uix_wchar_t *d, const uix_wchar_t *s);
int          uix_wcscmp (const uix_wchar_t *s1, const uix_wchar_t *s2);
int          uix_wcsncmp(const uix_wchar_t *s1, const uix_wchar_t *s2, uix_size_t n);
uix_wchar_t *uix_wcschr (const uix_wchar_t *s, uix_wchar_t c);
uix_wchar_t *uix_wcsstr (const uix_wchar_t *h, const uix_wchar_t *n);

uix_size_t uix_wcstombs(char *dst, const uix_wchar_t *src, uix_size_t n); //Wide to multibyte conversion — POSIX wcstombs()
uix_size_t uix_mbstowcs(uix_wchar_t *dst, const char *src, uix_size_t n); //Multibyte to wide char — POSIX mbtowc()
int        uix_wctomb  (char *s, uix_wchar_t wc);
int        uix_mbtowc  (uix_wchar_t *pwc, const char *s, uix_size_t n);

uix_wint_t  uix_btowc  (int c);
int         uix_wctob  (uix_wint_t c);



#endif /* End of __UIX_WCHAR__H */
/* ***This is End of file, there is no more line should be added after this line*** */
