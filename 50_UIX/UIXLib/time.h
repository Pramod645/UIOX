
#ifndef __TIME__H
#define __TIME__H
/*
POSIX <time.h> header works, what it defines, and a full example program using it.  

Overview
• <time.h> provides types, macros, and functions for working with calendar time and CPU time.  
• It includes time structures like timet and struct tm, and functions such as time(), localtime(), strftime(), 
and difftime().

This header is part of both the C standard and the POSIX standard — so every Unix-like 
system (Linux, macOS, BSD, etc.) provides it.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>   // for clockt, timet, etc. /

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

// Types /
typedef long timet;     // Seconds since the Epoch /
typedef long clockt;    // Processor clock ticks /

struct tm {
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
timet   time(timet t);
double   difftime(timet end, timet beginning);
timet   mktime(struct tm timeptr);
char    ctime(const timet timep);
struct tm gmtime(const timet timep);
struct tm localtime(const timet timep);
sizet   strftime(char s, sizet maxsize, const char format,
                  const struct tm timeptr);
clockt  clock(void);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/

#endif /* End of __TIME__H */
/* ***This is End of file, there is no more line should be added after this line*** */