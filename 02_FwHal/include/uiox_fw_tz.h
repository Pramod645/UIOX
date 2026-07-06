/**
 * @file    uiox_fw_tz.h
 * @brief   UIOX Firmware — ARM TrustZone / EL3 setup.
 *
 * Configures the ARM Secure/Non-Secure world partitioning:
 *   - TZPC (TrustZone Protection Controller) — marks memory/MMIO regions
 *   - TZASC (TrustZone Address Space Controller) — DRAM access control
 *   - SCR_EL3 — Secure Configuration Register
 *   - SCTLR_EL3 — minimal EL3 control
 *   - GIC-600 security groups (Group 0 = secure, Group 1 = normal)
 *   - SMC vector table (EL3 → OP-TEE or UIOX secure monitor)
 *
 * On x86-64 and ARM32 without TrustZone, all functions are stubs
 * that return UIOX_FW_ERR_NOTSUP gracefully.
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
 * SCR_EL3 bit definitions (ARM DDI 0487)
 * ====================================================================== */
#define SCR_EL3_NS     (1u <<  0)  /**< Non-Secure bit                  */
#define SCR_EL3_IRQ    (1u <<  1)  /**< IRQ routed to EL3               */
#define SCR_EL3_FIQ    (1u <<  2)  /**< FIQ routed to EL3               */
#define SCR_EL3_EA     (1u <<  3)  /**< External abort routed to EL3    */
#define SCR_EL3_SMD    (1u <<  7)  /**< SMC disable (EL1/EL2)           */
#define SCR_EL3_HCE    (1u <<  8)  /**< HVC enable                      */
#define SCR_EL3_SIF    (1u <<  9)  /**< Secure instruction fetch        */
#define SCR_EL3_RW     (1u << 10)  /**< EL2/EL1 = AArch64               */
#define SCR_EL3_ST     (1u << 11)  /**< Secure EL1 access to timer regs */
#define SCR_EL3_TWED   (1u << 29)  /**< Delayed WFE trap                */

/* =========================================================================
 * TrustZone memory region types
 * ====================================================================== */
typedef enum {
    UIOX_TZ_MEM_SECURE     = 0, /**< Accessible only from Secure World  */
    UIOX_TZ_MEM_NONSECURE  = 1, /**< Accessible from Non-Secure World   */
    UIOX_TZ_MEM_SHARED     = 2, /**< Accessible from both worlds        */
} uiox_tz_mem_type_t;

/* =========================================================================
 * TrustZone memory region descriptor
 * ====================================================================== */
#define UIOX_TZ_MAX_REGIONS  16u

typedef struct {
    uintptr_t         base;
    uint64_t          size;
    uiox_tz_mem_type_t type;
    char              name[24];
} uiox_tz_region_t;

/* =========================================================================
 * EL3 / TrustZone context
 * ====================================================================== */
typedef struct {
    bool            tz_supported;    /**< platform has TrustZone         */
    bool            el3_active;      /**< firmware entered from EL3      */
    bool            ns_configured;   /**< NS bit set for normal world    */
    bool            gic_configured;  /**< GIC security groups set        */
    uint64_t        scr_el3_val;     /**< value written to SCR_EL3       */
    uiox_tz_region_t regions[UIOX_TZ_MAX_REGIONS];
    uint8_t         num_regions;
    uintptr_t       vbar_el3;        /**< EL3 vector base address        */
    uintptr_t       optee_entry;     /**< OP-TEE / secure monitor entry  */
} uiox_fw_tz_ctx_t;

/* =========================================================================
 * SMC function IDs (PSCI and monitor)
 * ====================================================================== */
#define UIOX_SMC_FW_VERSION    0x80000000u
#define UIOX_SMC_TZ_CONFIGURE  0x80000001u
#define UIOX_SMC_TZ_MEM_SETUP  0x80000002u
#define UIOX_SMC_DEBUG_LOCK    0x80000003u

/* =========================================================================
 * TrustZone API
 * ====================================================================== */

/** Detect if TrustZone is supported on this platform.                   */
bool          uiox_fw_tz_supported     (void);

/** Initialise EL3 / TrustZone context.                                  */
uiox_fw_err_t uiox_fw_tz_init         (uiox_fw_tz_ctx_t *ctx);

/** Configure SCR_EL3 — sets RW, NS policy, IRQ/FIQ routing.            */
uiox_fw_err_t uiox_fw_tz_configure_scr(uiox_fw_tz_ctx_t *ctx,
                                          uint64_t scr_bits);

/** Register a memory region with the TrustZone controller.              */
uiox_fw_err_t uiox_fw_tz_add_region   (uiox_fw_tz_ctx_t *ctx,
                                          uintptr_t base, uint64_t size,
                                          uiox_tz_mem_type_t type,
                                          const char *name);

/** Apply all registered regions to TZASC / TZPC hardware.              */
uiox_fw_err_t uiox_fw_tz_apply        (uiox_fw_tz_ctx_t *ctx);

/** Configure GIC-600 security groups (Group 0 = secure FIQ).          */
uiox_fw_err_t uiox_fw_tz_configure_gic(uiox_fw_tz_ctx_t *ctx,
                                          uintptr_t gicd_base,
                                          uintptr_t gicc_base);

/** Install EL3 vector table for SMC handling.                           */
uiox_fw_err_t uiox_fw_tz_install_vbar (uiox_fw_tz_ctx_t *ctx,
                                          uintptr_t vbar_addr);

/** Register OP-TEE or custom secure monitor entry point.               */
uiox_fw_err_t uiox_fw_tz_register_monitor(uiox_fw_tz_ctx_t *ctx,
                                             uintptr_t entry);

/** Drop from EL3 to EL1 (Normal World) for kernel handoff.
 *  Sets SPSR_EL3 and ELR_EL3 then executes ERET.                       */
void __attribute__((noreturn))
              uiox_fw_tz_drop_to_el1  (uiox_fw_tz_ctx_t *ctx,
                                          uintptr_t el1_entry,
                                          uint64_t  dtb_pa);

/** Print TrustZone configuration via firmware UART.                    */
void          uiox_fw_tz_print        (const uiox_fw_tz_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_TZ_H */
