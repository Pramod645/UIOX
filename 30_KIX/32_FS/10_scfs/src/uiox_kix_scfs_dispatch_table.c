/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_dispatch_table.c
 *
 * SCFS — Bach's sysent[]: the system call entry table.
 * Bach, The Design of the UNIX Operating System, Ch.5.
 *
 * ── what Bach says about this ──────────────────────────────────────────
 * Bach's kernel reaches the file system through sysent[], an array
 * indexed by system call number, each entry holding a function pointer,
 * the argument count, and a name.  The trap handler computes the number
 * from the instruction, indexes sysent[], checks the argument count, and
 * calls.
 *
 * "The system call name is used for maintenance, to map the numbers into
 * human-readable form."
 *
 * ── why the table is here and not in the arch code ─────────────────────
 * The arch trap handler (arch_init.c, one per target) knows ONE thing
 * about a system call: its number.  It does not know what the call does,
 * and must not.  So the table lives in the file layer, the arch stub
 * calls scfs_dispatch() with a number, and adding a syscall is one row
 * here — never a change to four arch files.
 *
 * ── the numbers ────────────────────────────────────────────────────────
 * A fixed private range, 1000 upward, defined once in
 * uiox_kix_scfs_dispatch.h and referenced from both this table and the
 * user-space stub, so the two cannot disagree.
 *
 * ── FIXED in this revision ─────────────────────────────────────────────
 *  1. the header comment named uiox_kix_scfs_table.c — the content was
 *     split out afterwards.  It now names this file, and no longer claims
 *     to hold the sys_* alias bodies (those are in syscalls.c).
 *  2. `struct scfs_args` and `scfs_fn_t` were declared here as well as in
 *     dispatch.h — two definitions of one contract.  This file now uses
 *     the header's `scfs_args_t` / `scfs_fn_t` and declares neither.
 *  3. the nargs column said 2 for stat / fstat / statfs, whose
 *     implementations now take a third `uint32_t bufsz`.  Those rows and
 *     their trampolines carry the third argument.
 *
 * @version 1.1.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

#include "uiox_kix_scfs_dispatch.h"

/* ═════════════════════════════════════════════════════════════════════
 * The carrier and the function type come from the dispatch header.
 *
 * They were previously declared here as well, which gave the build two
 * definitions of one contract.  `scfs_args_t` and `scfs_fn_t` are
 * declared exactly once, in uiox_kix_scfs_dispatch.h.
 * ═════════════════════════════════════════════════════════════════════ */

/* ── the rows ───────────────────────────────────────────────────────── */

/* ── descriptors ───────────────────────────────────────────────────── */
static intptr_t S_open (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_open ((const char *)a->a0, (int)a->a1, (uint16_t)a->a2); }

static intptr_t S_openat(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_openat((int)a->a0, (const char *)a->a1, (int)a->a2, (uint16_t)a->a3); }

static intptr_t S_creat(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_creat((const char *)a->a0, (uint16_t)a->a1); }

static intptr_t S_close(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_close((int)a->a0); }

static intptr_t S_lseek(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_lseek((int)a->a0, (int32_t)a->a1, (int)a->a2); }

static intptr_t S_read (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_read ((int)a->a0, (char *)a->a1, (uint32_t)a->a2); }

static intptr_t S_write(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_write((int)a->a0, (const char *)a->a1, (uint32_t)a->a2); }

static intptr_t S_dup  (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_dup  ((int)a->a0); }

static intptr_t S_dup2 (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_dup2 ((int)a->a0, (int)a->a1); }

static intptr_t S_fcntl(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_fcntl((int)a->a0, (int)a->a1, (int)a->a2); }

static intptr_t S_ioctl(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_ioctl((int)a->a0, (uint32_t)a->a1, (void *)a->a2); }

/* ── names and directories ─────────────────────────────────────────── */
static intptr_t S_link  (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_link  ((const char *)a->a0, (const char *)a->a1); }

static intptr_t S_unlink(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_unlink((const char *)a->a0); }

static intptr_t S_rename(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_rename((const char *)a->a0, (const char *)a->a1); }

static intptr_t S_mkdir (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_mkdir ((const char *)a->a0, (uint16_t)a->a1); }

static intptr_t S_rmdir (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_rmdir ((const char *)a->a0); }

static intptr_t S_mknod (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_mknod ((const char *)a->a0, (uint16_t)a->a1,
                                        (uint8_t)a->a2, (uint8_t)a->a3); }

static intptr_t S_chdir (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_chdir ((const char *)a->a0); }

