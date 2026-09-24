/*
 *  30_KIX/32_FS/01_fsa/include/superblock.h
 *
 *  Super block, and the four allocation algorithms.
 *  Bach, The Design of the UNIX Operating System, Ch.4 §1, §5–§8.
 *
 *  ── CHANGED in this revision ───────────────────────────────────────────
 *  1. fs_alloc() is SPLIT into fs_alloc_begin() / fs_alloc_commit().
 *
 *     The one-shot form did two things at once: it detached the block
 *     from the free list AND zeroed + locked a buffer for it.  bmap_alloc()
 *     needs those apart, because the block number has to be attached to
 *     the inode's map BEFORE the buffer is dirtied — otherwise a failure
 *     in between leaves a block that is on no free list and named by no
 *     inode, i.e. leaked.  The split lets the caller order it correctly.
 *
 *     fs_alloc() is kept as the simple wrapper for callers that want the
 *     buffer in one step and will use it immediately.
 *
 *  2. Every allocator takes the DEVICE.
 *
 *     superblock.c keeps `static SuperBlock sb` for ONE filesystem.  The
 *     buffer layer below is multi-device, so the dev argument is threaded
 *     through and the accessors are indexed by it; a second mount gets
 *     its own super block rather than sharing the root's counters.
 *
 *  3. sb_get() is declared here.
 *
 *     superblock.c's `sb` is file-static, so no other unit can reach it.
 *     10_scfs's statfs needs the counters, and the named accessors in
 *     sb_access.c read through this one accessor.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#ifndef UIOX_SUPERBLOCK_H
#define UIOX_SUPERBLOCK_H

#include "fs_types.h"
#include "buffer.h"
#include "inode.h"

/* ═════════════════════════════════════════════════════════════════════
 * Super block
 *
 * One per mounted filesystem, indexed by device.  Bach's field names are
 * kept in the comments so the correspondence to the book is visible.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    uint32_t  fs_size;          /* Bach s_fsize — total blocks        */
    uint32_t  free_block_count; /* Bach s_nfree — free data blocks    */
    uint32_t  free_inode_count; /* Bach s_ninode — free inodes        */
    uint32_t  inode_start;      /* first block of the inode list      */
    uint32_t  data_start;       /* first data block                   */
    uint32_t  max_inodes;       /* total inodes in the filesystem     */

    /* Free-block list.  Bach: the FIRST entry points at a block that
     * itself holds the next group of free block numbers — the free list
     * is a linked list of block-number arrays, not a plain array. */
    uint32_t  free_blocks[SB_FREE_BLOCK_MAX];
    int       free_block_idx;   /* next slot to use / fill            */

    /* Free-inode cache.  Bach caches a run of free inode numbers so a
     * new file usually needs no disk search. */
    uint32_t  free_inodes[SB_FREE_INODE_MAX];
    int       free_inode_idx;
    uint32_t  remembered_inode; /* lowest known free inode on disk    */

    bool      locked;
    bool      modified;
} SuperBlock;

/* ═════════════════════════════════════════════════════════════════════
 * Super block API
 * ═════════════════════════════════════════════════════════════════════ */
void          sb_init(uint8_t dev);

/* The accessor 10_scfs's statfs needs — sb is file-static otherwise. */
SuperBlock   *sb_get(uint8_t dev);

/* ═════════════════════════════════════════════════════════════════════
 * Algorithm alloc  (§5) — file system block allocation
 *
 * Bach, verbatim:
 *   input: file system number
 *   output: buffer for a new block
 *   {
 *       while (super block locked)
 *           sleep(event super block not locked);
 *       remove block from super block free list;
 *       if (removed last block from free list)
 *       {
 *           lock super block;
 *           read block just taken from free list (algorithm bread);
 *           copy block numbers in block into super block;
 *           release block buffer (algorithm brelse);
 *           unlock super block;
 *           wakeup process(event super block not locked);
 *       }
 *       get buffer for block removed from super block list (getblk);
 *       zero buffer contents;
 *       decrement total count of free blocks;
 *       mark super block modified;
 *       return buffer;
 *   }
 *
 * The SPLIT form — preferred, because it lets the caller attach the block
 * number before the buffer is dirtied:
 *
 *      fs_alloc_begin(dev, &blkno)   → locked, zeroed BufHdr, or NULL
 *      ... attach blkno to the inode's map ...
 *      fs_alloc_commit(blkno)        → mark dirty, release
 * ═════════════════════════════════════════════════════════════════════ */
BufHdr       *fs_alloc_begin (uint8_t dev, uint32_t *blkno_out);
void          fs_alloc_commit_dev(uint8_t dev, uint32_t blkno);

/* The one-step form, for a caller that uses the buffer immediately. */
BufHdr       *fs_alloc(uint8_t dev);

/* ═════════════════════════════════════════════════════════════════════
 * Algorithm free  (§6) — return a block to the free pool
 * ═════════════════════════════════════════════════════════════════════ */
void          fs_free(uint8_t dev, uint32_t blkno);

/* ═════════════════════════════════════════════════════════════════════
 * Algorithm ialloc  (§7) — assign a free inode
 *
 * Bach, verbatim:
 *   input: file system
 *   output: locked inode
 *   {
 *       while (not done)
 *       {
 *           if (super block locked) { sleep; continue; }
 *           if (inode list in super block is empty)
 *           {
 *               lock super block;
 *               get remembered inode for free inode search;
 *               search disk for free inode until super block full, or no
 *                   more free inodes (algorithm bread and brelse);
 *               unlock super block;
 *               wakeup(event super block becomes free);
 *               if (no free inodes found on disk) return (no inode);
 *               set remembered inode for next free inode search;
 *           }
 *           get inode number from super block inode list;
 *           get inode (algorithm iget);
 *           if (inode not free after all)
 *           { write inode to disk; release inode (iput); continue; }
 *           initialize inode;
 *           write inode to disk;
 *           decrement file system free inode count;
 *           return (inode);
 *       }
 *   }
 * ═════════════════════════════════════════════════════════════════════ */
InCoreInode  *ialloc_dev(uint8_t dev, FileType ftype, uint16_t mode,
                         uint16_t uid, uint16_t gid);
InCoreInode  *ialloc(FileType ftype, uint16_t mode, uint16_t uid, uint16_t gid);

/* ═════════════════════════════════════════════════════════════════════
 * Algorithm ifree  (§8) — return an inode to the free pool
 *
 * Bach, verbatim:
 *   input: file system inode number
 *   output: none
 *   {
 *       increment file system free inode count;
 *       if (super block locked) return;
 *       if (inode list full)
 *       {
 *           if (inode number less than remembered inode for search)
 *               set remembered inode for search = input inode number;
 *       }
 *       else store inode number in inode list;
 *       return;
 *   }
 * ═════════════════════════════════════════════════════════════════════ */
void          ifree(uint32_t ino);

/* ═════════════════════════════════════════════════════════════════════
 * Free all data blocks belonging to an inode.
 * Called from iput when the link count reaches 0.
 * ═════════════════════════════════════════════════════════════════════ */
void          fs_free_inode_blocks(InCoreInode *ip);

/* Debug dump */
void          sb_print(uint8_t dev);

#endif /* UIOX_SUPERBLOCK_H */
