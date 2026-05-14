
#ifndef __UIX_TIME__H
#define __UIX_TIME__H
/*
POSIX <time.h> header works, what it defines, and a full example program using it.  

Overview
• <time.h> provides types, macros, and functions for working with calendar time and CPU time.  
• It includes time structures like timet and struct tm, and functions such as time(), localtime(), strftime(), 
and difftime().

This header is part of both the C standard and the POSIX standard — so every Unix-like 
system (Linux, macOS, BSD, etc.) provides it.

*/
/* This is for only POXIS and __GLIBC*/

//#include "features.h"

#include "../sys/uix_types.h"   // for clockt, timet, etc. ./

//#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

// Types /
typedef long uix_timet;     // Seconds since the Epoch /
typedef long uix_clockt;    // Processor clock ticks /

struct uix_tm {
    int tmsec;   // seconds [0-60] /
    int tmmin;   // minutes [0-59] /
    int tmhour;  // hours [0-23] /
    int tmmday;  // day of the month [1-31] /
    int tmmon;   // months since January [0-11] /
    int tmyear;  // years since 1900 /
    int tmwday;  // days since Sunday [0-6] /
    int tmyday;  // days since January 1 [0-365] /
    int tmisdst; // daylight saving time flag /
};

// Macros /
#define CLOCKSPERSEC 1000000L

// Functions /
uix_timet   uix_time(uix_timet t);
double   uix_difftime(uix_timet end, uix_timet beginning);
uix_timet   uix_mktime(struct tm timeptr);
//char    uix_ctime(const timet timep);
struct uix_tm uix_gmtime(const uix_timet timep);
struct uix_tm uix_localtime(const uix_timet timep);
uix_size_t   uix_strftime(char s, uix_size_t maxsize, const char format,
                  const struct tm timeptr);
uix_clockt  uix_clock(void);

#ifdef cplusplus
}
#endif


//#endif /* End  of POXIS and STDLIB*/

#endif /* End of __UIX_TIME__H */
/* ***This is End of file, there is no more line should be added after this line*** */

