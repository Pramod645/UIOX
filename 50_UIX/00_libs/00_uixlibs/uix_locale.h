
#ifndef __LOCALE__H
#define __LOCALE__H
/*
locale.h is a C standard library header that defines facilities for locale management, which determines how your 
program handles region‑specific formatting:  
• decimal points,  
• currency symbols,  
• date/time representations, and  
• character classification (isalpha, tolower, etc.) based on regional conventions.
*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

// Typical structure describing formatting conventions /
struct lconv {
    char decimalpoint;
    char thousandssep;
    char grouping;
    char intcurrsymbol;
    char currencysymbol;
    char mondecimalpoint;
    char monthousandssep;
    char mongrouping;
    char positivesign;
    char negativesign;
    char intfracdigits;
    char fracdigits;
    char pcsprecedes;
    char psepbyspace;
    char ncsprecedes;
    char nsepbyspace;
    char psignposn;
    char nsignposn;
};

// Functions for locale management /
char setlocale(int category, const char locale);
struct lconv localeconv(void);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/



/* include/uix_locale.h */
#ifndef UIX_LOCALE_H
#define UIX_LOCALE_H

#include "uix_types.h"

#define UIX_LC_ALL      0
#define UIX_LC_COLLATE  1
#define UIX_LC_CTYPE    2
#define UIX_LC_MONETARY 3
#define UIX_LC_NUMERIC  4
#define UIX_LC_TIME     5
#define UIX_LC_MESSAGES 6

typedef struct uix_lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char  int_frac_digits;
    char  frac_digits;
    char  p_cs_precedes;
    char  p_sep_by_space;
    char  n_cs_precedes;
    char  n_sep_by_space;
    char  p_sign_posn;
    char  n_sign_posn;
} uix_lconv_t;

char         *uix_setlocale(int category, const char *locale);
uix_lconv_t  *uix_localeconv(void);

#endif /* UIX_LOCALE_H */




#endif /* End of __LOCALE__H */
/* ***This is End of file, there is no more line should be added after this line*** */