/**
 * @file  uiox_kix_kp_arg.h
 * @brief UIOX kpatch — user-pointer validation for the syscall boundary.
 *
 * ── why this exists ────────────────────────────────────────────────────
 * The four SYS_KPATCH_* handlers take user-supplied addresses.  The
 * versions this replaces dereferenced every one directly:
 *
 *   *(uiox_kp_state_t *)buf = p->state;
 *   uint32_t *out = (uint32_t *)buf;
 *   uiox_kp_module_t *mod = (uiox_kp_module_t *)mod_addr;
 *
 * sys_kpatch_load was the worst case: it handed mod_addr straight to
 * uiox_kp_module_load(), which reads mod->num_patches and then walks
 * mod->patches[] — an array of descriptors holding orig_func and
 * new_func addresses — and overwrites the first bytes of each orig_func
 * with a jump.  A caller passing a kernel address therefore had a
 * text-write primitive at whatever address it chose.
 *
 * ── what is NOT gated here ────────────────────────────────────────────
 * orig_func and new_func inside a module descriptor name KERNEL
 * functions.  They are not user addresses and must not be range-checked
 * as such — a patch engine that refused to patch kernel text would be
 * useless.  The boundary is: everything the CALLER supplies is checked;
 * everything the DESCRIPTOR names is trusted.
 *
 * @version 1.0.0
 * @date    2026-10-02
 */

 #ifndef UIOX_KIX_KP_ARG_H
 #define UIOX_KIX_KP_ARG_H
 
 #include "uiox_kix_kp_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_KIX_KP_USER_BASE   ((uint64_t)0x00001000ull)
 #define UIOX_KIX_KP_USER_TOP    ((uint64_t)0xC0000000ull)
 
 /**
  * Validate a user-supplied range.
  *
  * Three checks, and the second is the one a naive version omits:
  *   1. the low end is at or above the user window's base
  *   2. the high end is at or below its top — computed as a sum and
  *      tested for WRAP, because base + len can overflow
  *   3. alignment, when the caller states one
  *
  * A zero length names no byte and is accepted.
  *
  * @return 0 when valid, UIOX_KIX_KP_EFAULT otherwise
  */
 static inline int uiox_kix_kp_check_user_ptr(uintptr_t p, uint64_t len,
                                               uint64_t align)
 {
     uint64_t a = (uint64_t)p;
     uint64_t end;
 
     if (len == 0u) return 0;
     if (a  < UIOX_KIX_KP_USER_BASE) return UIOX_KIX_KP_EFAULT;
 
     end = a + len;
     if (end < a) return UIOX_KIX_KP_EFAULT;
     if (end > UIOX_KIX_KP_USER_TOP) return UIOX_KIX_KP_EFAULT;
 
     if (align && (a % align) != 0u) return UIOX_KIX_KP_EFAULT;
 
     return 0;
 }
 
 /**
  * Validate a NUL-terminated string in user memory.
  *
  * Bounded rather than unbounded: a string that never terminates would
  * otherwise be walked past the end of the user window.
  */
 static inline int uiox_kix_kp_check_user_str(uintptr_t s, uint64_t max_len)
 {
     const char *p = (const char *)s;
     uint64_t    i;
 
     if (uiox_kix_kp_check_user_ptr(s, 1u, 1u) != 0) return UIOX_KIX_KP_EFAULT;
 
     for (i = 0u; i < max_len; i++) {
         if (uiox_kix_kp_check_user_ptr(s + i, 1u, 1u) != 0)
             return UIOX_KIX_KP_EFAULT;
         if (p[i] == '\0') return 0;
     }
     return UIOX_KIX_KP_EFAULT;
 }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KP_ARG_H */
 