/*
 * 30_KIX/32_FS/src/uiox_fs_populate.c
 * First-boot populate: put the userspace image into the volume.
 *
 * Runs once, after the root is mounted and before the first exec.  The
 * volume mkfs produced holds a superblock, bitmaps and an empty root --
 * no /bin, no files -- so execve("/bin/uios") would return ENOENT.
 *
 * The bytes come from the embedded copy of uios (uios_blob.o, linked
 * into the kernel image), because there is no userspace yet to write
 * them with.
 */
#include "superblock.h"   /* ialloc_dev, iget, iput          */
#include "inode.h"        /* ialloc_dev, iupdate, InCoreInode  */
#include "namei.h"        /* dir_add, namei                    */
#include "readwrite.h"    /* writei                            */
#include "fs_types.h"     /* PERM_*                            */
#include "unfs_format.h"  /* UNFS_IF*, UNFS_DT_*, ROOT_DEV     */

extern const char _uios_blob_start[];
extern const char _uios_blob_end[];

#define UIOS_BIN_NAME  "uios"
#define UIOS_DIR_NAME  "bin"

int uiox_fs_populate_root(void)
{
    InCoreInode *root, *dir, *file;
    const char  *bytes;
    uint32_t     len, off;
    int32_t      n;

    root = iget(UNFS_ROOT_INO);
    if (!root) return -1;

    /* already populated: idempotent, so a second boot is a no-op */
    if (namei("/" UIOS_DIR_NAME "/" UIOS_BIN_NAME, root, 0u, 0u))
        return 0;

    /* ── /bin ─────────────────────────────────────────────────────── */
    dir = ialloc_dev(ROOT_DEV, UNFS_IFDIR,
                     PERM_UR | PERM_UW | PERM_UX, 0u, 0u);
    if (!dir) { iput(root); return -1; }
    dir->nlink = 2u;
    dir->flags |= IFLAG_CHANGED;
    if (dir_add(root, UIOS_DIR_NAME, 3u, dir->ino, UNFS_DT_DIR) != 0) {
        iput(dir); iput(root); return -1;
    }
    iupdate(dir);

    /* ── /bin/uios ────────────────────────────────────────────────── */
    file = ialloc_dev(ROOT_DEV, UNFS_IFREG,
                      PERM_UR | PERM_UW | PERM_UX, 0u, 0u);
    if (!file) { iput(dir); iput(root); return -1; }

    bytes = _uios_blob_start;
    len   = (uint32_t)(_uios_blob_end - _uios_blob_start);
    off   = 0u;

    while (off < len) {
        n = writei(file, bytes + off, len - off, &off);
        if (n <= 0) { iput(file); iput(dir); iput(root); return -1; }
    }
    iupdate(file);

    if (dir_add(dir, UIOS_BIN_NAME, 4u, file->ino, UNFS_DT_REG) != 0) {
        iput(file); iput(dir); iput(root); return -1;
    }

    iput(file); iput(dir); iput(root);
    return 0;
}
