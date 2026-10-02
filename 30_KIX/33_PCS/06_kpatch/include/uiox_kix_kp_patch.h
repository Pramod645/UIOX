/**
 * @file  uiox_kix_kp_patch.h
 * @brief UIOX Live Kernel Patching — core patch engine.
 *
 * ── THREE THINGS THAT ARE TRUE TODAY AND MUST NOT BE ASSUMED PERMANENT ─
 *
 *   1. Kernel text is WRITABLE.  Every arch's make_writable() is a no-op
 *      because UIOX maps kernel text RWX during firmware init.  The
 *      moment text becomes read-only, four functions must be implemented:
 *        uiox_kix_kp_arch_arm64.c    make_writable / restore_protect
 *        uiox_kix_kp_arch_arm32.c    make_writable / restore_protect
 *        uiox_kix_kp_arch_x86.c      make_writable / restore_protect
 *        uiox_kix_kp_arch_riscv64.c  make_writable / restore_protect
 *
 *   2. The trampoline pool does not RECLAIM — roughly 256 disable/enable
 *      cycles at a 16-byte stub.  See uiox_kix_kp_mem.h.
 *
 *   3. This build cannot DETECT an SMP race.  stop_machine is a compiler
 *      barrier, not quiescence.
 *
 * ── thread safety ──────────────────────────────────────────────────────
 * enable() / disable() must be called with all CPUs quiesced.  See (3):
 * this build does not enforce the requirement.
 *
 * ── the trampoline does not survive a disable/enable cycle ────────────
 * disable() frees the trampoline and clears patch->trampoline.  A later
 * enable() allocates a NEW one.
 *
 * The address may differ.  Anything that cached patch->trampoline across
 * a disable/enable cycle is holding a stale pointer, and on this
 * allocator it is stale in a way that reads as valid: mem_free() marks
 * the slot unused but does not reclaim it, so the old address is still
 * inside the pool.  A replacement that calls it after a re-enable
 * executes whatever was allocated into that slot next — or the zeroed
 * bytes, which trap on every architecture this builds for.
 *
 * Read patch->trampoline afresh on every use, or keep it only while the
 * patch is ENABLED.
 *
 * @version 1.2.0
 * @date    2026-10-02
 */

 #ifndef UIOX_KIX_KP_PATCH_H
 #define UIOX_KIX_KP_PATCH_H
 
 #include "uiox_kix_kp_types.h"
 #include "uiox_kix_kp_arch.h"
 #include "uiox_kix_kp_mem.h"
 #include "uiox_kix_kp_arg.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── Engine ──────────────────────────────────────────────────────────── */
 uiox_kix_kp_err_t uiox_kix_kp_engine_init   (void);
 void              uiox_kix_kp_engine_deinit (void);
 
 /* ── Patch lifecycle ─────────────────────────────────────────────────── */
 uiox_kix_kp_err_t uiox_kix_kp_register   (uiox_kix_kp_patch_t *patch);
 uiox_kix_kp_err_t uiox_kix_kp_unregister (uiox_kix_kp_patch_t *patch);
 uiox_kix_kp_err_t uiox_kix_kp_enable     (uiox_kix_kp_patch_t *patch);
 
 /**
  * Disable an enabled patch.  Restores the saved bytes and frees the
  * trampoline — see the disable/enable note in the banner.
  */
 uiox_kix_kp_err_t uiox_kix_kp_disable    (uiox_kix_kp_patch_t *patch);
 
 /* ── Module operations ───────────────────────────────────────────────── */
 
 /**
  * Load a module: register + enable all patches.
  *
  * On a mid-sequence failure the patches already applied are rolled back.
  * If a rollback step itself fails the module is left PARTIALLY applied,
  * which is a different outcome from "did not load" — the error returned
  * reflects the rollback failure in that case.
  */
 uiox_kix_kp_err_t uiox_kix_kp_module_load   (uiox_kix_kp_module_t *mod);
 
 /**
  * Unload a module: disable + unregister all patches.
  *
  * A patch that cannot be disabled is LEFT REGISTERED — unregistering it
  * would drop the only record of which bytes to restore.  Returns
  * UIOX_KIX_KP_ERR_FAULT in that case.
  */
 uiox_kix_kp_err_t uiox_kix_kp_module_unload (uiox_kix_kp_module_t *mod);
 
 /* ── Query ───────────────────────────────────────────────────────────── */
 uiox_kix_kp_patch_t *uiox_kix_kp_find_by_addr(uintptr_t orig_func);
 uiox_kix_kp_patch_t *uiox_kix_kp_find_by_name(const char *name);
 uint32_t             uiox_kix_kp_count       (void);
 uint32_t             uiox_kix_kp_active_count(void);
 void                 uiox_kix_kp_print_table (void);
 
 /* ── CPU quiesce — BARRIERS, not quiescence ──────────────────────────── */
 void uiox_kix_kp_stop_machine (void);
 void uiox_kix_kp_start_machine(void);
 
 /* ── Convenience macro ───────────────────────────────────────────────── */
 #define UIOX_KIX_KP_LOAD_PATCH(orig_fn, new_fn)                      \
     do {                                                               \
         static uiox_kix_kp_patch_t _kp_patch_ = UIOX_KIX_KP_PATCH(   \
             #orig_fn, orig_fn, new_fn);                               \
         uiox_kix_kp_register(&_kp_patch_);                            \
         uiox_kix_kp_enable  (&_kp_patch_);                            \
     } while (0)
 
 /* =========================================================================
  * Syscall interface (40_SystemCallInterface)
  *
  * ── PRIVILEGE: these four are not safe for unprivileged callers ────────
  * The pointers are validated — see uiox_kix_kp_arg.h — but the CONTENTS
  * of a module descriptor are not, and cannot be meaningfully:
  *
  *   sys_kpatch_load(mod_addr)   the kernel reads mod->num_patches and
  *                               walks mod->patches[], whose entries carry
  *                               orig_func and new_func addresses, and
  *                               writes a jump at each orig_func.
  *
  * So a caller who can place a descriptor in user memory chooses which
  * KERNEL ADDRESS receives executable code.  The address is checked; the
  * target is not, because a patch engine that refused to patch kernel text
  * would have no purpose.
  *
  * Acceptable while reachable only from a privileged context.  IF IT IS
  * EVER MADE REACHABLE UNPRIVILEGED, IT IS A PRIVILEGE-ESCALATION
  * PRIMITIVE.  The check belongs in the dispatcher, before the call.
  * ====================================================================== */
 #define SYS_KPATCH_LOAD     210u
 #define SYS_KPATCH_UNLOAD   211u
 #define SYS_KPATCH_STATUS   212u
 #define SYS_KPATCH_LIST     213u
 
 long sys_kpatch_load  (long mod_addr, long flags, long a2, long a3);
 long sys_kpatch_unload(long mod_addr, long flags, long a2, long a3);
 long sys_kpatch_status(long name_ptr, long buf,   long a2, long a3);
 long sys_kpatch_list  (long buf,      long max,   long a2, long a3);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KP_PATCH_H */
 