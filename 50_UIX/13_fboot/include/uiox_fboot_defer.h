/**
 * @file  uiox_fboot_defer.h
 * @brief UIOX Fast Boot — deferred / lazy driver initialisation.
 *
 * Non-critical subsystems (USB, audio, secondary Ethernet, full GPU,
 * background update daemon) are registered here and executed after
 * the shell is already visible. The user sees a prompt within the
 * target window while heavy init finishes in the background.
 *
 * Execution order: sorted by priority (lowest integer = first).
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_FBOOT_DEFER_H
#define UIOX_FBOOT_DEFER_H

#include "uiox_fboot_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Deferred init registry
 * ====================================================================== */
typedef struct {
    uiox_fb_deferred_t  queue[UIOX_FB_MAX_DEFERRED];
    uint32_t            count;
    uint32_t            completed;
    uint32_t            failed;
    bool                running;
} uiox_fb_defer_ctx_t;

/* =========================================================================
 * API
 * ====================================================================== */

/** Initialise the deferred init registry. */
uiox_fb_err_t uiox_fb_defer_init(uiox_fb_defer_ctx_t *ctx);

/**
 * @brief Register a deferred initialiser.
 * @param name      Human-readable name for tracing.
 * @param fn        Init function to call.
 * @param arg       Opaque argument passed to @fn.
 * @param priority  Execution priority (0 = highest).
 */
uiox_fb_err_t uiox_fb_defer_register(uiox_fb_defer_ctx_t *ctx,
                                       const char          *name,
                                       uiox_fb_init_fn_t    fn,
                                       void                *arg,
                                       uint32_t             priority);

/**
 * @brief Run all registered deferred initialisers in priority order.
 *        Called after UIOX_FB_PHASE_SHELL_READY.
 *        In a threaded build, this runs on a low-priority background thread.
 */
uiox_fb_err_t uiox_fb_defer_run_all(uiox_fb_defer_ctx_t *ctx,
                                      const uiox_fb_timer_t *timer);

/** Query whether all deferred inits are complete. */
bool          uiox_fb_defer_all_done(const uiox_fb_defer_ctx_t *ctx);

/** Print deferred init status table. */
void          uiox_fb_defer_print(const uiox_fb_defer_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FBOOT_DEFER_H */
