/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_table.c
 *
 * SCFS — the three kernel data structures, and the helpers that manage
 * them.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach names exactly three, for this layer ───────────────────────────
 *   1. the FILE TABLE
 *      "one entry allocated for every opened file in the system"
 *      scfs_file_table[NFILE]
 *
 *   2. the USER FILE DESCRIPTOR TABLE
 *      "one entry allocated for every file descriptor known to a process"
 *      scfs_u->ufd_file[NOFILE]   (one per process)
 *
 *   3. the MOUNT TABLE
 *      "containing information for every active file system"
 *      scfs_mount_table[NMOUNT]
 *
 * Bach's two allocation steps are kept separate here, as he keeps them:
 *   scfs_falloc()     allocate a FILE TABLE entry, set count and offset
 *   scfs_ufd_alloc()  allocate a USER FD slot pointing at it
 * dup() and pipe() need them apart — dup() links a second descriptor to an
 * existing entry, pipe() allocates two entries over one inode.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

/* ═════════════════════════════════════════════════════════════════════
 * STRUCTURE 1 — the file table
 * ═════════════════════════════════════════════════════════════════════ */
scfs_file_t scfs_file_table[NFILE];

/* ═════════════════════════════════════════════════════════════════════
 * STRUCTURE 2 — the user file descriptor table
 *
 * One per process.  33_PCS rebinds scfs_u to the process structure with
 * scfs_ufdt_bind(); the static instance keeps single-process bring-up
 * working before 33_PCS links.
 * ═════════════════════════════════════════════════════════════════════ */
scfs_ufdt_t  scfs_u_default;
scfs_ufdt_t *scfs_u = &scfs_u_default;

void scfs_ufdt_bind(scfs_ufdt_t *t) { scfs_u = t ? t : &scfs_u_default; }

/* ═════════════════════════════════════════════════════════════════════
 * STRUCTURE 3 — the mount table
 * ═════════════════════════════════════════════════════════════════════ */
scfs_mount_t scfs_mount_table[NMOUNT];

/* ═════════════════════════════════════════════════════════════════════
 * Process state — cwd, root, umask  (Bach's u area)
 *
 * 33_PCS owns these in a multi-process kernel.  The statics here are the
 * bring-up instance, so the tree resolves before 33_PCS links.
 * ═════════════════════════════════════════════════════════════════════ */
static InCoreInode *s_cwd  = (InCoreInode *)0;
static InCoreInode *s_root = (InCoreInode *)0;
static uint16_t     s_umask = 0u;

InCoreInode *scfs_cwd_get (void) { return s_cwd;  }
InCoreInode *scfs_root_get(void) { return s_root; }

void scfs_cwd_set (InCoreInode *ip) { s_cwd  = ip; }
void scfs_root_set(InCoreInode *ip) { s_root = ip; }

uint16_t scfs_umask_get(void)             { return s_umask; }
uint16_t scfs_umask_set(uint16_t mask)
{
    uint16_t old = s_umask;
    s_umask = (uint16_t)(mask & 0777u);
    return old;
}

/* ═════════════════════════════════════════════════════════════════════
 * scfs_getf — the hinge
 *
 * descriptor number -> file table entry, or NULL.  Four checks, cheapest
 * first.  Returning NULL for all of them is deliberate: the caller
 * reports EBADF, and no stale pointer can escape.
 * ═════════════════════════════════════════════════════════════════════ */
scfs_file_t *scfs_getf(int fd)
{
    if (fd < 0 || fd >= NOFILE) return (scfs_file_t *)0;
    if (!scfs_u)                return (scfs_file_t *)0;

    scfs_file_t *f = scfs_u->ufd_file[fd];
    if (!f || !f->f_inuse)      return (scfs_file_t *)0;
    if (!f->f_inode)            return (scfs_file_t *)0;  /* half-torn */

    return f;
}

/* ═════════════════════════════════════════════════════════════════════
 * scfs_falloc — STEP 1 of Bach's two: a FILE TABLE entry
 *
 * Sets the reference count to 1 and the offset to 0 — except for an
 * append-mode open, which starts at the file's end.
 * ═════════════════════════════════════════════════════════════════════ */
