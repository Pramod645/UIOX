/**
 * @file  uiox_fboot.h
 * @brief UIOX Fast Boot — master header, top-level orchestration API.
 *
 * Single include for all consumers. Provides the full boot pipeline:
 *
 *   COLD BOOT path:
 *     uiox_fb_init()
 *     uiox_fb_phase_begin(RESET) … uiox_fb_phase_end(RESET)
 *     … (CLK_PLL, DDR_INIT, FW_VERIFY, DECOMPRESS, DEVTREE,
 *          EARLY_DRIVERS, FS_MOUNT, INIT_SPAWN, SHELL_READY)
 *     uiox_fb_report()
 *     uiox_fb_defer_run_all()        ← background, after shell visible
 *
 *   SNAPSHOT RESUME path:
 *     uiox_fb_init()
 *     uiox_fb_snap_probe()           ← valid? jump to restore
 *     uiox_fb_snap_restore()         ← does NOT return on success
 *     (falls through to cold path)
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_FBOOT_H
#define UIOX_FBOOT_H

#include "uiox_fboot_types.h"
#include "uiox_fboot_timer.h"
#include "uiox_fboot_snapshot.h"
#include "uiox_fboot_defer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Syscall numbers (registered in 40_SystemCallInterface)
 * ====================================================================== */
#define SYS_BOOT_STATUS     230u   /**< Query boot timing / phase results  */
#define SYS_BOOT_SNAP_SAVE  231u   /**< Trigger snapshot capture           */
#define SYS_BOOT_SNAP_CLEAR 232u   /**< Invalidate snapshot                */

/* =========================================================================
 * Global fast-boot context
 * ====================================================================== */
typedef struct {
    uiox_fb_ctx_t        timing;
    uiox_fb_timer_t      timer;
    uiox_fb_snap_ctx_t   snap;
    uiox_fb_defer_ctx_t  defer;
    bool                 initialized;
} uiox_fb_master_ctx_t;

/* =========================================================================
 * Core pipeline API
 * ====================================================================== */

/** Initialise master context. Call before any other fb_ function. */
uiox_fb_err_t uiox_fb_init(uiox_fb_master_ctx_t *ctx,
                             uiox_fb_mode_t        mode,
                             uint64_t              target_us);

/** Mark a phase as started — records start timestamp. */
uiox_fb_err_t uiox_fb_phase_begin(uiox_fb_master_ctx_t *ctx,
                                    uiox_fb_phase_t       phase);

/** Mark a phase as ended — records duration, checks budget. */
uiox_fb_err_t uiox_fb_phase_end  (uiox_fb_master_ctx_t *ctx,
                                    uiox_fb_phase_t       phase,
                                    uiox_fb_err_t         result);

/** Skip a phase (e.g. DDR_INIT skipped on snapshot resume). */
uiox_fb_err_t uiox_fb_phase_skip (uiox_fb_master_ctx_t *ctx,
                                    uiox_fb_phase_t       phase);

/** Record that the shell is ready — final milestone timestamp. */
uiox_fb_err_t uiox_fb_shell_ready(uiox_fb_master_ctx_t *ctx);

/* =========================================================================
 * Reporting
 * ====================================================================== */

/** Print full boot timing report to console. */
void uiox_fb_report(const uiox_fb_master_ctx_t *ctx);

/** Fill a compact summary for sys_boot_status() syscall. */
void uiox_fb_fill_status(const uiox_fb_master_ctx_t *ctx,
                          uint8_t *buf, size_t buf_size);

/* =========================================================================
 * Syscall handlers
 * ====================================================================== */
long sys_boot_status    (long buf, long buf_size, long a2, long a3);
long sys_boot_snap_save (long kver, long a1,      long a2, long a3);
long sys_boot_snap_clear(long a0,   long a1,      long a2, long a3);

/* =========================================================================
 * Error string helper
 * ====================================================================== */
const char *uiox_fb_err_str(uiox_fb_err_t e);

/* =========================================================================
 * Phase name helper
 * ====================================================================== */
const char *uiox_fb_phase_str(uiox_fb_phase_t p);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FBOOT_H */
