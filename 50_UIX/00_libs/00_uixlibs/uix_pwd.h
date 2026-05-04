
#ifndef __UIX_PWD__H
#define __UIX_PWD__H
/*
pwd.h and pwd.c are about in POSIX systems.

Overview
• <pwd.h> is a standard POSIX header that defines the password database API.  
  It allows programs to get user account information (like username, UID, home directory, etc.) 
  from the system’s user database — usually /etc/passwd.

• There is no standardized pwd.c file defined by POSIX — but you can implement a simple version demonstrating 
how <pwd.h> is used (e.g., to fetch and print user info).

pwd.h (provided by the system)

You don’t write this file yourself — it’s typically in /usr/include/pwd.h.  
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
struct passwd {
    char   pwname;   // Username /
    char   pwpasswd; // Encrypted password /
    uidt   pwuid;    // User ID /
    gidt   pwgid;    // Group ID /
    char   pwgecos;  // User information /
    char   pwdir;    // Home directory /
    char   pwshell;  // Login shell /
};

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

struct passwd getpwuid(uidt uid);
struct passwd getpwnam(const char name);

struct passwd getpwent(void);
void setpwent(void);
void endpwent(void);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



#ifndef UIX_PWD_H
#define UIX_PWD_H

#include "uix_types.h"

typedef struct uix_passwd {
    char      *pw_name;
    char      *pw_passwd;
    uix_uid_t  pw_uid;
    uix_gid_t  pw_gid;
    char      *pw_gecos;
    char      *pw_dir;
    char      *pw_shell;
} uix_passwd_t;

uix_passwd_t *uix_getpwuid  (uix_uid_t uid);
uix_passwd_t *uix_getpwnam  (const char *name);
int           uix_getpwuid_r(uix_uid_t uid, uix_passwd_t *pwd,
                              char *buf, uix_size_t buflen,
                              uix_passwd_t **result);
int           uix_getpwnam_r(const char *name, uix_passwd_t *pwd,
                              char *buf, uix_size_t buflen,
                              uix_passwd_t **result);
void          uix_setpwent  (void);
void          uix_endpwent  (void);
uix_passwd_t *uix_getpwent  (void);

#endif /* UIX_PWD_H */




#endif /* End of __UIX_PWD__H */
/* ***This is End of file, there is no more line should be added after this line*** */