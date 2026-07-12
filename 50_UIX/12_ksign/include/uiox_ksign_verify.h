/**
 * @file  uiox_ksign_verify.h
 * @brief UIOX Signed Kernel — signature verification engine.
 *
 * Full verification pipeline:
 *   1. Parse image header
 *   2. Verify header magic + version
 *   3. Anti-rollback check
 *   4. Hash kernel payload (SHA-256 + SHA-384)
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
 * @version 1.0.0
 */

 #ifndef UIOX_KSIGN_VERIFY_H
 #define UIOX_KSIGN_VERIFY_H
 
 #include "uiox_ksign_image.h"
 
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
     uiox_ks_keystore_t *keystore;
     uint32_t            min_kernel_version; /**< Anti-rollback floor    */
     uint64_t            current_time_unix;  /**< For expiry check       */
     bool                allow_test_keys;    /**< Allow UIOX_KS_FLAG_TEST */
     bool                sim_mode;           /**< Skip real crypto (QEMU) */
 } uiox_ks_verify_ctx_t;
 
 /* =========================================================================
  * Verification API
  * ====================================================================== */
 
 /**
  * Initialise the verification context.
  * @param ctx         Output context.
  * @param keystore    Initialised key store with root CA loaded.
  * @param min_ver     Anti-rollback minimum kernel version.
  * @param sim_mode    true = QEMU/dev (skip real RSA/ECDSA, accept all sigs)
  */
 uiox_ks_err_t uiox_ks_verify_init      (uiox_ks_verify_ctx_t *ctx,
                                            uiox_ks_keystore_t *keystore,
                                            uint32_t min_ver,
                                            bool sim_mode);
 
 /**
  * Full kernel image verification.
  * @param ctx      Verification context.
  * @param image    Pointer to the start of the signed image in memory.
  * @param img_size Total image size in bytes.
  * @param report   Optional output report (pass NULL to ignore).
  */
 uiox_ks_err_t uiox_ks_verify_image     (uiox_ks_verify_ctx_t *ctx,
                                            const void *image, size_t img_size,
                                            uiox_ks_verify_report_t *report);
 
 /**
  * Verify a single signature entry from the .uiox_sig section.
  */
 uiox_ks_err_t uiox_ks_verify_sig_entry (uiox_ks_verify_ctx_t *ctx,
                                            const uiox_ks_sig_entry_t *entry,
                                            const uint8_t hash[UIOX_KS_SHA256_LEN]);
 
 /**
  * Verify the full certificate chain of @entry's signing key.
  */
 uiox_ks_err_t uiox_ks_verify_cert_chain(uiox_ks_verify_ctx_t *ctx,
                                            const uiox_ks_sig_entry_t *entry);
 
 /** Print verification report. */
 void          uiox_ks_verify_print     (const uiox_ks_verify_report_t *r);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KSIGN_VERIFY_H */
 