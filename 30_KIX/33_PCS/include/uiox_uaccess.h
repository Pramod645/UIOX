/*
 * 30_KIX/33_PCS/include/uiox_uaccess.h
 *
 * User-space memory access primitives — the privilege boundary.
 *
 *   uiox_copy_to_user    kernel buffer → user buffer
 *   uiox_copy_from_user  user buffer   → kernel buffer
 *   uiox_uaccess_ok      validate a user pointer against the VA ceiling
 *
 * ── WHY THIS IS IN 33_PCS ─────────────────────────────────────────────
 * Not because it is a syscall — it is not, and no number in any table
 * names it.  Because it is the ONE place where a kernel address and a
 * user address are allowed to meet, and the process is what defines that
 * boundary: the ceiling below is the shape of the process's own address
 * space, set by TTBR0_EL1 / satp / CR3 and owned by 33_PCS/02_MemMngnt.
 *
 * ── THE RANGE CHECK IS NOT A VALIDITY CHECK ───────────────────────────
 * uiox_uaccess_ok() tests that [addr, addr+size) lies inside the user VA
 * range and does not wrap.  It cannot know whether that range is MAPPED
 * in the calling process's page table.
 *
 * The implementation uses memcpy after that test.  That is correct only
 * where the MMU faults on an unmapped user address AND the fault is
 * recoverable.  Today neither holds for every path: 02_MemMngnt has no
 * user page table in service yet, so a pointer that passes the range
 * test and is not mapped will fault in the kernel rather than returning
 * EFAULT.
 *
 * @version 1.0.0  @date 2026-10-03
 */
#ifndef UIOX_UACCESS_H
#define UIOX_UACCESS_H

#include "uiox_klibc.h"   /* uintptr_t, uint8_t..uint64_t, size_t, NULL */

/* ── The user VA ceiling ─────────────────────────────────────────────── */
#if defined(__aarch64__)
#  define UIOX_USER_VA_MAX   0x00007FFFFFFFFFFFUL
#elif defined(__arm__)
#  define UIOX_USER_VA_MAX   0xBFFFFFFFUL
#elif defined(__riscv)
#  define UIOX_USER_VA_MAX   0x00007FFFFFFFFFFFUL
#elif defined(__x86_64__)
#  define UIOX_USER_VA_MAX   0x00007FFFFFFFFFFFUL
#else
#  error "uiox_uaccess.h: unsupported architecture"
#endif

#define UIOX_UACCESS_OK        0
#define UIOX_UACCESS_EFAULT  (-14)   /* bad address          */
#define UIOX_UACCESS_EINVAL  (-22)   /* invalid argument     */

static inline int uiox_uaccess_ok(const void *uaddr, size_t size)
{
    uintptr_t start;
    uintptr_t end;

    if (size == 0u)                     return 1;
    if (uaddr == (const void *)0)       return 0;

    start = (uintptr_t)uaddr;
    end   = start + (uintptr_t)size - 1u;
    if (end < start)                       return 0;   /* wrapped      */
    if (end > (uintptr_t)UIOX_USER_VA_MAX) return 0;   /* past the top */

    return 1;
}

int uiox_copy_to_user  (void       *udst, const void *ksrc, size_t n);
int uiox_copy_from_user(void       *kdst, const void *usrc, size_t n);

int uiox_put_user_u8 (uint8_t  val, uint8_t  *uaddr);
int uiox_put_user_u16(uint16_t val, uint16_t *uaddr);
int uiox_put_user_u32(uint32_t val, uint32_t *uaddr);
int uiox_put_user_u64(uint64_t val, uint64_t *uaddr);

int uiox_get_user_u8 (uint8_t  *out, const uint8_t  *uaddr);
int uiox_get_user_u16(uint16_t *out, const uint16_t *uaddr);
int uiox_get_user_u32(uint32_t *out, const uint32_t *uaddr);
int uiox_get_user_u64(uint64_t *out, const uint64_t *uaddr);

#endif /* UIOX_UACCESS_H */
