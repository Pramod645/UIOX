
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

#endif /* End of __UIX_DIRENT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
