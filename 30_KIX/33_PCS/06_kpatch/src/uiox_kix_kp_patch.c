/**
 * @file  uiox_kix_kp_patch.c
 * @brief UIOX kpatch — core patch engine. No libc.
 *
 * ── what changed in 1.2.0 ─────────────────────────────────────────────
 *   1. The module-load rollback REPORTS a partial failure, and so does
 *      unload.  A patch that cannot be disabled is LEFT REGISTERED.
 *   2. stop_machine's limitation is stated in the code.
 *
 * @version 1.2.0
 * @date    2026-10-02
 */

 #include "../include/uiox_kix_kp_patch.h"

 /* =========================================================================
  * Global state
  * ====================================================================== */
 static const uiox_kix_kp_arch_ops_t *s_arch = NULL;
 static uiox_kix_kp_patch_t          *s_table[UIOX_KIX_KP_MAX_PATCHES];
 static uint32_t                      s_count = 0u;
 static bool                          s_init  = false;
 
 static uint64_t (*s_get_uptime_ms)(void) = NULL;
 
 extern void uiox_fw_printf(const char *fmt, ...);
 
 /* =========================================================================
  * Arch registration
  * ====================================================================== */
 
 void uiox_kix_kp_arch_register(const uiox_kix_kp_arch_ops_t *ops)
 { s_arch = ops; }
 
 const uiox_kix_kp_arch_ops_t *uiox_kix_kp_arch_get(void)
 { return s_arch; }
 
 uiox_kix_kp_arch_t uiox_kix_kp_arch_current(void)
 {
 #if defined(__aarch64__)
     return UIOX_KIX_KP_ARCH_ARM64;
 #elif defined(__arm__)
     return UIOX_KIX_KP_ARCH_ARM32;
 #elif defined(__riscv)
     return UIOX_KIX_KP_ARCH_RISCV64;
 #else
     return UIOX_KIX_KP_ARCH_X86_64;
 #endif
 }
 
 const char *uiox_kix_kp_arch_name(uiox_kix_kp_arch_t arch)
 {
     switch (arch) {
     case UIOX_KIX_KP_ARCH_ARM64:   return "arm64";
     case UIOX_KIX_KP_ARCH_ARM32:   return "arm32";
     case UIOX_KIX_KP_ARCH_RISCV64: return "riscv64";
     case UIOX_KIX_KP_ARCH_X86_64:  return "x86_64";
     default:                       return "unknown";
     }
 }
 
 /* =========================================================================
  * No-libc helpers
  * ====================================================================== */
 
 static void kp_memcpy_p(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s;
   while(n--)*dp++=*sp++; }
 
 static int kp_strncmp(const char *a, const char *b, size_t n)
 {
     while (n-- && *a && *b) {
         if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
         a++; b++;
     }
     return 0;
 }
 
 static void patch_write_bytes(uintptr_t addr,
                                 const uint8_t *bytes, size_t len)
 {
     volatile uint8_t *p = (volatile uint8_t *)addr;
     for (size_t i = 0u; i < len; i++) p[i] = bytes[i];
     __asm__ volatile("" ::: "memory");
 }
 
 /* =========================================================================
  * Engine init / deinit
  * ====================================================================== */
 
 uiox_kix_kp_err_t uiox_kix_kp_engine_init(void)
 {
 #if defined(__aarch64__)
     uiox_kix_kp_arch_arm64_register();
 #elif defined(__arm__)
     uiox_kix_kp_arch_arm32_register();
 #elif defined(__riscv)
     uiox_kix_kp_arch_riscv64_register();
 #else
     uiox_kix_kp_arch_x86_register();
 #endif
 
     if (!s_arch) return UIOX_KIX_KP_ERR_UNSUP;
 
     uiox_kix_kp_err_t rc = uiox_kix_kp_mem_init();
     if (rc != UIOX_KIX_KP_OK) return rc;
 
     for (uint32_t i = 0u; i < UIOX_KIX_KP_MAX_PATCHES; i++)
         s_table[i] = NULL;
     s_count = 0u;
     s_init  = true;
 
     uiox_fw_printf("[kpatch] engine init OK  arch=%s  pool=%zu B\n",
                     s_arch->name, uiox_kix_kp_mem_avail());
     return UIOX_KIX_KP_OK;
 }
 
 void uiox_kix_kp_engine_deinit(void)
 {
     for (uint32_t i = 0u; i < s_count; i++) {
         if (s_table[i] && s_table[i]->state == UIOX_KIX_KP_STATE_ENABLED)
             uiox_kix_kp_disable(s_table[i]);
     }
     s_count = 0u;
     s_init  = false;
 }
 
 /* =========================================================================
  * Register / Unregister
  * ====================================================================== */
 
 uiox_kix_kp_err_t uiox_kix_kp_register(uiox_kix_kp_patch_t *patch)
 {
     if (!s_init || !patch) return UIOX_KIX_KP_ERR_INVAL;
     if (s_count >= UIOX_KIX_KP_MAX_PATCHES) return UIOX_KIX_KP_ERR_BUSY;
 
     for (uint32_t i = 0u; i < s_count; i++) {
         if (s_table[i] && s_table[i]->orig_func == patch->orig_func)
             return UIOX_KIX_KP_ERR_ALREADY;
     }
 
     patch->state           = UIOX_KIX_KP_STATE_REGISTERED;
     patch->call_count      = 0u;
     patch->trampoline      = 0u;
     patch->saved_len       = 0u;
     patch->tramp_alloc_len = 0u;
 
     s_table[s_count++] = patch;
     uiox_fw_printf("[kpatch] registered '%s'  orig=0x%016llx  new=0x%016llx\n",
                     patch->name,
                     (unsigned long long)patch->orig_func,
                     (unsigned long long)patch->new_func);
     return UIOX_KIX_KP_OK;
 }
 
 uiox_kix_kp_err_t uiox_kix_kp_unregister(uiox_kix_kp_patch_t *patch)
 {
     if (!patch) return UIOX_KIX_KP_ERR_INVAL;
     if (patch->state == UIOX_KIX_KP_STATE_ENABLED)
         return UIOX_KIX_KP_ERR_ACTIVE;
 
     for (uint32_t i = 0u; i < s_count; i++) {
         if (s_table[i] == patch) {
             s_table[i] = s_table[--s_count];
             s_table[s_count] = NULL;
             patch->state = UIOX_KIX_KP_STATE_UNREGISTERED;
             uiox_fw_printf("[kpatch] unregistered '%s'\n", patch->name);
             return UIOX_KIX_KP_OK;
         }
     }
     return UIOX_KIX_KP_ERR_NOTFOUND;
 }
 
 /* =========================================================================
  * Enable patch
  * ====================================================================== */
 
 uiox_kix_kp_err_t uiox_kix_kp_enable(uiox_kix_kp_patch_t *patch)
 {
     uint8_t  jsize;
     size_t   tramp_size;
     void    *tramp_mem;
     uint8_t  tramp_buf[64];
     int      tlen;
     uint8_t  jump_buf[UIOX_KIX_KP_SAVED_BYTES_MAX];
     int      jlen;
 
     if (!s_init || !s_arch || !patch) return UIOX_KIX_KP_ERR_INVAL;
     if (patch->state == UIOX_KIX_KP_STATE_ENABLED)
         return UIOX_KIX_KP_ERR_ALREADY;
     if (patch->state != UIOX_KIX_KP_STATE_REGISTERED &&
         patch->state != UIOX_KIX_KP_STATE_DISABLED)
         return UIOX_KIX_KP_ERR_INVAL;
 
     /* 1. Jump stub size */
     jsize = s_arch->jump_size(patch->orig_func, patch->new_func);
 
     /* 2. Save original bytes */
     kp_memcpy_p(patch->saved_bytes, (const void *)patch->orig_func, jsize);
     patch->saved_len = jsize;
 
     /* 3. Allocate trampoline from the arch's OWN far size */
     tramp_size = (size_t)jsize + (size_t)s_arch->max_jump_size;
     tramp_mem  = uiox_kix_kp_mem_alloc(tramp_size);
     if (!tramp_mem) {
         patch->state = UIOX_KIX_KP_STATE_ERROR;
         return UIOX_KIX_KP_ERR_NOMEM;
     }
     patch->trampoline      = (uintptr_t)tramp_mem;
     patch->tramp_alloc_len = (uint8_t)((tramp_size > 255u) ? 255u
                                                            : (uint8_t)tramp_size);
 
     /* 4. Build trampoline */
     tlen = s_arch->build_trampoline(tramp_buf, sizeof(tramp_buf),
                                      patch->trampoline,
                                      patch->saved_bytes, patch->saved_len,
                                      patch->orig_func);
     if (tlen < 0) {
         uiox_kix_kp_mem_free(tramp_mem, tramp_size);
         patch->trampoline      = 0u;
         patch->tramp_alloc_len = 0u;
         patch->state = UIOX_KIX_KP_STATE_ERROR;
         return UIOX_KIX_KP_ERR_FAULT;
     }
     patch_write_bytes(patch->trampoline, tramp_buf, (size_t)tlen);
     s_arch->icache_flush(patch->trampoline, (size_t)tlen);
 
     /* 5. Quiesce */
     uiox_kix_kp_stop_machine();
 
     /* 6. Make writable */
     if (s_arch->make_writable(patch->orig_func, jsize) != 0) {
         uiox_kix_kp_start_machine();
         uiox_kix_kp_mem_free(tramp_mem, tramp_size);
         patch->trampoline      = 0u;
         patch->tramp_alloc_len = 0u;
         patch->state = UIOX_KIX_KP_STATE_ERROR;
         return UIOX_KIX_KP_ERR_FAULT;
     }
 
     /* 7. Write jump stub */
     jlen = s_arch->write_jump(jump_buf, patch->orig_func, patch->new_func,
                                UIOX_KIX_KP_SAVED_BYTES_MAX);
     if (jlen < 0) {
         s_arch->restore_protect(patch->orig_func, jsize);
         uiox_kix_kp_start_machine();
         uiox_kix_kp_mem_free(tramp_mem, tramp_size);
         patch->trampoline      = 0u;
         patch->tramp_alloc_len = 0u;
         patch->state = UIOX_KIX_KP_STATE_ERROR;
         return UIOX_KIX_KP_ERR_FAULT;
     }
     patch_write_bytes(patch->orig_func, jump_buf, (size_t)jlen);
 
     /* 8/9. Flush and restore */
     s_arch->icache_flush(patch->orig_func, (size_t)jsize);
     s_arch->restore_protect(patch->orig_func, jsize);
 
     /* 10. Resume */
     uiox_kix_kp_start_machine();
 
     patch->state           = UIOX_KIX_KP_STATE_ENABLED;
     patch->install_time_ms = s_get_uptime_ms ? s_get_uptime_ms() : 0u;
 
     uiox_fw_printf("[kpatch] ENABLED  '%s'  tramp=0x%016llx\n",
                     patch->name,
                     (unsigned long long)patch->trampoline);
     return UIOX_KIX_KP_OK;
 }
 
 /* =========================================================================
  * Disable patch
  * ====================================================================== */
 
 uiox_kix_kp_err_t uiox_kix_kp_disable(uiox_kix_kp_patch_t *patch)
 {
     if (!s_init || !s_arch || !patch) return UIOX_KIX_KP_ERR_INVAL;
     if (patch->state != UIOX_KIX_KP_STATE_ENABLED)
         return UIOX_KIX_KP_ERR_INVAL;
 
     uiox_kix_kp_stop_machine();
 
     if (s_arch->make_writable(patch->orig_func, patch->saved_len) != 0) {
         uiox_kix_kp_start_machine();
         return UIOX_KIX_KP_ERR_FAULT;
     }
 
     patch_write_bytes(patch->orig_func, patch->saved_bytes, patch->saved_len);
 
     s_arch->icache_flush(patch->orig_func, patch->saved_len);
     s_arch->restore_protect(patch->orig_func, patch->saved_len);
 
     uiox_kix_kp_start_machine();
 
     /* Freed with the length RECORDED at allocation.  NOTE: the pool does
      * not reclaim — this only zeroes the region. */
     if (patch->trampoline) {
         uiox_kix_kp_mem_free((void *)patch->trampoline,
                               (size_t)patch->tramp_alloc_len);
         patch->trampoline      = 0u;
         patch->tramp_alloc_len = 0u;
     }
 
     patch->state = UIOX_KIX_KP_STATE_DISABLED;
     uiox_fw_printf("[kpatch] DISABLED '%s'\n", patch->name);
     return UIOX_KIX_KP_OK;
 }
 
 /* =========================================================================
  * Module operations
  * ====================================================================== */
 
 uiox_kix_kp_err_t uiox_kix_kp_module_load(uiox_kix_kp_module_t *mod)
 {
     uiox_kix_kp_err_t rc;
     uint32_t          i;
 
     if (!mod) return UIOX_KIX_KP_ERR_INVAL;
     if (mod->loaded) return UIOX_KIX_KP_ERR_ALREADY;
     if (mod->num_patches > UIOX_KIX_KP_MAX_MODULE_PATCHES)
         return UIOX_KIX_KP_ERR_INVAL;
 
     for (i = 0u; i < mod->num_patches; i++) {
         rc = uiox_kix_kp_register(&mod->patches[i]);
         if (rc != UIOX_KIX_KP_OK) break;
 
         rc = uiox_kix_kp_enable(&mod->patches[i]);
         if (rc != UIOX_KIX_KP_OK) {
             uiox_kix_kp_unregister(&mod->patches[i]);
             break;
         }
     }
 
     if (i == mod->num_patches) {
         mod->loaded = true;
         uiox_fw_printf("[kpatch] module '%s' loaded  (%u patches)\n",
                         mod->name, mod->num_patches);
         return UIOX_KIX_KP_OK;
     }
 
     /* ── rollback ────────────────────────────────────────────────────
      * Undo what was applied.  This can itself FAIL: disable() returns
      * FAULT when make_writable() refuses, and a patch stuck ENABLED is
      * still redirecting its function.  Reporting the original error would
      * make "module did not load" and "two functions are still patched"
      * look identical from outside. */
     {
         bool stuck = false;
 
         for (uint32_t j = 0u; j < i; j++) {
             if (mod->patches[j].state == UIOX_KIX_KP_STATE_ENABLED) {
                 uiox_kix_kp_err_t drc = uiox_kix_kp_disable(&mod->patches[j]);
                 if (drc != UIOX_KIX_KP_OK) {
                     stuck = true;
                     uiox_fw_printf("[kpatch] ROLLBACK FAILED on '%s' "
                                    "(%s) — function is STILL PATCHED\n",
                                    mod->patches[j].name,
                                    uiox_kix_kp_err_str(drc));
                     continue;
                 }
             }
             uiox_kix_kp_unregister(&mod->patches[j]);
         }
 
         if (stuck) {
             uiox_fw_printf("[kpatch] module '%s' PARTIALLY applied — "
                            "%u of %u patches could not be rolled back\n",
                            mod->name, i, mod->num_patches);
             return UIOX_KIX_KP_ERR_FAULT;
         }
     }
 
     uiox_fw_printf("[kpatch] module '%s' load failed (%s) — "
                    "rolled back cleanly\n",
                    mod->name, uiox_kix_kp_err_str(rc));
     return rc;
 }
 
 uiox_kix_kp_err_t uiox_kix_kp_module_unload(uiox_kix_kp_module_t *mod)
 {
     bool stuck = false;
 
     if (!mod || !mod->loaded) return UIOX_KIX_KP_ERR_INVAL;
 
     for (uint32_t i = 0u; i < mod->num_patches; i++) {
         if (mod->patches[i].state == UIOX_KIX_KP_STATE_ENABLED) {
             uiox_kix_kp_err_t drc = uiox_kix_kp_disable(&mod->patches[i]);
             if (drc != UIOX_KIX_KP_OK) {
                 stuck = true;
                 uiox_fw_printf("[kpatch] unload: '%s' could not be "
                                "disabled (%s) — left registered\n",
                                mod->patches[i].name,
                                uiox_kix_kp_err_str(drc));
                 continue;
             }
         }
         uiox_kix_kp_unregister(&mod->patches[i]);
     }
 
     mod->loaded = false;
 
     if (stuck) {
         uiox_fw_printf("[kpatch] module '%s' PARTIALLY unloaded — "
                        "see the entries above\n", mod->name);
         return UIOX_KIX_KP_ERR_FAULT;
     }
 
     uiox_fw_printf("[kpatch] module '%s' unloaded\n", mod->name);
     return UIOX_KIX_KP_OK;
 }
 
 /* =========================================================================
  * Query
  * ====================================================================== */
 
 uiox_kix_kp_patch_t *uiox_kix_kp_find_by_addr(uintptr_t orig_func)
 {
     for (uint32_t i = 0u; i < s_count; i++)
         if (s_table[i] && s_table[i]->orig_func == orig_func)
             return s_table[i];
     return NULL;
 }
 
 uiox_kix_kp_patch_t *uiox_kix_kp_find_by_name(const char *name)
 {
     if (!name) return NULL;
     for (uint32_t i = 0u; i < s_count; i++)
         if (s_table[i] &&
             kp_strncmp(s_table[i]->name, name, UIOX_KIX_KP_NAME_LEN) == 0)
             return s_table[i];
     return NULL;
 }
 
 uint32_t uiox_kix_kp_count(void) { return s_count; }
 
 uint32_t uiox_kix_kp_active_count(void)
 {
     uint32_t n = 0u;
     for (uint32_t i = 0u; i < s_count; i++)
         if (s_table[i] && s_table[i]->state == UIOX_KIX_KP_STATE_ENABLED) n++;
     return n;
 }
 
 void uiox_kix_kp_print_table(void)
 {
     uiox_fw_printf("[kpatch] Patch table (%u registered, %u active):\n",
                     s_count, uiox_kix_kp_active_count());
     for (uint32_t i = 0u; i < s_count; i++) {
         const uiox_kix_kp_patch_t *p = s_table[i];
         if (!p) continue;
         uiox_fw_printf("  [%u] %-32s  state=%-12s  calls=%u\n"
                         "       orig=0x%016llx  new=0x%016llx\n"
                         "       tramp=0x%016llx  saved=%u B  alloc=%u B\n",
                         i, p->name,
                         uiox_kix_kp_state_name(p->state),
                         p->call_count,
                         (unsigned long long)p->orig_func,
                         (unsigned long long)p->new_func,
                         (unsigned long long)p->trampoline,
                         p->saved_len, p->tramp_alloc_len);
     }
 }
 
 /* =========================================================================
  * CPU quiesce stubs
  *
  * ── BARRIERS, NOT QUIESCENCE — read this before using on SMP ──────────
  * Both functions below are compiler barriers and nothing more.  On a
  * single-CPU build that is correct.  On SMP it is INSUFFICIENT and
  * undetectable here: a CPU inside the patched prologue while
  * patch_write_bytes runs will fetch a partially written instruction
  * stream, non-deterministically.  The requirement in
  * uiox_kix_kp_patch.h's banner is real and this build does not enforce it.
  *
  * A real implementation IPIs every secondary CPU, waits for each to
  * acknowledge from a point where it is not executing text, then proceeds.
  * ====================================================================== */
 
 void uiox_kix_kp_stop_machine(void)
 {
     __asm__ volatile("" ::: "memory");
 }
 
 void uiox_kix_kp_start_machine(void)
 {
     __asm__ volatile("" ::: "memory");
 }
 
 /* =========================================================================
  * Syscall handlers
  *
  * Every user pointer is validated first — see uiox_kix_kp_arg.h.  The
  * ADDRESS is checked, the CONTENT is not, and SYS_KPATCH_LOAD must
  * therefore be registered for privileged callers only.
  * ====================================================================== */
 
 long sys_kpatch_load(long mod_addr, long flags, long a2, long a3)
 {
     UIOX_KIX_KP_UNUSED(flags); UIOX_KIX_KP_UNUSED(a2); UIOX_KIX_KP_UNUSED(a3);
 
     if (uiox_kix_kp_check_user_ptr((uintptr_t)mod_addr,
                                      (uint64_t)sizeof(uiox_kix_kp_module_t),
                                      (uint64_t)_Alignof(uiox_kix_kp_module_t)) != 0)
         return (long)UIOX_KIX_KP_EFAULT;
 
     return (long)uiox_kix_kp_module_load(
                        (uiox_kix_kp_module_t *)(uintptr_t)mod_addr);
 }
 
 long sys_kpatch_unload(long mod_addr, long flags, long a2, long a3)
 {
     UIOX_KIX_KP_UNUSED(flags); UIOX_KIX_KP_UNUSED(a2); UIOX_KIX_KP_UNUSED(a3);
 
     if (uiox_kix_kp_check_user_ptr((uintptr_t)mod_addr,
                                      (uint64_t)sizeof(uiox_kix_kp_module_t),
                                      (uint64_t)_Alignof(uiox_kix_kp_module_t)) != 0)
         return (long)UIOX_KIX_KP_EFAULT;
 
     return (long)uiox_kix_kp_module_unload(
                        (uiox_kix_kp_module_t *)(uintptr_t)mod_addr);
 }
 
 long sys_kpatch_status(long name_ptr, long buf, long a2, long a3)
 {
     uiox_kix_kp_patch_t *p;
 
     UIOX_KIX_KP_UNUSED(a2); UIOX_KIX_KP_UNUSED(a3);
 
     if (uiox_kix_kp_check_user_str((uintptr_t)name_ptr,
                                      (uint64_t)UIOX_KIX_KP_NAME_LEN) != 0)
         return (long)UIOX_KIX_KP_EFAULT;
 
     if (uiox_kix_kp_check_user_ptr((uintptr_t)buf,
                                      (uint64_t)sizeof(uiox_kix_kp_state_t),
                                      (uint64_t)_Alignof(uiox_kix_kp_state_t)) != 0)
         return (long)UIOX_KIX_KP_EFAULT;
 
     p = uiox_kix_kp_find_by_name((const char *)(uintptr_t)name_ptr);
     if (!p) return (long)UIOX_KIX_KP_ERR_NOTFOUND;
 
     *(uiox_kix_kp_state_t *)(uintptr_t)buf = p->state;
     return (long)UIOX_KIX_KP_OK;
 }
 
 long sys_kpatch_list(long buf, long max, long a2, long a3)
 {
     uint32_t *out;
     uint32_t  n;
     uint32_t  cnt;
 
     UIOX_KIX_KP_UNUSED(a2); UIOX_KIX_KP_UNUSED(a3);
 
     if (max < 0) return (long)UIOX_KIX_KP_ERR_INVAL;
 
     n   = (uint32_t)max;
     cnt = (s_count < n) ? s_count : n;
 
     if (uiox_kix_kp_check_user_ptr((uintptr_t)buf,
                                      (uint64_t)cnt * sizeof(uint32_t),
                                      (uint64_t)_Alignof(uint32_t)) != 0)
         return (long)UIOX_KIX_KP_EFAULT;
 
     out = (uint32_t *)(uintptr_t)buf;
     for (uint32_t i = 0u; i < cnt; i++)
         out[i] = s_table[i] ? (uint32_t)i : 0xFFFFFFFFu;
 
     return (long)cnt;
 }
 