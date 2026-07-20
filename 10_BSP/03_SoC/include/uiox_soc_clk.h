/**
 * @file    uiox_soc_clk.h
 * @brief   UIOX SoC — Unified clock subsystem.
 *
 * Merges two previously separate clock headers:
 *
 *   uiox_soc_clk.h    — low-level clock tree, PLL config, baud helpers,
 *                        stateful uiox_clk_ctx_t
 *   uiox_soc_clock.h  — high-level enable/disable/set-Hz API,
 *                        uiox_soc_clock_t descriptor struct
 *
 * Provides:
 *   ┌─ Clock identifiers ──────────────────────────────────────────────┐
 *   │  uiox_soc_clk_id_t  — unified enum covering all clock domains   │
 *   │  uiox_soc_clock_t   — per-clock descriptor (id, freq, enabled)  │
 *   └──────────────────────────────────────────────────────────────────┘
 *   ┌─ Low-level clock tree ───────────────────────────────────────────┐
 *   │  uiox_soc_pll_cfg_t — PLL configuration (ref, mul, div, out)    │
 *   │  uiox_clk_ctx_t     — platform clock context (freq table + PLL) │
 *   │  UIOX_SOC_CLK_REF_* — reference frequency constants             │
 *   │  UIOX_SOC_UART_IBRD/FBRD — baud-rate divisor helpers            │
 *   └──────────────────────────────────────────────────────────────────┘
 *   ┌─ Stateful API (takes uiox_clk_ctx_t *) ─────────────────────────┐
 *   │  uiox_soc_clk_init / get_hz / enable / disable / print          │
 *   └──────────────────────────────────────────────────────────────────┘
 *   ┌─ Stateless API (global state, driver-facing) ────────────────────┐
 *   │  uiox_soc_clock_init / enable / disable / get_hz / set_hz /     │
 *   │  print                                                           │
 *   └──────────────────────────────────────────────────────────────────┘
 *   ┌─ Backwards-compatible aliases ──────────────────────────────────┐
 *   │  UIOX_CLK_*, UIOX_FW_CLK_*, uiox_clk_id_t, uiox_pll_cfg_t     │
 *   │  uiox_clk_init/get_hz/enable/disable/print                      │
 *   └──────────────────────────────────────────────────────────────────┘
 *
 * The subsystem is intentionally minimal — no full CCF (Common Clock
 * Framework) — providing just enough to configure boot CPU frequency,
 * UART baud divisor, and system timer period.
 *
 * @version 2.0.0  (merged from 1.0 uiox_soc_clk.h + uiox_soc_clock.h)
 * @date    2026-07-18
 */

 /**
 * @file    uiox_soc_clk.h
 * @brief   UIOX SoC — Unified clock subsystem.
 * @version 2.0.1  (fixed: removed duplicate fw_clock inline wrappers)
 * @date    2026-07-18
 */

#ifndef UIOX_SOC_CLK_H
#define UIOX_SOC_CLK_H

#include "uiox_soc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Clock identifiers — unified enum
 * ====================================================================== */
typedef enum {
    UIOX_SOC_CLK_CPU        =  0,  /**< Boot CPU — alias for CPU0          */
    UIOX_SOC_CLK_CPU0       =  0,  /**< Boot CPU (cluster 0, core 0)       */
    UIOX_SOC_CLK_CPU_ALL    =  1,  /**< All CPU cores                      */
    UIOX_SOC_CLK_BUS        =  2,  /**< System / AXI bus                   */
    UIOX_SOC_CLK_UART0      =  3,  /**< UART0 reference clock              */
    UIOX_SOC_CLK_UART1      =  4,  /**< UART1 reference clock              */
    UIOX_SOC_CLK_TIMER0     =  5,  /**< Periodic timer reference           */
    UIOX_SOC_CLK_GPIO       =  6,  /**< GPIO controller clock              */
    UIOX_SOC_CLK_GIC        =  7,  /**< GIC / interrupt controller clock   */
    UIOX_SOC_CLK_I2C        =  8,  /**< I2C master clock                   */
    UIOX_SOC_CLK_SPI        =  9,  /**< SPI master clock                   */
    UIOX_SOC_CLK_ETH        = 10,  /**< Ethernet MAC clock                 */
    UIOX_SOC_CLK_USB        = 11,  /**< USB PHY reference                  */
    UIOX_SOC_CLK_EMMC       = 12,  /**< eMMC / SDIO clock                  */
    UIOX_SOC_CLK_STORAGE    = 12,  /**< Storage alias — same slot as EMMC  */
    UIOX_SOC_CLK_PCIE       = 13,  /**< PCIe reference                     */
    UIOX_SOC_CLK_GENERIC_TIMER = 14, /**< ARM CNTFRQ_EL0 / generic counter */
    UIOX_SOC_CLK_MAX        = 15,  /**< Total named clocks (for arrays)    */
    UIOX_SOC_CLK__COUNT     = 15,  /**< Alias — use in array declarations  */
} uiox_soc_clk_id_t;

