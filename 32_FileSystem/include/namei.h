#ifndef UIOX_NAMEI_H
#define UIOX_NAMEI_H

#include "inode.h"

/* ─────────────────────────────────────────────────────────────
 * Directory entry  (stored in directory data blocks)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint32_t ino;
    char     name[MAX_NAME_LEN];
} DirEntry;

/* ─────────────────────────────────────────────────────────────
 * Filesystem root inode number
 * ───────────────────────────────────────────────────────────── */
#define ROOT_INO 1

/* ─────────────────────────────────────────────────────────────
 * namei API
 * ───────────────────────────────────────────────────────────── */

/*
 * Algorithm namei  (§4)
 *
 * Convert a path string to a locked InCoreInode.
 * Absolute paths start with '/'; relative paths start from cwd.
 *
 * Parameters:
 *   path   — null-terminated path string
 *   cwd    — current-working-directory inode (used for relative paths)
 *   uid, gid — credentials for permission checks
 *
 * Returns a locked InCoreInode, or NULL if not found / no permission.
 */
InCoreInode *namei(const char *path, InCoreInode *cwd,
                   uint16_t uid, uint16_t gid);

/*
 * dir_lookup — search directory 'dir' for component 'name'.
 * Returns the inode number of the match, or 0 if not found.
 */
uint32_t    dir_lookup(InCoreInode *dir, const char *name);

/*
 * dir_add — add a (name, ino) entry to directory 'dir'.
 * Returns 0 on success, -1 on failure.
 */
int         dir_add(InCoreInode *dir, const char *name, uint32_t ino);

/*
 * dir_remove — remove the entry with 'name' from directory 'dir'.
 * Returns 0 on success, -1 if not found.
 */
int         dir_remove(InCoreInode *dir, const char *name);

/* Initialise the root directory (called once by fs_mkfs) */
void        fs_mkfs(void);

#endif /* UIOX_NAMEI_H */
