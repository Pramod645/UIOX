
#ifndef __UIX_GRP__H
#define __UIX_GRP__H
/*
POSIX grp.h header and a simple example program that uses it.  

Overview
• <grp.h> provides functions for accessing the group database, typically /etc/group.  
• Like <pwd.h>, this header is part of POSIX and already exists on UNIX/Linux/macOS systems 
(you don’t write it yourself).  
• It lets you look up information about groups, such as names, GIDs, and member lists.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>

struct group {
    char   grname;   // Group name /
    char   grpasswd; // Group password /
    gidt   grgid;    // Group ID /
    char  *grmem;    // Null-terminated list of member names /
};

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Retrieve group entry by name or GID /
struct group getgrnam(const char name);
struct group getgrgid(gidt gid);

// Sequential group access /
struct group getgrent(void);
void setgrent(void);
void endgrent(void);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __UIX_GRP__H */
/* ***This is End of file, there is no more line should be added after this line*** */