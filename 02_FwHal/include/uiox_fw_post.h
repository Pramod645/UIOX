/**
 * @file  uiox_fw_post.h
 * @brief UIOX Firmware — Power-On Self Test (POST).
 *
 * Runs before the main firmware pipeline to verify:
 *   - CPU register sanity (MIDR, MPIDR, CNTFRQ on ARM64)
 *   - DRAM walking pattern test (first 64 KB of each RAM region)
 *   - ROM / flash CRC32 integrity check
 *   - Critical peripheral smoke tests (UART TX, timer read, GIC status)
 *   - Stack sentinel (magic word at stack base)
 *
 * Placed in 02_FwHal because POST must run after TrustZone/EL3 setup
 * but before any firmware subsystem initialisation.
 *
 * Integrates with: uiox_fw_hw.h (HAL ops for UART and timer readback)
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_POST_H
 #define UIOX_FW_POST_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * POST result codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_POST_OK             =  0,
     UIOX_POST_FAIL_CPU       = -1,  /**< CPU register unexpected value   */
     UIOX_POST_FAIL_RAM       = -2,  /**< RAM walk pattern mismatch       */
     UIOX_POST_FAIL_ROM_CRC   = -3,  /**< ROM / flash CRC32 mismatch      */
     UIOX_POST_FAIL_UART      = -4,  /**< UART loopback failed            */
     UIOX_POST_FAIL_TIMER     = -5,  /**< System timer not counting       */
     UIOX_POST_FAIL_GIC       = -6,  /**< GIC distributor not responding  */
     UIOX_POST_FAIL_STACK     = -7,  /**< Stack sentinel corrupted        */
     UIOX_POST_FAIL_GENERIC   = -8,
 } uiox_post_result_t;
 
 /* =========================================================================
  * POST test IDs (bitmask — select which tests to run)
  * ====================================================================== */
 
 #define UIOX_POST_TEST_CPU      (1u << 0)
 #define UIOX_POST_TEST_RAM      (1u << 1)
 #define UIOX_POST_TEST_ROM_CRC  (1u << 2)
 #define UIOX_POST_TEST_UART     (1u << 3)
 #define UIOX_POST_TEST_TIMER    (1u << 4)
 #define UIOX_POST_TEST_GIC      (1u << 5)
 #define UIOX_POST_TEST_STACK    (1u << 6)
 #define UIOX_POST_TEST_ALL      0x7Fu
 
 /* =========================================================================
  * RAM test region descriptor
  * ====================================================================== */
 
 typedef struct {
     uint64_t base;       /**< Physical base address of RAM region        */
     uint64_t size;       /**< Size in bytes (test walks first 64 KB)     */
 } uiox_post_ram_region_t;
 
 #define UIOX_POST_MAX_RAM_REGIONS   8u
 
 /* =========================================================================
  * POST configuration
  * ====================================================================== */
 
 typedef struct {
     uint32_t              test_mask;  /**< UIOX_POST_TEST_* bitmask     */
     uintptr_t             rom_base;   /**< Base address of ROM / flash  */
     uint32_t              rom_size;   /**< Bytes to CRC                 */
     uint32_t              rom_crc32_expected; /**< Expected CRC32        */
     uiox_post_ram_region_t ram[UIOX_POST_MAX_RAM_REGIONS];
     uint32_t              ram_count;
     uintptr_t             uart_base;  /**< UART base for smoke test     */
     uintptr_t             gic_dist_base;
     uintptr_t             stack_base; /**< Lowest stack address (sentinel)*/
     uint32_t              stack_sentinel; /**< Magic word written at base*/
 } uiox_post_cfg_t;
 
 /* =========================================================================
  * POST result report
  * ====================================================================== */
 
 typedef struct {
     uiox_post_result_t  overall;
     uint32_t            failed_tests;  /**< Bitmask of failed test IDs   */
     uint32_t            passed_tests;
     /* Per-test details */
     uint32_t            cpu_midr;      /**< MIDR_EL1 / CPUID            */
     uint64_t            timer_freq_hz; /**< Measured CNTFRQ_EL0         */
     uint32_t            rom_crc32_actual;
     uint64_t            ram_tested_bytes;
     uint32_t            gic_typer;     /**< GICD_TYPER read             */
     char                fail_msg[128]; /**< Human-readable failure desc  */
 } uiox_post_report_t;
 
 /* =========================================================================
  * POST API
  * ====================================================================== */
 
 /**
  * Run all POST tests selected in @cfg.
  * Fills @report with detailed results.
  * Returns UIOX_POST_OK if all selected tests pass.
  * On failure, firmware should either halt or fall to recovery mode.
  */
 uiox_post_result_t uiox_fw_post_run    (const uiox_post_cfg_t *cfg,
                                          uiox_post_report_t *report);
 
 /** Print the POST report to the debug UART. */
 void               uiox_fw_post_print  (const uiox_post_report_t *report);
 
 /** Write stack sentinel (call at firmware startup before any stack use). */
 void               uiox_fw_post_stack_mark(uintptr_t stack_base,
                                             uint32_t sentinel);
 
 /** CRC32 (IEEE 802.3 polynomial) — used for ROM integrity check. */
 uint32_t           uiox_fw_crc32       (const uint8_t *data, size_t len);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_POST_H */
 