/**
 * @file  uiox_kp_patch.h
 * @brief UIOX Live Kernel Patching — core patch engine.
 *
 * Public API for registering, enabling, disabling, and querying patches.
 *
 * Thread safety:
 *   uiox_kp_enable() / uiox_kp_disable() must be called with all CPUs
 *   quiesced (stop_machine equivalent) to prevent races with threads
 *   currently executing inside the patched function prologue.
 *   On UIOX single-CPU simulation this is a no-op.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_KP_PATCH_H
 #define UIOX_KP_PATCH_H
 
 #include "uiox_kp_types.h"
 #include "uiox_kp_arch.h"
 #include "uiox_kp_mem.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Engine initialisation
  * ====================================================================== */
 
 /**
  * Initialise the patch engine.
  * Registers the arch ops, initialises the trampoline pool, clears table.
  * Must be called once during kernel Stage 3 (device init).
  */
 uiox_kp_err_t uiox_kp_engine_init   (void);
 
 /**
  * Shut down the patch engine — disables and unregisters all patches.
  */
 void          uiox_kp_engine_deinit (void);
 
 /* =========================================================================
  * Patch lifecycle
  * ====================================================================== */
 
 /**
  * Register a patch descriptor with the engine.
  * Does NOT yet modify the original function.
  * @param patch  Caller-allocated patch descriptor (persists until unregister).
  */
 uiox_kp_err_t uiox_kp_register   (uiox_kp_patch_t *patch);
 
 /**
  * Unregister a patch. Patch must be in DISABLED state.
  */
 uiox_kp_err_t uiox_kp_unregister (uiox_kp_patch_t *patch);
 
 /**
  * Enable a registered patch:
  *   1. Build trampoline (so new_func can call original).
  *   2. Make orig_func memory writable.
  *   3. Write jump stub at orig_func.
  *   4. Flush instruction caches.
  *   5. Restore memory protection.
  * State: REGISTERED → ENABLED
  */
 uiox_kp_err_t uiox_kp_enable     (uiox_kp_patch_t *patch);
 
 /**
  * Disable an enabled patch:
  *   1. Restore original bytes at orig_func.
  *   2. Flush instruction caches.
  * State: ENABLED → DISABLED
  */
 uiox_kp_err_t uiox_kp_disable    (uiox_kp_patch_t *patch);
 
 /* =========================================================================
  * Module-level operations
  * ====================================================================== */
 
 /**
  * Load a patch module: register + enable all patches in @mod.
  */
 uiox_kp_err_t uiox_kp_module_load   (uiox_kp_module_t *mod);
 
 /**
  * Unload a patch module: disable + unregister all patches in @mod.
  */
 uiox_kp_err_t uiox_kp_module_unload (uiox_kp_module_t *mod);
 
 /* =========================================================================
  * Query / introspection
  * ====================================================================== */
 
 /** Find a patch by original function address. Returns NULL if not found. */
 uiox_kp_patch_t *uiox_kp_find_by_addr(uintptr_t orig_func);
 
 /** Find a patch by name. */
 uiox_kp_patch_t *uiox_kp_find_by_name(const char *name);
 
 /** Return number of currently registered patches. */
 uint32_t         uiox_kp_count       (void);
 
 /** Return number of currently enabled patches. */
 uint32_t         uiox_kp_active_count(void);
 
 /** Print patch table to kernel console. */
 void             uiox_kp_print_table (void);
 
 /* =========================================================================
  * CPU quiesce (stop_machine equivalent)
  * On UIOX single-CPU: no-op stubs.
  * On SMP: stops all secondary CPUs before patch install.
  * ====================================================================== */
 
 void uiox_kp_stop_machine  (void);  /**< Quiesce all CPUs              */
 void uiox_kp_start_machine  (void); /**< Resume all CPUs               */
 
 /* =========================================================================
  * Convenience macro — define and immediately enable a patch
  * ====================================================================== */
 
 #define UIOX_KP_LOAD_PATCH(orig_fn, new_fn)                          \
     do {                                                               \
         static uiox_kp_patch_t _kp_patch_ = UIOX_KP_PATCH(           \
             #orig_fn, orig_fn, new_fn);                               \
         uiox_kp_register(&_kp_patch_);                                \
         uiox_kp_enable  (&_kp_patch_);                                \
     } while (0)
 
 /* =========================================================================
  * Syscall interface (40_SystemCallInterface)
  * ====================================================================== */
 
 #define SYS_KPATCH_LOAD     210u
 #define SYS_KPATCH_UNLOAD   211u
 #define SYS_KPATCH_STATUS   212u
 #define SYS_KPATCH_LIST     213u
 
 /* Syscall handlers — registered in 40_SystemCallInterface dispatch table */
 long sys_kpatch_load  (long mod_addr, long flags, long a2, long a3);
 long sys_kpatch_unload(long mod_addr, long flags, long a2, long a3);
 long sys_kpatch_status(long name_ptr, long buf,   long a2, long a3);
 long sys_kpatch_list  (long buf,      long max,   long a2, long a3);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KP_PATCH_H */
 