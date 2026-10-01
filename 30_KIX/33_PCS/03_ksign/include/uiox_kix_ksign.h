/**
 * @file  uiox_kix_ksign.h
 * @brief UIOX Signed Kernel — master umbrella include.
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 * This header used to advertise six functions — uiox_ksign_boot_verify,
 * uiox_ksign_runtime_start, uiox_ksign_tick, and three context
 * accessors — that nothing defined.  uiox_kix_ksign.c defines a different
 * set, keyed on uiox_ks_boot_entry, which is what 02_FwHal's Stage 0d
 * actually calls.  A header promising an interface no source implements
 * is the defect class this module had in four of its files; here it is
 * resolved in favour of the specific names, because they are what the
 * boot path calls.
 *
 * @version 1.1.0
 * @date    2026-10-01
 */

 #ifndef UIOX_KIX_KSIGN_H
 #define UIOX_KIX_KSIGN_H
 
 #include "uiox_kix_ksign_types.h"
 #include "uiox_kix_ksign_crypto.h"
 #include "uiox_kix_ksign_key.h"
 #include "uiox_kix_ksign_image.h"
 #include "uiox_kix_ksign_verify.h"
 #include "uiox_kix_ksign_measure.h"
 #include "uiox_kix_ksign_runtime.h"
 
 #define UIOX_KIX_KSIGN_VERSION_STR  "UIOX ksign v1.1"
 #define UIOX_KIX_KSIGN_URL          "github.com/Pramod645/UIOX"
 
 /* =========================================================================
  * Boot path — 02_FwHal's uiox_fw_secboot Stage 0d calls uiox_ks_boot_entry.
  *
  * The five steps below run in order.  Each returns UIOX_KS_ERR_* and a
  * non-OK result from any of them is fatal: the boot entry halts via
  * uiox_ks_plat_halt() rather than continuing with an unverified kernel.
  * ====================================================================== */
 
 /** Step 1 — initialise every sub-context. */
 uiox_ks_err_t uiox_ks_boot_init(void);
 
 /** Step 2 — full signature, certificate chain and anti-rollback check. */
 uiox_ks_err_t uiox_ks_boot_verify(const void *image, size_t image_size);
 
 /** Step 3 — extend the PCR chain with the verified image's metadata. */
 uiox_ks_err_t uiox_ks_boot_measure(const void *image, size_t image_size);
 
 /**
  * Step 4 — seed the runtime monitor from the VERIFIED payload hash.
  *
  * The monitored region is the payload — the extent the signature covered
  * — so the caller passes the image, not section boundaries.  See
  * uiox_ks_rt_seed_from_image().
  *
  * text_base and text_size are NOT parameters: under option A the extent
  * comes from the header.
  */
 uiox_ks_err_t uiox_ks_boot_arm_runtime(const void *image,
                                           uintptr_t   rodata_base,
                                           size_t      rodata_size);
 
 /** Step 5 — lock the PCRs and jump to the kernel entry point.  Does not
  *  return on success. */
 uiox_ks_err_t uiox_ks_boot_handoff(const void *image);
 
 /**
  * Master entry — called from uiox_fw_secboot Stage 0d.
  *
  * Runs the five steps in order and halts on any failure.  Halting rather
  * than returning is the whole point: a kernel that fails verification
  * must not reach kernel_main, and a caller that could ignore the result
  * eventually would.
  *
  * text_base and text_size are retained so the BSP call site does not
  * change; they are informational under option A.
  */
 void uiox_ks_boot_entry(const void *image,
                           size_t      image_size,
                           uintptr_t   text_base,
                           size_t      text_size,
                           uintptr_t   rodata_base,
                           size_t      rodata_size);
 
 /* =========================================================================
  * Scheduler integration — 33_ProcessControlSubsystem tick
  * ====================================================================== */
 
 /**
  * Call from the scheduler tick.  Runs uiox_ks_rt_tick() and reports a
  * tamper.  Policy on a violation is the platform's: the default logs,
  * and a platform wanting strict mode overrides uiox_ks_plat_halt().
  */
 void uiox_ks_scheduler_tick(void);
 
 /* =========================================================================
  * Attestation — 40_SystemCallInterface's sys_ksign_quote
  * ====================================================================== */
 
 /** Fill quote with SHA-256 over every PCR.  See the measure module. */
 void uiox_ks_get_attestation_quote(uint8_t quote[UIOX_KS_SHA256_LEN]);
 
 /* =========================================================================
  * Context accessors
  *
  * These replace the three the old header advertised; those had no
  * definitions at all.  Each returns the static context the boot entry
  * owns, so a diagnostic can read state without a second copy.
  * ====================================================================== */
 uiox_ks_verify_ctx_t   *uiox_ksign_get_verify_ctx (void);
 uiox_ks_measure_ctx_t  *uiox_ksign_get_measure_ctx(void);
 uiox_ks_rt_ctx_t       *uiox_ksign_get_rt_ctx     (void);
 
 /* =========================================================================
  * Platform hooks — override in the BSP / board layer.
  *
  * Each has a weak default in uiox_kix_ksign.c.  The defaults are for a
  * development build: the OTP read returns zeroes and the time source
  * returns 0, which means no clock — and no clock now means the expiry
  * window is reported as NOT CHECKED rather than silently passing.
  * ====================================================================== */
 void     uiox_ks_plat_read_rot_key_id(uint8_t out[UIOX_KS_KEY_ID_LEN]);
 uint32_t uiox_ks_plat_read_min_version(void);
 uint64_t uiox_ks_plat_get_time_ms(void);
 void     uiox_ks_plat_halt(void);
 
 #endif /* UIOX_KIX_KSIGN_H */
 