/*
 * 02_FwHal/include/uiox_soc_pm.h
 * UIOX SoC abstraction layer — power domains and reset controller.
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
    UIOX_SOC_PD_CPU0    = 0,
    UIOX_SOC_PD_CPU1    = 1,
    UIOX_SOC_PD_CPU2    = 2,
    UIOX_SOC_PD_CPU3    = 3,
    UIOX_SOC_PD_GPU     = 4,
    UIOX_SOC_PD_DSP     = 5,
    UIOX_SOC_PD_USB     = 6,
    UIOX_SOC_PD_PCIE    = 7,
    UIOX_SOC_PD_DRAM    = 8,
    UIOX_SOC_PD_PERIPH  = 9,
    UIOX_SOC_PD__COUNT  = 10,
} uiox_soc_pd_id_t;

#define UIOX_PD_CPU0    UIOX_SOC_PD_CPU0
#define UIOX_PD_CPU1    UIOX_SOC_PD_CPU1
#define UIOX_PD_CPU2    UIOX_SOC_PD_CPU2
#define UIOX_PD_CPU3    UIOX_SOC_PD_CPU3
#define UIOX_PD_GPU     UIOX_SOC_PD_GPU
#define UIOX_PD_DSP     UIOX_SOC_PD_DSP
#define UIOX_PD_USB     UIOX_SOC_PD_USB
#define UIOX_PD_PCIE    UIOX_SOC_PD_PCIE
#define UIOX_PD_DRAM    UIOX_SOC_PD_DRAM
#define UIOX_PD_PERIPH  UIOX_SOC_PD_PERIPH
#define UIOX_PD__COUNT  UIOX_SOC_PD__COUNT
typedef uiox_soc_pd_id_t uiox_pd_id_t;

/* =========================================================================
 * Reset domain identifiers
 * ====================================================================== */
typedef enum {
    UIOX_SOC_RST_GLOBAL  = 0,
    UIOX_SOC_RST_CPU     = 1,
    UIOX_SOC_RST_PERIPH  = 2,
    UIOX_SOC_RST_UART0   = 3,
    UIOX_SOC_RST_TIMER   = 4,
    UIOX_SOC_RST_GIC     = 5,
    UIOX_SOC_RST_ETH     = 6,
    UIOX_SOC_RST_USB     = 7,
    UIOX_SOC_RST__COUNT  = 8,
} uiox_soc_rst_id_t;

#define UIOX_RST_GLOBAL  UIOX_SOC_RST_GLOBAL
#define UIOX_RST_CPU     UIOX_SOC_RST_CPU
#define UIOX_RST_PERIPH  UIOX_SOC_RST_PERIPH
#define UIOX_RST_UART0   UIOX_SOC_RST_UART0
#define UIOX_RST_TIMER   UIOX_SOC_RST_TIMER
#define UIOX_RST_GIC     UIOX_SOC_RST_GIC
#define UIOX_RST_ETH     UIOX_SOC_RST_ETH
#define UIOX_RST_USB     UIOX_SOC_RST_USB
#define UIOX_RST__COUNT  UIOX_SOC_RST__COUNT
typedef uiox_soc_rst_id_t uiox_rst_id_t;

/* =========================================================================
 * System-level power actions
 * ====================================================================== */
typedef enum {
    UIOX_SOC_PM_POWEROFF  = 0,
    UIOX_SOC_PM_REBOOT    = 1,
    UIOX_SOC_PM_SUSPEND   = 2,
    UIOX_SOC_PM_HIBERNATE = 3,
} uiox_soc_pm_action_t;

#define UIOX_PM_ACTION_POWEROFF   UIOX_SOC_PM_POWEROFF
#define UIOX_PM_ACTION_REBOOT     UIOX_SOC_PM_REBOOT
#define UIOX_PM_ACTION_SUSPEND    UIOX_SOC_PM_SUSPEND
#define UIOX_PM_ACTION_HIBERNATE  UIOX_SOC_PM_HIBERNATE
typedef uiox_soc_pm_action_t uiox_pm_action_t;

/* =========================================================================
 * PSCI / SBI / ACPI constants
 * ====================================================================== */
