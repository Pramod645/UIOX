
#ifndef __SYS_UIX_TIMEB__H
#define __SYS_UIX_TIMEB__H
/*
let’s look at <sys/timeb.h>, what it does, and how you might use it.  

Overview
• <sys/timeb.h> defines the timeb structure and the ftime() function, which gives you the current time with millisecond resolution.  
• It’s considered legacy (historical) but still available on most POSIX and Linux systems for compatibility with older C code.

Newer code usually uses gettimeofday() (from <sys/time.h>) or clockgettime(), but <sys/timeb.h> is useful for educational and legacy purposes.

*/
/* This is for only POXIS */

//#include "uix_features.h"//?

#include "uix_types.h"

typedef struct uix_timeb {
    uix_time_t time;
    unsigned short millitm;  // Milliseconds within current second
    short timezone;  // Minutes west of GMT — largely obsolete
    short dstflag;
} uix_timeb_t;  // Seconds since epoch — same as time_t

int uix_ftime(uix_timeb_t *tp);  // ftime() — obsolete POSIX function, superseded by gettimeofday(). Fills uix_timeb_t



#endif /* End of __SYS_UIX_TIMEB__H */
/* ***This is End of file, there is no more line should be added after this line*** */
