
#ifndef __TIMEB_H
#define __TIMEB_H
/*
let’s look at <sys/timeb.h>, what it does, and how you might use it.  

Overview
• <sys/timeb.h> defines the timeb structure and the ftime() function, which gives you the current time with millisecond resolution.  
• It’s considered legacy (historical) but still available on most POSIX and Linux systems for compatibility with older C code.

Newer code usually uses gettimeofday() (from <sys/time.h>) or clockgettime(), but <sys/timeb.h> is useful for educational and legacy purposes.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <time.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif


struct timeb {
    timet time;        // seconds since Epoch /
    unsigned short millitm; // milliseconds part /
    short timezone;     // minutes west of UTC /
    short dstflag;      // daylight saving time flag /
};

/* Fill a struct timeb with current time */
int ftime(struct timeb tp);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */


/* include/uix_timeb.h */
#ifndef UIX_TIMEB_H
#define UIX_TIMEB_H

#include "uix_types.h"

typedef struct uix_timeb {
    uix_time_t time;
    unsigned short millitm;  // Milliseconds within current second
    short timezone;  // Minutes west of GMT — largely obsolete
    short dstflag;
} uix_timeb_t;  // Seconds since epoch — same as time_t

int uix_ftime(uix_timeb_t *tp);  // ftime() — obsolete POSIX function, superseded by gettimeofday(). Fills uix_timeb_t

#endif /* UIX_TIMEB_H */



#endif /* End of __TIMEB_H */
/* ***This is End of file, there is no more line should be added after this line*** */