/**
 * @file    uiox_fw_psci.h
 * @brief   UIOX Firmware — PSCI (Power State Coordination Interface).
 *
 * Implements the ARM PSCI 1.1 specification (DEN0022D) as a firmware
 * service called via SMC (EL3) or HVC (EL2).
 *
 * Supported functions:
 *   PSCI_VERSION        (0x84000000)
 *   CPU_SUSPEND         (0x84000001)
 *   CPU_OFF             (0x84000002)
 *   CPU_ON              (0x84000003)
 *   AFFINITY_INFO       (0x84000004)
 *   MIGRATE_INFO_TYPE   (0x84000006)
 *   SYSTEM_OFF          (0x84000008)
 *   SYSTEM_RESET        (0x84000009)
 *   PSCI_FEATURES       (0x8400000A)
 *   CPU_FREEZE          (0x8400000B)
 *   CPU_DEFAULT_SUSPEND (0x8400000C)
 *   SYSTEM_RESET2       (0x84000012)
 *
 * On x86-64 all CPU power calls are routed to ACPI equivalents.
 * On ARM32 without EL3 support, PSCI is provided via HVC.
 *
 * @version 1.0.0
 * @date    2026-07-06
 */
#ifndef UIOX_FW_PSCI_H
#define UIOX_FW_PSCI_H

#include "uiox_fw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * PSCI function IDs (ARM DEN0022D §5)
 * ====================================================================== */
#define PSCI_FN_VERSION           0x84000000u
#define PSCI_FN_CPU_SUSPEND       0x84000001u
#define PSCI_FN_CPU_OFF           0x84000002u
#define PSCI_FN_CPU_ON            0x84000003u
#define PSCI_FN_AFFINITY_INFO     0x84000004u
#define PSCI_FN_MIGRATE           0x84000005u
#define PSCI_FN_MIGRATE_INFO_TYPE 0x84000006u
#define PSCI_FN_MIGRATE_INFO_UPPCPU 0x84000007u
#define PSCI_FN_SYSTEM_OFF        0x84000008u
#define PSCI_FN_SYSTEM_RESET      0x84000009u
#define PSCI_FN_FEATURES          0x8400000Au
#define PSCI_FN_CPU_FREEZE        0x8400000Bu
#define PSCI_FN_CPU_DEFAULT_SUSPEND 0x8400000Cu
#define PSCI_FN_NODE_HW_STATE     0x8400000Du
#define PSCI_FN_SYSTEM_SUSPEND    0x8400000Eu
#define PSCI_FN_SET_SUSPEND_MODE  0x8400000Fu
#define PSCI_FN_STAT_RESIDENCY    0x84000010u
#define PSCI_FN_STAT_COUNT        0x84000011u
#define PSCI_FN_SYSTEM_RESET2     0x84000012u
#define PSCI_FN_MEM_PROTECT       0x84000013u

/* =========================================================================
 * PSCI return codes (§5.2.2)
 * ====================================================================== */
#define PSCI_RET_SUCCESS          0
#define PSCI_RET_NOT_SUPPORTED   -1
#define PSCI_RET_INVALID_PARAMS  -2
#define PSCI_RET_DENIED          -3
#define PSCI_RET_ALREADY_ON      -4
#define PSCI_RET_ON_PENDING      -5
#define PSCI_RET_INTERNAL_FAILURE -6
#define PSCI_RET_NOT_PRESENT     -7
#define PSCI_RET_DISABLED        -8
#define PSCI_RET_INVALID_ADDRESS -9

/* =========================================================================
 * PSCI version (major.minor packed)
 * ====================================================================== */
#define PSCI_VERSION_MAJOR    1u
#define PSCI_VERSION_MINOR    1u
#define PSCI_VERSION \
    ((PSCI_VERSION_MAJOR << 16u) | PSCI_VERSION_MINOR)

/* =========================================================================
 * CPU affinity state (AFFINITY_INFO return values)
 * ====================================================================== */
typedef enum {
    PSCI_AFF_ON      = 0,
    PSCI_AFF_OFF     = 1,
    PSCI_AFF_ON_PEND = 2,
} uiox_psci_aff_state_t;

