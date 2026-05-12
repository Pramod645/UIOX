
#ifndef __SYS_UIX_UN__H
#define __SYS_UIX_UN__H
/*
sys/un.h is the correct file for Unix domain sockets. It’s a standard POSIX header that defines the structure used for local (interprocess) socket communication on the same host.  

Here’s what a typical implementation looks like on Linux or BSD systems (simplified and portable):
A custom header file from a specific project or library (e.g., something you saw in legacy Unix source code).  
A mistyped or abbreviated form of another file — for example:
   - unistd.h (commonly used in Unix-like systems for POSIX APIs)
   - sys/un.h (which defines structures for Unix domain sockets)
*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#include "uix_socket.h"

#define UIX_UNIX_PATH_MAX 108          // Maximum path length in UNIX domain socket address
#if 0
typedef struct uix_sockaddr_un {
    uix_sa_family_t sun_family;                    // Must be AF_UNIX
    char            sun_path[UIX_UNIX_PATH_MAX];     // Filesystem path for socket
} uix_sockaddr_un_t;
#endif
#endif /* End of __SYS_UIX_UN__H */
/* ***This is End of file, there is no more line should be added after this line*** */
