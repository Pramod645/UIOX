
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

#include "uix_features.h"

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



#ifndef UIX_GRP_H
#define UIX_GRP_H

#include "sys/uix_types.h"

typedef struct uix_group {
    char      *gr_name;      // Group name
    char      *gr_passwd;
    uix_gid_t  gr_gid;       // Group ID
    char     **gr_mem;      // Null-terminated array of member names
} uix_group_t;

uix_group_t *uix_getgrgid  (uix_gid_t gid); // Looks up group by GID
uix_group_t *uix_getgrnam  (const char *name);  // Looks up group by name

/* Reentrant version — POSIX */
int          uix_getgrgid_r(uix_gid_t gid, uix_group_t *grp,
                             char *buf, uix_size_t buflen,
                             uix_group_t **result);
int          uix_getgrnam_r(const char *name, uix_group_t *grp,
                             char *buf, uix_size_t buflen,
                             uix_group_t **result);
void         uix_setgrent  (void);   // Rewinds group database
void         uix_endgrent  (void);
uix_group_t *uix_getgrent  (void);  // Returns next group entry

#endif /* UIX_GRP_H */



#endif /* End of __UIX_GRP__H */
/* ***This is End of file, there is no more line should be added after this line*** */