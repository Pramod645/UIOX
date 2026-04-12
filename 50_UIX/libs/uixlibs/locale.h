
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

#endif /* End of __LOCALE__H */
/* ***This is End of file, there is no more line should be added after this line*** */