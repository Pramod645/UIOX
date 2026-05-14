
#ifndef __UIX_UTIME__H
#define __UIX_UTIME__H
/*
utime.h header defines the utime() function, which lets a program set the modification and access times of a file.  
It’s the classic (POSIX) interface for adjusting timestamps, predating the newer utimensat() and utimes() functions.

*/
/* This is for only POXIS */

//#include "features.h"


#include "../sys/uix_types.h"

typedef struct uix_utimbuf {
    uix_time_t actime;  // Access time to set on file
    uix_time_t modtime;  // Modification time to set on file
} uix_utimbuf_t;

int uix_utime(const char *path, const uix_utimbuf_t *times); // utime() — POSIX, sets file access and modification times. Deprecated in favor of utimensat()



#endif /* End of __UIX_UTIME__H */
/* ***This is End of file, there is no more line should be added after this line*** */
