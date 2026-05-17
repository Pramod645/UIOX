#ifndef UIOX_SUPERBLOCK_H
#define UIOX_SUPERBLOCK_H

#include "fs_types.h"
#include "buffer.h"
#include "inode.h"

/* ─────────────────────────────────────────────────────────────
 * On-disk superblock
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint32_t  fs_size;          /* total blocks in filesystem       */
    uint32_t  free_block_count; /* total free data blocks           */
    uint32_t  free_inode_count; /* total free inodes                */
    uint32_t  inode_start;      /* first block of inode list        */
    uint32_t  data_start;       /* first data block                 */
    uint32_t  max_inodes;       /* total inodes in filesystem       */

    /* Free-block list: the first entry points to a block that
     * itself contains the next group of free block numbers.    */
    uint32_t  free_blocks[SB_FREE_BLOCK_MAX];
    int       free_block_idx;   /* next slot to use/fill            */

    /* Free-inode cache list */
    uint32_t  free_inodes[SB_FREE_INODE_MAX];
    int       free_inode_idx;   /* number of valid entries          */
    uint32_t  remembered_inode; /* lowest known free inode on disk  */

    bool      locked;
    bool      modified;
} SuperBlock;

/* ─────────────────────────────────────────────────────────────
 * Superblock API
 * ───────────────────────────────────────────────────────────── */
void          sb_init(void);
SuperBlock   *sb_get(void);

/*
 * Algorithm alloc  (§5)
 * Allocate one free data block.
 * Returns a zeroed, locked BufEntry for the new block, or NULL.
 */
BufEntry     *fs_alloc(void);

/*
 * Algorithm free  (§6)
 * Return block 'blkno' to the free pool.
 */
void          fs_free(uint32_t blkno);

/*
 * Algorithm ialloc  (§7)
 * Assign a free inode for a new file.
 * Returns a locked, initialised InCoreInode, or NULL.
 */
InCoreInode  *ialloc(FileType ftype, uint16_t mode,
                      uint16_t uid, uint16_t gid);

/*
 * Algorithm ifree  (§8)
 * Return inode 'ino' to the free pool.
 */
void          ifree(uint32_t ino);

/*
 * Free all data blocks belonging to an inode
 * (called from iput when nlink == 0).
 */
void          fs_free_inode_blocks(InCoreInode *ip);

/* Dump superblock state (debug) */
void          sb_print(void);

#endif /* UIOX_SUPERBLOCK_H */
