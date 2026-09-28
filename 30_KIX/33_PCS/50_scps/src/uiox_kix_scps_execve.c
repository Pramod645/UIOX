/*
 * 33_PCS/50_scps/src/uiox_kix_scps_execve.c
 *
 * execve() — Group 1, Memory Management.
 *
 * ── SCPS grouping ─────────────────────────────────────────────────────
 *   exec: detachreg, allocreg, attachreg, growreg, loadreg, mapreg
 * All six exist in region.c (verified: allocreg, dupreg, attachreg,
 * detachreg, freereg, growreg, loadreg are all present).
 *
 * ── SCPS Algorithms 6 and 7 ───────────────────────────────────────────
 *   Algorithm 6  exec    — verify header, detach old regions, load new
 *   Algorithm 7  xalloc  — allocate and initialize the text region
 *
 * ── why ENOSYS, and it is not the regions ─────────────────────────────
 * kernel_exec() cannot load anything.  It calls
 *
 *     exec_verify_file(ip, &hdr)      with ip = (struct inode *)0
 *
 * and exec_verify_file FABRICATES the header it then checks:
 *
 *     hdr->eh_magic = EXEC_MAGIC;              <- writes the magic
 *     if (hdr->eh_magic != EXEC_MAGIC && ...)  <- then tests it
 *
 * The test cannot fail; no file is read.  The NULL inode makes
 * kernel_exec fail on its own first line, which is at least honest.
 *
 * TO FIX: exec_verify_file reads the header from the inode through
 * 01_fsa's inode path, and the magic check tests what it READ.
 *
 * ── the user pointers ─────────────────────────────────────────────────
 * path is validated through the shared gate, so a bad address yields
 * EFAULT rather than a kernel-mode dereference.  argv/envp may be NULL
 * per POSIX and are not dereferenced either way.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_execve(uiox_uintptr_t path, uiox_uintptr_t argv,
                             uiox_uintptr_t envp)
{
    (void)argv; (void)envp;

    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    if (uiox_kix_scps_check_user_ptr(path, 1u, 1u) != 0) return SCPS_EFAULT;

    return SCPS_ENOSYS;   /* exec_verify_file fabricates its header */
}