static uint16_t scfs_flag_from(int flags)
{
    uint16_t m;

    switch (flags & 0x3) {
    case O_RDONLY: m = FREAD;          break;
    case O_WRONLY: m = FWRITE;         break;
    default:       m = FREAD | FWRITE; break;
    }
    if (flags & O_APPEND) m |= FAPPEND;
    return m;
}

scfs_file_t *scfs_falloc(InCoreInode *ip, int flags, uint32_t perm_mask)
{
    (void)perm_mask;
    if (!ip) return (scfs_file_t *)0;

    for (uint32_t i = 0u; i < NFILE; i++) {
        scfs_file_t *f = &scfs_file_table[i];
        if (f->f_inuse) continue;

        f->f_inode  = ip;
        f->f_count  = 1;
        f->f_flag   = scfs_flag_from(flags);
        f->f_offset = (flags & O_APPEND) ? ip->size : 0u;
        f->f_inuse  = 1u;
        f->f_pad    = 0u;

        /* The inode now holds a file table reference, so its own count
         * must rise — otherwise close() would free it while open. */
        ip->refcount++;

        return f;
    }
    return (scfs_file_t *)0;      /* ENFILE */
}

/*
 * scfs_fclose_entry — undo a falloc.  Used when a later step fails, and
 * by close() itself via scfs_fput().
 */
void scfs_fclose_entry(scfs_file_t *f)
{
    if (!f) return;

    if (f->f_inode) {
        iput(f->f_inode);         /* 01_fsa: release the inode ref */
        f->f_inode = (InCoreInode *)0;
    }
    f->f_inuse  = 0u;
    f->f_count  = 0u;
    f->f_offset = 0u;
    f->f_flag   = 0u;
}

/* scfs_fput — release one reference; free the entry at zero. */
void scfs_fput(scfs_file_t *f)
{
    if (!f || !f->f_inuse) return;

    if (f->f_count > 0u) f->f_count--;
    if (f->f_count == 0u) scfs_fclose_entry(f);
}

/* ═════════════════════════════════════════════════════════════════════
 * scfs_ufd_alloc — STEP 2: a slot in the USER FD TABLE
 *
 * The slot index IS the descriptor number.
 * ═════════════════════════════════════════════════════════════════════ */
int scfs_ufd_find(void)
{
    if (!scfs_u) return SCFS_EMFILE;

    for (int i = 0; i < NOFILE; i++)
        if (!scfs_u->ufd_file[i]) return i;

    return SCFS_EMFILE;
}

int scfs_ufd_alloc(scfs_file_t *f)
{
    if (!f) return SCFS_EINVAL;

    int fd = scfs_ufd_find();
    if (fd < 0) return fd;

    scfs_u->ufd_file[fd] = f;
    return fd;
}

/* scfs_ufd_link — dup(): a second descriptor on the SAME entry.  The
 * reference rises, so close() on one does not release the other. */
int scfs_ufd_link(scfs_file_t *f)
{
    if (!f) return SCFS_EINVAL;

    int fd = scfs_ufd_find();
    if (fd < 0) return fd;

    f->f_count++;
    scfs_u->ufd_file[fd] = f;
    return fd;
}

/* ═════════════════════════════════════════════════════════════════════
 * Path splitting
 *
 *   "/a/b/c" -> parent "/a/b",  name "c"
 *   "/a"     -> parent "/",     name "a"
 * ═════════════════════════════════════════════════════════════════════ */
int scfs_path_split(const char *path, char *parent, uint32_t sz,
                    const char **name)
{
    if (!path || !parent || !name) return SCFS_EINVAL;

    uint32_t n = 0u;
    while (path[n]) n++;
    if (n == 0u) return SCFS_EINVAL;

    while (n > 1u && path[n - 1u] == '/') n--;      /* trim trailing / */

    uint32_t cut = n;
    while (cut > 0u && path[cut - 1u] != '/') cut--;

    if (cut == 0u) {                                /* no slash */
        parent[0] = '/';
        parent[1] = '\0';
        *name = path;
        return SCFS_OK;
    }
    if (cut >= sz) return SCFS_ENOMEM;

    for (uint32_t i = 0u; i < cut; i++) parent[i] = path[i];
    parent[cut] = '\0';
    *name = path + cut;
    return SCFS_OK;
}