static intptr_t S_fchdir(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_fchdir((int)a->a0); }

static intptr_t S_chroot(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_chroot((const char *)a->a0); }

static intptr_t S_getcwd(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_getcwd((char *)a->a0, (uint32_t)a->a1); }

static intptr_t S_getdents64(const scfs_args_t *a)
{
    /* The offset is an in/out parameter; the arch stub passes its address. */
    return (intptr_t)uiox_kix_scfs_getdents64((int)a->a0, (uint8_t *)a->a1,
                                              (uint32_t)a->a2,
                                              (uint32_t *)a->a3);
}

/* ── status and metadata ───────────────────────────────────────────── */
/* stat / fstat now take a THIRD argument: the size of the caller's
 * status buffer, checked before anything is written.  See
 * uiox_kix_scfs_stat.h. */
static intptr_t S_stat  (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_stat  ((const char *)a->a0, (void *)a->a1,
                                        (uint32_t)a->a2); }

static intptr_t S_fstat (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_fstat ((int)a->a0, (void *)a->a1,
                                        (uint32_t)a->a2); }

static intptr_t S_lstat (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_lstat ((const char *)a->a0, (void *)a->a1,
                                        (uint32_t)a->a2); }

static intptr_t S_chmod (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_chmod ((const char *)a->a0, (uint16_t)a->a1); }

static intptr_t S_chown (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_chown ((const char *)a->a0,
                                        (uint16_t)a->a1, (uint16_t)a->a2); }

static intptr_t S_access(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_access((const char *)a->a0, (int)a->a1); }

static intptr_t S_umask (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_umask ((uint16_t)a->a0); }

static intptr_t S_truncate(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_truncate((const char *)a->a0, (uint32_t)a->a1); }

static intptr_t S_ftruncate(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_ftruncate((int)a->a0, (uint32_t)a->a1); }

static intptr_t S_utime (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_utime ((const char *)a->a0, (const int64_t *)a->a1); }

/* ── the mount table ───────────────────────────────────────────────── */
static intptr_t S_mount (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_mount ((const char *)a->a0,
                                        (const char *)a->a1, (int)a->a2); }

static intptr_t S_umount(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_umount((const char *)a->a0); }

/* statfs likewise takes the buffer size as a third argument. */
static intptr_t S_statfs(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_statfs((const char *)a->a0, (void *)a->a1,
                                        (uint32_t)a->a2); }

static intptr_t S_fstatfs(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_fstatfs((int)a->a0, (void *)a->a1,
                                         (uint32_t)a->a2); }

/* ── durability ────────────────────────────────────────────────────── */
static intptr_t S_sync  (const scfs_args_t *a)
{ (void)a; uiox_kix_scfs_sync(); return 0; }

static intptr_t S_fsync (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_fsync((int)a->a0); }

static intptr_t S_fdatasync(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_fdatasync((int)a->a0); }

/* ── extended attributes ───────────────────────────────────────────── */
static intptr_t S_setxattr(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_setxattr((const char *)a->a0, (const char *)a->a1,
                                          (const void *)a->a2, (uint32_t)a->a3,
                                          (int)a->a4); }

static intptr_t S_getxattr(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_getxattr((const char *)a->a0, (const char *)a->a1,
                                          (void *)a->a2, (uint32_t)a->a3); }

/* ── the calls the layer below cannot yet carry ────────────────────── */
static intptr_t S_pipe  (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_pipe  ((int *)a->a0); }

static intptr_t S_mmap  (const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_mmap((void *)a->a0, (uint32_t)a->a1,
                                      (int)a->a2, (int)a->a3,
                                      (int)a->a4, (uint32_t)a->a5); }

static intptr_t S_munmap(const scfs_args_t *a)
{ return (intptr_t)uiox_kix_scfs_munmap((void *)a->a0, (uint32_t)a->a1); }

/* ═════════════════════════════════════════════════════════════════════
 * TABLE 1 — the dispatch table itself
 *
 * Bach's sysent[] in shape: number, function, argument count, name.  The
 * name is for maintenance — a trap with an unknown number can print it,
 * and a debugger can resolve it without the symbol table.
 *
 * nargs is the number of argument registers the implementation READS,
 * and it must match the trampoline above.  A row that understates it
 * hides a mismatch the count check exists to catch.
 * ═════════════════════════════════════════════════════════════════════ */
