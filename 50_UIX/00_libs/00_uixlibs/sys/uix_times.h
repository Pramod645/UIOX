
#ifndef __SYS_TIMES__H
#define __SYS_TIMES__H
/*
sys/times.h and a matching minimal source file.

Unlike time.h, this header is specifically for the times() function, which reports process CPU usage.
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/*
  Structure filled by times().
  All values are measured in clock ticks.
 */
struct tms {
    clockt tmsutime;   // user CPU time /
    clockt tmsstime;   // system CPU time /
    clockt tmscutime;  // user CPU time of children /
    clockt tmscstime;  // system CPU time of children /
};

/*
  Return value:
    elapsed real time in clock ticks since an arbitrary point,
    typically system boot time.
 */
clockt times(struct tms buf);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */


/* include/uix_times.h */
#ifndef UIX_TIMES_H
#define UIX_TIMES_H

#include "uix_types.h"

typedef struct uix_tms {
    uix_clock_t tms_utime;
    uix_clock_t tms_stime;
    uix_clock_t tms_cutime;
    uix_clock_t tms_cstime;
} uix_tms_t;

uix_clock_t uix_times(uix_tms_t *buf);

#endif /* UIX_TIMES_H */



#endif /* End of __SYS_TIMES__H */
/* ***This is End of file, there is no more line should be added after this line*** */