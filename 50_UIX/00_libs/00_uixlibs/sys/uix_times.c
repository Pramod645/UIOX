/****************************  uix_times.c *****************************************/
#include "uix_times.h"
#include "../uix_string.h"
#include "../uix_errno.h"

uix_clock_t uix_times(uix_tms_t *buf)
{
    extern long sys_times(void *) __attribute__((weak));
    if (sys_times) return (uix_clock_t)sys_times(buf);
    if (buf) uix_memset(buf, 0, sizeof(uix_tms_t));
    return 0;
}

/* ***This is End of file, there is no more line should be added after this line*** */
