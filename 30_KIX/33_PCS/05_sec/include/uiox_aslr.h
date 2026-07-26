/**
 * @file  uiox_aslr.h
 * @brief UIOX Security — Address Space Layout Randomisation engine.
 *
 * Provides per-exec() randomisation of all user-space regions and the
 * kernel thread stack. The entropy source is a platform CSPRNG (weak
 * default provided; override uiox_sec_plat_random() in BSP).
 *
 * ASLR levels (controlled via uiox_aslr_ctx_t::level):
 *   0 — ASLR disabled (test / bring-up only)
 *   1 — Stack + heap randomised
 *   2 — Full: stack + heap + mmap + exec (PIE) + vDSO  [production default]
 *   3 — Full + kernel stack randomisation
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_ASLR_H
#define UIOX_ASLR_H

#include "uiox_sec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ASLR context (one per system — shared across all processes)
 * ====================================================================== */
typedef struct {
    uint8_t   level;                 /**< 0–3 randomisation level          */
    uint8_t   entropy_bits[UIOX_ASLR_REGION__COUNT]; /**< Per-region bits  */
    bool      compat32;              /**< True → 32-bit address space      */
    bool      initialized;
} uiox_aslr_ctx_t;

/* =========================================================================
 * Platform entropy hook — implement in BSP
 * ====================================================================== */

/**
 * @brief Fill @buf with @len cryptographically-strong random bytes.
 *        Production: read from hardware RNG (TRNG / RDRAND / /dev/hwrng).
 *        Default stub: LFSR-based PRNG seeded at boot — NOT production-safe.
 */
__attribute__((weak))
void uiox_sec_plat_random(uint8_t *buf, size_t len);

/**
 * @brief Return current time in milliseconds (for audit timestamps).
 */
__attribute__((weak))
uint64_t uiox_sec_plat_time_ms(void);

/* =========================================================================
 * ASLR API
 * ====================================================================== */

/**
 * @brief Initialise the ASLR context with default entropy levels.
 *        Call once at kernel init.
 * @param level  0=off, 1=partial, 2=full (default), 3=full+kstack
 */
uiox_sec_err_t uiox_aslr_init(uiox_aslr_ctx_t *ctx, uint8_t level);

/**
 * @brief Randomise all memory regions for a new process address space.
 *        Called by exec() path in 33_ProcessControlSubsystem.
 *
 *        Fills @mm with randomised base addresses for each enabled region.
 *        The caller is responsible for applying these addresses to the
 *        actual VMA structures (uiox_aslr_apply_mm).
 *
 * @param ctx       Global ASLR context.
 * @param mm        Output: randomised memory map.
 * @param is_pie    True if the executable is position-independent.
 * @param is_compat True for 32-bit compat processes.
 */
uiox_sec_err_t uiox_aslr_randomise_mm(uiox_aslr_ctx_t *ctx,
                                        uiox_aslr_mm_t  *mm,
                                        bool             is_pie,
                                        bool             is_compat);

/**
 * @brief Randomise the kernel stack base for one thread.
 *        Called by the scheduler when creating a new kernel thread.
 * @param ctx           Global ASLR context.
 * @param stack_area    Physical base of the pre-allocated kernel stack area.
 * @param stack_size    Size of the kernel stack area.
 * @return  Randomised stack pointer (within [stack_area, stack_area+size]).
 */
uint64_t uiox_aslr_kstack(const uiox_aslr_ctx_t *ctx,
                            uint64_t               stack_area,
                            size_t                 stack_size);

/**
 * @brief Re-randomise the mmap base on each mmap() call (ASLR level ≥ 2).
 *        Prevents heap-spray attacks that assume a fixed mmap gap.
 */
uint64_t uiox_aslr_mmap_hint(const uiox_aslr_ctx_t *ctx,
                               uint64_t               preferred);

/**
 * @brief Override entropy bits for one region (admin / sysctl interface).
 *        Clamped to [UIOX_ASLR_ENTROPY_MIN_BITS, UIOX_ASLR_ENTROPY_MAX_BITS].
 */
uiox_sec_err_t uiox_aslr_set_entropy(uiox_aslr_ctx_t  *ctx,
                                       uiox_aslr_region_t region,
                                       uint8_t            bits);

/** Print current ASLR configuration and last randomised layout. */
void uiox_aslr_print(const uiox_aslr_ctx_t *ctx,
                      const uiox_aslr_mm_t  *mm);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_ASLR_H */
