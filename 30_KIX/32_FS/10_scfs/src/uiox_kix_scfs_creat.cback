/*
 *  30_KIX/32_FS/10_scfs/src/creat.c
 *
 *  Freestanding fixes (v1.1):
 *    FIXED: #include "../../33_PCS/..."  →  #include "uiox_klibc.h"
 *    FIXED: dirname()  →  fs_dirname() — inline freestanding implementation
 *    FIXED: fprintf(stderr, ...)  →  printf(...)
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include "uiox_klibc.h"

/* ─────────────────────────────────────────────────────────────
 * fs_dirname — freestanding replacement for POSIX dirname().
 *
 * Writes the parent directory portion of 'path' into 'out'
 * (which must be at least MAX_PATH_LEN bytes).
 * Returns 'out'.  Equivalent to:
 *   "/foo/bar"  → "/foo"
 *   "/foo"      → "/"
 *   "foo"       → "."
 *   "/"         → "/"
 * ───────────────────────────────────────────────────────────── */
static char *fs_dirname(const char *path, char *out, size_t outsz)
{
    size_t len;
    const char *last_slash;

    if (!path || path[0] == '\0') {
        strncpy(out, ".", outsz - 1);
        out[outsz - 1] = '\0';
        return out;
    }

    strncpy(out, path, outsz - 1);
    out[outsz - 1] = '\0';
    len = strlen(out);

    /* Strip trailing slashes */
    while (len > 1 && out[len - 1] == '/') {
        out[--len] = '\0';
    }

    /* Find last slash */
    last_slash = NULL;
    {
        const char *p = out;
        while (*p) {
            if (*p == '/') last_slash = p;
            p++;
        }
    }

    if (!last_slash) {
        /* No slash — parent is current directory */
        strncpy(out, ".", outsz - 1);
        out[outsz - 1] = '\0';
    } else if (last_slash == out) {
        /* Root */
        out[1] = '\0';
    } else {
        /* Truncate at last slash */
        out[last_slash - out] = '\0';
    }
    return out;
}

/*
 * Algorithm creat
 * input : file name, permission settings
 * output: file descriptor
 */
int fs_creat(const char *path, uint16_t mode)
{
    inode_t *ip;
    file_t  *fp;
    int      fd;
    int      existed = 0;

    /* Get inode for file name (algorithm namei) */
    ip = namei(path);

    if (ip) {
        /* File already exists */
        existed = 1;
        if (!iaccess(ip, 2)) {
            iput(ip);
            return FS_EACCES;
        }
    } else {
        /* File does not exist — create it */
        char     parent_buf[256];
        char    *parent_path;
        inode_t *parent_ip;

        fs_dirname(path, parent_buf, sizeof parent_buf);
        parent_path = parent_buf;

        parent_ip = namei(parent_path);
        if (!parent_ip) return FS_ENOENT;
        if (!iaccess(parent_ip, 2)) {
            iput(parent_ip);
            return FS_EACCES;
        }

        ip = ialloc(parent_ip->i_dev);
        if (!ip) { iput(parent_ip); return FS_ENFILE; }

        ip->i_mode  = (uint16_t)(IFREG | (mode & ~(uint16_t)u.u_umask));
        ip->i_nlink = 1;
        ip->i_uid   = u.u_uid;
        ip->i_gid   = u.u_gid;
        ip->i_flag |= IUPD;

        iput(parent_ip);
    }

    if (!existed)
        itrunc(ip);

    /* Allocate file table entry */
    fp = falloc();
    if (!fp) { iput(ip); return FS_ENFILE; }

    /* Allocate user file descriptor */
    fd = ufalloc();
    if (fd < 0) { f_close(fp); iput(ip); return FS_ENFILE; }

    fp->f_flag   = O_WRONLY;
    fp->f_count  = 1;
    fp->f_inode  = ip;
    fp->f_offset = 0;

    u.u_ofile.ufd_file[fd] = fp;

    printf("[creat] path='%s'  fd=%d  existed=%d\n", path, fd, existed);
    return fd;
}
