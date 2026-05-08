
#ifndef __GLOB__H
#define __GLOB__H
/*
glob.h header defines the pathname pattern-matching (globbing) API in POSIX systems.  
It provides the functions glob() and globfree() that expand wildcard patterns 
like .c or /usr/include/ into actual file path lists — much like the shell does when you type such patterns on the 
command line.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <stddef.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Error codes returned by glob() /
#define GLOBNOSPACE 1  // Ran out of memory /
#define GLOBABORTED 2  // Read error or unexpected condition /
#define GLOBNOMATCH 3  // No matches found /

// Flags controlling behavior /
#define GLOBERR       0x01  // Stop on read errors /
#define GLOBMARK      0x02  // Append '/' to directory names /
#define GLOBNOSORT    0x04  // Do not sort the results /
#define GLOBDOOFFS    0x08  // Reserve gloffs slots at the beginning /
#define GLOBNOCHECK   0x10  // Return the pattern itself if no matches /
#define GLOBAPPEND    0x20  // Append to results of previous call /
#define GLOBNOESCAPE  0x40  // Disable backslash escaping /
#define GLOBPERIOD    0x80  // Match files beginning with '.' /

// Structure storing results of glob() /
typedef struct {
    sizet glpathc;  // Count of paths matched /
    char *glpathv;  // List of matched path strings /
    sizet gloffs;   // Slots to reserve at start of glpathv /
} globt;

// Function prototypes /
int glob(const char pattern, int flags,
         int (errfunc)(const char epath, int eerrno),
         globt pglob);

void globfree(globt pglob);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */


/* include/uix_glob.h */
#ifndef UIX_GLOB_H
#define UIX_GLOB_H

#include "uix_types.h"

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

#endif /* UIX_GLOB_H */




#endif /* End of __GLOB__H */
/* ***This is End of file, there is no more line should be added after this line*** */