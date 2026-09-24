/*
 *  30_KIX/32_FS/01_fsa/src/sb_access.c
 *
 *  sb_get — the accessor 10_scfs needs, and the field-name shim.
 *
 *  ── why this is a separate file ─────────────────────────────────────
 *  superblock.c declares  `static SuperBlock sb;`  — file-static, so no
 *  other translation unit can reach it.  That is deliberate for the
 *  algorithms (only fs_alloc, fs_free, ialloc and ifree touch it), but
 *  the syscall layer needs to READ the counters for statfs.
 *
 *  Rather than break the encapsulation in superblock.c — which would put
 *  a non-static super block in reach of every file in the build — this
 *  unit asks superblock.c for the pointer through a single accessor.
 *  The change to superblock.c is two lines:
 *
 *      static SuperBlock sb;                 // unchanged
 *
 *      SuperBlock *sb_get(void) { return &sb; }   // add
 *
 *  Put that function at the bottom of superblock.c, next to sb_print().
 *  Everything else in this file is ready to compile against it.
 *
 *  ── the field-name shim ─────────────────────────────────────────────
 *  The syscall layer's statfs wants Bach's names (s_fsize, s_nfree,
 *  s_isize, s_ninode).  This filesystem uses its own (fs_size,
 *  free_block_count, max_inodes, free_inode_count).  Rather than make
 *  either side adopt the other's vocabulary, the mapping lives here in
 *  one place — named functions, so a reader can see the correspondence
 *  instead of guessing at it.
 *
 *  v1.0: written against fs_types.h, inode.h, superblock.h.
 */
#include "superblock.h"
#include "uiox_klibc.h"

/* ═════════════════════════════════════════════════════════════
 * The accessor — see the header note.  Declared here so this file
 * compiles on its own; the definition belongs in superblock.c.
 * ═════════════════════════════════════════════════════════════ */
#ifndef UIOX_SB_ACCESSOR_DEFINED
extern SuperBlock *sb_get(void);
#endif

/* ═════════════════════════════════════════════════════════════
 * Bach's statfs fields, named against this filesystem's own
 * ═════════════════════════════════════════════════════════════ */

/* Bach s_fsize — "the number of blocks in the file system" */
uint32_t sb_total_blocks(void)
{
    SuperBlock *sb = sb_get();
    return sb ? sb->fs_size : 0u;
}

/* Bach s_nfree — "the number of free blocks in the free list" */
uint32_t sb_free_blocks(void)
{
    SuperBlock *sb = sb_get();
    return sb ? sb->free_block_count : 0u;
}

/* Bach s_isize — the size of the inode list.  This filesystem stores
 * max_inodes rather than a block count, which is the number a caller
 * actually uses for the f_files field. */
uint32_t sb_total_inodes(void)
{
    SuperBlock *sb = sb_get();
    return sb ? sb->max_inodes : 0u;
}

/* Bach s_ninode — "the number of free inodes in the cached list" */
uint32_t sb_free_inodes(void)
{
    SuperBlock *sb = sb_get();
    return sb ? sb->free_inode_count : 0u;
}

/* The block size is a compile-time constant, not a super block field —
 * fs_types.h fixes it.  Exposed as a function so a caller does not have
 * to remember whether it is 512 or configurable. */
uint32_t sb_block_size(void)
{
    return (uint32_t)BLOCK_SIZE;
}

/*
 * sb_is_modified / sb_clear_modified
 *
 * Bach's super block carries a modified flag the sync path tests to
 * decide whether to write it back.  This filesystem has the flag; these
 * two give the layer above a way to read and clear it without reaching
 * into the struct.
 */
int sb_is_modified(void)
{
    SuperBlock *sb = sb_get();
    return (sb && sb->modified) ? 1 : 0;
}

void sb_clear_modified(void)
{
    SuperBlock *sb = sb_get();
    if (sb) sb->modified = false;
}
