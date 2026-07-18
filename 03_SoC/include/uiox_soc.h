/**
 * @file    uiox_soc.h
 * @brief   UIOX SoC — master umbrella include (v1.2).
 *
 * Single include for all consumers of the UIOX SoC HAL.
 *
 * ── HAL modules ──────────────────────────────────────────────────────────
 *   uiox_soc_types.h  — SoC ID enum, capability flags, descriptor struct,
 *                        base integer types, error codes, MMIO helpers
 *   uiox_soc_map.h    — MMIO base addresses and IRQ numbers per platform
 *   uiox_soc_clk.h    — Clock tree, PLL definitions, freq-query API
 *   uiox_soc_pm.h     — Power domains and reset controller
 *   uiox_soc_irq.h    — IRQ manager (GIC / 8259A / LAPIC abstraction)
 *   uiox_soc_clock.h  — Clock / PLL enable/disable/set-Hz API
 *   uiox_soc_power.h  — Power management (PSCI for ARM, ACPI for x86)
 *   uiox_soc_mem.h    — Physical memory map and early MMU/MPU init
 *   uiox_soc_post.h   — Power-On Self Test
 *   uiox_soc_psci.h   — PSCI 1.1 dispatch table
 *   uiox_soc_secboot.h — Secure boot chain of trust
 *   uiox_soc_tz.h     — TrustZone / EL3 secure world setup
 *   uiox_soc_dma.h    — DMA controller abstraction (PL080 / 8237A)
 *   uiox_soc_pcie.h   — PCIe ECAM early init + BAR assignment
 *
 * ── Global SoC lifecycle ─────────────────────────────────────────────────
 *   uiox_soc_init()         — detect + initialise all sub-systems
 *   uiox_soc_fini()         — tear down at shutdown
 *   uiox_soc_get_desc()     — read-only SoC descriptor
 *   uiox_soc_get_clk()      — clock context pointer
 *   uiox_soc_get_pm()       — power-management context pointer
 *   uiox_soc_print()        — one-page SoC summary to console
 *
 * ── Architecture backends ─────────────────────────────────────────────────
 *   Implemented in 02_FwHal/src/uiox_soc_<arch>.c:
 *   uiox_soc_init_arm64(), uiox_soc_init_arm32(),
 *   uiox_soc_init_x86(), uiox_soc_init_riscv64()
 *
 * ── Usage ─────────────────────────────────────────────────────────────────
 *   #include "../../02_FwHal/include/uiox_soc.h"
 *
 * @version 1.2.0
 * @date    2026-07-07
 */

 #ifndef UIOX_SOC_H
 #define UIOX_SOC_H
 
 /* =========================================================================
  * 1. Core types — must come before everything else
  *    Provides: SoC ID enum, capability flags, uiox_soc_desc_t,
  *              portable integer types, uiox_soc_err_t, MMIO helpers.
  * ====================================================================== */
 #include "uiox_soc_types.h"
 
 /* =========================================================================
  * 2. Platform map — MMIO base addresses and IRQ numbers
  *    Provides: UIOX_SOC_MEM_ARM64_*, UIOX_SOC_IRQ_ARM64_*, etc.
  * ====================================================================== */
 #include "uiox_soc_map.h"
 
 /* =========================================================================
  * 3. Clock tree — PLL definitions and frequency-query API
  *    Provides: uiox_clk_ctx_t, uiox_clk_id_t, UIOX_CLK_* identifiers
  * ====================================================================== */
 #include "uiox_soc_clk.h"
 
 /* =========================================================================
  * 4. Power management — domains and reset controller
  *    Provides: uiox_pm_ctx_t, power-domain enable/disable API
  * ====================================================================== */
 #include "uiox_soc_pm.h"
 
 /* =========================================================================
  * 5. IRQ manager — GIC / 8259A / LAPIC abstraction
  *    Provides: uiox_soc_irq_handler_t, uiox_soc_irq_desc_t,
  *              uiox_soc_irq_init/register/dispatch/enable/print
  * ====================================================================== */
 #include "uiox_soc_irq.h"
 
 /* =========================================================================
  * 6. Clock HAL — enable/disable/set-Hz API (higher-level than soc_clk.h)
  *    Provides: uiox_soc_clk_id_t, uiox_soc_clock_t,
  *              uiox_soc_clock_init/enable/disable/get_hz/set_hz/print
  * ====================================================================== */
 //#include "uiox_soc_clock.h"
 
 /* =========================================================================
  * 7. Power HAL — PSCI (ARM) / ACPI (x86) runtime power management
  *    Provides: uiox_soc_pwr_state_t, uiox_soc_power_ctx_t,
  *              uiox_soc_power_init/idle/cpu_on/cpu_off/reset/shutdown
  * ====================================================================== */
 #include "uiox_soc_power.h"
 
 /* =========================================================================
  * 8. Memory HAL — physical memory map and early MMU/MPU init
  *    Provides: uiox_soc_mem_map_t, uiox_soc_mem_region_t,
  *              uiox_soc_mem_init/print/mmu_early,
  *              uiox_soc_memset/memcpy/memcmp/strlen
  * ====================================================================== */
 #include "uiox_soc_mem.h"
 
 /* =========================================================================
  * 9. POST — Power-On Self Test
  *    Provides: uiox_soc_post_cfg_t, uiox_soc_post_report_t,
  *              uiox_soc_post_run/print/stack_mark, uiox_soc_crc32
  * ====================================================================== */
 #include "uiox_soc_post.h"
 
 /* =========================================================================
  * 10. PSCI 1.1 — dispatch table and per-CPU power state machine
  *     Provides: uiox_soc_psci_ctx_t, uiox_soc_psci_init/dispatch/print,
  *               individual handler declarations for unit testing
  * ====================================================================== */
 #include "uiox_soc_psci.h"
 
 /* =========================================================================
  * 11. Secure boot — chain of trust, SHA-256, PCR extension
  *     Provides: uiox_soc_secboot_ctx_t, uiox_soc_secboot_report_t,
  *               uiox_soc_secboot_init/verify_cert/verify_image/print,
  *               uiox_soc_sha256_*
  * ====================================================================== */
 #include "uiox_soc_secboot.h"
 
 /* =========================================================================
  * 12. TrustZone / EL3 — secure world setup, TZC-400, VBAR, ERET
  *     Provides: uiox_soc_tz_cfg_t, uiox_soc_tz_report_t,
  *               uiox_soc_tz_init/gic_secure/tzc_set_region/
  *               tz_set_vbar/tz_eret_to_ns/tz_print
  * ====================================================================== */
 #include "uiox_soc_tz.h"
 
 /* =========================================================================
  * 13. DMA — controller abstraction (PL080 / PL330 / Intel 8237A)
  *     Provides: uiox_soc_dma_ctrl_t, uiox_soc_dma_xfer_t,
  *               uiox_soc_dma_init/transfer/wait/abort/memcpy
  * ====================================================================== */
 #include "uiox_soc_dma.h"
 
 /* =========================================================================
  * 14. PCIe — ECAM early init, BAR assignment, device scan
  *     Provides: uiox_soc_pcie_ctrl_t, uiox_soc_pcie_dev_t,
  *               uiox_soc_pcie_init/scan/assign_bars/enable_dev,
  *               uiox_soc_pcie_cfg_read32/write32/find_class
  * ====================================================================== */
 #include "uiox_soc_pcie.h"

