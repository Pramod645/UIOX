/**
 * @file    uiox_fw_tz.h
 * @brief   UIOX Firmware — ARM TrustZone setup (EL3 / Secure World).
 *
 * Configures the ARM TrustZone security partitioning:
 *   - TZPC  (TrustZone Protection Controller) — memory window security
 *   - TZASC (TrustZone Address Space Controller) — DRAM region security
 *   - GIC   — interrupt routing between Secure and Non-Secure worlds
 *   - SCR_EL3 / SCTLR_EL3 — EL3 security control registers
 *   - ACTLR_EL3 / CPTR_EL3 — auxiliary + crypto access control
 *
 * This module is only compiled when __aarch64__ is defined.
 * On ARM32 a reduced set of TZ registers is used.
 * On x86-64 this module is a no-op stub.
 *
 * @version 1.0.0
 * @date    2026-07-06
 */
#ifndef UIOX_FW_TZ_H
#define UIOX_FW_TZ_H

#include "uiox_fw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * TrustZone memory region security attributes
 * ====================================================================== */
typedef enum {
    UIOX_TZ_MEM_SECURE     = 0,  /**< accessible only from Secure World */
    UIOX_TZ_MEM_NONSECURE  = 1,  /**< accessible from both worlds       */
    UIOX_TZ_MEM_INVALID    = 2,  /**< all accesses generate AXI error   */
} uiox_tz_mem_attr_t;

/* =========================================================================
 * TrustZone memory region descriptor
 * ====================================================================== */
typedef struct {
    uint64_t         base;
    uint64_t         size;
    uiox_tz_mem_attr_t attr;
    char             name[24];
} uiox_fw_tz_region_t;

/* =========================================================================
 * GIC interrupt security assignment
 * ====================================================================== */
typedef enum {
    UIOX_TZ_IRQ_SECURE     = 0,  /**< Group 0 — routed to FIQ in EL3   */
    UIOX_TZ_IRQ_NONSECURE  = 1,  /**< Group 1 — routed to IRQ in EL1   */
} uiox_tz_irq_sec_t;

/* =========================================================================
 * SCR_EL3 bit definitions (AArch64)
 * ====================================================================== */
#define UIOX_SCR_EL3_NS    (1u <<  0)  /**< Non-Secure state bit        */
#define UIOX_SCR_EL3_IRQ   (1u <<  1)  /**< IRQ taken to EL3            */
#define UIOX_SCR_EL3_FIQ   (1u <<  2)  /**< FIQ taken to EL3            */
#define UIOX_SCR_EL3_EA    (1u <<  3)  /**< External abort to EL3       */
#define UIOX_SCR_EL3_RW    (1u << 10)  /**< EL2/EL1 is AArch64          */
#define UIOX_SCR_EL3_ST    (1u << 11)  /**< Secure EL1 timer access     */
#define UIOX_SCR_EL3_TWI   (1u << 12)  /**< Trap WFI from EL2/EL1/EL0  */
#define UIOX_SCR_EL3_TWE   (1u << 13)  /**< Trap WFE from EL2/EL1/EL0  */
#define UIOX_SCR_EL3_HCE   (1u << 8)   /**< HVC instruction enable      */
#define UIOX_SCR_EL3_SIF   (1u <<  9)  /**< Secure instruction fetch    */

/* =========================================================================
 * TrustZone context
 * ====================================================================== */
#define UIOX_TZ_MAX_REGIONS  16u
#define UIOX_TZ_MAX_IRQS     32u

typedef struct {
    uiox_fw_tz_region_t regions[UIOX_TZ_MAX_REGIONS];
    uint8_t             num_regions;
    uint32_t            irq_group[UIOX_TZ_MAX_IRQS]; /**< UIOX_TZ_IRQ_*  */
    uint8_t             num_irqs;
    uint32_t            scr_el3_val;  /**< programmed SCR_EL3 value      */
    bool                tz_enabled;
    bool                el3_present;
    char                world[16];    /**< "SECURE" or "NONSECURE"        */
} uiox_fw_tz_ctx_t;

/* =========================================================================
 * API
 * ====================================================================== */

/**
 * Probe whether the CPU is at EL3 and TrustZone is available.
 * Must be called before any other TZ function.
 */
uiox_fw_err_t uiox_fw_tz_probe       (uiox_fw_tz_ctx_t *ctx);

/**
 * Full TrustZone setup sequence:
 *   1. Initialise SCR_EL3 / CPTR_EL3 / ACTLR_EL3
 *   2. Configure TZPC / TZASC memory windows
 *   3. Assign GIC interrupt groups
 *   4. Configure Secure Monitor Call (SMC) routing
 *   5. Set NS bit to prepare drop to EL1
 */
uiox_fw_err_t uiox_fw_tz_setup       (uiox_fw_tz_ctx_t *ctx);

/** Add a memory region to the TrustZone security map. */
uiox_fw_err_t uiox_fw_tz_add_region  (uiox_fw_tz_ctx_t    *ctx,
                                         uint64_t             base,
                                         uint64_t             size,
                                         uiox_tz_mem_attr_t   attr,
                                         const char          *name);

/** Assign a GIC interrupt to Secure or Non-Secure group. */
uiox_fw_err_t uiox_fw_tz_assign_irq  (uiox_fw_tz_ctx_t *ctx,
                                         uint32_t          irq,
                                         uiox_tz_irq_sec_t sec);

/**
 * Drop from EL3 to EL1 (Non-Secure) to continue normal boot.
 * Configures SPSR_EL3 and ELR_EL3, then issues ERET.
 * This function does NOT return — control passes to entry_pa.
 */
void __attribute__((noreturn))
      uiox_fw_tz_drop_to_el1         (uiox_fw_tz_ctx_t *ctx,
                                         uint64_t          entry_pa,
                                         uint64_t          arg0);

/** Print TrustZone configuration. */
void  uiox_fw_tz_print               (const uiox_fw_tz_ctx_t *ctx);

/* Low-level EL3 register access (internal use + arch drivers) */
uint64_t uiox_fw_read_scr_el3  (void);
void     uiox_fw_write_scr_el3 (uint64_t val);
uint32_t uiox_fw_current_el    (void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_TZ_H */
