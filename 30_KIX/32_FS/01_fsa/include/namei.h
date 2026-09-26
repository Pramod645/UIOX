/*
 *  30_KIX/32_FS/01_fsa/include/namei.h
 *
 *  Algorithm namei, and the directory operations it uses.
 *  Bach, The Design of the UNIX Operating System, Ch.4 §4.
 *
 *  ── CHANGED in this revision (v3.1.0) ─────────────────────────────────
 *  DirEntry is GONE.  UNFS directory entries are VARIABLE-LENGTH, and the
 *  field layout below is taken VERBATIM from unfs_format.h's
 *  struct unfs_dirent — which is itself copied from the bootloader's
 *  uiox_boot_unfs.h, the source of truth:
 *
 *      offset  0   d_ino        uint32_t
 *      offset  4   d_rec_len    uint16_t   ← THIS RECORD'S OWN LENGTH
 *      offset  6   d_name_len   uint8_t
 *      offset  7   d_type       uint8_t    UNFS_DT_* values
 *      offset  8   d_name[]     char[d_name_len]
 *
 *  ── what an earlier revision got wrong ───────────────────────────────
 *  It declared the header as 6 bytes and the order as
 *  {d_ino, d_type, d_name_len}, and computed a record's length from
 *  d_name_len instead of reading d_rec_len.  On a real UNFS volume that
 *  writes d_type into the low byte of d_rec_len, so EVERY entry corrupts
 *  its own length — and the walk, which trusts d_rec_len, then jumps to
 *  the wrong offset.  The 8-byte header and the field ORDER are the fix.
 *
 *  ── why an entry's length is read, not computed ──────────────────────
 *  d_rec_len is stored on disk and is the walk's authority, because a
 *  record may be LONGER than its name needs: the last entry in a block
 *  absorbs the block's spare bytes, and a removed entry's slot keeps the
 *  length it was given.  Computing from d_name_len would re-derive a
 *  shorter number and re-open the space that was already absorbed.
 *  dirent_reclen() therefore reads the field, and only falls back to the
 *  padded 8 + d_name_len when d_rec_len reads as 0.
 *
 *  ── alignment ────────────────────────────────────────────────────────
 *  UNFS_DIRENT_ALIGN is 4.  The macro is defined here only when
 *  unfs_format.h does not supply one, so the format header remains the
 *  authority if it later adds it.
 *
 *  ── d_type is a THIRD encoding ───────────────────────────────────────
 *  There are three type encodings in this stack, and they are not
 *  interchangeable:
 *
 *      FileType enum        FT_DIR = 2          (in-core only, going away)
 *      UNFS_IF* in i_mode   UNFS_IFDIR = 0040000 (on disk, inode)
 *      UNFS_DT_* in d_type  UNFS_DT_DIR = 4      (on disk, directory)
 *
 *  dirent's d_type takes UNFS_DT_* — NOT UNFS_IF* and NOT FileType.
 *
 *  ── signatures kept ──────────────────────────────────────────────────
 *  dir_lookup / dir_add / dir_remove / namei keep their shapes.  dir_add
 *  now takes a UNFS_DT_* type rather than a FileType, which is the one
 *  parameter whose MEANING changed.
 *
 *  @version 3.1.0  @date 2026-09-26
 */
#ifndef UIOX_NAMEI_H
#define UIOX_NAMEI_H

#include "fs_types.h"       /* brings in unfs_format.h */
#include "inode.h"
#include "buffer.h"

/* ═════════════════════════════════════════════════════════════════════
 * Directory entry — UNFS's variable-length record
 *
 * The on-disk struct is declared in unfs_format.h so the bootloader and
 * the kernel share one definition; this aliases it under the name
 * 01_fsa's sources use.
 * ═════════════════════════════════════════════════════════════════════ */
typedef unfs_dirent_t DirEntry;

/* Bytes before d_name[]: d_ino(4) + d_rec_len(2) + d_name_len(1) +
 * d_type(1) = 8.  Derived from the struct, not a literal, so a field
 * added to unfs_dirent moves it automatically. */
#define DIRENT_HDR_SIZE   ((uint32_t)offsetof(unfs_dirent_t, d_name))

/* Entries are 4-byte aligned.  unfs_format.h does not define an align
 * macro, so it stands here until it does. */
#ifndef UNFS_DIRENT_ALIGN
#define UNFS_DIRENT_ALIGN 4u
#endif

/* The four inline extents in unfs_inode.  unfs_format.h declares
 * i_extents[4] as a literal, so the count is named here for the code
 * that loops over it. */
#ifndef UNFS_INLINE_EXTENTS
#define UNFS_INLINE_EXTENTS 4u
#endif

