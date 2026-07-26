/**
 * @file  uiox_kp_types.h
 * @brief UIOX Live Kernel Patching — base types, error codes, patch descriptor.
 *
 * Integrates with:
 *   33_ProcessControlSubsystem  — stop_machine / quiesce all CPUs
 *   34_CAS                      — atomic operations during patch install
 *   10_Arch                     — cache flush, instruction sync
 *   40_SystemCallInterface      — sys_kpatch_load / sys_kpatch_unload
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_KP_TYPES_H
 #define UIOX_KP_TYPES_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Error codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_KP_OK              =  0,
     UIOX_KP_ERR_INVAL       = -1,
     UIOX_KP_ERR_NOMEM       = -2,
     UIOX_KP_ERR_ALREADY     = -3,  /**< Function already patched        */
     UIOX_KP_ERR_NOTFOUND    = -4,  /**< Patch not registered            */
     UIOX_KP_ERR_BUSY        = -5,  /**< Patch table full                */
     UIOX_KP_ERR_FAULT       = -6,  /**< Memory write fault              */
     UIOX_KP_ERR_UNSUP       = -7,  /**< Arch not supported              */
     UIOX_KP_ERR_ACTIVE      = -8,  /**< Cannot unload active patch      */
     UIOX_KP_ERR_PERM        = -9,  /**< Permission denied               */
 } uiox_kp_err_t;
 
 /* =========================================================================
  * Patch state machine
  * ====================================================================== */
 
 typedef enum {
     UIOX_KP_STATE_UNREGISTERED = 0,
     UIOX_KP_STATE_REGISTERED,      /**< Registered, not yet applied     */
     UIOX_KP_STATE_ENABLED,         /**< Jump installed, patch active    */
     UIOX_KP_STATE_DISABLED,        /**< Original code restored          */
     UIOX_KP_STATE_ERROR,
 } uiox_kp_state_t;
 
 /* =========================================================================
  * Architecture identifiers
  * ====================================================================== */
 
 typedef enum {
     UIOX_KP_ARCH_ARM64  = 0,
     UIOX_KP_ARCH_ARM32  = 1,
     UIOX_KP_ARCH_X86_64 = 2,
 } uiox_kp_arch_t;
 
 /* =========================================================================
  * Jump stub size constants
  *
  * ARM64: 4 bytes  (B <offset> if within ±128 MB)
  *        16 bytes (LDR x16, #8; BR x16; .quad target) for far targets
  * ARM32: 4 bytes  (B <offset>) near
  *        8 bytes  (LDR pc, [pc, #0]; .word target) far
  * x86_64: 5 bytes (E9 <rel32>) near (±2 GB)
  *         14 bytes (FF 25 00 00 00 00; .quad target) far
  * ====================================================================== */
 
 #define UIOX_KP_JMP_SIZE_ARM64_NEAR  4u
 #define UIOX_KP_JMP_SIZE_ARM64_FAR  16u
 #define UIOX_KP_JMP_SIZE_ARM32_NEAR  4u
 #define UIOX_KP_JMP_SIZE_ARM32_FAR   8u
 #define UIOX_KP_JMP_SIZE_X86_NEAR    5u
 #define UIOX_KP_JMP_SIZE_X86_FAR    14u
 
 /* Maximum saved bytes (must be >= largest jump stub) */
 #define UIOX_KP_SAVED_BYTES_MAX     16u
 
 /* =========================================================================
  * Patch descriptor
  *
  * One descriptor per patched function. The patch engine maintains a table
  * of these, indexed by original function address.
  * ====================================================================== */
 
 #define UIOX_KP_NAME_LEN    48u
 #define UIOX_KP_MAX_PATCHES 64u
 
 typedef struct uiox_kp_patch {
     /* Identity */
     char       name[UIOX_KP_NAME_LEN];  /**< Human name, e.g. "fix_oops" */
     uint32_t   version;                  /**< Patch version (monotonic)    */
 
     /* Addresses */
     uintptr_t  orig_func;   /**< Address of function being patched         */
     uintptr_t  new_func;    /**< Address of replacement function           */
     uintptr_t  trampoline;  /**< Trampoline stub (call orig from new_func) */
 
     /* Saved original bytes */
     uint8_t    saved_bytes[UIOX_KP_SAVED_BYTES_MAX];
     uint8_t    saved_len;   /**< How many bytes were overwritten           */
 
     /* State */
     uiox_kp_state_t state;
     uint64_t        install_time_ms;  /**< Firmware uptime at install      */
     uint32_t        call_count;       /**< Times new_func has been called  */
 
     /* Chain */
     struct uiox_kp_patch *next;
 } uiox_kp_patch_t;
 
 /* =========================================================================
  * Patch module descriptor (groups related patches, like a .ko module)
  * ====================================================================== */
 
 #define UIOX_KP_MODULE_NAME_LEN 32u
 #define UIOX_KP_MAX_MODULE_PATCHES 16u
 
 typedef struct {
     char             name[UIOX_KP_MODULE_NAME_LEN];
     uint32_t         version;
     uiox_kp_patch_t  patches[UIOX_KP_MAX_MODULE_PATCHES];
     uint32_t         num_patches;
     bool             loaded;
 } uiox_kp_module_t;
 
 /* =========================================================================
  * Utility macros
  * ====================================================================== */
 
 #define UIOX_KP_UNUSED(x)       ((void)(x))
 #define UIOX_KP_ARRAY_SIZE(a)   (sizeof(a)/sizeof((a)[0]))
 
 /* Helper: define a patch entry */
 #define UIOX_KP_PATCH(fname, orig, repl) \
     { .name = (fname), .version = 1u,    \
       .orig_func = (uintptr_t)(orig),     \
       .new_func  = (uintptr_t)(repl),     \
       .state = UIOX_KP_STATE_UNREGISTERED }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KP_TYPES_H */
  