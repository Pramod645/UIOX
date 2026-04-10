
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

#endif /* End of __GLOB__H */
/* ***This is End of file, there is no more line should be added after this line*** */