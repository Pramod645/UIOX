/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_dup.c
 *
 * SCFS — Algorithm dup.  Bach, The Design of the UNIX Operating System.
 *
 * ── what Bach says ─────────────────────────────────────────────────────
 * dup() duplicates a file descriptor.  The new descriptor points at the
 * SAME file table entry as the old one, and the file table entry's
 * reference count rises.  There is no new inode reference: both
 * descriptors share one entry, so they share its offset and flags.
 *
 *   input:  user file descriptor
 *   output: a new user file descriptor
 *   {
 *       get the file table entry from the old user file descriptor;
 *       if (the old descriptor is not active) return (error);
 *       allocate a new user file descriptor entry;
 *       set it to point to the same file table entry;
 *       increment the file table entry reference count;
 *       return (the new user file descriptor);
 *   }
 *
 * ── why the shared entry matters ───────────────────────────────────────
 * Because f_offset lives in the file table entry, a write through one
 * descriptor moves the position of the other.  That is correct and
 * intended — the shell relies on it when it dups stdin.  It is also why
 * pread/pwrite must pass the offset by value rather than moving it.
 *
 * ── dup2 / dup3 ────────────────────────────────────────────────────────
 * Bach predates these.  dup2 takes the TARGET descriptor number instead
 * of allocating the lowest free one; if the target is in use it is closed
 * first, silently, which is what makes shell redirection work.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_dup(int fd)
{
    /* ── get the file table entry from the old descriptor ───────────── */
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* ── allocate a new fd slot pointing at the SAME entry ──────────── */
    /* scfs_ufd_link() does both halves: finds the slot and raises
     * f_count.  The inode reference does NOT rise, because there is
     * still only one file table entry naming it. */
    return scfs_ufd_link(f);
}

/*
 * dup2() — duplicate to a CHOSEN descriptor number.
 *
 * If newfd is already open it is closed first, silently — that is the
 * documented behaviour and the reason `2>&1` works.
 */
int uiox_kix_scfs_dup2(int oldfd, int newfd)
{
    if (oldfd < 0 || oldfd >= (int)NOFILE) return SCFS_EBADF;
    if (newfd < 0 || newfd >= (int)NOFILE) return SCFS_EBADF;
    if (oldfd == newfd) {
        /* POSIX: oldfd must still be valid, then nothing happens. */
        return scfs_getf(oldfd) ? newfd : SCFS_EBADF;
    }

    scfs_file_t *f = scfs_getf(oldfd);
    if (!f) return SCFS_EBADF;

    /* Close the occupant, if any.  Through the same path close() uses,
     * so the reference counting cannot diverge. */
    if (scfs_u->ufd_file[newfd]) uiox_kix_scfs_close(newfd);

    f->f_count++;
    scfs_u->ufd_file[newfd] = f;

    return newfd;
}

/* dup3() — dup2 with flags.  The only flag POSIX defines is O_CLOEXEC,
 * which this kernel has no exec() to act on, so it is recorded and
 * ignored rather than refused. */
int uiox_kix_scfs_dup3(int oldfd, int newfd, int flags)
{
    if (oldfd == newfd) return SCFS_EINVAL;
    (void)flags;
    return uiox_kix_scfs_dup2(oldfd, newfd);
}
