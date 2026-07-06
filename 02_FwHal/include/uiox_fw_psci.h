/**
 * @file    uiox_fw_psci.h
 * @brief   UIOX Firmware — PSCI (Power State Coordination Interface).
 *
 * Implements the PSCI v1.1 specification (ARM DEN0022D) as a
 * firmware-resident SMC/HVC handler.  Supports:
 *   - PSCI_VERSION
 *   - CPU_SUSPEND     — enter CPU low-power state
 *   - CPU_OFF         — power down current CPU
 *   - CPU_ON          — power on a secondary CPU
 *   - AFFINITY_INFO   — query CPU affinity state
 *   - MIGRATE / MIGRATE_INFO_TYPE (stub)
 *   - SYSTEM_OFF      — system shutdown
 *   - SYSTEM_RESET    — system reset
 *   - SYSTEM_RESET2   — extended reset (v1.1)
 *   - PSCI_FEATURES   — feature query
 *   - PSCI_MEM_PROTECT — DMA protection query (v1.1)
 *
 * The PSCI handler is installed as the SMC dispatcher in EL3.
 * When the kernel (EL1) issues an SMC/HVC with a PSCI function ID,
 * the EL3 exception vector dispatches here.
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
 * 32-bit calling convention (SMC32) — W0 = function ID
 * 64-bit calling convention (SMC64) — X0 = function ID
 * ====================================================================== */
#define PSCI_FN_VERSION             0x84000000u
#define PSCI_FN_CPU_SUSPEND_32      0x84000001u
#define PSCI_FN_CPU_SUSPEND_64      0xC4000001u
#define PSCI_FN_CPU_OFF             0x84000002u
#define PSCI_FN_CPU_ON_32           0x84000003u
#define PSCI_FN_CPU_ON_64           0xC4000003u
#define PSCI_FN_AFFINITY_INFO_32    0x84000004u
#define PSCI_FN_AFFINITY_INFO_64    0xC4000004u
#define PSCI_FN_MIGRATE_32          0x84000005u
#define PSCI_FN_MIGRATE_64          0xC4000005u
#define PSCI_FN_MIGRATE_INFO_TYPE   0x84000006u
#define PSCI_FN_MIGRATE_INFO_UP_CPU_32 0x84000007u
#define PSCI_FN_MIGRATE_INFO_UP_CPU_64 0xC4000007u
#define PSCI_FN_SYSTEM_OFF          0x84000008u
#define PSCI_FN_SYSTEM_RESET        0x84000009u
#define PSCI_FN_SYSTEM_RESET2_32    0x84000012u
#define PSCI_FN_SYSTEM_RESET2_64    0xC4000012u
#define PSCI_FN_PSCI_FEATURES       0x8400000Au
#define PSCI_FN_MEM_PROTECT         0x84000013u
#define PSCI_FN_MEM_PROTECT_CHK_32  0x84000014u
#define PSCI_FN_MEM_PROTECT_CHK_64  0xC4000014u

/* =========================================================================
 * PSCI return codes
 * ====================================================================== */
#define PSCI_RET_SUCCESS            0
#define PSCI_RET_NOT_SUPPORTED     -1
#define PSCI_RET_INVALID_PARAMS    -2
#define PSCI_RET_DENIED            -3
#define PSCI_RET_ALREADY_ON        -4
#define PSCI_RET_ON_PENDING        -5
#define PSCI_RET_INTERNAL_FAILURE  -6
#define PSCI_RET_NOT_PRESENT       -7
#define PSCI_RET_DISABLED          -8
#define PSCI_RET_INVALID_ADDRESS   -9

/* =========================================================================
 * PSCI version
 * ====================================================================== */
#define PSCI_VERSION_MAJOR  1u
#define PSCI_VERSION_MINOR  1u
#define PSCI_VERSION_VAL    ((PSCI_VERSION_MAJOR << 16) | PSCI_VERSION_MINOR)

/* =========================================================================
 * Affinity states
 * ====================================================================== */
