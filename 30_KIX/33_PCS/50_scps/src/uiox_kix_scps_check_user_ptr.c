/*
 * 33_PCS/50_scps/src/uiox_kix_scps_check_user_ptr.c
 *
 * The single gate between a syscall wrapper and a user address.
 *
 * There is no copy_from_user / copy_to_user in this tree.  That means a
 * wrapper has exactly two honest options for a user pointer:
 *
 *   a) refuse it                    -> EFAULT
 *   b) VALIDATE it, then dereference
 *
 * Doing (b) without validating is how a syscall becomes an arbitrary
 * read/write primitive in kernel mode.  This is the check that makes
 * (b) defensible: the range must be inside the user address window and
 * correctly aligned for the access width.
 *
 * The window is 10BSP's, via the SoC memory map: user space is
 * [USER_BASE, USER_TOP) which on the arm32 model is the low 3 GB.
 * Restated here rather than included, because 02_MemMngnt and 10BSP
 * disagree about the symbol names.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

#define SCPS_USER_BASE   ((uiox_uint64_t)0x00001000ull)
#define SCPS_USER_TOP    ((uiox_uint64_t)0xC0000000ull)   /* 32-bit VA model */

int uiox_kix_scps_check_user_ptr(uiox_uintptr_t p, uiox_uint64_t len,
                                 uiox_uint64_t align)
{
    uiox_uint64_t a = (uiox_uint64_t)p;
    uiox_uint64_t end;

    if (len == 0u) return 0;              /* zero-length is always valid */
    if (a  < SCPS_USER_BASE) return SCPS_EFAULT;

    end = a + len;
    if (end < a) return SCPS_EFAULT;      /* wrapped */
    if (end > SCPS_USER_TOP) return SCPS_EFAULT;

    if (align && (a % align) != 0u) return SCPS_EFAULT;
    return 0;
}
