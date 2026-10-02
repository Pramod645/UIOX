/**
 * @file  uiox_kix_sec_types.h
 * @brief UIOX Security — base types, error codes, ASLR entropy constants,
 *        MAC label structures, and policy decision codes.
 *
 * Covers two orthogonal security mechanisms:
 *
 *   ASLR (Address Space Layout Randomisation)
 *     — randomises base addresses of stack, heap, mmap, vDSO, and kernel
 *       text on every exec(), reducing exploitability of memory-corruption
 *       bugs by making target addresses unpredictable.
 *
 *   MAC (Mandatory Access Control)
 *     — label-based policy engine inspired by SELinux / SMACK / AppArmor.
 *       Every subject (process) and object (file, socket, IPC endpoint)
 *       carries a uiox_mac_label_t. All access decisions go through
 *       uiox_mac_check() before the kernel grants access.
 *
 * ── what changed in 1.0.1 ─────────────────────────────────────────────
 *   The three system includes are gone:
 *
 *       #include <stdint.h>
 *       #include <stdbool.h>
 *       #include <stddef.h>
 *
 *   This was the only file in the module pulling in system headers, while
 *   its four siblings define their own mp_/mc_/sec_ helpers and declare
 *   uiox_fw_printf extern rather than include <stdio.h>.  Under the
 *   module Makefiles' -nostdinc those three includes do not resolve and
 *   the module stops at its first compile.
 *
 *   Types now come from the BSP authority, 10_BSP/03_SoC/include/
 *   uiox_base_types.h.
 *
 * @version 1.0.1
 * @date    2026-10-02
 */
#ifndef UIOX_SEC_TYPES_H
#define UIOX_SEC_TYPES_H

#include "uiox_base_types.h"   /* uint8_t/32/64, size_t, bool — BSP authority */

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ASLR level constants (for uiox_sec_init / sysctl)
 * ====================================================================== */
#define UIOX_ASLR_LEVEL_OFF      0u
#define UIOX_ASLR_LEVEL_PARTIAL  1u
#define UIOX_ASLR_LEVEL_FULL     2u  /**< Default: exec+stack+heap+mmap+vDSO */
#define UIOX_ASLR_LEVEL_KSTACK   3u  /**< Full + kernel stack               */

/* =========================================================================
 * Error codes
 * ====================================================================== */
typedef enum {
    UIOX_SEC_OK             =  0,
    UIOX_SEC_ERR_INVAL      = -1,
    UIOX_SEC_ERR_NOMEM      = -2,
    UIOX_SEC_ERR_PERM       = -3,   /**< MAC policy denied               */
    UIOX_SEC_ERR_BADLABEL   = -4,   /**< Malformed or unknown label       */
    UIOX_SEC_ERR_NOPOLICY   = -5,   /**< No policy loaded                 */
    UIOX_SEC_ERR_OVERFLOW   = -6,
    UIOX_SEC_ERR_NOTFOUND   = -7,
    UIOX_SEC_ERR_BADMAGIC   = -8,
    UIOX_SEC_ERR_READONLY   = -9,   /**< Policy is sealed / immutable     */
    UIOX_SEC_ERR_RANGE      = -10,  /**< Value out of entropy range       */
} uiox_sec_err_t;

/* =========================================================================
 * ASLR — entropy and region identifiers
 * ====================================================================== */

#define UIOX_ASLR_ENTROPY_MIN_BITS    8u
#define UIOX_ASLR_ENTROPY_MAX_BITS   32u

#define UIOX_ASLR_STACK_BITS         20u
#define UIOX_ASLR_HEAP_BITS          13u
#define UIOX_ASLR_MMAP_BITS          28u
#define UIOX_ASLR_EXEC_BITS          28u   /**< PIE executable base        */
#define UIOX_ASLR_VDSO_BITS          8u
#define UIOX_ASLR_KSTACK_BITS        10u   /**< Kernel stack per-thread    */

#define UIOX_ASLR_PAGE_SHIFT         12u
#define UIOX_ASLR_PAGE_SIZE          (1u << UIOX_ASLR_PAGE_SHIFT)

typedef enum {
    UIOX_ASLR_REGION_EXEC    = 0,   /**< PIE executable base              */
    UIOX_ASLR_REGION_STACK   = 1,   /**< User-space stack                 */
    UIOX_ASLR_REGION_HEAP    = 2,   /**< brk() heap start                 */
    UIOX_ASLR_REGION_MMAP    = 3,   /**< mmap() base (libs, anon maps)    */
    UIOX_ASLR_REGION_VDSO    = 4,   /**< vDSO / vvar pages                */
    UIOX_ASLR_REGION_KSTACK  = 5,   /**< Kernel thread stack              */
    UIOX_ASLR_REGION__COUNT  = 6,
} uiox_aslr_region_t;

typedef struct {
    uiox_aslr_region_t  region;
    uint8_t             entropy_bits;  /**< Randomness injected (bits)     */
    uint64_t            base_hint;     /**< Canonical base (pre-ASLR)      */
    uint64_t            randomised;    /**< Final randomised address        */
    bool                enabled;
} uiox_aslr_region_cfg_t;

