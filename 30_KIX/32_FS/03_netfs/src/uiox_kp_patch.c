/**
 * @file  uiox_kp_patch.c
 * @brief UIOX kpatch — core patch engine. No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_kp_patch.h"

 /* =========================================================================
  * Global state
  * ====================================================================== */
 
 static const uiox_kp_arch_ops_t *s_arch = NULL;
 static uiox_kp_patch_t          *s_table[UIOX_KP_MAX_PATCHES];
 static uint32_t                   s_count = 0u;
 static bool                       s_init  = false;
 
 /* Patch-table uptime counter (set by whoever integrates the engine) */
 static uint64_t (*s_get_uptime_ms)(void) = NULL;
 
 /* kprintf shim */
 extern void uiox_fw_printf(const char *fmt, ...);
 
 /* =========================================================================
  * Arch registration
  * ====================================================================== */
 
 static const uiox_kp_arch_ops_t *s_arch_ops = NULL;
 
 void uiox_kp_arch_register(const uiox_kp_arch_ops_t *ops)
 { s_arch_ops = ops; }
 
 const uiox_kp_arch_ops_t *uiox_kp_arch_get(void)
 { return s_arch_ops; }
 
 uiox_kp_arch_t uiox_kp_arch_current(void)
 {
 #if defined(__aarch64__)
     return UIOX_KP_ARCH_ARM64;
 #elif defined(__arm__)
     return UIOX_KP_ARCH_ARM32;
 #else
     return UIOX_KP_ARCH_X86_64;
 #endif
 }
 
 const char *uiox_kp_arch_name(uiox_kp_arch_t arch)
 {
     switch (arch) {
     case UIOX_KP_ARCH_ARM64:  return "arm64";
     case UIOX_KP_ARCH_ARM32:  return "arm32";
     case UIOX_KP_ARCH_X86_64: return "x86_64";
     default:                   return "unknown";
     }
 }
 
 /* =========================================================================
  * No-libc helpers
  * ====================================================================== */
 
 static void kp_strncpy(char *dst, const char *src, size_t n)
 {
     size_t i = 0u;
     while (i < n - 1u && src[i]) { dst[i] = src[i]; i++; }
     dst[i] = '\0';
 }
 
 static int kp_strncmp(const char *a, const char *b, size_t n)
 {
     while (n-- && *a && *b) {
         if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
         a++; b++;
     }
     return 0;
 }
 
 static void kp_memcpy_p(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s;
   while(n--)*dp++=*sp++; }
 
 /* =========================================================================
  * Atomic write (used to overwrite function prologue)
  * Single 32-bit or 64-bit write is atomic on all supported arches.
  * For multi-word stubs we use stop_machine to ensure no CPU is in the
  * prologue while we write.
  * ====================================================================== */
 
 static void patch_write_bytes(uintptr_t addr,
                                 const uint8_t *bytes, size_t len)
 {
     /* Write byte-by-byte — in real SMP kernel we'd use stop_machine  */
     volatile uint8_t *p = (volatile uint8_t *)addr;
     for (size_t i = 0u; i < len; i++) p[i] = bytes[i];
     /* Full memory barrier */
     __asm__ volatile("" ::: "memory");
 }
 
 /* =========================================================================
  * Engine init / deinit
  * ====================================================================== */
 
 uiox_kp_err_t uiox_kp_engine_init(void)
 {
     /* Register arch ops */
 #if defined(__aarch64__)
     uiox_kp_arch_arm64_register();
 #elif defined(__arm__)
     uiox_kp_arch_arm32_register();
 #else
     uiox_kp_arch_x86_register();
 #endif
     s_arch = uiox_kp_arch_get();
     if (!s_arch) return UIOX_KP_ERR_UNSUP;
 
     uiox_kp_err_t rc = uiox_kp_mem_init();
     if (rc != UIOX_KP_OK) return rc;
 
     for (uint32_t i = 0u; i < UIOX_KP_MAX_PATCHES; i++)
         s_table[i] = NULL;
     s_count = 0u;
     s_init  = true;
 
     uiox_fw_printf("[kpatch] engine init OK  arch=%s  pool=%zu B\n",
                     s_arch->name, uiox_kp_mem_avail());
     return UIOX_KP_OK;
 }
 
 void uiox_kp_engine_deinit(void)
 {
     for (uint32_t i = 0u; i < s_count; i++) {
         if (s_table[i] &&
             s_table[i]->state == UIOX_KP_STATE_ENABLED)
             uiox_kp_disable(s_table[i]);
     }
     s_count = 0u;
     s_init  = false;
 }
 
 /* =========================================================================
  * Register / Unregister
  * ====================================================================== */
 
 uiox_kp_err_t uiox_kp_register(uiox_kp_patch_t *patch)
 {
     if (!s_init || !patch) return UIOX_KP_ERR_INVAL;
     if (s_count >= UIOX_KP_MAX_PATCHES) return UIOX_KP_ERR_BUSY;
 
     /* Check not already registered */
     for (uint32_t i = 0u; i < s_count; i++) {
         if (s_table[i] && s_table[i]->orig_func == patch->orig_func)
             return UIOX_KP_ERR_ALREADY;
     }
 
     patch->state        = UIOX_KP_STATE_REGISTERED;
     patch->call_count   = 0u;
     patch->trampoline   = 0u;
     patch->saved_len    = 0u;
 
     s_table[s_count++] = patch;
     uiox_fw_printf("[kpatch] registered '%s'  orig=0x%016llx  new=0x%016llx\n",
                     patch->name,
                     (unsigned long long)patch->orig_func,
                     (unsigned long long)patch->new_func);
     return UIOX_KP_OK;
 }
 
 uiox_kp_err_t uiox_kp_unregister(uiox_kp_patch_t *patch)
 {
     if (!patch) return UIOX_KP_ERR_INVAL;
     if (patch->state == UIOX_KP_STATE_ENABLED)
         return UIOX_KP_ERR_ACTIVE;
 
     for (uint32_t i = 0u; i < s_count; i++) {
         if (s_table[i] == patch) {
             s_table[i] = s_table[--s_count];
             s_table[s_count] = NULL;
             patch->state = UIOX_KP_STATE_UNREGISTERED;
             uiox_fw_printf("[kpatch] unregistered '%s'\n", patch->name);
             return UIOX_KP_OK;
         }
     }
     return UIOX_KP_ERR_NOTFOUND;
 }
 
 /* =========================================================================
  * Enable patch
  * ====================================================================== */
 
 uiox_kp_err_t uiox_kp_enable(uiox_kp_patch_t *patch)
 {
     if (!s_init || !s_arch || !patch) return UIOX_KP_ERR_INVAL;
     if (patch->state == UIOX_KP_STATE_ENABLED)
         return UIOX_KP_ERR_ALREADY;
     if (patch->state != UIOX_KP_STATE_REGISTERED &&
         patch->state != UIOX_KP_STATE_DISABLED)
         return UIOX_KP_ERR_INVAL;
 
     /* 1. Determine jump stub size */
     uint8_t jsize = s_arch->jump_size(patch->orig_func, patch->new_func);
 
     /* 2. Save original bytes */
     kp_memcpy_p(patch->saved_bytes,
                  (const void *)patch->orig_func, jsize);
     patch->saved_len = jsize;
 
     /* 3. Allocate trampoline */
     size_t tramp_size = (size_t)jsize + UIOX_KP_JMP_SIZE_ARM64_FAR;
     void *tramp_mem = uiox_kp_mem_alloc(tramp_size);
     if (!tramp_mem) {
         patch->state = UIOX_KP_STATE_ERROR;
         return UIOX_KP_ERR_NOMEM;
     }
     patch->trampoline = (uintptr_t)tramp_mem;
 
     /* 4. Build trampoline */
     uint8_t tramp_buf[64];
     int tlen = s_arch->build_trampoline(
                    tramp_buf, sizeof(tramp_buf),
                    patch->trampoline,
                    patch->saved_bytes, patch->saved_len,
                    patch->orig_func);
     if (tlen < 0) {
         uiox_kp_mem_free(tramp_mem, tramp_size);
         patch->state = UIOX_KP_STATE_ERROR;
         return UIOX_KP_ERR_FAULT;
     }
     /* Write trampoline to executable pool */
     patch_write_bytes(patch->trampoline, tramp_buf, (size_t)tlen);
     s_arch->icache_flush(patch->trampoline, (size_t)tlen);
 
     /* 5. Quiesce CPUs */
     uiox_kp_stop_machine();
 
     /* 6. Make orig_func writable */
     if (s_arch->make_writable(patch->orig_func, jsize) != 0) {
         uiox_kp_start_machine();
         uiox_kp_mem_free(tramp_mem, tramp_size);
         patch->state = UIOX_KP_STATE_ERROR;
         return UIOX_KP_ERR_FAULT;
     }
 
     /* 7. Write jump stub at orig_func */
     uint8_t jump_buf[UIOX_KP_SAVED_BYTES_MAX];
     int jlen = s_arch->write_jump(jump_buf,
                                     patch->orig_func,
                                     patch->new_func,
                                     UIOX_KP_SAVED_BYTES_MAX);
     if (jlen < 0) {
         s_arch->restore_protect(patch->orig_func, jsize);
         uiox_kp_start_machine();
         uiox_kp_mem_free(tramp_mem, tramp_size);
         patch->state = UIOX_KP_STATE_ERROR;
         return UIOX_KP_ERR_FAULT;
     }
     patch_write_bytes(patch->orig_func, jump_buf, (size_t)jlen);
 
     /* 8. Flush caches */
     s_arch->icache_flush(patch->orig_func, (size_t)jsize);
 
     /* 9. Restore protection */
     s_arch->restore_protect(patch->orig_func, jsize);
 
     /* 10. Resume CPUs */
     uiox_kp_start_machine();
 
     patch->state = UIOX_KP_STATE_ENABLED;
 
     uiox_fw_printf("[kpatch] ENABLED  '%s'  tramp=0x%016llx\n",
                     patch->name,
                     (unsigned long long)patch->trampoline);
     return UIOX_KP_OK;
 }
 
 /* =========================================================================
  * Disable patch — restore original bytes
  * ====================================================================== */
 
 uiox_kp_err_t uiox_kp_disable(uiox_kp_patch_t *patch)
 {
     if (!s_init || !s_arch || !patch) return UIOX_KP_ERR_INVAL;
     if (patch->state != UIOX_KP_STATE_ENABLED)
         return UIOX_KP_ERR_INVAL;
 
     uiox_kp_stop_machine();
 
     if (s_arch->make_writable(patch->orig_func, patch->saved_len) != 0) {
         uiox_kp_start_machine();
         return UIOX_KP_ERR_FAULT;
     }
 
     /* Restore saved bytes */
     patch_write_bytes(patch->orig_func,
                        patch->saved_bytes, patch->saved_len);
 
     s_arch->icache_flush(patch->orig_func, patch->saved_len);
     s_arch->restore_protect(patch->orig_func, patch->saved_len);
 
     uiox_kp_start_machine();
 
     /* Free trampoline */
     if (patch->trampoline) {
         uiox_kp_mem_free((void *)patch->trampoline,
                           (size_t)patch->saved_len + UIOX_KP_JMP_SIZE_ARM64_FAR);
         patch->trampoline = 0u;
     }
 
     patch->state = UIOX_KP_STATE_DISABLED;
     uiox_fw_printf("[kpatch] DISABLED '%s'\n", patch->name);
     return UIOX_KP_OK;
 }
 
 /* =========================================================================
  * Module operations
  * ====================================================================== */
 
 uiox_kp_err_t uiox_kp_module_load(uiox_kp_module_t *mod)
 {
     if (!mod) return UIOX_KP_ERR_INVAL;
     if (mod->loaded) return UIOX_KP_ERR_ALREADY;
 
     for (uint32_t i = 0u; i < mod->num_patches; i++) {
         uiox_kp_err_t rc = uiox_kp_register(&mod->patches[i]);
         if (rc != UIOX_KP_OK) return rc;
         rc = uiox_kp_enable(&mod->patches[i]);
         if (rc != UIOX_KP_OK) return rc;
     }
     mod->loaded = true;
     uiox_fw_printf("[kpatch] module '%s' loaded  (%u patches)\n",
                     mod->name, mod->num_patches);
     return UIOX_KP_OK;
 }
 
 uiox_kp_err_t uiox_kp_module_unload(uiox_kp_module_t *mod)
 {
     if (!mod || !mod->loaded) return UIOX_KP_ERR_INVAL;
 
     for (uint32_t i = 0u; i < mod->num_patches; i++) {
         if (mod->patches[i].state == UIOX_KP_STATE_ENABLED)
             uiox_kp_disable(&mod->patches[i]);
         uiox_kp_unregister(&mod->patches[i]);
     }
     mod->loaded = false;
     uiox_fw_printf("[kpatch] module '%s' unloaded\n", mod->name);
     return UIOX_KP_OK;
 }
 
 /* =========================================================================
  * Query
  * ====================================================================== */
 
 uiox_kp_patch_t *uiox_kp_find_by_addr(uintptr_t orig_func)
 {
     for (uint32_t i = 0u; i < s_count; i++)
         if (s_table[i] && s_table[i]->orig_func == orig_func)
             return s_table[i];
     return NULL;
 }
 
 uiox_kp_patch_t *uiox_kp_find_by_name(const char *name)
 {
     if (!name) return NULL;
     for (uint32_t i = 0u; i < s_count; i++)
         if (s_table[i] &&
             kp_strncmp(s_table[i]->name, name, UIOX_KP_NAME_LEN) == 0)
             return s_table[i];
     return NULL;
 }
 
 uint32_t uiox_kp_count(void) { return s_count; }
 
 uint32_t uiox_kp_active_count(void)
 {
     uint32_t n = 0u;
     for (uint32_t i = 0u; i < s_count; i++)
         if (s_table[i] && s_table[i]->state == UIOX_KP_STATE_ENABLED) n++;
     return n;
 }
 
 void uiox_kp_print_table(void)
 {
     uiox_fw_printf("[kpatch] Patch table (%u registered, %u active):\n",
                     s_count, uiox_kp_active_count());
     for (uint32_t i = 0u; i < s_count; i++) {
         const uiox_kp_patch_t *p = s_table[i];
         if (!p) continue;
         uiox_fw_printf("  [%u] %-32s  state=%-12s  calls=%u\n"
                         "       orig=0x%016llx  new=0x%016llx\n"
                         "       tramp=0x%016llx  saved=%u B\n",
                         i, p->name,
                         uiox_kp_state_name(p->state),
                         p->call_count,
                         (unsigned long long)p->orig_func,
                         (unsigned long long)p->new_func,
                         (unsigned long long)p->trampoline,
                         p->saved_len);
     }
 }
 
 /* =========================================================================
  * CPU quiesce stubs (single-CPU UIOX: no-op)
  * ====================================================================== */
 
 void uiox_kp_stop_machine(void)
 {
     /* On real SMP: IPI all secondary CPUs → spin loop */
     __asm__ volatile("" ::: "memory");
 }
 
 void uiox_kp_start_machine(void)
 {
     __asm__ volatile("" ::: "memory");
 }
 
 /* =========================================================================
  * Syscall handlers
  * ====================================================================== */
 
 long sys_kpatch_load(long mod_addr, long flags, long a2, long a3)
 {
     UIOX_KP_UNUSED(flags); UIOX_KP_UNUSED(a2); UIOX_KP_UNUSED(a3);
     uiox_kp_module_t *mod = (uiox_kp_module_t *)mod_addr;
     return (long)uiox_kp_module_load(mod);
 }
 
 long sys_kpatch_unload(long mod_addr, long flags, long a2, long a3)
 {
     UIOX_KP_UNUSED(flags); UIOX_KP_UNUSED(a2); UIOX_KP_UNUSED(a3);
     uiox_kp_module_t *mod = (uiox_kp_module_t *)mod_addr;
     return (long)uiox_kp_module_unload(mod);
 }
 
 long sys_kpatch_status(long name_ptr, long buf, long a2, long a3)
 {
     UIOX_KP_UNUSED(a2); UIOX_KP_UNUSED(a3);
     const char *name = (const char *)name_ptr;
     uiox_kp_patch_t *p = uiox_kp_find_by_name(name);
     if (!p) return (long)UIOX_KP_ERR_NOTFOUND;
     *(uiox_kp_state_t *)buf = p->state;
     return (long)UIOX_KP_OK;
 }
 
 long sys_kpatch_list(long buf, long max, long a2, long a3)
 {
     UIOX_KP_UNUSED(a2); UIOX_KP_UNUSED(a3);
     uint32_t *out = (uint32_t *)buf;
     uint32_t  n   = (uint32_t)max;
     uint32_t  cnt = (s_count < n) ? s_count : n;
     for (uint32_t i = 0u; i < cnt; i++)
         out[i] = s_table[i] ? (uint32_t)i : 0xFFFFFFFFu;
     return (long)cnt;
 }
 