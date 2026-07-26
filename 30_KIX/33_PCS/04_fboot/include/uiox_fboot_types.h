/**
 * @file  uiox_fboot_types.h
 * @brief UIOX Fast Boot — base types, error codes, phase IDs, timing.
 *
 * FIX: Replaced #include <stdint.h>, <stdbool.h>, <stddef.h> with
 * #include "uiox_fw_types.h" — this is a freestanding build (-nostdinc)
 * so system headers are unavailable.  uiox_fw_types.h provides all
 * primitive types via compiler built-ins.
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_FBOOT_TYPES_H
#define UIOX_FBOOT_TYPES_H

#include "uiox_fw_types.h"   /* uint8/16/32/64_t, bool, size_t, uintptr_t */

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
 * ====================================================================== */
typedef enum {
    UIOX_FB_PHASE_RESET          = 0,
    UIOX_FB_PHASE_CLK_PLL        = 1,
    UIOX_FB_PHASE_DDR_INIT       = 2,
    UIOX_FB_PHASE_FW_VERIFY      = 3,
    UIOX_FB_PHASE_DECOMPRESS     = 4,
    UIOX_FB_PHASE_DEVTREE        = 5,
    UIOX_FB_PHASE_EARLY_DRIVERS  = 6,
    UIOX_FB_PHASE_FS_MOUNT       = 7,
    UIOX_FB_PHASE_SUSPEND_RESUME = 8,
    UIOX_FB_PHASE_INIT_SPAWN     = 9,
    UIOX_FB_PHASE_SHELL_READY    = 10,
    UIOX_FB_PHASE__COUNT         = 11,
} uiox_fb_phase_t;

/* =========================================================================
 * Boot mode
 * ====================================================================== */
typedef enum {
    UIOX_FB_MODE_COLD        = 0,
    UIOX_FB_MODE_RESUME      = 1,
    UIOX_FB_MODE_SNAPSHOT    = 2,
    UIOX_FB_MODE_WARMRESET   = 3,
} uiox_fb_mode_t;

/* =========================================================================
 * Per-phase timing record
 * ====================================================================== */
typedef struct {
    uiox_fb_phase_t  phase;
    uint64_t         start_us;
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
    uint64_t                boot_start_us;
    uint64_t                shell_ready_us;
    uint64_t                target_us;
    uiox_fb_phase_record_t  phases[UIOX_FB_MAX_PHASES];
    uint32_t                phases_done;
    bool                    budget_exceeded;
    bool                    snapshot_valid;
} uiox_fb_ctx_t;

/* =========================================================================
 * Snapshot header
 * ====================================================================== */
#define UIOX_FB_SNAP_MAGIC      0x55465350u   /**< "UFSP"                  */
#define UIOX_FB_SNAP_VERSION    1u
#define UIOX_FB_SNAP_HASH_LEN   32u

typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  version;
    uint64_t  capture_time;
    uint64_t  image_size;
    uint64_t  raw_size;
    uint8_t   hash[UIOX_FB_SNAP_HASH_LEN];
    uint32_t  kernel_version;
    uint32_t  flags;
    uint8_t   _pad[16];
} uiox_fb_snap_hdr_t;

#define UIOX_FB_SNAP_FLAG_VALID       (1u << 0)
#define UIOX_FB_SNAP_FLAG_COMPRESSED  (1u << 1)
#define UIOX_FB_SNAP_FLAG_ENCRYPTED   (1u << 2)

/* =========================================================================
 * Deferred init descriptor
 * ====================================================================== */
#define UIOX_FB_DEFER_NAME_LEN  48u
#define UIOX_FB_MAX_DEFERRED    32u

typedef uiox_fb_err_t (*uiox_fb_init_fn_t)(void *arg);

typedef struct {
    char               name[UIOX_FB_DEFER_NAME_LEN];
    uiox_fb_init_fn_t  fn;
    void              *arg;
    uint32_t           priority;
    bool               registered;
    bool               completed;
    uiox_fb_err_t      result;
    uint64_t           duration_us;
} uiox_fb_deferred_t;

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FBOOT_TYPES_H */
