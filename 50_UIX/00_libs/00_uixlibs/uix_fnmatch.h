
#ifndef __FNMATCH__H
#define __FNMATCH__H
/*
fnmatch.h header defines the filename pattern-matching API used to compare strings against shell-style 
wildcard patterns such as .txt, file?.c, or [a-z].

Unlike glob(), which expands patterns into matching file paths from the filesystem, 
fnmatch() only compares a pattern against a string. It does not read directories.

*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Return value when string does not match pattern */
#define FNMNOMATCH 1

/* Flags controlling matching behavior */
#define FNMPATHNAME  0x01  // Slash must be matched explicitly /
#define FNMNOESCAPE  0x02  // Backslash loses special meaning /
#define FNMPERIOD    0x04  // Leading '.' must be matched explicitly /
#define FNMCASEFOLD  0x08  // Case-insensitive match (GNU extension) /

/* Function prototype */
int fnmatch(const char pattern, const char string, int flags);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



/* include/uix_fnmatch.h */
#ifndef UIX_FNMATCH_H
#define UIX_FNMATCH_H

#define UIX_FNM_NOMATCH  1
#define UIX_FNM_NOESCAPE 0x01
#define UIX_FNM_PATHNAME 0x02
#define UIX_FNM_PERIOD   0x04
#define UIX_FNM_CASEFOLD 0x08

int uix_fnmatch(const char *pattern, const char *string, int flags);

#endif /* UIX_FNMATCH_H */




#endif /* End of __FNMATCH__H */
/* ***This is End of file, there is no more line should be added after this line*** */