/* ═════════════════════════════════════════════════════════════════════
 * scfs_create_node — the O_CREAT path
 *
 * Bach's Algorithm creat, steps 3 and 4: assign a free inode (algorithm
 * ialloc), then create the directory entry in the parent, recording the
 * new name and the newly assigned inode number.
 * ═════════════════════════════════════════════════════════════════════ */
InCoreInode *scfs_create_node(const char *path, uint16_t perm)
{
    char        parent[SCFS_PATH_MAX];
    const char *name = (const char *)0;

    int rc = scfs_path_split(path, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) return (InCoreInode *)0;

    InCoreInode *dir = namei(parent, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!dir) return (InCoreInode *)0;
    if (!SCFS_IS_DIR(dir->mode)) { iput(dir); return (InCoreInode *)0; }
    if (!inode_access_ok(dir, 0u, 0u, 0, 1, 0)) { iput(dir); return (InCoreInode *)0; }

    /* The name must not already be there. */
    if (dir_lookup(dir, name) != 0u) { iput(dir); return (InCoreInode *)0; }

    InCoreInode *ip = ialloc(FT_REGULAR, (uint16_t)(perm & 0777u), 0u, 0u);
    if (!ip) { iput(dir); return (InCoreInode *)0; }

    ip->nlink = 1;
    if (dir_add(dir, name, ip->ino) != 0) {      /* 01_fsa */
        iput(ip);
        iput(dir);
        return (InCoreInode *)0;
    }

    iupdate(ip);
    iput(dir);            /* the parent is done with */
    return ip;            /* caller owns the new inode */
}

/* ═════════════════════════════════════════════════════════════════════
 * The mount table helpers
 * ═════════════════════════════════════════════════════════════════════ */
scfs_mount_t *scfs_mount_alloc(void)
{
    for (uint32_t i = 0u; i < NMOUNT; i++)
        if (!scfs_mount_table[i].m_inuse) {
            scfs_mount_t *mp = &scfs_mount_table[i];
            mp->m_inuse = 1u;
            return mp;
        }
    return (scfs_mount_t *)0;
}

void scfs_mount_free(scfs_mount_t *mp)
{
    if (!mp) return;
    if (mp->m_mountpt) iput(mp->m_mountpt);
    if (mp->m_root)    iput(mp->m_root);
    if (mp->m_sb_buf)  brelse(mp->m_sb_buf);

    mp->m_mountpt = (InCoreInode *)0;
    mp->m_root    = (InCoreInode *)0;
    mp->m_sb_buf  = (BufEntry *)0;
    mp->m_inuse   = 0u;
}

scfs_mount_t *scfs_mount_find(uint16_t dev)
{
    for (uint32_t i = 0u; i < NMOUNT; i++)
        if (scfs_mount_table[i].m_inuse && scfs_mount_table[i].m_dev == dev)
            return &scfs_mount_table[i];
    return (scfs_mount_t *)0;
}

/* ═════════════════════════════════════════════════════════════════════
 * scfs_init — bring the syscall layer's three tables up
 *
 * 01_fsa must be initialised first: buf_init, inode_cache_init, sb_init
 * and fs_mkfs all belong to it.  This function only clears SCFS's own
 * tables and adopts the root inode as the initial cwd.
 * ═════════════════════════════════════════════════════════════════════ */
int scfs_init(void)
{
    for (uint32_t i = 0u; i < NFILE; i++)  {
        scfs_file_table[i].f_inuse = 0u;
        scfs_file_table[i].f_inode = (InCoreInode *)0;
    }
    for (uint32_t i = 0u; i < NOFILE; i++)
        scfs_u_default.ufd_file[i] = (scfs_file_t *)0;
    for (uint32_t i = 0u; i < NMOUNT; i++)
        scfs_mount_table[i].m_inuse = 0u;

    /* Bach: "when the system is first booted, process 0 makes the file
     * system root its current directory during initialization." */
    InCoreInode *root = iget(ROOT_INO);          /* 01_fsa */
    if (!root) return SCFS_EIO;

    s_root = root;
    s_cwd  = root;
    s_umask = 0u;

    printf("[scfs] tables ready: %u file entries, %u fds, %u mounts\n",
           (unsigned)NFILE, (unsigned)NOFILE, (unsigned)NMOUNT);
    return SCFS_OK;
}
