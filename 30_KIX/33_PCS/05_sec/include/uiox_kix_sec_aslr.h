/**
 * @file  uiox_kix_sec_aslr.h
 * @brief UIOX Security — Address Space Layout Randomisation engine.
 *
 * ── what changed in 1.0.1 ─────────────────────────────────────────────
 *   compat32 removed from the context.  It was written by
 *   uiox_aslr_randomise_mm() — a PER-PROCESS function — into a GLOBAL
 *   struct, so two processes, one 64-bit and one compat, overwrote each
 *   other's value.  Nothing but uiox_aslr_print read it.
 *
 *   The is_compat PARAMETER stays: it selects which base addresses
 *   randomise_mm uses, which is per-call and correct.
 *
 * @version 1.0.1
 * @date    2026-10-02
 */
#ifndef UIOX_ASLR_H
#define UIOX_ASLR_H

#include "uiox_kix_sec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ASLR context (one per system — shared across all processes)
 * ====================================================================== */
typedef struct {
    uint8_t   level;                 /**< 0–3 randomisation level          */
    uint8_t   entropy_bits[UIOX_ASLR_REGION__COUNT]; /**< Per-region bits  */
    bool      initialized;
} uiox_aslr_ctx_t;

/* =========================================================================
 * Platform entropy hook — implement in BSP
 * ====================================================================== */

/**
 * @brief Fill @buf with @len cryptographically-strong random bytes.
 *        Default stub: 32-bit xorshift seeded at boot — NOT production-safe.
 *        See the note on uiox_aslr_init(): every boot produces the SAME
 *        addresses with no BSP override.
 */
__attribute__((weak))
void uiox_sec_plat_random(uint8_t *buf, size_t len);

/**
 * @brief Return current time in milliseconds (for audit timestamps).
 *        One weak definition, in uiox_kix_sec_aslr.c.  uiox_kix_sec_mac.c
 *        declares it extern — a declaration, not a second definition.
 */
__attribute__((weak))
uint64_t uiox_sec_plat_time_ms(void);

/* =========================================================================
 * ASLR API
 * ====================================================================== */

/**
 * @brief Initialise the ASLR context with default entropy levels.
 *
 * ── THE DEFAULT ENTROPY SOURCE SEEDS ITSELF ───────────────────────────
 * This function seeds the weak default's LFSR by CALLING the weak
 * default:
 *
 *     s_lfsr_state = 0xDEADBEEF
 *       → uiox_sec_plat_random() produces 8 bytes from that state
 *       → those 8 bytes are folded back into s_lfsr_state
 *
 * With no BSP override, the post-init state is a deterministic function
 * of the initial constant.  Every boot produces the SAME "randomised"
 * addresses, and uiox_aslr_print shows them looking perfectly random.
 *
 * A BSP must override uiox_sec_plat_random() before ASLR means anything.
 */
uiox_sec_err_t uiox_aslr_init(uiox_aslr_ctx_t *ctx, uint8_t level);

/**
 * @brief Randomise all memory regions for a new process address space.
 * @param is_compat True for 32-bit compat processes.  Drives the base
 *                  addresses chosen; NOT stored on the context.
 */
uiox_sec_err_t uiox_aslr_randomise_mm(uiox_aslr_ctx_t *ctx,
                                        uiox_aslr_mm_t  *mm,
                                        bool             is_pie,
                                        bool             is_compat);

uint64_t uiox_aslr_kstack(const uiox_aslr_ctx_t *ctx,
                            uint64_t               stack_area,
                            size_t                 stack_size);

uint64_t uiox_aslr_mmap_hint(const uiox_aslr_ctx_t *ctx,
                               uint64_t               preferred);

uiox_sec_err_t uiox_aslr_set_entropy(uiox_aslr_ctx_t   *ctx,
                                       uiox_aslr_region_t region,
                                       uint8_t            bits);

void uiox_aslr_print(const uiox_aslr_ctx_t *ctx,
                      const uiox_aslr_mm_t  *mm);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_ASLR_H */
