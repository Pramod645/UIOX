
#ifndef __UTIME__H
#define __UTIME__H
/*
utime.h header defines the utime() function, which lets a program set the modification and access times of a file.  
It’s the classic (POSIX) interface for adjusting timestamps, predating the newer utimensat() and utimes() functions.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>  // for timet /

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Structure used by utime() to set file access and modification times /
struct utimbuf {
    timet actime;   // access time /
    timet modtime;  // modification time /
};

// Function prototype /
int utime(const char filename, const struct utimbuf times)

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



/* include/uix_utime.h */
#ifndef UIX_UTIME_H
#define UIX_UTIME_H

#include "uix_types.h"

typedef struct uix_utimbuf {
    uix_time_t actime;
    uix_time_t modtime;
} uix_utimbuf_t;

int uix_utime(const char *path, const uix_utimbuf_t *times);

#endif /* UIX_UTIME_H */



#endif /* End of __UTIME__H */
/* ***This is End of file, there is no more line should be added after this line*** */