/* =========================================================================
 * Per-CPU PSCI record
 * ====================================================================== */
#define UIOX_PSCI_MAX_CPUS  8u

typedef struct {
    uint64_t             mpidr;         /**< CPU MPIDR / affinity value  */
    uiox_psci_aff_state_t state;
    uintptr_t            entry_point;   /**< set by CPU_ON               */
    uint64_t             context_id;
    uint32_t             suspend_count;
    uint32_t             on_count;
    uint32_t             off_count;
} uiox_psci_cpu_t;

/* =========================================================================
 * PSCI handler function type (called from EL3 SMC vector)
 * ====================================================================== */
typedef int64_t (*uiox_psci_handler_t)(uint64_t fn_id,
                                        uint64_t arg0,
                                        uint64_t arg1,
                                        uint64_t arg2);

/* =========================================================================
 * PSCI context
 * ====================================================================== */
typedef struct {
    uiox_psci_cpu_t  cpus[UIOX_PSCI_MAX_CPUS];
    uint8_t          num_cpus;
    bool             registered;
    bool             smc_enabled;       /**< using SMC (EL3)             */
    bool             hvc_enabled;       /**< using HVC (EL2 virt)        */
    uint32_t         total_cpu_on;
    uint32_t         total_cpu_off;
    uint32_t         total_suspend;
    uint32_t         total_system_reset;
    uintptr_t        warm_boot_entry;   /**< secondary CPU entry         */
} uiox_fw_psci_ctx_t;

/* =========================================================================
 * PSCI API
 * ====================================================================== */

/** Initialise PSCI context and register all CPUs.                       */
uiox_fw_err_t uiox_fw_psci_init      (uiox_fw_psci_ctx_t *ctx,
                                         uint8_t num_cpus,
                                         uintptr_t warm_boot_entry);

/** Register this firmware's PSCI handler into the EL3 SMC dispatch.    */
uiox_fw_err_t uiox_fw_psci_register  (uiox_fw_psci_ctx_t *ctx);

/** Main PSCI SMC dispatcher — called from EL3 vector on SMC instruction.
 *  @param fn_id  SMC function ID from x0/r0
 *  @param arg0   x1/r1
 *  @param arg1   x2/r2
 *  @param arg2   x3/r3
 *  @return PSCI return code placed in x0/r0                             */
int64_t       uiox_fw_psci_dispatch  (uiox_fw_psci_ctx_t *ctx,
                                         uint64_t fn_id,
                                         uint64_t arg0,
                                         uint64_t arg1,
                                         uint64_t arg2);

/* Individual PSCI function implementations */
uint32_t      uiox_fw_psci_version    (void);
int64_t       uiox_fw_psci_cpu_on     (uiox_fw_psci_ctx_t *ctx,
                                          uint64_t target_cpu,
                                          uintptr_t entry_point,
                                          uint64_t context_id);
int64_t       uiox_fw_psci_cpu_off    (uiox_fw_psci_ctx_t *ctx);
int64_t       uiox_fw_psci_cpu_suspend(uiox_fw_psci_ctx_t *ctx,
                                          uint32_t power_state,
                                          uintptr_t entry_point,
                                          uint64_t context_id);
int64_t       uiox_fw_psci_affinity_info(uiox_fw_psci_ctx_t *ctx,
                                            uint64_t target_affinity,
                                            uint32_t lowest_affinity_level);
void __attribute__((noreturn))
              uiox_fw_psci_system_off  (uiox_fw_psci_ctx_t *ctx);
void __attribute__((noreturn))
              uiox_fw_psci_system_reset(uiox_fw_psci_ctx_t *ctx);
int64_t       uiox_fw_psci_features   (uint32_t fn_id);

/** Set warm-boot entry address written to a per-CPU hold location.      */
void          uiox_fw_psci_set_entry  (uiox_fw_psci_ctx_t *ctx,
                                          uint8_t cpu_idx,
                                          uintptr_t entry);

/** Print PSCI state via firmware UART.                                  */
void          uiox_fw_psci_print      (const uiox_fw_psci_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_PSCI_H */