typedef struct {
    uiox_aslr_region_cfg_t  regions[UIOX_ASLR_REGION__COUNT];
    uint64_t                exec_base;
    uint64_t                stack_base;
    uint64_t                heap_base;
    uint64_t                mmap_base;
    uint64_t                vdso_base;
    uint64_t                kstack_base;
    uint32_t                generation;   /**< Incremented on each exec()  */
} uiox_aslr_mm_t;

/* =========================================================================
 * MAC — label and policy types
 * ====================================================================== */

#define UIOX_MAC_LABEL_LEN       64u
#define UIOX_MAC_CONTEXT_LEN    128u
#define UIOX_MAC_MAX_RULES      512u
#define UIOX_MAC_MAX_TYPES       64u
#define UIOX_MAC_HASH_LEN        32u    /**< SHA-256 of policy binary      */

typedef enum {
    UIOX_MAC_MODE_DISABLED   = 0,  /**< No MAC checks — permissive dev     */
    UIOX_MAC_MODE_PERMISSIVE = 1,  /**< Log denials; do not enforce        */
    UIOX_MAC_MODE_ENFORCING  = 2,  /**< Deny and log on policy violation   */
} uiox_mac_mode_t;

/** Security label.  NOTE ON level — type enforcement is what the engine
 *  implements; the MLS/MCS sensitivity stored below is carried and parsed
 *  but never read by uiox_mac_check(), because uiox_mac_rule_t has no
 *  level field and the rule match is four-way.  Either the rule struct
 *  gains a level field and the match becomes five-way, or this field and
 *  the ":sN" suffix are reserved, not enforced. */
typedef struct {
    char      context[UIOX_MAC_CONTEXT_LEN]; /**< "user:role:type:level"  */
    uint32_t  type_id;                        /**< Resolved type index     */
    uint32_t  level;                          /**< MLS/MCS lvl — see note  */
    bool      valid;
} uiox_mac_label_t;

typedef enum {
    UIOX_MAC_PERM_NONE      = 0u,
    UIOX_MAC_PERM_READ      = (1u << 0),
    UIOX_MAC_PERM_WRITE     = (1u << 1),
    UIOX_MAC_PERM_EXEC      = (1u << 2),
    UIOX_MAC_PERM_CREATE    = (1u << 3),
    UIOX_MAC_PERM_DELETE    = (1u << 4),
    UIOX_MAC_PERM_APPEND    = (1u << 5),
    UIOX_MAC_PERM_CONNECT   = (1u << 6),
    UIOX_MAC_PERM_BIND      = (1u << 7),
    UIOX_MAC_PERM_SEND      = (1u << 8),
    UIOX_MAC_PERM_RECV      = (1u << 9),
    UIOX_MAC_PERM_PTRACE    = (1u << 10),
    UIOX_MAC_PERM_SETLABEL  = (1u << 11),
    UIOX_MAC_PERM_ALL       = 0x00000FFFu,
} uiox_mac_perm_t;

typedef enum {
    UIOX_MAC_CLASS_FILE     = 0,
    UIOX_MAC_CLASS_DIR      = 1,
    UIOX_MAC_CLASS_SOCKET   = 2,
    UIOX_MAC_CLASS_IPC      = 3,
    UIOX_MAC_CLASS_PROCESS  = 4,
    UIOX_MAC_CLASS_DEVICE   = 5,
    UIOX_MAC_CLASS_KERNEL   = 6,
    UIOX_MAC_CLASS__COUNT   = 7,
} uiox_mac_class_t;

/** One allow/deny rule.  The match key is (subject_type, object_type,
 *  obj_class) — see the note on uiox_mac_label_t::level. */
typedef struct {
    uint32_t        subject_type;   /**< Source type index (subject)       */
    uint32_t        object_type;    /**< Target type index (object)        */
    uiox_mac_class_t obj_class;
    uiox_mac_perm_t  allow;         /**< Permitted permissions bitmask     */
    uiox_mac_perm_t  audit;         /**< Log even if allowed               */
    bool             active;
} uiox_mac_rule_t;

typedef enum {
    UIOX_MAC_ALLOW   = 0,  /**< Access granted                             */
    UIOX_MAC_DENY    = 1,  /**< Access denied                              */
    UIOX_MAC_AUDIT   = 2,  /**< Allowed but logged (permissive violation)  */
} uiox_mac_decision_t;

typedef struct {
    uiox_mac_label_t    subject;
    uiox_mac_label_t    object;
    uiox_mac_class_t    obj_class;
    uiox_mac_perm_t     requested;
    uiox_mac_decision_t decision;
    uint64_t            timestamp_ms;
    uint32_t            pid;
    char                comm[16];   /**< Process name                      */
} uiox_mac_audit_entry_t;

/* =========================================================================
 * Helpers
 * ====================================================================== */
const char *uiox_mac_decision_str (uiox_mac_decision_t d);
const char *uiox_mac_class_str    (uiox_mac_class_t c);
const char *uiox_mac_mode_str     (uiox_mac_mode_t m);
const char *uiox_sec_err_str      (uiox_sec_err_t e);

#define UIOX_MAC_AUDIT_LOG_SIZE  256u


#ifdef __cplusplus
}
#endif
#endif /* UIOX_SEC_TYPES_H */
