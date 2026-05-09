
#ifndef __SYS_UIX_TIMES__H
#define __SYS_UIX_TIMES__H
/*
sys/times.h and a matching minimal source file.

Unlike time.h, this header is specifically for the times() function, which reports process CPU usage.
*/
/* This is for only POXIS */

#include "uix_features.h"//??

#include "uix_types.h"

typedef struct uix_tms {
    uix_clock_t tms_utime;  // User CPU time of process — in clock ticks
    uix_clock_t tms_stime;  // Kernel CPU time of process
    uix_clock_t tms_cutime;  // User time of waited-for children
    uix_clock_t tms_cstime;  // Kernel time of waited-for children
} uix_tms_t;

uix_clock_t uix_times(uix_tms_t *buf);  // times() — POSIX, returns elapsed real time in ticks, fills tms struct



#endif /* End of __SYS_UIX_TIMES__H */
/* ***This is End of file, there is no more line should be added after this line*** */