const scfs_sysent_t scfs_sysent_table[] = {

    /* number                     function   nargs  name            */
    { SCFS_SYS_open,              S_open,        3, "open"          },
    { SCFS_SYS_openat,            S_openat,      4, "openat"        },
    { SCFS_SYS_creat,             S_creat,       2, "creat"         },
    { SCFS_SYS_close,             S_close,       1, "close"         },
    { SCFS_SYS_lseek,             S_lseek,       3, "lseek"         },
    { SCFS_SYS_read,              S_read,        3, "read"          },
    { SCFS_SYS_write,             S_write,       3, "write"         },
    { SCFS_SYS_dup,               S_dup,         1, "dup"           },
    { SCFS_SYS_dup2,              S_dup2,        2, "dup2"          },
    { SCFS_SYS_fcntl,             S_fcntl,       3, "fcntl"         },
    { SCFS_SYS_ioctl,             S_ioctl,       3, "ioctl"         },

    { SCFS_SYS_link,              S_link,        2, "link"          },
    { SCFS_SYS_unlink,            S_unlink,      1, "unlink"        },
    { SCFS_SYS_rename,            S_rename,      2, "rename"        },
    { SCFS_SYS_mkdir,             S_mkdir,       2, "mkdir"         },
    { SCFS_SYS_rmdir,             S_rmdir,       1, "rmdir"         },
    { SCFS_SYS_mknod,             S_mknod,       4, "mknod"         },
    { SCFS_SYS_chdir,             S_chdir,       1, "chdir"         },
    { SCFS_SYS_fchdir,            S_fchdir,      1, "fchdir"        },
    { SCFS_SYS_chroot,            S_chroot,      1, "chroot"        },
    { SCFS_SYS_getcwd,            S_getcwd,      2, "getcwd"        },
    { SCFS_SYS_getdents64,        S_getdents64,  4, "getdents64"    },

    /* the three that carry a buffer size — nargs 3, not 2 */
    { SCFS_SYS_stat,              S_stat,        3, "stat"          },
    { SCFS_SYS_fstat,             S_fstat,       3, "fstat"         },
    { SCFS_SYS_lstat,             S_lstat,       3, "lstat"         },
    { SCFS_SYS_chmod,             S_chmod,       2, "chmod"         },
    { SCFS_SYS_chown,             S_chown,       3, "chown"         },
    { SCFS_SYS_access,            S_access,      2, "access"        },
    { SCFS_SYS_umask,             S_umask,       1, "umask"         },
    { SCFS_SYS_truncate,          S_truncate,    2, "truncate"      },
    { SCFS_SYS_ftruncate,         S_ftruncate,   2, "ftruncate"     },
    { SCFS_SYS_utime,             S_utime,       2, "utime"         },

    { SCFS_SYS_mount,             S_mount,       3, "mount"         },
    { SCFS_SYS_umount,            S_umount,      1, "umount"        },
    { SCFS_SYS_statfs,            S_statfs,      3, "statfs"        },

    { SCFS_SYS_sync,              S_sync,        0, "sync"          },
    { SCFS_SYS_fsync,             S_fsync,       1, "fsync"         },
    { SCFS_SYS_fdatasync,         S_fdatasync,   1, "fdatasync"     },

    { SCFS_SYS_setxattr,          S_setxattr,    5, "setxattr"      },
    { SCFS_SYS_getxattr,          S_getxattr,    4, "getxattr"      },

    { SCFS_SYS_pipe,              S_pipe,        1, "pipe"          },
    { SCFS_SYS_mmap,              S_mmap,        6, "mmap"          },

    /* Bach's sentinel: a null function ends the walk. */
    { 0u, (scfs_fn_t)0, 0, (const char *)0 }
};

const uint32_t scfs_sysent_count =
    (uint32_t)(sizeof(scfs_sysent_table) / sizeof(scfs_sysent_table[0]) - 1u);

/* ═════════════════════════════════════════════════════════════════════
 * TABLE 2 — the number-to-row index
 *
 * A linear scan of 43 rows is fine at this size and needs no sorted
 * invariant to maintain.  When the table passes a hundred rows, this is
 * the place to put a binary search — the callers do not change.
 * ═════════════════════════════════════════════════════════════════════ */
const scfs_sysent_t *scfs_sysent_lookup(uint32_t num)
{
    for (uint32_t i = 0u; i < scfs_sysent_count; i++)
        if (scfs_sysent_table[i].number == num) return &scfs_sysent_table[i];

    return (const scfs_sysent_t *)0;
}

static intptr_t scfs_sysent_badcall(void) { return (intptr_t)SCFS_ENOSYS; }

