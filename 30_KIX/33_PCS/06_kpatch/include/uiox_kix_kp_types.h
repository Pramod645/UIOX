/**
 * @file  uiox_kix_kp_types.h
 * @brief UIOX Live Kernel Patching — base types, error codes, patch descriptor.
 *
 * Integrates with:
 *   33_ProcessControlSubsystem  — stop_machine / quiesce all CPUs
 *   34_CAS                      — atomic operations during patch install
 *   10_Arch                     — cache flush, instruction sync
 *   40_SystemCallInterface      — sys_kpatch_load / sys_kpatch_unload
 *
 * @version 1.1.0
 * @date    2026-10-02
 */

 #ifndef UIOX_KIX_KP_TYPES_H
 #define UIOX_KIX_KP_TYPES_H
 
#include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Error codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_KIX_KP_OK              =  0,
     UIOX_KIX_KP_ERR_INVAL       = -1,
     UIOX_KIX_KP_ERR_NOMEM       = -2,
     UIOX_KIX_KP_ERR_ALREADY     = -3,  /**< Function already patched    */
     UIOX_KIX_KP_ERR_NOTFOUND    = -4,  /**< Patch not registered        */
     UIOX_KIX_KP_ERR_BUSY        = -5,  /**< Patch table full            */
     UIOX_KIX_KP_ERR_FAULT       = -6,  /**< Memory write fault          */
     UIOX_KIX_KP_ERR_UNSUP       = -7,  /**< Arch not supported          */
     UIOX_KIX_KP_ERR_ACTIVE      = -8,  /**< Cannot unload active patch  */
     UIOX_KIX_KP_ERR_PERM        = -9,  /**< Permission denied           */
     UIOX_KIX_KP_EFAULT          = -14, /**< bad user address (arg.h)    */
 } uiox_kix_kp_err_t;
 
 /* =========================================================================
  * Patch state machine
  * ====================================================================== */
 
 typedef enum {
     UIOX_KIX_KP_STATE_UNREGISTERED = 0,
     UIOX_KIX_KP_STATE_REGISTERED,      /**< Registered, not yet applied */
     UIOX_KIX_KP_STATE_ENABLED,         /**< Jump installed, active      */
     UIOX_KIX_KP_STATE_DISABLED,        /**< Original code restored      */
     UIOX_KIX_KP_STATE_ERROR,
 } uiox_kix_kp_state_t;
 
 /* =========================================================================
  * Architecture identifiers
  * ====================================================================== */
 
 typedef enum {
     UIOX_KIX_KP_ARCH_ARM64   = 0,
     UIOX_KIX_KP_ARCH_ARM32   = 1,
     UIOX_KIX_KP_ARCH_X86_64  = 2,
     UIOX_KIX_KP_ARCH_RISCV64 = 3,
 } uiox_kix_kp_arch_t;
 
 /* =========================================================================
  * Jump stub size constants
  *
  * ARM64:   4 bytes  (B <offset> if within ±128 MB)
  *          16 bytes (LDR x16, #8; BR x16; .quad target) for far targets
  * ARM32:   4 bytes  (B <offset>) near
  *          8 bytes  (LDR pc, [pc, #-4]; .word target) far
  * x86_64:  5 bytes  (E9 <rel32>) near (±2 GB)
  *          14 bytes (FF 25 00 00 00 00; .quad target) far
  * RISCV64: 4 bytes  (JAL x0, <offset>) near (±1 MB)
  *          8 bytes  (AUIPC x6, hi20; JALR x0, x6, lo12) far
  * ====================================================================== */
 
 #define UIOX_KIX_KP_JMP_SIZE_ARM64_NEAR  4u
 #define UIOX_KIX_KP_JMP_SIZE_ARM64_FAR  16u
 #define UIOX_KIX_KP_JMP_SIZE_ARM32_NEAR  4u
 #define UIOX_KIX_KP_JMP_SIZE_ARM32_FAR   8u
 #define UIOX_KIX_KP_JMP_SIZE_X86_NEAR    5u
 #define UIOX_KIX_KP_JMP_SIZE_X86_FAR    14u
 #define UIOX_KIX_KP_JMP_SIZE_RISCV64_NEAR  4u
 #define UIOX_KIX_KP_JMP_SIZE_RISCV64_FAR   8u
 
 /* Maximum saved bytes (must be >= the largest jump stub on any arch).
  * arm64's far form is the widest at 16, so this is exactly enough. */
 #define UIOX_KIX_KP_SAVED_BYTES_MAX     16u
 
 /* =========================================================================
  * Patch descriptor
  * ====================================================================== */
 
 #define UIOX_KIX_KP_NAME_LEN    48u
 #define UIOX_KIX_KP_MAX_PATCHES 64u
 
 typedef struct uiox_kix_kp_patch {
     /* Identity */
     char       name[UIOX_KIX_KP_NAME_LEN];
     uint32_t   version;
 
     /* Addresses */
     uintptr_t  orig_func;
     uintptr_t  new_func;
     uintptr_t  trampoline;
 
     /* Saved original bytes */
     uint8_t    saved_bytes[UIOX_KIX_KP_SAVED_BYTES_MAX];
     uint8_t    saved_len;
 
     /**
      * Trampoline bytes ALLOCATED, not bytes used.
      *
      * Recorded at enable time so disable frees the same count.  The two
      * differ: allocation is saved_len + the arch's max_jump_size, and the
      * trampoline actually built may be shorter if the near branch fits.
      */
     uint8_t    tramp_alloc_len;
 
     /* State */
     uiox_kix_kp_state_t state;
     uint64_t        install_time_ms;
     uint32_t        call_count;
 
     /* Chain */
     struct uiox_kix_kp_patch *next;
 } uiox_kix_kp_patch_t;
 
 /* =========================================================================
  * Patch module descriptor
  * ====================================================================== */
 
 #define UIOX_KIX_KP_MODULE_NAME_LEN 32u
 #define UIOX_KIX_KP_MAX_MODULE_PATCHES 16u
 
 typedef struct {
     char             name[UIOX_KIX_KP_MODULE_NAME_LEN];
     uint32_t         version;
     uiox_kix_kp_patch_t  patches[UIOX_KIX_KP_MAX_MODULE_PATCHES];
     uint32_t         num_patches;
     bool             loaded;
 } uiox_kix_kp_module_t;
 
 /* =========================================================================
  * Utility macros
  * ====================================================================== */
 
 #define UIOX_KIX_KP_UNUSED(x)       ((void)(x))
 #define UIOX_KIX_KP_ARRAY_SIZE(a)   (sizeof(a)/sizeof((a)[0]))
 
 #define UIOX_KIX_KP_PATCH(fname, orig, repl) \
     { .name = (fname), .version = 1u,    \
       .orig_func = (uintptr_t)(orig),     \
       .new_func  = (uintptr_t)(repl),     \
       .state = UIOX_KIX_KP_STATE_UNREGISTERED }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KP_TYPES_H */
 