/* ── Backwards-compatible aliases — old UIOX_FW_CLK_* names ──────────── */
#define UIOX_FW_CLK_CPU      UIOX_SOC_CLK_CPU
#define UIOX_FW_CLK_BUS      UIOX_SOC_CLK_BUS
#define UIOX_FW_CLK_UART0    UIOX_SOC_CLK_UART0
#define UIOX_FW_CLK_UART1    UIOX_SOC_CLK_UART1
#define UIOX_FW_CLK_TIMER0   UIOX_SOC_CLK_TIMER0
#define UIOX_FW_CLK_GPIO     UIOX_SOC_CLK_GPIO
#define UIOX_FW_CLK_I2C      UIOX_SOC_CLK_I2C
#define UIOX_FW_CLK_SPI      UIOX_SOC_CLK_SPI
#define UIOX_FW_CLK_ETH      UIOX_SOC_CLK_ETH
#define UIOX_FW_CLK_STORAGE  UIOX_SOC_CLK_STORAGE
#define UIOX_FW_CLK_MAX      UIOX_SOC_CLK_MAX

/* ── Backwards-compatible aliases — old UIOX_CLK_* names ─────────────── */
#define UIOX_CLK_CPU0            UIOX_SOC_CLK_CPU0
#define UIOX_CLK_CPU_ALL         UIOX_SOC_CLK_CPU_ALL
#define UIOX_CLK_BUS             UIOX_SOC_CLK_BUS
#define UIOX_CLK_UART0           UIOX_SOC_CLK_UART0
#define UIOX_CLK_UART1           UIOX_SOC_CLK_UART1
#define UIOX_CLK_TIMER0          UIOX_SOC_CLK_TIMER0
#define UIOX_CLK_GIC             UIOX_SOC_CLK_GIC
#define UIOX_CLK_ETH             UIOX_SOC_CLK_ETH
#define UIOX_CLK_USB             UIOX_SOC_CLK_USB
#define UIOX_CLK_EMMC            UIOX_SOC_CLK_EMMC
#define UIOX_CLK_PCIE            UIOX_SOC_CLK_PCIE
#define UIOX_CLK_GENERIC_TIMER   UIOX_SOC_CLK_GENERIC_TIMER
#define UIOX_CLK__COUNT          UIOX_SOC_CLK__COUNT

/* Typedef alias for old code using uiox_clk_id_t */
typedef uiox_soc_clk_id_t uiox_clk_id_t;

/* =========================================================================
 * Per-clock descriptor
 * ====================================================================== */
#define UIOX_SOC_CLOCK_MAX   16u

typedef struct {
    uiox_soc_clk_id_t id;
    uint32_t          freq_hz;
    bool              enabled;
    char              name[16];
} uiox_soc_clock_t;

/* =========================================================================
 * PLL configuration descriptor
 * ====================================================================== */
typedef struct {
    uint32_t  ref_hz;
    uint32_t  mul;
    uint32_t  div;
    uint32_t  out_hz;
    bool      locked;
} uiox_soc_pll_cfg_t;

/* Backwards-compatible typedef alias */
typedef uiox_soc_pll_cfg_t uiox_pll_cfg_t;

/* =========================================================================
 * Clock context — stateful, one per platform
 * ====================================================================== */
typedef struct {
    uint32_t           freq_hz[UIOX_SOC_CLK__COUNT];
    bool               enabled[UIOX_SOC_CLK__COUNT];
    uiox_soc_pll_cfg_t pll_sys;
    uiox_soc_pll_cfg_t pll_cpu;
    bool               initialized;
} uiox_clk_ctx_t;

/* =========================================================================
 * Reference frequency constants
 * ====================================================================== */
#define UIOX_SOC_CLK_REF_24MHZ    24000000u
#define UIOX_SOC_CLK_REF_25MHZ    25000000u
#define UIOX_SOC_CLK_REF_100MHZ  100000000u