/* ═════════════════════════════════════════════════════════════════════
 * TABLE 3 — the gap table: calls that exist as numbers but have no
 * working implementation on the architecture in use.
 *
 * Bach has no such table.  This one exists so the arch stub can say WHY
 * a call failed rather than only that it did, and so a bring-up log lists
 * the gaps in one place instead of one at a time as programs discover
 * them.
 * ═════════════════════════════════════════════════════════════════════ */
const scfs_gap_t scfs_gap_table[] = {
    { "pipe",   "no pipe buffer in 01_fsa's inode — structure built, no data path"  },
    { "mount",  "buffer cache has no device field — second device unreadable"       },
    { "mmap",   "no MMU paging in 33_PCS — file side validated, no address side"    },
    { "flock",  "no lock owner or lock table — inode lock is per-call only"         },
    { "fcntl",  "F_SETLK/F_GETLK need a process identity this layer lacks"          },
    { "fdatasync","implemented over IFLAG_DIRTY; coarser than per-range flush"      },
    { "xattr",  "no i_xattr on the inode and no xattr_* in 01_fsa"                  },
    { "symlink","FT_SYMLINK exists but no unit stores a target"                     },

    { (const char *)0, (const char *)0 }
};

/* ═════════════════════════════════════════════════════════════════════
 * scfs_dispatch — the one entry point the arch trap handler calls
 *
 * Bach's trap handler: compute the number, index sysent[], check the
 * argument count, call.  That is all.
 * ═════════════════════════════════════════════════════════════════════ */
intptr_t scfs_dispatch(uint32_t num, const scfs_args_t *args)
{
    const scfs_sysent_t *e = scfs_sysent_lookup(num);

    /* An unknown number is a user-space bug, not a kernel gap: the call
     * was never defined.  ENOSYS is the answer POSIX prescribes. */
    if (!e) return scfs_sysent_badcall();

    /* A null carrier cannot be read for any call that reads arguments.
     * A count of 0 means the implementation takes the carrier and does
     * its own validation. */
    if (e->nargs != 0u && args == (const scfs_args_t *)0)
        return (intptr_t)SCFS_EFAULT;

    return e->fn(args);
}

/* ═════════════════════════════════════════════════════════════════════
 * scfs_syscall_init — the boot-time hook
 *
 * Called from the file layer's initialisation after 01_fsa is up.  It
 * verifies that the table is well-formed — every row has a function and a
 * name, no number is duplicated, no nargs exceeds the register count —
 * and prints the count and the gaps, so a bring-up log records what this
 * build can and cannot do.
 * ═════════════════════════════════════════════════════════════════════ */
int scfs_syscall_init(void)
{
    uint32_t i, j;

    /* Every row has a function and a name. */
    for (i = 0u; i < scfs_sysent_count; i++) {
        if (!scfs_sysent_table[i].fn || !scfs_sysent_table[i].name) {
            printf("[scfs] FATAL: malformed sysent row %u\n", (unsigned)i);
            return SCFS_EIO;
        }
    }

    /* No nargs exceeds the registers the trap handler saves — a row that
     * claims more than the carrier holds would read past it. */
    for (i = 0u; i < scfs_sysent_count; i++) {
        if (scfs_sysent_table[i].nargs > SCFS_MAX_ARGS) {
            printf("[scfs] FATAL: %s claims %u args, carrier holds %u\n",
                   scfs_sysent_table[i].name,
                   (unsigned)scfs_sysent_table[i].nargs,
                   (unsigned)SCFS_MAX_ARGS);
            return SCFS_EIO;
        }
    }

    /* No number appears twice — a duplicate would silently shadow one
     * call with another, which is the kind of bug that costs a day. */
    for (i = 0u; i < scfs_sysent_count; i++)
        for (j = i + 1u; j < scfs_sysent_count; j++)
            if (scfs_sysent_table[i].number == scfs_sysent_table[j].number) {
                printf("[scfs] FATAL: duplicate syscall number %u (%s / %s)\n",
                       (unsigned)scfs_sysent_table[i].number,
                       scfs_sysent_table[i].name,
                       scfs_sysent_table[j].name);
                return SCFS_EIO;
            }

    printf("[scfs] dispatch ready: %u syscalls, %u argument registers\n",
           (unsigned)scfs_sysent_count, (unsigned)SCFS_MAX_ARGS);

    /* The gaps, listed once, so a bring-up log is a checklist. */
    for (i = 0u; scfs_gap_table[i].name; i++)
        printf("[scfs] gap: %s — %s\n",
               scfs_gap_table[i].name, scfs_gap_table[i].reason);

    return SCFS_OK;
}
