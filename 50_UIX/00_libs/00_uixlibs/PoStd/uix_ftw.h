
#ifndef __UIX_FTW__H
#define __UIX_FTW__H
/*
ftw.h header defines the File Tree Walk (FTW) interface, which provides functions like ftw() and nftw() that recursively 
traverse directories and call a user-supplied callback function for each file or subdirectory encountered.  

It’s part of the POSIX standard library and is typically implemented as a wrapper around directory-reading 
functions (opendir, readdir, etc.).

*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#include "sys/uix_stat.h"

#define UIX_FTW_F    0       // Entry is regular file
#define UIX_FTW_D    1      // Entry is directory
#define UIX_FTW_DNR  2    // Directory that can't be read
#define UIX_FTW_NS   3     // File that stat() failed on
#define UIX_FTW_SL   4     // Symbolic link
#define UIX_FTW_DP   5     // Call function after directory contents
#define UIX_FTW_SLN  6

#define UIX_FTW_PHYS      1
#define UIX_FTW_MOUNT     2
#define UIX_FTW_CHDIR     4
#define UIX_FTW_DEPTH     8

typedef struct uix_FTW {
    int base;       // Offset of filename in path string
    int level;      /// Depth relative to start directory
} uix_FTW_t;

typedef int (*uix_ftw_fn)(const char *path, const uix_stat_t *sb,
                           int typeflag);
typedef int (*uix_nftw_fn)(const char *path, const uix_stat_t *sb,
                            int typeflag, uix_FTW_t *ftwbuf);

int uix_ftw (const char *path, uix_ftw_fn fn, int nopenfd);              // Walks directory tree, calls fn for each entry
int uix_nftw(const char *path, uix_nftw_fn fn, int nopenfd, int flags);  // Enhanced tree walk — POSIX.1-2001

//#endif /* UIX_FTW_H */

#endif /* End of __UIX_FTW__H */
/* ***This is End of file, there is no more line should be added after this line*** */
