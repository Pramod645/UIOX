#include "../../40_SystemCallInterface/uix_sys.h"

int main(void)
{
    /* time() */
    uix_time_t t = sys_time((uix_time_t*)0);

    /* gettimeofday() */
    uix_timeval_t tv;
    sys_gettimeofday(&tv, (void*)0);

    /* clock_gettime() */
    uix_timespec_t ts;
    sys_clock_gettime(0 /* CLOCK_REALTIME */, &ts);

    /* times() */
    uix_tms_t tms;
    sys_times(&tms);

    /* utime() */
    uix_utimbuf_t ut;
    ut.actime  = t;
    ut.modtime = t;
    sys_utime("/tmp/uix_fstest.txt", &ut);

    /* uname() */
    uix_utsname_t uts;
    sys_uname(&uts);

    return 0;
}