/* ─────────────────────────────────────────────────────────────
 * dirent_reclen — how far to advance past this entry.
 *
 * Reads d_rec_len, because the stored length is authoritative: the last
 * entry in a block owns the block's spare bytes and a freed entry keeps
 * the slot it was given.  Recomputing from d_name_len would re-derive a
 * shorter number and lose that space.
 *
 * Falls back to the padded minimum only if d_rec_len is 0 — a field that
 * was never written, which no entry the kernel creates should have.
 * ───────────────────────────────────────────────────────────── */
static inline uint32_t dirent_reclen(const DirEntry *de)
{
    uint32_t raw;
    uint32_t min;

    if (de->d_rec_len != 0u) {
        return (uint32_t)de->d_rec_len;
    }

    /* never written: the smallest record that could hold this name */
    raw = DIRENT_HDR_SIZE + (uint32_t)de->d_name_len;
    min = (raw + (UNFS_DIRENT_ALIGN - 1u))
          & ~(uint32_t)(UNFS_DIRENT_ALIGN - 1u);
    return min;
}

/* The smallest record that can hold a name of 'len' bytes */
static inline uint32_t dirent_needed(uint32_t len)
{
    uint32_t raw = DIRENT_HDR_SIZE + len;
    return (raw + (UNFS_DIRENT_ALIGN - 1u))
           & ~(uint32_t)(UNFS_DIRENT_ALIGN - 1u);
}

/* An entry the walk should stop at: d_name_len 0 marks the terminating
 * slot that fills the rest of a block.  A FREED entry is NOT a stop —
 * it has d_ino == 0 but keeps a real d_name_len, and the walk must step
 * over it to reach the entries after it. */
static inline bool dirent_valid(const DirEntry *de)
{
    return de->d_ino != 0u && de->d_name_len != 0u;
}

/* A slot that is free but still occupies space (a removed entry) */
static inline bool dirent_free(const DirEntry *de)
{
    return de->d_ino == 0u && de->d_name_len != 0u;
}

/* Advance to the next entry in the same block.
 *
 * Returns NULL when the step would run past 'end'.  The caller owns that
 * test — this cannot know the block boundary. */
static inline DirEntry *dirent_next(DirEntry *de, const uint8_t *end)
{
    uint8_t *p = (uint8_t *)de + dirent_reclen(de);
    if (p >= end) return NULL;
    return (DirEntry *)p;
}

/* Does this entry name the component 'name' of length 'len'? */
static inline bool dirent_match(const DirEntry *de,
                                const char *name, uint32_t len)
{
    uint32_t i;

    if (!dirent_valid(de)) return false;
    if ((uint32_t)de->d_name_len != len) return false;

    for (i = 0u; i < len; i++) {
        if (de->d_name[i] != name[i]) return false;
    }
    return true;
}

/* ═════════════════════════════════════════════════════════════════════
 * Directory operations
 *
 * Entry count per 4 KB block is not a constant: a block holds between one
 * long-named entry and (4096 / DIRENT_HDR_SIZE) minimal ones.
 * ═════════════════════════════════════════════════════════════════════ */
#define DIRENTS_PER_BLOCK_MAX  ((uint32_t)(UNFS_BLOCK_SIZE / (DIRENT_HDR_SIZE)))

/* Look up 'name' (of length 'len') in directory inode 'dir'.
 * Returns the inode number, or 0 if the component is not present. */
uint32_t dir_lookup(InCoreInode *dir, const char *name, uint32_t len);

/* Add 'name' -> 'ino' to directory 'dir'.
 *
 * 'type' is a UNFS_DT_* value (UNFS_DT_REG, UNFS_DT_DIR, …), NOT a
 * FileType and NOT a UNFS_IF* — d_type carries the DT encoding.
 *
 * Returns 0 on success, -1 when the directory cannot be extended. */
int      dir_add(InCoreInode *dir, const char *name, uint32_t len,
                 uint32_t ino, uint8_t type);

/* Remove 'name' from directory 'dir'.  The slot keeps its length so the
 * block stays walkable, and d_ino goes to 0 so it can be reused.
 * Returns 0 on success, -1 if the component is absent. */
int      dir_remove(InCoreInode *dir, const char *name, uint32_t len);

/* Algorithm namei — resolve a path to a locked inode.
 *
 * 'cwd' supplies BOTH the starting inode and the device the whole walk
 * stays on; an absolute path starts at the root on 'cwd's device.  The
 * caller must iput() the result. */
InCoreInode *namei(const char *path, InCoreInode *cwd,
                   uint16_t uid, uint16_t gid);

/* Build a filesystem image: format 'dev' and create root with "." and
 * ".." in it.  Returns 0 on success, -1 on failure. */
int      fs_mkfs(uint8_t dev);

#endif /* UIOX_NAMEI_H */