typedef enum {
    PSCI_AFF_STATE_ON         = 0,
    PSCI_AFF_STATE_OFF        = 1,
    PSCI_AFF_STATE_ON_PENDING = 2,
} psci_affinity_state_t;

/* =========================================================================
 * Per-CPU PSCI state record
 * ====================================================================== */
#define UIOX_PSCI_MAX_CPUS  8u

typedef struct {
    uint64_t              mpidr;        /**< CPU MPIDR affinity value    */
    psci_affinity_state_t state;
    uint64_t              warm_boot_pa; /**< entry address for CPU_ON    */
    uint64_t              context_id;   /**< opaque context from CPU_ON  */
    bool                  present;
} uiox_psci_cpu_t;

/* =========================================================================
 * PSCI context
 * ====================================================================== */
typedef struct {
    uiox_psci_cpu_t cpus[UIOX_PSCI_MAX_CPUS];
    uint32_t        num_cpus;
    bool            initialised;
    uint32_t        smc_count;     /**< total SMC calls handled         */
    uint32_t        cpu_on_count;
    uint32_t        cpu_off_count;
    uint32_t        suspend_count;
    uint32_t        reset_count;
    uint32_t        off_count;
} uiox_fw_psci_ctx_t;

/* =========================================================================
 * SMC call frame — passed by the EL3 exception handler
 * ====================================================================== */
typedef struct {
    uint64_t x[8];   /**< X0=fn_id, X1-X7=args; X0 written with return  */
} uiox_fw_smc_frame_t;

/* =========================================================================
 * API
 * ====================================================================== */

/**
 * Initialise the PSCI subsystem.
 * Enumerates CPUs from MPIDR, sets all secondaries to OFF state.
 * Installs the SMC/HVC exception handler in the EL3 vector table.
 *
 * @param ctx  caller-allocated context
 * @return UIOX_FW_OK on success
 */
uiox_fw_err_t uiox_fw_psci_init     (uiox_fw_psci_ctx_t *ctx);

/**
 * Main PSCI SMC dispatcher.
 * Called from the EL3 synchronous exception handler.
 * Reads frame->x[0] (function ID), dispatches, writes return in x[0].
 */
void          uiox_fw_psci_smc_handler(uiox_fw_psci_ctx_t *ctx,
                                         uiox_fw_smc_frame_t *frame);

/** Query the PSCI state of a CPU by MPIDR. */
psci_affinity_state_t
              uiox_fw_psci_affinity_info(const uiox_fw_psci_ctx_t *ctx,
                                           uint64_t mpidr);

/** Power on a secondary CPU (called internally by PSCI_CPU_ON handler). */
uiox_fw_err_t uiox_fw_psci_cpu_on   (uiox_fw_psci_ctx_t *ctx,
                                         uint64_t mpidr,
                                         uint64_t entry_point_pa,
                                         uint64_t context_id);

/** Power off the calling CPU. */
uiox_fw_err_t uiox_fw_psci_cpu_off  (uiox_fw_psci_ctx_t *ctx);

/** Enter CPU suspend (low-power) state. */
uiox_fw_err_t uiox_fw_psci_suspend  (uiox_fw_psci_ctx_t *ctx,
                                         uint32_t power_state,
                                         uint64_t entry_pa,
                                         uint64_t context_id);

/** System shutdown (SYSTEM_OFF). Does not return. */
void __attribute__((noreturn))
              uiox_fw_psci_system_off(uiox_fw_psci_ctx_t *ctx);

/** System reset (SYSTEM_RESET / SYSTEM_RESET2). Does not return. */
void __attribute__((noreturn))
              uiox_fw_psci_system_reset(uiox_fw_psci_ctx_t *ctx,
                                          uint32_t reset_type,
                                          uint64_t cookie);

/** Print PSCI statistics. */
void          uiox_fw_psci_print    (const uiox_fw_psci_ctx_t *ctx);

/** Register PSCI DT node information in the device tree (stub). */
uiox_fw_err_t uiox_fw_psci_dt_register(uiox_fw_psci_ctx_t *ctx,
                                          uint64_t dtb_pa);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_PSCI_H */
