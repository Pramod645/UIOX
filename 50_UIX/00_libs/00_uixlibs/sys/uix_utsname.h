
#ifndef __SYS_UTSNAME__H
#define __SYS_UTSNAME__H
/*
sys/utsname.h is another standard POSIX header.  
It defines the struct utsname structure and declares the uname() function, which provides information about the 
current system (like OS name, version, and machine type).

*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Structure returned by uname() */
struct utsname {
    char sysname[65];   // Operating system name (e.g., "Linux") /
    char nodename[65];  // Network node name (hostname) /
    char release[65];   // OS release level /
    char version[65];   // OS version level /
    char machine[65];   // Hardware type (e.g., "x8664") /
};

/* Function prototype */
int uname(struct utsname buf);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#ifndef UIX_UTSNAME_H
#define UIX_UTSNAME_H

#define UIX_UTSNAME_LENGTH 65

typedef struct uix_utsname {
    char sysname   [UIX_UTSNAME_LENGTH];
    char nodename  [UIX_UTSNAME_LENGTH];
    char release   [UIX_UTSNAME_LENGTH];
    char version   [UIX_UTSNAME_LENGTH];
    char machine   [UIX_UTSNAME_LENGTH];
    char domainname[UIX_UTSNAME_LENGTH];
} uix_utsname_t;

int uix_uname(uix_utsname_t *buf);

#endif /* UIX_UTSNAME_H */


#endif /* End of __SYS_UTSNAME__H */
/* ***This is End of file, there is no more line should be added after this line*** */