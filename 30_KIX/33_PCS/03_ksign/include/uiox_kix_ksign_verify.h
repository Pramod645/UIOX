/**
 * @file  uiox_kix_ksign_verify.h
 * @brief UIOX Signed Kernel — signature verification engine.
 *
 * Full verification pipeline:
 *   1. Parse image header
 *   2. Verify header magic + version
 *   3. Anti-rollback check
 *   4. Hash kernel payload (SHA-256)
 *   5. Compare against header hashes
 *   6. Locate .uiox_sig section
 *   7. For each signature entry:
 *      a. Find signing key in key store
 *      b. Check key not revoked in KRL
 *      c. Check key expiry
 *      d. Verify certificate chain to root CA
 *      e. Verify signature over payload hash
 *   8. Extend PCR measurements
 *   9. Return OK only if at least one valid signature found
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 *   sim_mode is gone.  It was never set from anywhere — it was whatever
 *   static zeroing left, i.e. false — so its branches were unreachable
 *   and the "accept an unsigned image" path could never be taken.  A
 *   branch that cannot be reached is worse than no branch: it reads as a
 *   supported mode.  An unsigned kernel no longer boots, which is the
 *   behaviour the branch was pretending to offer a way around.
 *
 *   The context records WHERE the verified image lives (image_base and
 *   image_size), so a caller arming the runtime monitor does not have to
 *   be handed the image a second time.
 *
 *   expiry_checked is reported, so "the time window was evaluated and
 *   passed" is distinguishable from "there was no clock and it was not
 *   evaluated".  Before this, both looked like a pass.
 *
 * @version 1.1.0
 * @date    2026-10-01
 */

 #ifndef UIOX_KIX_KSIGN_VERIFY_H
 #define UIOX_KIX_KSIGN_VERIFY_H
 
 #include "uiox_kix_ksign_image.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Verification report
  * ====================================================================== */
 
 typedef struct {
     uiox_ks_err_t  result;
     bool           header_ok;
     bool           hash_ok;
     bool           sig_ok;
     bool           cert_chain_ok;
     bool           krl_ok;
     bool           rollback_ok;
     bool           expiry_checked;  /**< false = no clock; window not evaluated */
     uint32_t       sigs_checked;
     uint32_t       sigs_valid;
     uint32_t       kernel_version;
     uint8_t        payload_hash[UIOX_KS_SHA256_LEN];
     uint8_t        signing_key_id[UIOX_KS_KEY_ID_LEN];
     char           fail_reason[128];
 } uiox_ks_verify_report_t;
 
 /* =========================================================================
  * Verification context
  * ====================================================================== */
 
 typedef struct {
     uiox_ks_keystore_t    *keystore;
     uiox_ks_measure_ctx_t *measure;            /**< PCR extends on sig events */
     uint32_t               min_kernel_version; /**< Anti-rollback floor       */
     uint64_t               current_time_unix;  /**< For expiry check          */
     uintptr_t              image_base;         /**< What was verified         */
     size_t                 image_size;         /**< And how much of it        */
     bool                   allow_test_keys;    /**< Accept UIOX_KS_FLAG_TEST_SIGNED */
 } uiox_ks_verify_ctx_t;
 
 /* =========================================================================
  * Verification API
  * ====================================================================== */
 
 /**
  * Initialise the verification context.
  *
  * @param ctx        Output context.
  * @param keystore   Initialised key store with the root CA loaded.
  * @param measure    Measurement context, extended on signature events.
  *                   May be NULL — verification then runs without
  *                   measuring, which is weaker and is visible as an
  *                   absent PCR rather than a silent one.
  * @param min_ver    Anti-rollback minimum kernel version.
  */
 uiox_ks_err_t uiox_ks_verify_init(uiox_ks_verify_ctx_t   *ctx,
                                      uiox_ks_keystore_t    *keystore,
                                      uiox_ks_measure_ctx_t *measure,
                                      uint32_t               min_ver);
 
 /**
  * Full kernel image verification.
  * @param ctx      Verification context.
  * @param image    Pointer to the start of the signed image in memory.
  * @param img_size Total image size in bytes.
  * @param report   Optional output report (pass NULL to ignore).
  */
 uiox_ks_err_t uiox_ks_verify_image(uiox_ks_verify_ctx_t *ctx,
                                       const void *image, size_t img_size,
                                       uiox_ks_verify_report_t *report);
 
 /** Verify a single signature entry from the .uiox_sig section. */
 uiox_ks_err_t uiox_ks_verify_sig_entry(uiox_ks_verify_ctx_t *ctx,
                                           const uiox_ks_sig_entry_t *entry,
                                           const uint8_t hash[UIOX_KS_SHA256_LEN]);
 
 /**
  * Verify the full certificate chain of @entry's signing key.
  *
  * Delegates to uiox_ks_key_walk_chain() in the key module — the walk
  * reads the keystore and nothing else, so it lives there.  This wrapper
  * exists so the verification pipeline reads as one sequence.
  */
 uiox_ks_err_t uiox_ks_verify_cert_chain(uiox_ks_verify_ctx_t *ctx,
                                            const uiox_ks_sig_entry_t *entry);
 
 /** Print verification report. */
 void          uiox_ks_verify_print(const uiox_ks_verify_report_t *r);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KSIGN_VERIFY_H */
 