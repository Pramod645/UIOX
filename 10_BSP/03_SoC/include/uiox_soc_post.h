/**
 * @file    uiox_soc_post.h
 * @brief   UIOX SoC — Power-On Self Test (POST).
 *
 * Runs before the main SoC pipeline to verify:
 *   - CPU register sanity
 *   - DRAM walking pattern test (first 64 KB of each RAM region)
 *   - ROM / flash CRC32 integrity check
 *   - Critical peripheral smoke tests (UART TX, timer read, GIC status)
 *   - Stack sentinel (magic word at stack base)
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_SOC_POST_H
 #define UIOX_SOC_POST_H
 
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── POST result codes ──────────────────────────────────── */
 typedef enum {
     UIOX_SOC_POST_OK            =  0,
     UIOX_SOC_POST_FAIL_CPU      = -1,  /**< CPU register unexpected val  */
     UIOX_SOC_POST_FAIL_RAM      = -2,  /**< RAM walk pattern mismatch    */
     UIOX_SOC_POST_FAIL_ROM_CRC  = -3,  /**< ROM / flash CRC32 mismatch   */
     UIOX_SOC_POST_FAIL_UART     = -4,  /**< UART loopback failed         */
     UIOX_SOC_POST_FAIL_TIMER    = -5,  /**< System timer not counting    */
     UIOX_SOC_POST_FAIL_GIC      = -6,  /**< GIC distributor not OK       */
     UIOX_SOC_POST_FAIL_STACK    = -7,  /**< Stack sentinel corrupted     */
     UIOX_SOC_POST_FAIL_GENERIC  = -8,
 } uiox_soc_post_result_t;
 
 /* ── POST test bitmask ──────────────────────────────────── */
 #define UIOX_SOC_POST_TEST_CPU     (1u << 0)
 #define UIOX_SOC_POST_TEST_RAM     (1u << 1)
 #define UIOX_SOC_POST_TEST_ROM_CRC (1u << 2)
 #define UIOX_SOC_POST_TEST_UART    (1u << 3)
 #define UIOX_SOC_POST_TEST_TIMER   (1u << 4)
 #define UIOX_SOC_POST_TEST_GIC     (1u << 5)
 #define UIOX_SOC_POST_TEST_STACK   (1u << 6)
 #define UIOX_SOC_POST_TEST_ALL     0x7Fu
 
 /* ── RAM test region ────────────────────────────────────── */
 typedef struct {
     uiox_uint64_t base;   /**< Physical base address                          */
     uiox_uint64_t size;   /**< Size in bytes (test walks first 64 KB)         */
 } uiox_soc_post_ram_region_t;
 
 #define UIOX_SOC_POST_MAX_RAM_REGIONS   8u
 
 /* ── POST configuration ─────────────────────────────────── */
 typedef struct {
     uiox_uint32_t                   test_mask;
     uiox_uintptr_t                  rom_base;
     uiox_uint32_t                   rom_size;
     uiox_uint32_t                   rom_crc32_expected;
     uiox_soc_post_ram_region_t ram[UIOX_SOC_POST_MAX_RAM_REGIONS];
     uiox_uint32_t                   ram_count;
     uiox_uintptr_t                  uart_base;
     uiox_uintptr_t                  gic_dist_base;
     uiox_uintptr_t                  stack_base;
     uiox_uint32_t                   stack_sentinel;
 } uiox_soc_post_cfg_t;
 
 /* ── POST result report ─────────────────────────────────── */
 typedef struct {
     uiox_soc_post_result_t overall;
     uiox_uint32_t               failed_tests;
     uiox_uint32_t               passed_tests;
     uiox_uint32_t               cpu_midr;
     uiox_uint32_t               timer_freq_hz;
     uiox_uint32_t               rom_crc32_actual;
     uiox_uint32_t               ram_tested_bytes;
     uiox_uint32_t               gic_typer;
     char                   fail_msg[128];
 } uiox_soc_post_report_t;
 
 /* ── POST API ───────────────────────────────────────────── */
 
 /**
  * Run all POST tests selected in @cfg.
  * Returns UIOX_SOC_POST_OK if all selected tests pass.
  */
 uiox_soc_post_result_t uiox_soc_post_run   (const uiox_soc_post_cfg_t *cfg,
                                               uiox_soc_post_report_t *report);
 
 /** Print the POST report to the debug UART. */
 void                   uiox_soc_post_print (const uiox_soc_post_report_t *r);
 
 /** Write stack sentinel at startup. */
 void                   uiox_soc_post_stack_mark(uiox_uintptr_t stack_base,
                                                   uiox_uint32_t  sentinel);
 
 /** CRC32 (IEEE 802.3) — used for ROM integrity check. */
 uiox_uint32_t               uiox_soc_crc32     (const uiox_uint8_t *data, uiox_size_t len);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_POST_H */
 