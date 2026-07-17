/*
 * 02_FwHal/include/uiox_soc_clk.h
 * UIOX SoC abstraction layer — clock tree, PLL definitions,
 * and clock-enable / frequency-query API.
 *
 * The clock subsystem is intentionally minimal: it provides enough
 * to configure the boot CPU frequency, UART baud divisor, and timer
 * period without requiring a full CCF (Common Clock Framework).
 */
#ifndef UIOX_SOC_CLK_H
#define UIOX_SOC_CLK_H

#include "uiox_soc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Clock identifiers
 * ====================================================================== */
typedef enum {
    UIOX_CLK_CPU0       = 0,   /**< Boot CPU (cluster 0, core 0)          */
    UIOX_CLK_CPU_ALL    = 1,   /**< All CPU cores                         */
    UIOX_CLK_BUS        = 2,   /**< System / AXI bus                      */
    UIOX_CLK_UART0      = 3,   /**< UART0 reference clock                 */
    UIOX_CLK_UART1      = 4,
    UIOX_CLK_TIMER0     = 5,   /**< Periodic timer reference              */
    UIOX_CLK_GIC        = 6,   /**< GIC / interrupt controller clock      */
    UIOX_CLK_ETH        = 7,   /**< Ethernet MAC clock                    */
    UIOX_CLK_USB        = 8,   /**< USB PHY reference                     */
    UIOX_CLK_EMMC       = 9,   /**< eMMC / SDIO clock                     */
    UIOX_CLK_PCIE       = 10,  /**< PCIe reference                        */
    UIOX_CLK_GENERIC_TIMER = 11, /**< ARM generic counter (CNTFRQ_EL0)   */
    UIOX_CLK__COUNT     = 12,
} uiox_clk_id_t;

/* =========================================================================
 * PLL configuration descriptor
 * ====================================================================== */
typedef struct {
    uint32_t    ref_hz;        /**< PLL input reference (Hz)              */
    uint32_t    mul;           /**< VCO multiply factor                   */
    uint32_t    div;           /**< VCO output divider                    */
    uint32_t    out_hz;        /**< Resulting output frequency (Hz)       */
    bool        locked;        /**< PLL lock status                       */
} uiox_pll_cfg_t;

/* =========================================================================
 * Clock context — one per platform
 * ====================================================================== */
typedef struct {
    uint32_t    freq_hz[UIOX_CLK__COUNT]; /**< Configured frequency table */
    bool        enabled[UIOX_CLK__COUNT]; /**< Per-clock enable state      */
    uiox_pll_cfg_t pll_sys;               /**< System PLL configuration   */
    uiox_pll_cfg_t pll_cpu;               /**< CPU PLL configuration      */
    bool        initialized;
} uiox_clk_ctx_t;

/* =========================================================================
 * Default reference frequencies (QEMU / simulation defaults)
 * ====================================================================== */
#define UIOX_CLK_REF_24MHZ      24000000u
#define UIOX_CLK_REF_25MHZ      25000000u
#define UIOX_CLK_REF_100MHZ    100000000u

/* ── UART baud-rate divisor helper ─────────────────────────────────── */
/* IBRD = ref_clk / (16 * baud)   FBRD = round((frac) * 64)            */
#define UIOX_UART_IBRD(ref, baud)  ((ref) / (16u * (baud)))
#define UIOX_UART_FBRD(ref, baud)  \
    ((uint32_t)(((((ref) % (16u*(baud))) * 64u) + ((baud)/2u)) / (baud)))

/* =========================================================================
 * Clock API
 * ====================================================================== */

/** Initialise clock context for the detected SoC. */
int  uiox_clk_init(uiox_clk_ctx_t *ctx, const uiox_soc_desc_t *soc);

/** Return configured frequency of @id in Hz; 0 if unknown. */
uint32_t uiox_clk_get_hz(const uiox_clk_ctx_t *ctx, uiox_clk_id_t id);

/** Enable a clock domain. No-op on simulation builds. */
int  uiox_clk_enable (uiox_clk_ctx_t *ctx, uiox_clk_id_t id);

/** Disable a clock domain. */
int  uiox_clk_disable(uiox_clk_ctx_t *ctx, uiox_clk_id_t id);

/** Print clock table to console. */
void uiox_clk_print  (const uiox_clk_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_CLK_H */
