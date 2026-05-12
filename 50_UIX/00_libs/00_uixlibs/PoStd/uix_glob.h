
#ifndef __UIX_GLOB__H
#define __UIX_GLOB__H
/*
glob.h header defines the pathname pattern-matching (globbing) API in POSIX systems.  
It provides the functions glob() and globfree() that expand wildcard patterns 
like .c or /usr/include/ into actual file path lists — much like the shell does when you type such patterns on the 
command line.

*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#include "sys/uix_types.h"

#define UIX_GLOB_ERR      0x001     // Abort on read error
#define UIX_GLOB_MARK     0x002     // Append / to directory names
#define UIX_GLOB_NOSORT   0x004     // Don't sort results
#define UIX_GLOB_DOOFFS   0x008
#define UIX_GLOB_NOCHECK  0x010     // Return pattern if no match
#define UIX_GLOB_APPEND   0x020    // Append to previous glob() results
#define UIX_GLOB_NOESCAPE 0x040
#define UIX_GLOB_ALTDIRFUNC 0x080
#define UIX_GLOB_BRACE    0x100
#define UIX_GLOB_NOMAGIC  0x200
#define UIX_GLOB_TILDE    0x400    // Expand ~ to home directory — glibc extension

#define UIX_GLOB_NOSPACE  1
#define UIX_GLOB_ABORTED  2
#define UIX_GLOB_NOMATCH  3       // No matches found

typedef struct uix_glob {
    uix_size_t  gl_pathc;     // Number of matched paths
    char      **gl_pathv;      // Number of matched paths
    uix_size_t  gl_offs;
    int         gl_flags;
} uix_glob_t;

int  uix_glob  (const char *pattern, int flags,
                 int (*errfunc)(const char *path, int eerrno),
                 uix_glob_t *pglob);                          // Expands shell glob pattern to list of pathnames
void uix_globfree(uix_glob_t *pglob);                         // 


#endif /* End of __UIX_GLOB__H */
/* ***This is End of file, there is no more line should be added after this line*** */
