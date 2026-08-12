/*
 * 30_KIX/33_PCS/include/uiox_uaccess.h
 *
 * User-space memory access primitives.
 *
 * copy_to_user   — kernel buffer → user buffer
 * copy_from_user — user buffer   → kernel buffer
 * uiox_uaccess_ok — validate user pointer is within user VA range
 *
 * All four architectures:
 *   ARM64:    user VA  0x0000_0000_0000_0000 – 0x0000_7FFF_FFFF_FFFF
 *   ARM32:    user VA  0x0000_0000 – 0xBFFF_FFFF
 *   RISC-V64: user VA  0x0000_0000_0000_0000 – 0x0000_7FFF_FFFF_FFFF
 *   x86-64:   user VA  0x0000_0000_0000_0000 – 0x0000_7FFF_FFFF_FFFF
 *
 * @version 1.0.0
 * @date    2026-07-28
 */

 #ifndef UIOX_UACCESS_H
 #define UIOX_UACCESS_H
 
 #include "uiox_soc.h"   /* uintptr_t, size_t, uint8_t */
 
 /* ── User VA ceiling per architecture ─────────────────────────────── */
 #if defined(__aarch64__)
 #  define UIOX_USER_VA_MAX   0x0000_7FFF_FFFF_FFFFul
 #elif defined(__arm__)
 #  define UIOX_USER_VA_MAX   0xBFFFFFFFul
 #elif defined(__riscv)
 #  define UIOX_USER_VA_MAX   0x0000_7FFF_FFFF_FFFFul
 #elif defined(__x86_64__)
 #  define UIOX_USER_VA_MAX   0x00007FFFFFFFFFFFul
 #else
 #  error "uiox_uaccess.h: unsupported architecture"
 #endif
 
 /* ── Error codes ───────────────────────────────────────────────────── */
 #define UIOX_UACCESS_OK       0
 #define UIOX_UACCESS_EFAULT  -14   /* bad address — matches EFAULT */
 #define UIOX_UACCESS_EINVAL  -22   /* invalid argument */
 
 /*
  * uiox_uaccess_ok — validate that [uaddr, uaddr+size) lies entirely
  * within the user VA range and does not wrap around.
  *
  * Returns 1 (true) if safe, 0 (false) if not.
  */
 static inline int uiox_uaccess_ok(const void *uaddr, uiox_size_t size)
 {
    uiox_uintptr_t start = (uiox_uintptr_t)uaddr;
    uiox_uintptr_t end;
 
     if (size == 0u)           return 1;
     if (uaddr == (void *)0u)  return 0;
 
     /* check for wrap-around */
     end = start + (uiox_uintptr_t)size - 1u;
     if (end < start)          return 0;   /* wrapped */
 
     /* check upper bound */
     if (end > (uiox_uintptr_t)UIOX_USER_VA_MAX) return 0;
 
     return 1;
 }
 
 /*
  * copy_to_user — copy 'n' bytes from kernel address 'ksrc'
  * to user address 'udst'.
  *
  * Returns 0 on success, UIOX_UACCESS_EFAULT on bad user pointer.
  */
 int uiox_copy_to_user(void       *udst,
                       const void *ksrc,
                       uiox_size_t      n);
 
 /*
  * copy_from_user — copy 'n' bytes from user address 'usrc'
  * to kernel address 'kdst'.
  *
  * Returns 0 on success, UIOX_UACCESS_EFAULT on bad user pointer.
  */
 int uiox_copy_from_user(void       *kdst,
                         const void *usrc,
                         uiox_size_t      n);
 
 /*
  * put_user / get_user — single scalar copy helpers
  * (avoids overhead of full copy_to/from_user for one value)
  */
 int uiox_put_user_u8 (uiox_uint8_t  val, uiox_uint8_t  *uaddr);
 int uiox_put_user_u16(uiox_uint16_t val, uiox_uint16_t *uaddr);
 int uiox_put_user_u32(uiox_uint32_t val, uiox_uint32_t *uaddr);
 int uiox_put_user_u64(uiox_uint64_t val, uiox_uint64_t *uaddr);
 
 int uiox_get_user_u8 (uiox_uint8_t  *out, const uiox_uint8_t  *uaddr);
 int uiox_get_user_u16(uiox_uint16_t *out, const uiox_uint16_t *uaddr);
 int uiox_get_user_u32(uiox_uint32_t *out, const uiox_uint32_t *uaddr);
 int uiox_get_user_u64(uiox_uint64_t *out, const uiox_uint64_t *uaddr);
 
 #endif /* UIOX_UACCESS_H */
 