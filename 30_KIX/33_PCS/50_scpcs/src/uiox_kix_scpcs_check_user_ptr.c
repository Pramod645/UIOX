/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_check_user_ptr.c
 *
 * The single gate between a syscall wrapper and a user address.
 *
 * ── why a gate rather than a copy ───────────────────────────────────
 * There is no copy_from_user or copy_to_user in this tree.  That leaves
 * a wrapper exactly two honest options for a user pointer:
 *
 *   a) refuse it                      -> EFAULT
 *   b) VALIDATE it, then dereference
 *
 * Option (b) without validation is how a syscall becomes an arbitrary
 * read/write primitive in kernel mode: the caller supplies an address
 * inside the kernel's own image and the kernel faithfully reads or
 * writes it.  This function is what makes (b) defensible.
 *
 * ── the three checks ────────────────────────────────────────────────
 *   1. the LOW end must be at or above the user window's base
 *   2. the HIGH end must be at or below its top — computed as a sum and
 *      tested for wrap, because base + len can overflow and a wrapped
 *      end would pass a naive comparison
 *   3. alignment, when the caller states one for the access width
 *
 * A zero length is always valid: it names no byte, so there is nothing
 * to be outside the window.  POSIX treats a zero-length transfer as
 * legal for the same reason.
 *
 * ── where the window comes from ─────────────────────────────────────
 * 10BSP's SoC memory map decides the real boundaries.  The values here
 * restate the 32-bit model rather than including that header, because
 * 02_MemMngnt and 10BSP use different symbol names for the same region
 * and including both would collide.  When the two are reconciled, this
 * is the one place to change: the constants, not the logic.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scps.h"

#define SCPS_USER_BASE   ((uiox_uint64_t)0x00001000ull)  /* skip NULL page */
#define SCPS_USER_TOP    ((uiox_uint64_t)0xC0000000ull)  /* 32-bit VA model */

int uiox_kix_scps_check_user_ptr(uiox_uintptr_t p, uiox_uint64_t len,
                                 uiox_uint64_t align)
{
    uiox_uint64_t a   = (uiox_uint64_t)p;
    uiox_uint64_t end;

    if (len == 0u) return 0;          /* nothing named, nothing to check */
    if (a  < SCPS_USER_BASE) return SCPS_EFAULT;

    end = a + len;
    if (end < a) return SCPS_EFAULT;  /* the sum wrapped                */
    if (end > SCPS_USER_TOP) return SCPS_EFAULT;

    if (align && (a % align) != 0u) return SCPS_EFAULT;

    return 0;
}
