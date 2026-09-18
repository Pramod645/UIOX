/*
 * 30_KIX/32_FS/01_fsa/vfs.c
 *
 * UIOX Virtual File System implementation.
 *
 * This layer sits between sys_open/read/write/close (33_PCS)
 * and the concrete filesystem (10_scfs, 02_journal, 03_netfs).
 *
 * Data flow — kernel DRAM to userspace:
 *
 *   DRAM (physical page)
 *     ← owned by page cache (this file, inode->i_pages[])
 *     ← populated by scfs_read_block() from block device
 *
 *   vfs_read(file, kbuf, count)
 *     → calls file->f_ops->read(file, kbuf, count, &pos)
 *     → concrete FS copies block data into kbuf (kernel buffer)
 *     → returns to sys_read() in 33_PCS
 *
 *   sys_read(fd, ubuf, count) in 33_PCS
 *     → calls vfs_read() → gets data in kbuf
 *     → calls uiox_copy_to_user(ubuf, kbuf, n) ← CROSSING POINT
 *     → data now in user buffer
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "uiox_vfs.h"
 #include "uiox_soc_string.h"   /* memset, memcpy, strlen — freestanding */
 #include "uiox_soc_stdio.h"    /* early_puts for debug                  */
 
 /* ── Global tables ─────────────────────────────────────────────────── */
 
 /* Registered filesystem types */
 #define VFS_FS_MAX  8u
 static const uiox_fs_ops_t *s_fs_types[VFS_FS_MAX];
 static uint32_t              s_fs_count = 0u;
 
 /* Inode table — flat array, inodes never move */
 static uiox_inode_t  s_inodes [UIOX_INODE_MAX];
 static uint8_t       s_inode_used[UIOX_INODE_MAX];
 
 /* Dentry cache */
 static uiox_dentry_t s_dentries[UIOX_DENTRY_MAX];
 static uint8_t       s_dentry_used[UIOX_DENTRY_MAX];
 
 /* Open file table (kernel-global — per-process fd tables index this) */
 static uiox_file_t   s_files[UIOX_FD_MAX];
 static uint8_t       s_file_used[UIOX_FD_MAX];
 
 /* Mounted root superblock */
 static uiox_superblock_t s_root_sb;
 static uint8_t           s_mounted = 0u;
 
 /* ── Inode allocator ───────────────────────────────────────────────── */
 static uiox_inode_t *inode_alloc(void)
 {
     for (uint32_t i = 0u; i < UIOX_INODE_MAX; i++) {
         if (!s_inode_used[i]) {
             s_inode_used[i] = 1u;
             memset(&s_inodes[i], 0, sizeof(s_inodes[i]));
             s_inodes[i].i_ino = i + 1u;
             return &s_inodes[i];
         }
     }
     return (uiox_inode_t *)0;
 }
 
 static void inode_free(uiox_inode_t *inode)
 {
     if (!inode) return;
     uint32_t idx = (uint32_t)(inode - s_inodes);
     if (idx < UIOX_INODE_MAX) {
         s_inode_used[idx] = 0u;
         inode->i_inuse    = 0u;
     }
 }
 
 /* ── Dentry allocator ──────────────────────────────────────────────── */
 static uiox_dentry_t *dentry_alloc(void)
 {
     for (uint32_t i = 0u; i < UIOX_DENTRY_MAX; i++) {
         if (!s_dentry_used[i]) {
             s_dentry_used[i] = 1u;
             memset(&s_dentries[i], 0, sizeof(s_dentries[i]));
             return &s_dentries[i];
         }
     }
     return (uiox_dentry_t *)0;
 }
 
 /* ── File allocator ────────────────────────────────────────────────── */
 static uiox_file_t *file_alloc(void)
 {
     for (uint32_t i = 0u; i < UIOX_FD_MAX; i++) {
         if (!s_file_used[i]) {
             s_file_used[i] = 1u;
             memset(&s_files[i], 0, sizeof(s_files[i]));
             return &s_files[i];
         }
     }
     return (uiox_file_t *)0;
 }
 
 static void file_free(uiox_file_t *f)
 {
     if (!f) return;
     uint32_t idx = (uint32_t)(f - s_files);
     if (idx < UIOX_FD_MAX) {
         s_file_used[idx] = 0u;
         f->f_inuse = 0u;
     }
 }
 
 /* ── Path walker ───────────────────────────────────────────────────── */
 /*
  * path_lookup — resolve an absolute path to an inode.
  * Walks components one at a time via inode->i_ops->lookup().
  * Returns 0 and sets *out_inode on success.
  */
 static int path_lookup(const char *path, uiox_inode_t **out_inode)
 {
     if (!path || path[0] != '/' || !out_inode)
         return UIOX_FS_EINVAL;
     if (!s_mounted)
         return UIOX_FS_ENOENT;
 
     uiox_inode_t *cur = s_root_sb.s_root;
     if (!cur) return UIOX_FS_ENOENT;
 
     /* Skip leading slash */
     const char *p = path + 1;
 
     while (*p != '\0') {
         /* Extract next component */
         char component[UIOX_NAME_MAX + 1u];
         uint32_t len = 0u;
         while (p[len] != '/' && p[len] != '\0' &&
                len < UIOX_NAME_MAX) {
             component[len] = p[len];
             len++;
         }
         component[len] = '\0';
         p += len;
        /* Skip trailing slash */
        if (*p == '/') p++;

        /* '.' means current directory — skip */
        if (component[0] == '.' && component[1] == '\0')
            continue;

        /* Lookup component in current directory */
        if (!cur->i_ops || !cur->i_ops->lookup)
            return UIOX_FS_ENOTDIR;

        uiox_inode_t *next = (uiox_inode_t *)0;
        int rc = cur->i_ops->lookup(cur, component, &next);
        if (rc != 0) return rc;
        cur = next;
    }

    *out_inode = cur;
    return UIOX_FS_OK;
}

