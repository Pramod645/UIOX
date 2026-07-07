/**
 * @file  uiox_fw_psci.h
 * @brief UIOX Firmware — PSCI (Power State Coordination Interface) registration.
 *
 * Implements PSCI 1.1 (ARM DEN0022D) as the EL3 power management interface.
 * Upper software (kernel, hypervisor) calls PSCI functions via HVC or SMC.
 * This module:
 *   - Defines the PSCI dispatch table (function ID → handler)
 *   - Implements all mandatory PSCI functions:
 *       PSCI_VERSION, CPU_ON, CPU_OFF, CPU_SUSPEND,
 *       AFFINITY_INFO, SYSTEM_OFF, SYSTEM_RESET
 *   - Provides the SMC/HVC handler that dispatches incoming calls
 *   - Manages the per-CPU power state machine
 *   - Integrates with uiox_fw_power.c for actual SoC power sequencing
 *
 * For ARM32: uses SMC calling convention (32-bit SMCCC).
 * For ARM64: uses both HVC and SMC paths.
 * For x86_64: PSCI does not apply; this module registers ACPI S-state
 *             handlers instead (stub).
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_PSCI_H
 #define UIOX_FW_PSCI_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PSCI function IDs (ARM SMCCC — 32-bit and 64-bit variants)
  * ====================================================================== */
 
 /* Version */
 #define PSCI_FN_VERSION                 0x84000000u
 
 /* CPU management */
 #define PSCI_FN32_CPU_SUSPEND           0x84000001u
 #define PSCI_FN32_CPU_OFF               0x84000002u
 #define PSCI_FN32_CPU_ON                0x84000003u
 #define PSCI_FN32_AFFINITY_INFO         0x84000004u
 #define PSCI_FN32_MIGRATE               0x84000005u
 #define PSCI_FN32_MIGRATE_INFO_TYPE     0x84000006u
 #define PSCI_FN32_MIGRATE_INFO_UP_CPU   0x84000007u
 
 /* System */
 #define PSCI_FN_SYSTEM_OFF              0x84000008u
 #define PSCI_FN_SYSTEM_RESET            0x84000009u
 #define PSCI_FN_SYSTEM_RESET2           0x84000012u
 
 /* 64-bit variants */
 #define PSCI_FN64_CPU_SUSPEND           0xC4000001u
 #define PSCI_FN64_CPU_ON                0xC4000003u
 #define PSCI_FN64_AFFINITY_INFO         0xC4000004u
 #define PSCI_FN64_MIGRATE               0xC4000005u
 #define PSCI_FN64_MIGRATE_INFO_UP_CPU   0xC4000007u
 
 /* PSCI features */
 #define PSCI_FN_FEATURES                0x8400000Au
 
 /* PSCI 1.1 */
 #define PSCI_FN_CPU_FREEZE              0x8400000Bu
 #define PSCI_FN_CPU_DEFAULT_SUSPEND     0x8400000Cu
 #define PSCI_FN64_CPU_DEFAULT_SUSPEND   0xC400000Cu
 #define PSCI_FN_NODE_HW_STATE           0x8400000Du
 #define PSCI_FN64_NODE_HW_STATE         0xC400000Du
 #define PSCI_FN_SYSTEM_SUSPEND          0x8400000Eu
 #define PSCI_FN64_SYSTEM_SUSPEND        0xC400000Eu
 #define PSCI_FN_SET_SUSPEND_MODE        0x8400000Fu
 #define PSCI_FN_STAT_RESIDENCY          0x84000010u
 #define PSCI_FN_STAT_COUNT              0x84000011u
 #define PSCI_FN_MEM_PROTECT             0x84000013u
 #define PSCI_FN_MEM_PROTECT_CHK         0x84000014u
 
 /* =========================================================================
  * PSCI return codes
  * ====================================================================== */
 
 #define PSCI_RET_SUCCESS            0
 #define PSCI_RET_NOT_SUPPORTED     (-1)
 #define PSCI_RET_INVALID_PARAMS    (-2)
 #define PSCI_RET_DENIED            (-3)
 #define PSCI_RET_ALREADY_ON        (-4)
 #define PSCI_RET_ON_PENDING        (-5)
 #define PSCI_RET_INTERNAL_FAILURE  (-6)
 #define PSCI_RET_NOT_PRESENT       (-7)
 #define PSCI_RET_DISABLED          (-8)
 #define PSCI_RET_INVALID_ADDRESS   (-9)
 
 /* =========================================================================
  * PSCI version encoding: [31:16]=major, [15:0]=minor
  * ====================================================================== */
 
 #define PSCI_VERSION_MAJOR          1u
 #define PSCI_VERSION_MINOR          1u
 #define PSCI_VERSION_VALUE          \
     ((PSCI_VERSION_MAJOR << 16u) | PSCI_VERSION_MINOR)
 
 /* =========================================================================
  * AFFINITY_INFO return values
  * ====================================================================== */
 
 #define PSCI_AFFINITY_LEVEL_ON      0u
 #define PSCI_AFFINITY_LEVEL_OFF     1u
 #define PSCI_AFFINITY_LEVEL_ON_PEND 2u
 
 /* =========================================================================
  * Per-CPU power state machine
  * ====================================================================== */
 
 typedef enum {
     UIOX_CPU_STATE_OFF      = 0,
     UIOX_CPU_STATE_ON       = 1,
     UIOX_CPU_STATE_SUSPEND  = 2,
     UIOX_CPU_STATE_ON_PEND  = 3,  /**< CPU_ON called, not yet running  */
 } uiox_cpu_state_t;
 
 #define UIOX_PSCI_MAX_CPUS          8u
 
 typedef struct {
     uiox_cpu_state_t  state;
     uint64_t          affinity;   /**< MPIDR_EL1 value for this CPU     */
     uintptr_t         warm_entry; /**< Warm-boot entry address          */
     uint64_t          context_id; /**< Passed in CPU_ON x2              */
     uint64_t          suspend_state; /**< Last CPU_SUSPEND state         */
 } uiox_psci_cpu_t;
 
 /* =========================================================================
  * PSCI handler function type
  * SMCCC ABI: a0=fn_id, a1–a3=args, return value in a0.
  * ====================================================================== */
 
 typedef int64_t (*uiox_psci_handler_t)(uint64_t a1, uint64_t a2,
                                          uint64_t a3, uint64_t a4);
 
 /* =========================================================================
  * PSCI dispatch table entry
  * ====================================================================== */
 
 typedef struct {
     uint32_t              fn_id;
     const char           *name;
     uiox_psci_handler_t   handler;
 } uiox_psci_entry_t;
 
 /* =========================================================================
  * PSCI registration context
  * ====================================================================== */
 
 typedef struct {
     uiox_psci_cpu_t   cpus[UIOX_PSCI_MAX_CPUS];
     uint32_t          num_cpus;
     bool              smc_enabled;   /**< true = SMC; false = HVC       */
     bool              initialized;
     /* Platform callbacks (filled by SoC-specific code) */
     void (*platform_cpu_on)  (uint32_t cpu_id, uintptr_t entry);
     void (*platform_cpu_off) (uint32_t cpu_id);
     void (*platform_reset)   (void) __attribute__((noreturn));
     void (*platform_off)     (void) __attribute__((noreturn));
     /* Stats */
     uint32_t  cpu_on_count;
     uint32_t  cpu_off_count;
     uint32_t  suspend_count;
     uint32_t  reset_count;
     uint32_t  unknown_fn_count;
 } uiox_psci_ctx_t;
 
 /* =========================================================================
  * PSCI API
  * ====================================================================== */
 
 /**
  * Initialise the PSCI subsystem and register all mandatory handlers.
  * Must be called from EL3 after TrustZone setup.
  *
  * @param ctx       PSCI context (caller-allocated, persistent).
  * @param num_cpus  Number of CPUs on the platform (1–8).
  * @param use_smc   true = SMC calling convention, false = HVC.
  */
 uiox_fw_err_t uiox_fw_psci_init       (uiox_psci_ctx_t *ctx,
                                          uint32_t num_cpus,
                                          bool use_smc);
 
 /**
  * Register a platform callback for CPU_ON warm boot.
  * Called by PSCI when a secondary CPU is powered on.
  */
 void          uiox_fw_psci_set_cpu_on (uiox_psci_ctx_t *ctx,
                                          void (*fn)(uint32_t, uintptr_t));
 
 /**
  * Register platform reset / power-off callbacks.
  */
 void uiox_fw_psci_set_reset(uiox_psci_ctx_t *ctx,
    void __attribute__((noreturn)) (*fn)(void));
