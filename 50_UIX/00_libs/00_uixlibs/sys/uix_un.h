
#ifndef __SYS_SOCKET__H
#define __SYS_SOCKET__H
/*
sys/un.h is the correct file for Unix domain sockets. It’s a standard POSIX header that defines the structure used for local (interprocess) socket communication on the same host.  

Here’s what a typical implementation looks like on Linux or BSD systems (simplified and portable):
A custom header file from a specific project or library (e.g., something you saw in legacy Unix source code).  
A mistyped or abbreviated form of another file — for example:
   - unistd.h (commonly used in Unix-like systems for POSIX APIs)
   - sys/un.h (which defines structures for Unix domain sockets)
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/socket.h>  // for safamilyt

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Structure describing a UNIX domain socket address */
struct sockaddrun {
    safamilyt sunfamily;  // AFUNIX or AFLOCAL /
    char sunpath[108];      // pathname /
};

/* Backward compatibility macros */
#define UNIXPATHMAX sizeof(((struct sockaddrun )0)->sunpath)

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */


/* include/uix_un.h */
#ifndef UIX_UN_H
#define UIX_UN_H

#include "uix_socket.h"

#define UIX_UNIX_PATH_MAX 108          // Maximum path length in UNIX domain socket address

typedef struct uix_sockaddr_un {
    uix_sa_family_t sun_family;                    // Must be AF_UNIX
    char            sun_path[UIX_UNIX_PATH_MAX];     // Filesystem path for socket
} uix_sockaddr_un_t;

#endif /* UIX_UN_H */


#endif /* End of __SYS_SOCKET__H */
/* ***This is End of file, there is no more line should be added after this line*** */