/* ── vfs_init ──────────────────────────────────────────────────────── */
void vfs_init(void)
{
    memset(s_inodes,      0, sizeof(s_inodes));
    memset(s_inode_used,  0, sizeof(s_inode_used));
    memset(s_dentries,    0, sizeof(s_dentries));
    memset(s_dentry_used, 0, sizeof(s_dentry_used));
    memset(s_files,       0, sizeof(s_files));
    memset(s_file_used,   0, sizeof(s_file_used));
    memset(s_fs_types,    0, sizeof(s_fs_types));
    memset(&s_root_sb,    0, sizeof(s_root_sb));
    s_fs_count = 0u;
    s_mounted  = 0u;
}

/* ── vfs_register_fs ───────────────────────────────────────────────── */
int vfs_register_fs(const uiox_fs_ops_t *ops)
{
    if (!ops || !ops->name)         return UIOX_FS_EINVAL;
    if (s_fs_count >= VFS_FS_MAX)   return UIOX_FS_ENOSPC;
    s_fs_types[s_fs_count++] = ops;
    return UIOX_FS_OK;
}

/* ── vfs_mount_root ────────────────────────────────────────────────── */
int vfs_mount_root(void)
{
    /* Find registered filesystem — use first one (SCFS) */
    if (s_fs_count == 0u) {
        early_puts("[vfs] vfs_mount_root: no filesystem registered\n");
        return UIOX_FS_ENOENT;
    }

    memset(&s_root_sb, 0, sizeof(s_root_sb));
    s_root_sb.s_dev     = 0u;           /* boot device */
    s_root_sb.s_fsops   = s_fs_types[0];

    /* Call filesystem's mount — fills s_root_sb.s_root */
    int rc = s_root_sb.s_fsops->mount(&s_root_sb, 0u);
    if (rc != 0) {
        early_puts("[vfs] vfs_mount_root: mount failed\n");
        return rc;
    }

    if (!s_root_sb.s_root) {
        early_puts("[vfs] vfs_mount_root: no root inode\n");
        return UIOX_FS_ENOENT;
    }

    s_mounted = 1u;
    early_puts("[vfs] root mounted\n");
    return UIOX_FS_OK;
}

/* ── vfs_open ──────────────────────────────────────────────────────── */
/*
 * Called by sys_open() in 33_PCS after copy_from_user(path).
 * Returns a uiox_file_t* which is stored in the process fd table.
 */
