/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_execve.c
 *
 * execve() — Group 1, memory Management.
 *
 * ── the grouping, verbatim ──────────────────────────────────────────
 *   exec: detachreg, allocreg, attachreg, growreg, loadreg, mapreg
 *
 * All six are implemented in 40psa's region source:
 *   uiox_kix_psa_detachreg   Algorithm 8
 *   uiox_kix_psa_allocreg    Algorithm 3
 *   uiox_kix_psa_attachreg   Algorithm 4
 *   uiox_kix_psa_growreg     Algorithm 5
 *   uiox_kix_psa_loadreg     Algorithm 6
 *
 * So the region machinery is NOT what blocks exec.
 *
 * ── why this returns ENOSYS and it is not the regions ───────────────
 * The file-verification step fabricates what it then checks.  It writes
 * the expected magic into the header structure and then tests that
 * structure against the same constant, so the test cannot fail — and the
 * inode pointer passed in is NULL besides, so the load path never reads
 * a byte.
 *
 * That is the same defect class as loadreg marking pages valid without
 * reading them: a check that passes on data nobody retrieved.
 *
 * TO FIX: read the header from the inode through 01_fsa's inode path,
 * and test the magic against what was READ.  Then loadreg has a real
 * source and exec has a real image.
 *
 * ── the user pointers ───────────────────────────────────────────────
 * path must name a real string, so it goes through the shared gate and a
 * bad address yields EFAULT rather than a kernel-mode dereference.
 * argv and envp may be NULL per POSIX, and are not dereferenced either
 * way — so NULL is accepted for both.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_execve(uiox_uintptr_t path, uiox_uintptr_t argv,
                              uiox_uintptr_t envp)
{
    (void)argv;   /* NULL is legal and nothing dereferences it */
    (void)envp;

    if (!uiox_kix_scps_current()) return SCPCS_ESRCH;

    /* one byte, byte-aligned: just enough to prove the string is
     * inside the user window */
    if (uiox_kix_scps_check_user_ptr(path, 1u, 1u) != 0) return SCPCS_EFAULT;

    return SCPCS_ENOSYS;
}
