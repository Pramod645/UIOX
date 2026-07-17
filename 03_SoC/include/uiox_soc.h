/*
 * 02_FwHal/include/uiox_soc.h
 * UIOX SoC abstraction layer — master umbrella include.
 *
 * Single include for all consumers. Pulls in:
 *   uiox_soc_types.h  — SoC ID enum, capability flags, descriptor struct
 *   uiox_soc_map.h    — MMIO base addresses and IRQ numbers
 *   uiox_soc_clk.h    — clock tree and PLL definitions
 *   uiox_soc_pm.h     — power domains and reset controller
 *
 * Usage:
 *   #include "../../02_FwHal/include/uiox_soc.h"
 *
 * The global SoC descriptor is populated by uiox_soc_init_<arch>()
 * and accessible via uiox_soc_get_desc().
 */
#ifndef UIOX_SOC_H
#define UIOX_SOC_H

#include "uiox_soc_types.h"
#include "uiox_soc_map.h"
#include "uiox_soc_clk.h"
#include "uiox_soc_pm.h"

/* Pull in the existing firmware secure boot header */
#include "uiox_fw_secboot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Global SoC init / detect API
 *
 * Called once from arch_init() of each architecture backend.
 * ====================================================================== */

/**
 * @brief Detect and initialise the SoC.
 *        Populates the global descriptor, clock context, and PM context.
 *        Calls the architecture-specific backend (uiox_soc_init_arm64 etc.)
 * @return UIOX_SOC_OK on success, negative error code on failure.
 */
int uiox_soc_init(void);

/**
 * @brief Tear down all SoC sub-systems (called at shutdown).
 */
void uiox_soc_fini(void);

/**
 * @brief Return a pointer to the populated global SoC descriptor.
 *        Returns NULL if uiox_soc_init() has not been called.
 */
const uiox_soc_desc_t *uiox_soc_get_desc(void);

/**
 * @brief Return a pointer to the global clock context.
 */
uiox_clk_ctx_t *uiox_soc_get_clk(void);

/**
 * @brief Return a pointer to the global power management context.
 */
uiox_pm_ctx_t *uiox_soc_get_pm(void);

/**
 * @brief Print a one-page SoC summary to the console.
 */
void uiox_soc_print(void);

/* =========================================================================
 * Architecture-specific init entry points
 * Implemented in 02_FwHal/src/uiox_soc_<arch>.c
 * ====================================================================== */
int  uiox_soc_init_arm64 (uiox_soc_desc_t *desc);
int  uiox_soc_init_arm32 (uiox_soc_desc_t *desc);
int  uiox_soc_init_x86   (uiox_soc_desc_t *desc);
int  uiox_soc_init_riscv64(uiox_soc_desc_t *desc);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_H */
