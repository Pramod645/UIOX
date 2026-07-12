/*
 * 02_FwHal/include/uiox_soc_pm.h
 * UIOX SoC abstraction layer — power domains and reset controller.
 *
 * Provides a thin, architecture-independent layer over:
 *   ARM64  — PSCI (CPU_ON / CPU_OFF / SYSTEM_RESET / SYSTEM_OFF)
 *   ARM32  — PSCI SMC or MMIO power controller
 *   x86-64 — ACPI S-states, FADT, port 0xB004 (QEMU poweroff)
 *   RISC-V — SBI SRST extension (sbi_system_reset)
 */
#ifndef UIOX_SOC_PM_H
#define UIOX_SOC_PM_H

#include "uiox_soc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Power domain identifiers
 * ====================================================================== */
typedef enum {
    UIOX_PD_CPU0        = 0,   /**< Primary CPU core                      */
    UIOX_PD_CPU1        = 1,   /**< Secondary CPU cores (SMP)             */
    UIOX_PD_CPU2        = 2,
    UIOX_PD_CPU3        = 3,
    UIOX_PD_GPU         = 4,
    UIOX_PD_DSP         = 5,
    UIOX_PD_USB         = 6,
    UIOX_PD_PCIE        = 7,
    UIOX_PD_DRAM        = 8,
    UIOX_PD_PERIPH      = 9,   /**< General peripheral power rail         */
    UIOX_PD__COUNT      = 10,
} uiox_pd_id_t;

/* =========================================================================
 * Reset domain identifiers
 * ====================================================================== */
typedef enum {
    UIOX_RST_GLOBAL     = 0,   /**< Full SoC reset                        */
    UIOX_RST_CPU        = 1,   /**< CPU subsystem reset                   */
    UIOX_RST_PERIPH     = 2,   /**< Peripheral bus reset                  */
    UIOX_RST_UART0      = 3,
    UIOX_RST_TIMER      = 4,
    UIOX_RST_GIC        = 5,
    UIOX_RST_ETH        = 6,
    UIOX_RST_USB        = 7,
    UIOX_RST__COUNT     = 8,
} uiox_rst_id_t;

/* =========================================================================
 * System-level power actions
 * ====================================================================== */
typedef enum {
    UIOX_PM_ACTION_POWEROFF  = 0, /**< Full system shutdown               */
    UIOX_PM_ACTION_REBOOT    = 1, /**< Warm reboot                        */
    UIOX_PM_ACTION_SUSPEND   = 2, /**< Suspend-to-RAM (S3)                */
    UIOX_PM_ACTION_HIBERNATE = 3, /**< Suspend-to-disk (S4)               */
} uiox_pm_action_t;

/* =========================================================================
 * PSCI / SBI constants
 * ====================================================================== */
/* ARM PSCI function IDs (SMC32) */
#define PSCI_VERSION            0x84000000u
#define PSCI_CPU_SUSPEND        0x84000001u
#define PSCI_CPU_OFF            0x84000002u
#define PSCI_CPU_ON             0x84000003u
#define PSCI_SYSTEM_OFF         0x84000008u
#define PSCI_SYSTEM_RESET       0x84000009u
#define PSCI_SYSTEM_RESET2      0x84000012u

/* RISC-V SBI SRST extension */
#define SBI_EXT_SRST            0x53525354u
#define SBI_SRST_SHUTDOWN       0u
#define SBI_SRST_COLD_REBOOT    1u
#define SBI_SRST_WARM_REBOOT    2u

/* x86 QEMU power-off port */
#define X86_QEMU_PM_PORT        0xB004u
#define X86_QEMU_PM_POWEROFF    0x2000u
#define X86_ACPI_PM1A_CTL_PORT  0x0604u  /* QEMU Q35 ACPI PM1a control  */
#define X86_ACPI_SLP_EN         (1u << 13)
#define X86_ACPI_SLP_TYP_S5     (5u << 10)

/* =========================================================================
 * Power management context
 * ====================================================================== */
typedef struct {
    bool    pd_on[UIOX_PD__COUNT];  /**< Current power domain states      */
    bool    rst_asserted[UIOX_RST__COUNT];
    bool    initialized;
} uiox_pm_ctx_t;

/* =========================================================================
 * Power management API
 * ====================================================================== */

/** Initialise PM context. Call after SoC detect. */
int  uiox_pm_init          (uiox_pm_ctx_t *ctx, const uiox_soc_desc_t *soc);

/** Power on / off a domain. */
int  uiox_pm_domain_on     (uiox_pm_ctx_t *ctx, uiox_pd_id_t pd);
int  uiox_pm_domain_off    (uiox_pm_ctx_t *ctx, uiox_pd_id_t pd);

/** Assert / deassert a reset line. */
int  uiox_rst_assert       (uiox_pm_ctx_t *ctx, uiox_rst_id_t rst);
int  uiox_rst_deassert     (uiox_pm_ctx_t *ctx, uiox_rst_id_t rst);

/**
 * @brief Perform a system-level power action.
 *        On ARM64: issues PSCI SMC.
 *        On x86-64: writes to QEMU PM port / ACPI.
 *        On RISC-V: calls SBI SRST.
 *        This function does NOT return on POWEROFF / REBOOT.
 */
void uiox_pm_system        (uiox_pm_action_t action);

/** Secondary CPU bring-up (SMP) via PSCI CPU_ON / SBI HSM. */
int  uiox_pm_cpu_on        (uint32_t cpu_id, uint64_t entry_point,
                             uint64_t context_id);

/** Print PM state to console. */
void uiox_pm_print         (const uiox_pm_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_PM_H */