int vfs_open(const char *path, uint32_t flags,
             uint32_t mode, uiox_file_t **out)
{
    if (!path || !out) return UIOX_FS_EINVAL;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    int rc = path_lookup(path, &inode);

    if (rc == UIOX_FS_ENOENT && (flags & UIOX_O_CREAT)) {
        /*
         * File does not exist — create it.
         * Find parent directory and create entry there.
         */
        /* find last slash */
        uint32_t plen = 0u;
        const char *p = path;
        while (*p) { plen++; p++; }
        while (plen > 0u && path[plen-1u] != '/') plen--;

        char parent_path[UIOX_PATH_MAX];
        if (plen == 0u) {
            parent_path[0] = '/';
            parent_path[1] = '\0';
        } else {
            if (plen >= UIOX_PATH_MAX) return UIOX_FS_EINVAL;
            memcpy(parent_path, path, plen);
            parent_path[plen] = '\0';
        }

        uiox_inode_t *parent = (uiox_inode_t *)0;
        rc = path_lookup(parent_path, &parent);
        if (rc != 0)                            return rc;
        if (!parent->i_ops || !parent->i_ops->create)
            return UIOX_FS_ENOSYS;

        const char *name = path + plen;
        rc = parent->i_ops->create(parent, name, mode, &inode);
        if (rc != 0) return rc;

    } else if (rc != 0) {
        return rc;
    }

    if (!inode) return UIOX_FS_ENOENT;

    /* Allocate open file struct */
    uiox_file_t *file = file_alloc();
    if (!file) return UIOX_FS_ENOMEM;

    file->f_inode  = inode;
    file->f_flags  = flags;
    file->f_mode   = mode;
    file->f_pos    = (flags & UIOX_O_APPEND) ? inode->i_size : 0u;
    file->f_ops    = inode->i_fops;
    file->f_inuse  = 1u;

    /* Truncate if O_TRUNC */
    if ((flags & UIOX_O_TRUNC) &&
        inode->i_ops && inode->i_ops->truncate) {
        inode->i_ops->truncate(inode, 0u);
    }

    /* Call filesystem open hook if present */
    if (file->f_ops && file->f_ops->open) {
        rc = file->f_ops->open(inode, file);
        if (rc != 0) {
            file_free(file);
            return rc;
        }
    }

    *out = file;
    return UIOX_FS_OK;
}

/* ── vfs_close ─────────────────────────────────────────────────────── */
int vfs_close(uiox_file_t *file)
{
    if (!file || !file->f_inuse) return UIOX_FS_EBADF;

    if (file->f_ops && file->f_ops->close)
        file->f_ops->close(file->f_inode, file);

    file_free(file);
    return UIOX_FS_OK;
}

/* ── vfs_read ──────────────────────────────────────────────────────── */
/*
 * Reads file data into a KERNEL buffer (kbuf).
 * The caller (sys_read in 33_PCS) then calls
 * uiox_copy_to_user(ubuf, kbuf, n) to cross to userspace.
 *
 * This is the key function in the DRAM→userspace data path:
 *
 *   Physical DRAM page (in page cache)
 *     → concrete FS read (scfs_read / netfs_read)
 *     → kbuf  (kernel stack / kmalloc'd buffer)
 *     → [back in 33_PCS] copy_to_user(ubuf, kbuf, n)
 *     → user buffer  ← data arrives here
 */
ssize_t vfs_read(uiox_file_t *file, void *kbuf, size_t count)
{
    if (!file || !file->f_inuse)  return (ssize_t)UIOX_FS_EBADF;
    if (!kbuf || count == 0u)     return 0;
    if (file->f_flags == UIOX_O_WRONLY)
        return (ssize_t)UIOX_FS_EACCES;
    if (!file->f_ops || !file->f_ops->read)
        return (ssize_t)UIOX_FS_ENOSYS;

    return file->f_ops->read(file, kbuf, count, &file->f_pos);
}

/* ── vfs_write ─────────────────────────────────────────────────────── */
/*
 * Writes from a KERNEL buffer (kbuf) into the file.
 * The caller (sys_write in 33_PCS) called
 * uiox_copy_from_user(kbuf, ubuf, n) first.
 *
 * Data flow:
 *   user buffer
 *     → [in 33_PCS] copy_from_user(kbuf, ubuf, n)
 *     → kbuf  (kernel buffer)
 *     → vfs_write(file, kbuf, n)
 *     → concrete FS write → page cache → DRAM
 *     → [later] journal commit → block device
 */
ssize_t vfs_write(uiox_file_t *file, const void *kbuf, size_t count)
{
    if (!file || !file->f_inuse)            return (ssize_t)UIOX_FS_EBADF;
    if (!kbuf || count == 0u)               return 0;
    if (file->f_flags == UIOX_O_RDONLY)
        return (ssize_t)UIOX_FS_EACCES;
    if (!file->f_ops || !file->f_ops->write)
        return (ssize_t)UIOX_FS_ENOSYS;

    return file->f_ops->write(file, kbuf, count, &file->f_pos);
}

