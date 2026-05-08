
#ifndef __UIX_DIRENT__H
#define __UIX_DIRENT__H
/*
dirent.h header defines the directory entry API that allows C programs to open and read directories (i.e., list files).  
It provides functions such as opendir(), readdir(), closedir(), and rewinddir() — forming a higher-level abstraction over low-level filesystem calls like open() and read().

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>   // for inot, offt /
#include <stddef.h>      // for NULL /

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Directory stream type (opaque in real implementations) */
typedef struct dirstream DIR;

/* Directory entry structure */
struct dirent {
    inot          dino;       // Inode number /
    offt          doff;       // Offset to the next dirent /
    unsigned short dreclen;    // Length of this record /
    unsigned char  dtype;      // File type (DTREG, DTDIR, etc.) /
    char           dname[];    // Null-terminated filename /
};

/* File type macros (may not be supported on all systems) */
#define DTUNKNOWN  0
#define DTFIFO     1
#define DTCHR      2
#define DTDIR      4
#define DTBLK      6
#define DTREG      8
#define DTLNK      10
#define DTSOCK     12

/* Macros to test file types (dtype) */
#define IFTODT(mode) (((mode) & 0170000) >> 12)
#define DTTOIF(dirtype) ((dirtype) << 12)

/* Core directory functions */
DIR opendir(const char name);
struct dirent readdir(DIR dirp);
int closedir(DIR dirp);
void rewinddir(DIR dirp);

/* Additional POSIX functions */
int dirfd(DIR dirp);
long telldir(DIR dirp);
void seekdir(DIR dirp, long loc);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#ifndef UIX_DIRENT_H
#define UIX_DIRENT_H

#include "uix_types.h"

#define UIX_NAME_MAX 255
#define UIX_PATH_MAX 4096

#define UIX_DT_UNKNOWN 0
#define UIX_DT_FIFO    1
#define UIX_DT_CHR     2
#define UIX_DT_DIR     4         // Directory entry is directory
#define UIX_DT_BLK     6
#define UIX_DT_REG     8         // Directory entry is regular file
#define UIX_DT_LNK     10     // Directory entry is symlink
#define UIX_DT_SOCK    12

typedef struct uix_dirent {
    uix_ino_t      d_ino;    // Inode number of entry
    uix_off_t      d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[UIX_NAME_MAX + 1];  // Filename — up to NAME_MAX chars
} uix_dirent_t;

typedef struct uix_DIR {
    int          fd;
    char         buf[4096];
    int          buf_pos;
    int          buf_len;
    uix_off_t    offset;
    uix_dirent_t entry;
} uix_DIR;   // Opaque directory stream type

uix_DIR      *uix_opendir   (const char *name);  // Opens directory stream
uix_DIR      *uix_fdopendir (int fd);
uix_dirent_t *uix_readdir   (uix_DIR *dirp);    /// Returns next entry — NOT thread-safe
int           uix_readdir_r (uix_DIR *dirp, uix_dirent_t *entry,
                              uix_dirent_t **result); // Reentrant version — POSIX
int           uix_closedir  (uix_DIR *dirp);  // Closes directory stream
void          uix_rewinddir (uix_DIR *dirp);   // Resets stream to beginning
long          uix_telldir   (uix_DIR *dirp);   // Returns current stream position
void          uix_seekdir   (uix_DIR *dirp, long loc);  // Seeks to position from telldir()
int           uix_scandir   (const char *path, uix_dirent_t ***namelist,
                              int (*filter)(const uix_dirent_t *),
                              int (*compar)(const uix_dirent_t **,
                                            const uix_dirent_t **));// Reads entire directory into array, optionally sorted
int           uix_alphasort (const uix_dirent_t **a, const uix_dirent_t **b);// Comparator for alphabetic sort of scandir results

#endif /* UIX_DIRENT_H */


#endif /* End of __UIX_DIRENT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