/*
 * uiox_soc_map.h provides SOC_GIC_DIST_BASE, SOC_UART0_BASE,
 * SOC_DRAM_BASE etc. for whichever architecture is being compiled.
 * Include it here so every file that includes uiox_soc.h automatically
 * gets the platform MMIO map without needing a separate #include.
 */
//#include "uiox_soc_map.h"

 
 /* =========================================================================
  * Version and project identity
  * ====================================================================== */
 #define UIOX_SOC_VERSION_STR  "UIOX SoC HAL v1.2"
 #define UIOX_SOC_URL           "github.com/Pramod645/UIOX"
 
 /* =========================================================================
  * SoC-level console output  (backed by the registered UART)
  * ====================================================================== */
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 void uiox_soc_printf(const char *fmt, ...)
      __attribute__((format(printf, 1, 2)));
 void uiox_soc_puts  (const char *s);
 void uiox_soc_putc  (char c);
 
 /* =========================================================================
  * Stage log macros
  * ====================================================================== */
 
 /** Subsystem log line:  [SOC] <subsys>  : <message> */
 #define SOC_LOG(subsys, fmt, ...) \
     uiox_soc_printf("[SOC] %-8s: " fmt "\n", (subsys), ##__VA_ARGS__)
 
 /** Short OK stamp. */
 #define SOC_OK() \
     uiox_soc_puts("OK\n")
 
 /** Non-fatal error. */
 #define SOC_ERR(fmt, ...) \
     uiox_soc_printf("[SOC] ERROR: " fmt "\n", ##__VA_ARGS__)
 
 /** Fatal error — log then spin forever. */
 /* include/uiox_soc.h — replace existing SOC_FATAL with this version */

/*
 * SOC_FATAL — log a message and spin forever.
 *
 * Two forms:
 *   SOC_FATAL("literal message")          — no format args
 *   SOC_FATAL("fmt %u", val)              — with format args
 *
 * The two-macro pattern avoids the ISO C99 "empty __VA_ARGS__" warning
 * that fires with -Wpedantic when SOC_FATAL("msg") has no variadic part.
 */
#define SOC_FATAL_MSG(msg) \
do { uiox_soc_puts("[SOC] FATAL: " msg "\n"); \
     for (;;) __asm__ volatile("" ::: "memory"); } while (0)

#define SOC_FATAL(fmt, ...) \
do { uiox_soc_printf("[SOC] FATAL: " fmt "\n", ##__VA_ARGS__); \
     for (;;) __asm__ volatile("" ::: "memory"); } while (0)

 
 /* =========================================================================
  * Global SoC lifecycle API
  *
  * uiox_soc_init()  — called once from arch_init() of each backend.
  *                    Detects the SoC, populates the global descriptor,
  *                    clock context, and PM context, then delegates to the
  *                    architecture-specific backend below.
  * ====================================================================== */
 
 /**
  * @brief  Detect and initialise the SoC and all HAL sub-systems.
  * @return UIOX_SOC_OK (0) on success, negative uiox_soc_err_t on failure.
  */
 int uiox_soc_init(void);
 
 /**
  * @brief  Tear down all SoC sub-systems (called at orderly shutdown).
  */
 void uiox_soc_fini(void);
 
 /**
  * @brief  Return a pointer to the populated global SoC descriptor.
  *         Returns NULL if uiox_soc_init() has not yet been called.
  */
 const uiox_soc_desc_t *uiox_soc_get_desc(void);
 
 /**
  * @brief  Return a pointer to the global clock context.
  */
 uiox_clk_ctx_t *uiox_soc_get_clk(void);
 
 /**
  * @brief  Return a pointer to the global power-management context.
  */
 uiox_pm_ctx_t  *uiox_soc_get_pm(void);
 
 /**
  * @brief  Print a one-page SoC summary to the registered UART console.
  *         Outputs: SoC name, ISA, platform, clock table, memory map,
  *                  IRQ count, power state, HAL version.
  */
 void uiox_soc_print(void);
 
 /* =========================================================================
  * Architecture-specific init entry points
  *
  * Implemented in:
  *   02_FwHal/src/uiox_soc_arm64.c
  *   02_FwHal/src/uiox_soc_arm32.c
  *   02_FwHal/src/uiox_soc_x86.c
  *   02_FwHal/src/uiox_soc_riscv64.c
  *
  * Each function:
  *   1. Fills @desc with SoC identity (name, ISA, clock freqs, IRQ lines).
  *   2. Configures the underlying hardware (GIC, PLL, TZC, etc.).
  *   3. Returns 0 on success, negative error on failure.
  * ====================================================================== */
 int uiox_soc_init_arm64  (uiox_soc_desc_t *desc);
 int uiox_soc_init_arm32  (uiox_soc_desc_t *desc);
 int uiox_soc_init_x86    (uiox_soc_desc_t *desc);
 int uiox_soc_init_riscv64(uiox_soc_desc_t *desc);

 /* ── New SoC-specific init entry points added ────────────────────── */
//int  uiox_soc_init_imx8mp   (uiox_soc_desc_t *desc); /* NXP i.MX 8M Plus  */
//int  uiox_soc_init_rk3588   (uiox_soc_desc_t *desc); /* Rockchip RK3588    */
//int  uiox_soc_init_omap4430 (uiox_soc_desc_t *desc); /* TI OMAP4430        */
//int  uiox_soc_init_th1520   (uiox_soc_desc_t *desc); /* T-Head TH1520      */

 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* UIOX_SOC_H */
 