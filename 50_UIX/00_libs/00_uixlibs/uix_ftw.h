
#ifndef __FTW__H
#define __FTW__H
/*
ftw.h header defines the File Tree Walk (FTW) interface, which provides functions like ftw() and nftw() that recursively 
traverse directories and call a user-supplied callback function for each file or subdirectory encountered.  

It’s part of the POSIX standard library and is typically implemented as a wrapper around directory-reading 
functions (opendir, readdir, etc.).

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <sys/stat.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// File type indicators passed to the callback function /
#define FTWF   0   // Regular file /
#define FTWD   1   // Directory /
#define FTWDNR 2   // Directory not readable /
#define FTWNS  3   // File for which stat failed /

// Flags used with nftw() /
#define FTWPHYS  0x01  // Physical walk, do not follow symlinks /
#define FTWMOUNT 0x02  // Stay on the same filesystem /
#define FTWDEPTH 0x04  // Call function for directory after visiting contents /
#define FTWCHDIR 0x08  // Change to each directory before processing /

// Structure used by nftw() /
struct FTW {
    int base;    / Offset of filename part in path /
    int level;   / Depth from root of walk /
};

// Function prototypes /
int ftw(const char dirpath,
        int (fn)(const char fpath, const struct stat sb, int typeflag),
        int descriptors);

int nftw(const char dirpath,
         int (fn)(const char fpath, const struct stat sb, int typeflag, struct FTW ftwbuf),
         int descriptors, int flags);


#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



/* include/uix_ftw.h */
#ifndef UIX_FTW_H
#define UIX_FTW_H

#include "uix_stat.h"

#define UIX_FTW_F    0
#define UIX_FTW_D    1
#define UIX_FTW_DNR  2
#define UIX_FTW_NS   3
#define UIX_FTW_SL   4
#define UIX_FTW_DP   5
#define UIX_FTW_SLN  6

#define UIX_FTW_PHYS      1
#define UIX_FTW_MOUNT     2
#define UIX_FTW_CHDIR     4
#define UIX_FTW_DEPTH     8

typedef struct uix_FTW {
    int base;
    int level;
} uix_FTW_t;

typedef int (*uix_ftw_fn)(const char *path, const uix_stat_t *sb,
                           int typeflag);
typedef int (*uix_nftw_fn)(const char *path, const uix_stat_t *sb,
                            int typeflag, uix_FTW_t *ftwbuf);

int uix_ftw (const char *path, uix_ftw_fn fn, int nopenfd);
int uix_nftw(const char *path, uix_nftw_fn fn, int nopenfd, int flags);

#endif /* UIX_FTW_H */




#endif /* End of __FTW__H */
/* ***This is End of file, there is no more line should be added after this line*** */