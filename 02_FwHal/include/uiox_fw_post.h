/**
 * @file    uiox_fw_post.h
 * @brief   UIOX Firmware — Power-On Self Test (POST).
 *
 * Runs a structured sequence of hardware tests immediately after
 * the platform HAL is initialised and before the kernel is loaded.
 * Each test records its result, duration, and a human-readable
 * detail string.  A critical failure halts the system.
 *
 * Test IDs 0x01–0x09 are mandatory; 0x10+ are optional/platform.
 *
 * @version 1.0.0
 * @date    2026-07-06
 */
#ifndef UIOX_FW_POST_H
#define UIOX_FW_POST_H

#include "uiox_fw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * POST test IDs
 * ====================================================================== */
#define UIOX_POST_CPU          0x01u  /**< CPU sanity (registers, EL)  */
#define UIOX_POST_CACHE        0x02u  /**< L1 D-cache R/W test         */
#define UIOX_POST_RAM          0x03u  /**< DRAM march pattern test     */
#define UIOX_POST_UART         0x04u  /**< UART TX loopback            */
#define UIOX_POST_TIMER        0x05u  /**< System timer tick test      */
#define UIOX_POST_IRQ          0x06u  /**< IRQ controller sanity       */
#define UIOX_POST_CLOCK        0x07u  /**< PLL / clock lock            */
#define UIOX_POST_POWER        0x08u  /**< Power domain status         */
#define UIOX_POST_STORAGE      0x09u  /**< Block device presence       */
#define UIOX_POST_SECBOOT      0x0Au  /**< Secure boot config check    */
#define UIOX_POST_TZ           0x0Bu  /**< TrustZone / EL3 state       */
#define UIOX_POST_NUM_TESTS    0x0Cu  /**< Total mandatory test count  */

/* =========================================================================
 * POST result severity
 * ====================================================================== */
typedef enum {
    UIOX_POST_PASS     = 0,   /**< Test passed                         */
    UIOX_POST_WARN     = 1,   /**< Passed with advisory warning        */
    UIOX_POST_FAIL     = 2,   /**< Test failed — non-critical          */
    UIOX_POST_CRITICAL = 3,   /**< Test failed — system cannot boot    */
} uiox_post_result_t;

/* =========================================================================
 * Per-test record
 * ====================================================================== */
typedef struct {
    uint8_t           test_id;
    uiox_post_result_t result;
    uint64_t          duration_us;
    char              name  [32];
    char              detail[128];
} uiox_post_entry_t;

/* =========================================================================
 * POST context
 * ====================================================================== */
#define UIOX_POST_MAX_TESTS  16u

typedef struct {
    uiox_post_entry_t tests[UIOX_POST_MAX_TESTS];
    uint8_t           count;
    uint8_t           pass_count;
    uint8_t           fail_count;
    uint8_t           warn_count;
    uint64_t          total_us;
    bool              any_critical;
} uiox_fw_post_ctx_t;

/* =========================================================================
 * POST API
 * ====================================================================== */

/** Initialise context — call before any post_run_* function.           */
void uiox_fw_post_init     (uiox_fw_post_ctx_t *ctx);

/** Run every mandatory test in sequence.
 *  @return UIOX_FW_OK if all pass/warn, UIOX_FW_ERR_FAIL if critical. */
uiox_fw_err_t uiox_fw_post_run_all (uiox_fw_post_ctx_t *ctx,
                                      uintptr_t ram_base,
                                      uint64_t  ram_size);

/** Run a single test by test_id.                                        */
uiox_fw_err_t uiox_fw_post_run_one (uiox_fw_post_ctx_t *ctx,
                                      uint8_t test_id,
                                      uintptr_t ram_base,
                                      uint64_t  ram_size);

/** Print a formatted POST report via firmware UART.                     */
void          uiox_fw_post_print   (const uiox_fw_post_ctx_t *ctx);

/** Halt with a POST failure message (never returns).                    */
void __attribute__((noreturn))
              uiox_fw_post_panic   (const uiox_fw_post_ctx_t *ctx,
                                      uint8_t failed_test_id);

/* Individual test functions (also callable directly for diagnostics)   */
uiox_post_result_t uiox_fw_post_test_cpu    (uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_cache  (uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_ram    (uiox_post_entry_t *e,
                                               uintptr_t base,
                                               uint64_t  size);
uiox_post_result_t uiox_fw_post_test_uart   (uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_timer  (uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_irq    (uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_clock  (uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_power  (uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_storage(uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_secboot(uiox_post_entry_t *e);
uiox_post_result_t uiox_fw_post_test_tz     (uiox_post_entry_t *e);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_POST_H */
