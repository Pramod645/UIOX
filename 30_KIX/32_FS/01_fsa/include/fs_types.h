/*
 *  30_KIX/32_FS/01_fsa/include/fs_types.h
 *
 *  Freestanding FS type definitions — no system headers.
 *  v1.1: added #include "uiox_klibc.h" so bool/uint*_t/NULL flow
 *        into every file that includes fs_types.h transitively.
 */
#ifndef UIOX_FS_TYPES_H
#define UIOX_FS_TYPES_H

#include "uiox_klibc.h"   /* replaces <stdint.h> <stdbool.h> <stddef.h> */

/* ── Filesystem geometry ────────────────────────────────────── */
#define BLOCK_SIZE          512     /* bytes per disk block             */
#define MAX_BLOCKS          1024    /* total simulated disk blocks      */
#define MAX_INODES          128     /* total inodes on disk             */
#define INODES_PER_BLOCK    (BLOCK_SIZE / sizeof(DiskInode))
#define INODE_START_BLOCK   2       /* block where inode list begins    */
#define DATA_START_BLOCK    16      /* first data block                 */
#define MAX_INCACHE         32      /* in-core inode cache size         */
#define MAX_BUFS            64      /* buffer cache size                */
#define MAX_DIR_ENTRIES     16      /* entries per directory block      */
#define MAX_NAME_LEN        28      /* max filename component length    */
#define MAX_PATH_LEN        256     /* max full path length             */

/* ── Inode block address layout ────────────────────────────── */
#define NDIRECT     10              /* direct block pointers            */
#define NINDIRECT   1               /* single-indirect pointer          */
#define NDINDIRECT  1               /* double-indirect pointer          */
#define NTINDIRECT  1               /* triple-indirect pointer          */
#define PTRS_PER_BLOCK (BLOCK_SIZE / sizeof(uint32_t))

/* ── File types ─────────────────────────────────────────────── */
typedef enum {
    FT_FREE    = 0,
    FT_REGULAR = 1,
    FT_DIR     = 2,
    FT_CHAR    = 3,
    FT_BLOCK   = 4,
    FT_FIFO    = 5,
    FT_SYMLINK = 6
} FileType;

/* ── Permission bits ──────────────────────────────────────── */
#define PERM_UR  0400
#define PERM_UW  0200
#define PERM_UX  0100
#define PERM_GR  0040
#define PERM_GW  0020
#define PERM_GX  0010
#define PERM_OR  0004
#define PERM_OW  0002
#define PERM_OX  0001

/* ── Inode dirty flags ────────────────────────────────────── */
#define IFLAG_ACCESSED  0x01
#define IFLAG_CHANGED   0x02   /* inode metadata changed             */
#define IFLAG_MODIFIED  0x04   /* file data modified                 */

/* ── Superblock free-inode list size ─────────────────────── */
#define SB_FREE_INODE_MAX   32
#define SB_FREE_BLOCK_MAX   50

#endif /* UIOX_FS_TYPES_H */
