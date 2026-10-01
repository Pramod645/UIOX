/**
 * @file  uiox_kix_ksign_verify.c
 * @brief UIOX Signed Kernel — full verification pipeline.
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 *   verify_init takes the measurement context and no longer takes
 *   sim_mode.  The previous call passed &g_measure_ctx in the min_ver
 *   slot and omitted the last argument entirely.
 *
 *   ctx->image_base and ctx->image_size are ASSIGNED, after the header
 *   parses.  They were read by uiox_kix_ksign.c's boot_arm_runtime and
 *   never written, so that read returned NULL and its guard fired on
 *   every boot; runtime monitoring never armed.
 *
 *   The expiry check reports whether it ran.  now_sec == 0 means there is
 *   no clock — the state before timekeeping_init() — and skipping the
 *   window is necessary then, but it is now a recorded fact.
 *
 *   PCR[6] KEY_LOAD is extended when a signature VERIFIES, not when one
 *   is merely present.
 *
 * @version 1.1.0
 * @date    2026-10-01
 */

 #include "../include/uiox_kix_ksign_verify.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 static void vf_memset(void *d, int v, size_t n)
 { uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }
 
 static void vf_strncpy(char *d, const char *s, size_t n)
 { size_t i = 0; while (i < n - 1 && s[i]) { d[i] = s[i]; i++; } d[i] = '\0'; }
 
 static void vf_memcpy(void *d, const void *s, size_t n)
 { uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
   while (n--) *dp++ = *sp++; }
 
 /* =========================================================================
  * Init
  * ====================================================================== */
 uiox_ks_err_t uiox_ks_verify_init(uiox_ks_verify_ctx_t   *ctx,
                                      uiox_ks_keystore_t    *keystore,
                                      uiox_ks_measure_ctx_t *measure,
                                      uint32_t               min_ver)
 {
     if (!ctx || !keystore) return UIOX_KS_ERR_INVAL;
 
     vf_memset(ctx, 0, sizeof(*ctx));
     ctx->keystore           = keystore;
     ctx->measure            = measure;   /* may be NULL — allowed */
     ctx->min_kernel_version = min_ver;
     ctx->current_time_unix  = 0u;
     ctx->allow_test_keys    = false;
     ctx->image_base         = (uintptr_t)0;
     ctx->image_size         = 0u;
     return UIOX_KS_OK;
 }
 
 /* =========================================================================
  * Full image verification
  * ====================================================================== */
 uiox_ks_err_t uiox_ks_verify_image(uiox_ks_verify_ctx_t *ctx,
                                       const void *image, size_t img_size,
                                       uiox_ks_verify_report_t *report)
 {
     uiox_ks_verify_report_t local;
     uiox_ks_verify_report_t *r = report ? report : &local;
     uiox_ks_img_hdr_t        hdr;
     const uiox_ks_sig_section_hdr_t *sig_sec;
     const uint8_t           *entry_ptr;
     uint8_t                  actual_hash[UIOX_KS_SHA256_LEN];
     size_t                   sig_sec_size;
     uiox_ks_err_t            rc;
 
     vf_memset(r, 0, sizeof(*r));
 
     if (!ctx || !image || img_size == 0u) {
         r->result = UIOX_KS_ERR_INVAL;
         return r->result;
     }
 
     /* ── Step 1: Parse image header ─────────────────────────── */
     rc = uiox_ks_img_parse_hdr(image, img_size, &hdr);
     if (rc != UIOX_KS_OK) {
         r->result = rc;
         vf_strncpy(r->fail_reason, "bad image header", 127u);
         return rc;
     }
     r->header_ok      = true;
     r->kernel_version = hdr.kernel_version;
 
     /* Record what was verified.  uiox_kix_ksign.c's boot_arm_runtime
      * reads this to arm the runtime monitor; it was never assigned, so
      * that read returned NULL and the monitor never armed. */
     ctx->image_base = (uintptr_t)image;
     ctx->image_size = img_size;
 
     /* ── Step 2: Anti-rollback ───────────────────────────────── */
     rc = uiox_ks_img_check_version(&hdr, ctx->min_kernel_version);
     if (rc != UIOX_KS_OK) {
         r->result = rc;
         vf_strncpy(r->fail_reason, "rollback denied", 127u);
         return rc;
     }
     r->rollback_ok = true;
 
     /* ── Step 3: Hash payload ───────────────────────────────── */
     rc = uiox_ks_img_hash_payload(image, img_size, &hdr, actual_hash);
     if (rc != UIOX_KS_OK) {
         r->result = rc;
         vf_strncpy(r->fail_reason, "payload hash failed", 127u);
         return rc;
     }
 
     /* Compare against header hash.  Constant-time: the answer decides
      * whether the image is trusted. */
     if (uiox_ks_ct_memcmp(actual_hash, hdr.payload_hash,
                           UIOX_KS_SHA256_LEN) != 0) {
         r->result = UIOX_KS_ERR_HASH;
         vf_strncpy(r->fail_reason, "SHA-256 payload mismatch", 127u);
         return r->result;
     }
     r->hash_ok = true;
     vf_memcpy(r->payload_hash, actual_hash, UIOX_KS_SHA256_LEN);
 
     /* ── Step 4: Locate .uiox_sig section ────────────────────── */
     sig_sec      = (const uiox_ks_sig_section_hdr_t *)0;
     sig_sec_size = 0u;
 
     rc = uiox_ks_img_get_sig_section(image, img_size, &hdr,
                                        &sig_sec, &sig_sec_size);
     if (rc != UIOX_KS_OK || !sig_sec) {
         /* No signature section is FATAL.  There is no simulation path
          * any more — an unsigned kernel does not boot. */
         r->result = UIOX_KS_ERR_SIG;
         vf_strncpy(r->fail_reason, "no .uiox_sig section", 127u);
         return r->result;
     }
 
     /* ── Step 5: Verify each signature entry ────────────────── */
     entry_ptr = (const uint8_t *)sig_sec + sizeof(uiox_ks_sig_section_hdr_t);
 
     for (uint32_t i = 0u; i < sig_sec->sig_count; i++) {
         const uiox_ks_sig_entry_t *entry =
             (const uiox_ks_sig_entry_t *)entry_ptr;
         const uiox_ks_key_entry_t *key;
         bool expiry_checked = false;
 
         r->sigs_checked++;
 
         /* 5a. Check key in store */
         key = uiox_ks_keystore_find(ctx->keystore, entry->signer_key_id);
         if (!key) { entry_ptr += sizeof(*entry); continue; }
 
         /* 5b/5c. Revocation and expiry, in one call */
         rc = uiox_ks_key_check_valid(key, ctx->keystore->krl,
                                        ctx->current_time_unix,
                                        &expiry_checked);
 
         if (expiry_checked) r->expiry_checked = true;
 
         if (rc == UIOX_KS_ERR_REVOKED) {
             r->result = UIOX_KS_ERR_REVOKED;
             vf_strncpy(r->fail_reason, "signing key revoked", 127u);
             return r->result;
         }
         r->krl_ok = true;
 
         /* A key outside its window is SKIPPED, not fatal: another
          * signature entry may use a key that is in date. */
         if (rc != UIOX_KS_OK) { entry_ptr += sizeof(*entry); continue; }
 
         /* 5d. Verify certificate chain to the OTP-anchored root */
         rc = uiox_ks_verify_cert_chain(ctx, entry);
         if (rc != UIOX_KS_OK) { entry_ptr += sizeof(*entry); continue; }
         r->cert_chain_ok = true;
 
         /* 5e. Verify signature */
         rc = uiox_ks_verify_sig_entry(ctx, entry, actual_hash);
         if (rc == UIOX_KS_OK) {
             r->sigs_valid++;
             r->sig_ok = true;
             vf_memcpy(r->signing_key_id, entry->signer_key_id,
                       UIOX_KS_KEY_ID_LEN);
 
             /* PCR[6] KEY_LOAD — extended when a signature VERIFIES, not
              * when one is merely present. */
             if (ctx->measure) {
                 uiox_ks_measure_extend_hash(ctx->measure,
                                               UIOX_KS_PCR_KEY_LOAD,
                                               entry->signer_key_id,
                                               "kernel-signing-key",
                                               UIOX_KS_EVT_KEY_LOAD);
             }
         }
         entry_ptr += sizeof(*entry);
     }
 
     if (!r->sig_ok) {
         r->result = UIOX_KS_ERR_SIG;
         vf_strncpy(r->fail_reason, "no valid signature found", 127u);
         return r->result;
     }
 
     r->result = UIOX_KS_OK;
     return UIOX_KS_OK;
 }
 
 /* =========================================================================
  * Verify one signature entry
  * ====================================================================== */
 uiox_ks_err_t uiox_ks_verify_sig_entry(uiox_ks_verify_ctx_t *ctx,
                                           const uiox_ks_sig_entry_t *entry,
                                           const uint8_t hash[UIOX_KS_SHA256_LEN])
 {
     const uiox_ks_key_entry_t *key;
 
     if (!ctx || !entry || !hash) return UIOX_KS_ERR_INVAL;
 
     key = uiox_ks_keystore_find(ctx->keystore, entry->signer_key_id);
     if (!key) return UIOX_KS_ERR_NOTFOUND;
 
     if (key->alg == UIOX_KS_ALG_RSA2048_SHA256 ||
         key->alg == UIOX_KS_ALG_RSA4096_SHA256)
         return uiox_ks_rsa_verify(&key->rsa, entry->sig,
                                     entry->sig_len, hash);
 
     if (key->alg == UIOX_KS_ALG_ECDSA_P256)
         return uiox_ks_ecdsa_verify(&key->ecdsa, entry->sig, hash);
 
     return UIOX_KS_ERR_UNSUP;
 }
 
 /* =========================================================================
  * Verify a certificate chain
  *
  * The walk itself lives in uiox_kix_ksign_key.c — it reads the keystore
  * and nothing else.  This is the pipeline-facing wrapper.
  * ====================================================================== */
 uiox_ks_err_t uiox_ks_verify_cert_chain(uiox_ks_verify_ctx_t *ctx,
                                            const uiox_ks_sig_entry_t *entry)
 {
     const uiox_ks_key_entry_t *key;
     bool expiry_checked = false;
 
     if (!ctx || !entry) return UIOX_KS_ERR_INVAL;
 
     key = uiox_ks_keystore_find(ctx->keystore, entry->signer_key_id);
     if (!key) return UIOX_KS_ERR_NOTFOUND;
 
     return uiox_ks_key_walk_chain(ctx->keystore, key->key_id,
                                     ctx->keystore->krl,
                                     ctx->current_time_unix,
                                     &expiry_checked);
 }
 
 /* =========================================================================
  * Print the report
  * ====================================================================== */
 void uiox_ks_verify_print(const uiox_ks_verify_report_t *r)
 {
     if (!r) return;
     uiox_fw_printf("[ksign] Verify report:\n");
     uiox_fw_printf("  Result      : %s\n", uiox_ks_err_str(r->result));
     uiox_fw_printf("  Header OK   : %s\n", r->header_ok     ? "YES" : "NO");
     uiox_fw_printf("  Hash OK     : %s\n", r->hash_ok       ? "YES" : "NO");
     uiox_fw_printf("  Sig OK      : %s\n", r->sig_ok        ? "YES" : "NO");
     uiox_fw_printf("  Cert chain  : %s\n", r->cert_chain_ok ? "YES" : "NO");
     uiox_fw_printf("  KRL OK      : %s\n", r->krl_ok        ? "YES" : "NO");
     uiox_fw_printf("  Rollback OK : %s\n", r->rollback_ok   ? "YES" : "NO");
     uiox_fw_printf("  Expiry      : %s\n",
                    r->expiry_checked ? "CHECKED"
                                      : "NOT CHECKED (no clock)");
     uiox_fw_printf("  Sigs check  : %u  valid=%u\n",
                    r->sigs_checked, r->sigs_valid);
     uiox_fw_printf("  Kernel ver  : %u\n", r->kernel_version);
     if (r->result != UIOX_KS_OK)
         uiox_fw_printf("  Fail reason : %s\n", r->fail_reason);
 }
 