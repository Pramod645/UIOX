/**
 * @file  uiox_fboot_types.h
 * @brief UIOX Fast Boot — base types, error codes, phase IDs, timing.
 *
 * Integrates with:
 *   02_FwHal/uiox_fw_hal.h          — hardware abstraction (timer, GPIO)
 *   33_ProcessControlSubsystem       — process launch after handoff
 *   40_SystemCallInterface           — sys_boot_status()
 *   50_UIX/12_ksign                  — kernel verification feeds phase timing
 *
 * Design goal: power-on → shell ready in < 3 s on Cortex-A55 class SoC.
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_FBOOT_TYPES_H
#define UIOX_FBOOT_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Error codes
 * ====================================================================== */
typedef enum {
    UIOX_FB_OK               =  0,
    UIOX_FB_ERR_INVAL        = -1,
    UIOX_FB_ERR_NOMEM        = -2,
    UIOX_FB_ERR_TIMEOUT      = -3,
    UIOX_FB_ERR_BADMAGIC     = -4,
    UIOX_FB_ERR_BADVERSION   = -5,
    UIOX_FB_ERR_DEP          = -6,  /**< Dependency not satisfied        */
    UIOX_FB_ERR_SKIP         = -7,  /**< Phase skipped (suspend-resume)  */
    UIOX_FB_ERR_ALREADY      = -8,  /**< Phase already completed         */
    UIOX_FB_ERR_IO           = -9,
    UIOX_FB_ERR_OVERFLOW     = -10,
} uiox_fb_err_t;

/* =========================================================================
 * Boot phase identifiers
 *
 * Ordered by execution sequence. Each phase is independently timed.
 * ====================================================================== */
typedef enum {
    UIOX_FB_PHASE_RESET          = 0,   /**< CPU reset vector              */
    UIOX_FB_PHASE_CLK_PLL        = 1,   /**< Clock / PLL init              */
    UIOX_FB_PHASE_DDR_INIT       = 2,   /**< DDR training & calibration    */
    UIOX_FB_PHASE_FW_VERIFY      = 3,   /**< Kernel signature verify       */
    UIOX_FB_PHASE_DECOMPRESS     = 4,   /**< Kernel / initrd decompress    */
    UIOX_FB_PHASE_DEVTREE        = 5,   /**< Device-tree parse             */
    UIOX_FB_PHASE_EARLY_DRIVERS  = 6,   /**< UART, GPIO, minimal timer     */
    UIOX_FB_PHASE_FS_MOUNT       = 7,   /**< Root filesystem mount         */
    UIOX_FB_PHASE_SUSPEND_RESUME = 8,   /**< Snapshot restore (if avail.)  */
    UIOX_FB_PHASE_INIT_SPAWN     = 9,   /**< PID-1 / shell spawn           */
    UIOX_FB_PHASE_SHELL_READY    = 10,  /**< First prompt displayed        */
    UIOX_FB_PHASE__COUNT         = 11,
} uiox_fb_phase_t;

/* =========================================================================
 * Boot mode
 * ====================================================================== */
typedef enum {
    UIOX_FB_MODE_COLD        = 0,  /**< Full cold boot                     */
    UIOX_FB_MODE_RESUME      = 1,  /**< Suspend-to-RAM resume              */
    UIOX_FB_MODE_SNAPSHOT    = 2,  /**< Restore from disk snapshot         */
    UIOX_FB_MODE_WARMRESET   = 3,  /**< Warm reset (no DDR retrain)        */
} uiox_fb_mode_t;

/* =========================================================================
 * Per-phase timing record
 * ====================================================================== */
typedef struct {
    uiox_fb_phase_t  phase;
    uint64_t         start_us;      /**< µs since reset vector             */
    uint64_t         end_us;
    uint64_t         duration_us;
    bool             completed;
    bool             skipped;
    uiox_fb_err_t    result;
    char             label[32];
} uiox_fb_phase_record_t;

/* =========================================================================
 * Full boot timing context
 * ====================================================================== */
#define UIOX_FB_MAGIC           0x55464254u   /**< "UFBT"                  */
#define UIOX_FB_VERSION         1u
#define UIOX_FB_MAX_PHASES      UIOX_FB_PHASE__COUNT

typedef struct {
    uint32_t                magic;
    uint32_t                version;
    uiox_fb_mode_t          mode;
    uint64_t                boot_start_us;    /**< Anchor: reset vector t=0 */
    uint64_t                shell_ready_us;   /**< Time to first prompt     */
    uint64_t                target_us;        /**< Budget (default 3 000 000)*/
    uiox_fb_phase_record_t  phases[UIOX_FB_MAX_PHASES];
    uint32_t                phases_done;
    bool                    budget_exceeded;
    bool                    snapshot_valid;
} uiox_fb_ctx_t;

/* =========================================================================
 * Snapshot header (stored in dedicated flash / disk partition)
 * ====================================================================== */
#define UIOX_FB_SNAP_MAGIC      0x55465350u   /**< "UFSP"                  */
#define UIOX_FB_SNAP_VERSION    1u
#define UIOX_FB_SNAP_HASH_LEN   32u

typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  version;
    uint64_t  capture_time;         /**< Unix timestamp of snapshot        */
    uint64_t  image_size;           /**< Compressed snapshot size (bytes)  */
    uint64_t  raw_size;             /**< Uncompressed size                 */
    uint8_t   hash[UIOX_FB_SNAP_HASH_LEN]; /**< SHA-256 of compressed data*/
    uint32_t  kernel_version;       /**< Must match running kernel         */
    uint32_t  flags;
    uint8_t   _pad[16];
} uiox_fb_snap_hdr_t;

#define UIOX_FB_SNAP_FLAG_VALID       (1u << 0)
#define UIOX_FB_SNAP_FLAG_COMPRESSED  (1u << 1)
#define UIOX_FB_SNAP_FLAG_ENCRYPTED   (1u << 2)

/* =========================================================================
 * Deferred init descriptor
 *
 * Non-critical drivers / subsystems registered here are started
 * asynchronously after the shell is already visible.
 * ====================================================================== */
#define UIOX_FB_DEFER_NAME_LEN  48u
#define UIOX_FB_MAX_DEFERRED    32u

typedef uiox_fb_err_t (*uiox_fb_init_fn_t)(void *arg);

typedef struct {
    char               name[UIOX_FB_DEFER_NAME_LEN];
    uiox_fb_init_fn_t  fn;
    void              *arg;
    uint32_t           priority;       /**< Lower = run first              */
    bool               registered;
    bool               completed;
    uiox_fb_err_t      result;
    uint64_t           duration_us;
} uiox_fb_deferred_t;

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FBOOT_TYPES_H */
