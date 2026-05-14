
#ifndef __UIX_LOCALE__H
#define __UIX_LOCALE__H
/*
locale.h is a C standard library header that defines facilities for locale management, which determines how your 
program handles region‑specific formatting:  
• decimal points,  
• currency symbols,  
• date/time representations, and  
• character classification (isalpha, tolower, etc.) based on regional conventions.
*/
/* This is for only POXIS and standerd*/

//#include "uix_features.h"//?



#include "../sys/uix_types.h"

#define UIX_LC_ALL      0    // All locale categories
#define UIX_LC_COLLATE  1
#define UIX_LC_CTYPE    2     // Character classification and conversion
#define UIX_LC_MONETARY 3    // Currency formatting
#define UIX_LC_NUMERIC  4    // Number formatting (decimal point)
#define UIX_LC_TIME     5   // Date and time formatting
#define UIX_LC_MESSAGES 6   // Error messages and diagnostics

typedef struct uix_lconv {
    char *decimal_point;     // Decimal separator character
    char *thousands_sep;     // Thousands grouping separator
    char *grouping;
    char *int_curr_symbol;  
    char *currency_symbol;      // Local currency symbol
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

char         *uix_setlocale(int category, const char *locale);  // Sets/queries locale — POSIX
uix_lconv_t  *uix_localeconv(void);              // Returns current locale numeric/monetary conventions




#endif /* End of __UIX_LOCALE__H */
/* ***This is End of file, there is no more line should be added after this line*** */