#define UIOX_CLK_REF_24MHZ   UIOX_SOC_CLK_REF_24MHZ
#define UIOX_CLK_REF_25MHZ   UIOX_SOC_CLK_REF_25MHZ
#define UIOX_CLK_REF_100MHZ  UIOX_SOC_CLK_REF_100MHZ

/* =========================================================================
 * UART baud-rate divisor helpers
 * ====================================================================== */
#define UIOX_SOC_UART_IBRD(ref, baud) \
    ((uint32_t)((ref) / (16u * (baud))))

#define UIOX_SOC_UART_FBRD(ref, baud) \
    ((uint32_t)(((((ref) % (16u * (baud))) * 64u) + \
                  ((baud) / 2u)) / (baud)))

#define UIOX_UART_IBRD(ref, baud)  UIOX_SOC_UART_IBRD(ref, baud)
#define UIOX_UART_FBRD(ref, baud)  UIOX_SOC_UART_FBRD(ref, baud)

/* =========================================================================
 * Stateful clock API — declarations only (implemented in uiox_soc_clk.c)
 * ====================================================================== */
uiox_soc_err_t uiox_soc_clk_init    (uiox_clk_ctx_t        *ctx,
                                      const uiox_soc_desc_t *soc);
uint32_t       uiox_soc_clk_get_hz  (const uiox_clk_ctx_t  *ctx,
                                      uiox_soc_clk_id_t      id);
uiox_soc_err_t uiox_soc_clk_enable  (uiox_clk_ctx_t        *ctx,
                                      uiox_soc_clk_id_t      id);
uiox_soc_err_t uiox_soc_clk_disable (uiox_clk_ctx_t        *ctx,
                                      uiox_soc_clk_id_t      id);
void           uiox_soc_clk_print   (const uiox_clk_ctx_t  *ctx);

/* ── Backwards-compatible inline wrappers — old uiox_clk_* names ──────── */
static inline uiox_soc_err_t uiox_clk_init(uiox_clk_ctx_t *ctx,
    const uiox_soc_desc_t *soc) { return uiox_soc_clk_init(ctx, soc);  }
static inline uint32_t uiox_clk_get_hz(const uiox_clk_ctx_t *ctx,
    uiox_soc_clk_id_t id)       { return uiox_soc_clk_get_hz(ctx, id); }
static inline uiox_soc_err_t uiox_clk_enable(uiox_clk_ctx_t *ctx,
    uiox_soc_clk_id_t id)       { return uiox_soc_clk_enable(ctx, id); }
static inline uiox_soc_err_t uiox_clk_disable(uiox_clk_ctx_t *ctx,
    uiox_soc_clk_id_t id)       { return uiox_soc_clk_disable(ctx, id);}
static inline void uiox_clk_print(const uiox_clk_ctx_t *ctx)
                                  { uiox_soc_clk_print(ctx); }

/* =========================================================================
 * Stateless clock API — declarations only (implemented in uiox_soc_clk.c)
 * ====================================================================== */
uiox_soc_err_t uiox_soc_clock_init    (void);
uiox_soc_err_t uiox_soc_clock_enable  (uiox_soc_clk_id_t id);
uiox_soc_err_t uiox_soc_clock_disable (uiox_soc_clk_id_t id);
uint32_t       uiox_soc_clock_get_hz  (uiox_soc_clk_id_t id);
uiox_soc_err_t uiox_soc_clock_set_hz  (uiox_soc_clk_id_t id, uint32_t hz);
void           uiox_soc_clock_print   (void);

/*
 * ── Backwards-compatible wrappers — old uiox_fw_clock_* names ──────────
 *
 * KEY FIX: these are declared here as EXTERN (not static inline).
 * The definitions live in uiox_soc_clk.c.
 * This eliminates the "redefinition" errors that occurred when the .c file
 * also tried to define functions that the header had already given
 * static-inline bodies to.
 */
uiox_soc_err_t uiox_fw_clock_init    (void);
uiox_soc_err_t uiox_fw_clock_enable  (uiox_soc_clk_id_t id);
uiox_soc_err_t uiox_fw_clock_disable (uiox_soc_clk_id_t id);
uint32_t       uiox_fw_clock_get_hz  (uiox_soc_clk_id_t id);
uiox_soc_err_t uiox_fw_clock_set_hz  (uiox_soc_clk_id_t id, uint32_t hz);
void           uiox_fw_clock_print   (void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_CLK_H */