/* ── vfs_seek ──────────────────────────────────────────────────────── */
int vfs_seek(uiox_file_t *file, int64_t offset, int whence)
{
    if (!file || !file->f_inuse) return UIOX_FS_EBADF;

    uint64_t new_pos;
    switch (whence) {
    case UIOX_SEEK_SET:
        if (offset < 0) return UIOX_FS_EINVAL;
        new_pos = (uint64_t)offset;
        break;
    case UIOX_SEEK_CUR:
        new_pos = (uint64_t)((int64_t)file->f_pos + offset);
        break;
    case UIOX_SEEK_END:
        new_pos = (uint64_t)((int64_t)file->f_inode->i_size + offset);
        break;
    default:
        return UIOX_FS_EINVAL;
    }

    file->f_pos = new_pos;
    return UIOX_FS_OK;
}

/* ── vfs_stat ──────────────────────────────────────────────────────── */
int vfs_stat(const char *path, uiox_stat_t *out)
{
    if (!path || !out) return UIOX_FS_EINVAL;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    int rc = path_lookup(path, &inode);
    if (rc != 0) return rc;
    if (!inode->i_ops || !inode->i_ops->stat)
        return UIOX_FS_ENOSYS;

    return inode->i_ops->stat(inode, out);
}

/* ── vfs_mkdir ─────────────────────────────────────────────────────── */
int vfs_mkdir(const char *path, uint32_t mode)
{
    if (!path) return UIOX_FS_EINVAL;

    /* Find parent */
    uint32_t plen = 0u;
    const char *p = path;
    while (*p) { plen++; p++; }
    while (plen > 0u && path[plen-1u] != '/') plen--;

    char parent_path[UIOX_PATH_MAX];
    if (plen == 0u) {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    } else {
        memcpy(parent_path, path, plen);
        parent_path[plen] = '\0';
    }

    uiox_inode_t *parent = (uiox_inode_t *)0;
    int rc = path_lookup(parent_path, &parent);
    if (rc != 0) return rc;
    if (!parent->i_ops || !parent->i_ops->mkdir)
        return UIOX_FS_ENOSYS;

    return parent->i_ops->mkdir(parent, path + plen, mode);
}

/* ── vfs_unlink ────────────────────────────────────────────────────── */
int vfs_unlink(const char *path)
{
    if (!path) return UIOX_FS_EINVAL;

    uint32_t plen = 0u;
    const char *p = path;
    while (*p) { plen++; p++; }
    while (plen > 0u && path[plen-1u] != '/') plen--;

    char parent_path[UIOX_PATH_MAX];
    if (plen == 0u) {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    } else {
        memcpy(parent_path, path, plen);
        parent_path[plen] = '\0';
    }

    uiox_inode_t *parent = (uiox_inode_t *)0;
    int rc = path_lookup(parent_path, &parent);
    if (rc != 0) return rc;
    if (!parent->i_ops || !parent->i_ops->unlink)
        return UIOX_FS_ENOSYS;

    return parent->i_ops->unlink(parent, path + plen);
}

/* ── vfs_readdir ───────────────────────────────────────────────────── */
int vfs_readdir(uiox_file_t *dir, uiox_dirent_t *de, uint32_t idx)
{
    if (!dir || !dir->f_inuse) return UIOX_FS_EBADF;
    if (!de)                   return UIOX_FS_EINVAL;
    if (!dir->f_ops || !dir->f_ops->readdir)
        return UIOX_FS_ENOSYS;

    return dir->f_ops->readdir(dir, de, idx);
}

/* ── vfs_fsync ─────────────────────────────────────────────────────── */
int vfs_fsync(uiox_file_t *file)
{
    if (!file || !file->f_inuse) return UIOX_FS_EBADF;
    if (!file->f_ops || !file->f_ops->fsync)
        return UIOX_FS_OK;   /* no-op if not implemented */

    return file->f_ops->fsync(file);
}

/* ── vfs_mmap_page ─────────────────────────────────────────────────── */
/*
 * Returns the physical address of the DRAM page at offset 'off'.
 * Used by 33_PCS/uiox_mmap.c for zero-copy mmap:
 *   no copy_to_user needed — PTE inserted directly into user PT.
 */
uintptr_t vfs_mmap_page(uiox_file_t *file, uint64_t off)
{
    if (!file || !file->f_inuse)      return 0u;
    if (!file->f_ops || !file->f_ops->mmap_page)
        return 0u;

    return file->f_ops->mmap_page(file, off);
}
 