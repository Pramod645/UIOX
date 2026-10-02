/**
 * @file  uiox_kix_ksign_runtime.h
 * @brief UIOX Signed Kernel — runtime integrity monitoring.
 *
 * Periodically re-hashes critical kernel regions and compares against the
 * verified boot-time baseline.  Detects:
 *   - Live patching without authorization (unregistered kpatch)
 *   - Memory corruption of kernel code
 *   - Rootkit trampolines injected after boot
 *
 * ── the baseline comes from the VERIFIED HEADER ─────────────────────
 * uiox_ks_rt_seed_from_image() takes the hash from hdr->payload_hash,
 * NOT from a re-hash of live memory.  Re-hashing at seed time would
 * adopt whatever is in the region at that moment as the baseline, so a
 * kernel tampered with before arming would be blessed as correct and
 * never detected afterwards.
 *
 * ── option A: the monitored region is the PAYLOAD ───────────────────
 * hdr->payload_hash covers the kernel payload — payload_offset ..
 * payload_offset + payload_size — which is exactly what the signature
 * verified.  So the monitored extent is the payload, not an ELF section,
 * and the check covers the same bytes the signature did.
 *
 * @version 1.1.0
 * @date    2026-10-01
 */

 #ifndef UIOX_KIX_KSIGN_RUNTIME_H
 #define UIOX_KIX_KSIGN_RUNTIME_H
 
 #include "uiox_kix_ksign_measure.h"
 #include "uiox_kix_ksign_image.h"   /* uiox_ks_img_hdr_t, for the seed function */

 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_KS_RT_MAX_REGIONS     8u
 #define UIOX_KS_RT_CHECK_INTERVAL  60000u  /**< Recheck every 60 s     */
 #define UIOX_KS_RT_REGION_NAME_LEN 24u
 
 /* =========================================================================
  * Monitored region descriptor
  * ====================================================================== */
 typedef struct {
     char     name[UIOX_KS_RT_REGION_NAME_LEN];
     uintptr_t base;
     size_t   size;
     uint8_t  expected_hash[UIOX_KS_SHA256_LEN]; /**< From the verified hdr */
     uint64_t last_check_ms;
     uint32_t violation_count;
     bool     active;
 } uiox_ks_rt_region_t;
 
 /* =========================================================================
  * Runtime monitor context
  *
  * UNINIT and TAMPERED are both states the source sets, so both are in
  * the enum.
  * ====================================================================== */
 typedef enum {
     UIOX_KS_RT_STATE_UNINIT   = 0,   /**< rt_init ran; not seeded yet */
     UIOX_KS_RT_STATE_OK       = 1,
     UIOX_KS_RT_STATE_TAMPERED = 2,
     UIOX_KS_RT_STATE_DISABLED = 3,
 } uiox_ks_rt_state_t;
 
 typedef void (*uiox_ks_rt_violation_cb_t)(const uiox_ks_rt_region_t *region,
                                             const uint8_t actual[UIOX_KS_SHA256_LEN],
                                             void *priv);
 
 typedef struct {
     uiox_ks_rt_region_t   regions[UIOX_KS_RT_MAX_REGIONS];
     uint32_t               region_count;
     uiox_ks_rt_state_t     state;
     uiox_ks_measure_ctx_t *measure;    /**< For PCR extends on each check */
     uiox_ks_rt_violation_cb_t cb;
     void                  *cb_priv;
     uint32_t               check_count;      /**< ticks that ran a check */
     uint32_t               violation_count;  /**< total violations seen  */
     uint64_t               (*get_time_ms)(void);
     bool                   panic_on_violation;
     bool                   initialized;      /**< seeded from an image   */
 } uiox_ks_rt_ctx_t;
 
 /* =========================================================================
  * Runtime monitor API
  * ====================================================================== */
 uiox_ks_err_t uiox_ks_rt_init       (uiox_ks_rt_ctx_t *ctx,
                                         uiox_ks_measure_ctx_t *measure,
                                         uint64_t (*get_time_ms)(void));
 
 /**
  * Register a kernel region to monitor.
  * The region is registered INACTIVE — no expected hash yet — and is
  * activated by uiox_ks_rt_register_hash().
  */
 uiox_ks_err_t uiox_ks_rt_register   (uiox_ks_rt_ctx_t *ctx,
                                         const char *name,
                                         uintptr_t base, size_t size);
 
 /**
  * Bind a known-good hash to a registered region and activate it.
  * The hash must come from the VERIFIED image header.
  * Creates the region if it is not already registered.
  */
 uiox_ks_err_t uiox_ks_rt_register_hash(uiox_ks_rt_ctx_t *ctx,
                                           const char *name,
                                           uintptr_t base, size_t size,
                                           const uint8_t expected[UIOX_KS_SHA256_LEN]);
 
 /**
  * Seed the regions from a verified image header.
  *
  * Monitors the PAYLOAD — the extent the signature covered — rather than
  * an ELF section, so the check covers exactly what was verified.  The
  * caller passes the image; the payload base and length come from the
  * header.
  *
  * The baseline hash is hdr->payload_hash and is NOT recomputed from live
  * memory.  A re-hash at seed time would adopt whatever was in the region
  * at that moment, blessing a pre-boot tamper as correct.
  *
  * @param image   Start of the signed image, as passed to verification.
  */
 uiox_ks_err_t uiox_ks_rt_seed_from_image(uiox_ks_rt_ctx_t        *ctx,
                                             const uiox_ks_img_hdr_t *hdr,
                                             const void              *image,
                                             uintptr_t                rodata_base,
                                             size_t                   rodata_size);
 
 /**
  * Tick: call from the scheduler tick (every 60 s by default).
  * Rehashes each active region whose interval has elapsed and compares.
  * A now_ms of 0 falls back to ctx->get_time_ms().
  */
 void          uiox_ks_rt_tick        (uiox_ks_rt_ctx_t *ctx, uint64_t now_ms);
 
 /** Set violation callback. */
 void          uiox_ks_rt_set_cb      (uiox_ks_rt_ctx_t *ctx,
                                         uiox_ks_rt_violation_cb_t cb,
                                         void *priv);
 
 /** Force an immediate full check of all active regions. */
 uiox_ks_rt_state_t uiox_ks_rt_check_all(uiox_ks_rt_ctx_t *ctx);
 
 void          uiox_ks_rt_print       (const uiox_ks_rt_ctx_t *ctx);
 
 /* =========================================================================
  * Syscall interface
  * ====================================================================== */
 #define SYS_KERNEL_VERIFY    220u
 #define SYS_KSIGN_STATUS     221u
 #define SYS_KSIGN_QUOTE      222u
 
 long sys_kernel_verify(long image_addr, long image_size, long flags, long a3);
 long sys_ksign_status (long buf,        long buf_size,   long a2,    long a3);
 long sys_ksign_quote  (long buf,        long buf_size,   long a2,    long a3);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KSIGN_RUNTIME_H */
 