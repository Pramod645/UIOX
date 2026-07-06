/**
 * @file    uiox_fw_post.h
 * @brief   UIOX Firmware — Power-On Self Test (POST).
 *
 * Runs a structured series of hardware checks before the firmware
 * hands off to the kernel.  Each test is independent; a failure
 * sets a result flag but (by default) does not halt the boot —
 * the caller decides whether to abort or continue.
 *
 * Tests performed (in order):
 *   POST_CPU     — CPU identification and minimal register sanity
 *   POST_CACHE   — L1 D-cache read-after-write pattern
 *   POST_RAM     — Walking-ones march test over a small DRAM window
 *   POST_UART    — UART TX FIFO ready and loopback (if wired)
 *   POST_TIMER   — System-timer tick advances
 *   POST_IRQ     — IRQ controller responds to a software-trigger
 *   POST_STORAGE — Block device present and sector-0 readable
 *   POST_CRYPTO  — SHA-256 self-test against known vector
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
 * Test IDs
 * ====================================================================== */
#define UIOX_POST_CPU       0x01u
#define UIOX_POST_CACHE     0x02u
#define UIOX_POST_RAM       0x03u
#define UIOX_POST_UART      0x04u
#define UIOX_POST_TIMER     0x05u
#define UIOX_POST_IRQ       0x06u
#define UIOX_POST_STORAGE   0x07u
#define UIOX_POST_CRYPTO    0x08u
#define UIOX_POST_NUM       0x08u   /**< total number of POST tests     */

/* =========================================================================
 * Result record — one per test
 * ====================================================================== */
typedef struct {
    uint8_t          test_id;
    uiox_fw_err_t    result;          /**< UIOX_FW_OK = pass            */
    uint32_t         duration_us;     /**< wall-clock µs (approximate)  */
    char             name[24];
    char             detail[64];      /**< human-readable pass/fail note */
} uiox_fw_post_result_t;

/* =========================================================================
 * POST context — filled by uiox_fw_post_run_all()
 * ====================================================================== */
typedef struct {
    uiox_fw_post_result_t results[UIOX_POST_NUM];
    uint8_t               count;
    uint8_t               pass;
    uint8_t               fail;
    uint32_t              total_us;
} uiox_fw_post_ctx_t;

/* =========================================================================
 * Configuration flags
 * ====================================================================== */
#define UIOX_POST_FL_HALT_ON_FAIL  (1u << 0)  /**< stop at first failure */
#define UIOX_POST_FL_VERBOSE       (1u << 1)  /**< print each result     */
#define UIOX_POST_FL_SKIP_STORAGE  (1u << 2)  /**< skip if no block dev  */
#define UIOX_POST_FL_SKIP_IRQ      (1u << 3)  /**< skip SW-IRQ test      */

/* =========================================================================
 * API
 * ====================================================================== */

/**
 * Run all POST tests.
 * @param ctx    caller-allocated context; filled on return
 * @param flags  UIOX_POST_FL_* bitmask
 * @return UIOX_FW_OK if all tests passed, UIOX_FW_ERR_POST otherwise
 */
uiox_fw_err_t uiox_fw_post_run_all  (uiox_fw_post_ctx_t *ctx,
                                       uint32_t            flags);

/** Run a single test by ID (UIOX_POST_*). */
uiox_fw_err_t uiox_fw_post_run_one  (uiox_fw_post_result_t *r,
                                       uint8_t               test_id);

/** Print the full POST report via uiox_fw_printf(). */
void          uiox_fw_post_print    (const uiox_fw_post_ctx_t *ctx);

/** Return non-zero if any test failed. */
int           uiox_fw_post_any_fail (const uiox_fw_post_ctx_t *ctx);

/* Individual test entry points (also callable directly) */
uiox_fw_err_t uiox_fw_post_cpu     (uiox_fw_post_result_t *r);
uiox_fw_err_t uiox_fw_post_cache   (uiox_fw_post_result_t *r);
uiox_fw_err_t uiox_fw_post_ram     (uiox_fw_post_result_t *r);
uiox_fw_err_t uiox_fw_post_uart    (uiox_fw_post_result_t *r);
uiox_fw_err_t uiox_fw_post_timer   (uiox_fw_post_result_t *r);
uiox_fw_err_t uiox_fw_post_irq     (uiox_fw_post_result_t *r);
uiox_fw_err_t uiox_fw_post_storage (uiox_fw_post_result_t *r);
uiox_fw_err_t uiox_fw_post_crypto  (uiox_fw_post_result_t *r);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_POST_H */