#define PSCI_VERSION            0x84000000u
#define PSCI_CPU_SUSPEND        0x84000001u
#define PSCI_CPU_OFF            0x84000002u
#define PSCI_CPU_ON             0x84000003u
#define PSCI_SYSTEM_OFF         0x84000008u
#define PSCI_SYSTEM_RESET       0x84000009u
#define PSCI_SYSTEM_RESET2      0x84000012u

#define SBI_EXT_SRST            0x53525354u
#define SBI_SRST_SHUTDOWN       0u
#define SBI_SRST_COLD_REBOOT    1u
#define SBI_SRST_WARM_REBOOT    2u

#define X86_QEMU_PM_PORT        0xB004u
#define X86_QEMU_PM_POWEROFF    0x2000u
#define X86_ACPI_PM1A_CTL_PORT  0x0604u
#define X86_ACPI_SLP_EN         (1u << 13)
#define X86_ACPI_SLP_TYP_S5     (5u << 10)

/* =========================================================================
 * Power management context
 *
 * KEY FIX: use uiox_bool_t instead of raw 'bool'.
 * 'bool' requires <stdbool.h> which is not available in bare-metal builds.
 * uiox_bool_t is defined in uiox_soc_types.h (included above).
 * ====================================================================== */
typedef struct {
    uiox_bool_t  pd_on      [UIOX_SOC_PD__COUNT];
    uiox_bool_t  rst_asserted[UIOX_SOC_RST__COUNT];
    uiox_bool_t  initialized;
} uiox_pm_ctx_t;

/* =========================================================================
 * Power management API
 * ====================================================================== */

uiox_soc_err_t uiox_soc_pm_init       (uiox_pm_ctx_t        *ctx,
                                        const uiox_soc_desc_t *soc);
uiox_soc_err_t uiox_soc_pm_domain_on  (uiox_pm_ctx_t        *ctx,
                                        uiox_soc_pd_id_t      pd);
uiox_soc_err_t uiox_soc_pm_domain_off (uiox_pm_ctx_t        *ctx,
                                        uiox_soc_pd_id_t      pd);
uiox_soc_err_t uiox_soc_rst_assert    (uiox_pm_ctx_t        *ctx,
                                        uiox_soc_rst_id_t     rst);
uiox_soc_err_t uiox_soc_rst_deassert  (uiox_pm_ctx_t        *ctx,
                                        uiox_soc_rst_id_t     rst);
void           uiox_soc_pm_system     (uiox_soc_pm_action_t  action);
uiox_soc_err_t uiox_soc_pm_cpu_on     (uiox_uint32_t cpu_id,
                                        uiox_uint64_t entry_point,
                                        uiox_uint64_t context_id);
void           uiox_soc_pm_print      (const uiox_pm_ctx_t  *ctx);

/* Backwards-compatible inline wrappers */
static inline uiox_soc_err_t uiox_pm_init(uiox_pm_ctx_t *ctx,
    const uiox_soc_desc_t *soc)         { return uiox_soc_pm_init(ctx, soc); }
static inline uiox_soc_err_t uiox_pm_domain_on(uiox_pm_ctx_t *ctx,
    uiox_soc_pd_id_t pd)                { return uiox_soc_pm_domain_on(ctx, pd); }
static inline uiox_soc_err_t uiox_pm_domain_off(uiox_pm_ctx_t *ctx,
    uiox_soc_pd_id_t pd)                { return uiox_soc_pm_domain_off(ctx, pd); }
static inline uiox_soc_err_t uiox_rst_assert(uiox_pm_ctx_t *ctx,
    uiox_soc_rst_id_t rst)              { return uiox_soc_rst_assert(ctx, rst); }
static inline uiox_soc_err_t uiox_rst_deassert(uiox_pm_ctx_t *ctx,
    uiox_soc_rst_id_t rst)              { return uiox_soc_rst_deassert(ctx, rst); }
static inline void uiox_pm_system(uiox_soc_pm_action_t a)
                                         { uiox_soc_pm_system(a); }
static inline uiox_soc_err_t uiox_pm_cpu_on(uiox_uint32_t id,
    uiox_uint64_t ep, uiox_uint64_t ctx_id)
                                         { return uiox_soc_pm_cpu_on(id, ep, ctx_id); }
static inline void uiox_pm_print(const uiox_pm_ctx_t *ctx)
                                         { uiox_soc_pm_print(ctx); }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_PM_H */