void uiox_fw_psci_set_off  (uiox_psci_ctx_t *ctx,
    void __attribute__((noreturn)) (*fn)(void));
 
 /**
  * SMC/HVC dispatch entry point.
  * Called from the EL3 synchronous exception handler.
  *
  * @param fn_id  Function ID from x0 / r0.
  * @param a1–a3  Arguments from x1–x3 / r1–r3.
  * @return       Return value placed back in x0 / r0.
  */
 int64_t       uiox_fw_psci_dispatch   (uiox_psci_ctx_t *ctx,
                                          uint64_t fn_id,
                                          uint64_t a1, uint64_t a2,
                                          uint64_t a3);
 
 /** Query whether @fn_id is a supported PSCI function. */
 bool          uiox_fw_psci_supported  (uint32_t fn_id);
 
 /** Get current power state of CPU @cpu_id. */
 uiox_cpu_state_t uiox_fw_psci_cpu_state(const uiox_psci_ctx_t *ctx,
                                           uint32_t cpu_id);
 
 /** Print PSCI registration table and CPU states to debug UART. */
 void          uiox_fw_psci_print      (const uiox_psci_ctx_t *ctx);
 
 /* =========================================================================
  * Individual PSCI handler declarations (visible for unit testing)
  * ====================================================================== */
 
 int64_t uiox_psci_version         (uint64_t a1, uint64_t a2,
                                      uint64_t a3, uint64_t a4);
 int64_t uiox_psci_cpu_on          (uint64_t a1, uint64_t a2,
                                      uint64_t a3, uint64_t a4);
 int64_t uiox_psci_cpu_off         (uint64_t a1, uint64_t a2,
                                      uint64_t a3, uint64_t a4);
 int64_t uiox_psci_cpu_suspend     (uint64_t a1, uint64_t a2,
                                      uint64_t a3, uint64_t a4);
 int64_t uiox_psci_affinity_info   (uint64_t a1, uint64_t a2,
                                      uint64_t a3, uint64_t a4);
 int64_t uiox_psci_system_off      (uint64_t a1, uint64_t a2,
                                      uint64_t a3, uint64_t a4);
 int64_t uiox_psci_system_reset    (uint64_t a1, uint64_t a2,
                                      uint64_t a3, uint64_t a4);
 int64_t uiox_psci_features        (uint64_t a1, uint64_t a2,
                                      uint64_t a3, uint64_t a4);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_PSCI_H */
 