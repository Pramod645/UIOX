
#ifndef __UIX_FNMATCH__H
#define __UIX_FNMATCH__H
/*
fnmatch.h header defines the filename pattern-matching API used to compare strings against shell-style 
wildcard patterns such as .txt, file?.c, or [a-z].

Unlike glob(), which expands patterns into matching file paths from the filesystem, 
fnmatch() only compares a pattern against a string. It does not read directories.

*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#define UIX_FNM_NOMATCH  1      // Return value when no match
#define UIX_FNM_NOESCAPE 0x01     // Backslash is not escape character
#define UIX_FNM_PATHNAME 0x02      // Slash only matches slash in pattern
#define UIX_FNM_PERIOD   0x04     // Leading dot must be matched explicitly
#define UIX_FNM_CASEFOLD 0x08     // Case-insensitive matching — glibc extension

int uix_fnmatch(const char *pattern, const char *string, int flags);  // Shell-style pattern matching — POSIX.2


#endif /* End of __UIX_FNMATCH__H */
/* ***This is End of file, there is no more line should be added after this line*** */
