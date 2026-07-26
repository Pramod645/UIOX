/**
 * @file  uiox_fboot_snapshot.h
 * @brief UIOX Fast Boot — suspend-to-disk snapshot save/restore.
 *
 * Inspired by ChromeOS / Android fast resume. On clean shutdown the kernel
 * memory image is compressed (LZ4-style) and written to a dedicated snapshot
 * partition. On next boot Stage 0d detects a valid snapshot, skips
 * DDR training, decompresses directly into RAM, and resumes execution.
 *
 * Savings: DDR training (~300 ms) + kernel decompress (~150 ms) +
 *          driver init (~400 ms) ≈ 850 ms saved per boot.
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_FBOOT_SNAPSHOT_H
#define UIOX_FBOOT_SNAPSHOT_H

#include "uiox_fboot_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Snapshot context
 * ====================================================================== */
typedef struct {
    uiox_fb_snap_hdr_t  hdr;
    uintptr_t           snap_part_base;  /**< Physical addr of snap partition */
    size_t              snap_part_size;
    uintptr_t           ram_base;        /**< RAM region to save/restore      */
    size_t              ram_size;
    bool                initialized;
} uiox_fb_snap_ctx_t;

/* =========================================================================
 * API
 * ====================================================================== */

/** Initialise snapshot context with partition and RAM boundaries. */
uiox_fb_err_t uiox_fb_snap_init(uiox_fb_snap_ctx_t *ctx,
                                  uintptr_t snap_part_base,
                                  size_t    snap_part_size,
                                  uintptr_t ram_base,
                                  size_t    ram_size);

/**
 * @brief Probe the snapshot partition for a valid, current snapshot.
 * @return UIOX_FB_OK if a valid snapshot exists and can be restored.
 *         UIOX_FB_ERR_BADMAGIC / UIOX_FB_ERR_BADVERSION otherwise.
 */
uiox_fb_err_t uiox_fb_snap_probe(uiox_fb_snap_ctx_t *ctx,
                                   uint32_t current_kernel_version);

/**
 * @brief Restore RAM from snapshot partition.
 *        Must be called before MMU / caches are enabled.
 *        On success, execution resumes at the saved resume vector.
 *        This function does NOT return on success.
 */
uiox_fb_err_t uiox_fb_snap_restore(uiox_fb_snap_ctx_t *ctx);

/**
 * @brief Capture current RAM state into snapshot partition.
 *        Called on clean shutdown by the OS.
 */
uiox_fb_err_t uiox_fb_snap_capture(uiox_fb_snap_ctx_t *ctx,
                                     uint32_t kernel_version);

/** Invalidate snapshot (force cold boot next time). */
uiox_fb_err_t uiox_fb_snap_invalidate(uiox_fb_snap_ctx_t *ctx);

/** Print snapshot header info. */
void          uiox_fb_snap_print(const uiox_fb_snap_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FBOOT_SNAPSHOT_H */
