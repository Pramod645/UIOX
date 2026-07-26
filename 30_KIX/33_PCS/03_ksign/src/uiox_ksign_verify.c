/**
 * @file  uiox_ksign_verify.c
 * @brief UIOX Signed Kernel — full verification pipeline.
 * @date  2026-07-07
 */

 #include "../include/uiox_ksign_verify.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 static void vf_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void vf_strncpy(char *d,const char *s,size_t n)
 { size_t i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]='\0'; }
 static void vf_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 
 uiox_ks_err_t uiox_ks_verify_init(uiox_ks_verify_ctx_t *ctx,
                                      uiox_ks_keystore_t *keystore,
                                      uint32_t min_ver,
                                      bool sim_mode)
 {
     if (!ctx || !keystore) return UIOX_KS_ERR_INVAL;
     vf_memset(ctx, 0, sizeof(*ctx));
     ctx->keystore             = keystore;
     ctx->min_kernel_version   = min_ver;
     ctx->current_time_unix    = 0u;
     ctx->allow_test_keys      = sim_mode;
     ctx->sim_mode             = sim_mode;
     return UIOX_KS_OK;
 }
 
 uiox_ks_err_t uiox_ks_verify_image(uiox_ks_verify_ctx_t *ctx,
                                       const void *image, size_t img_size,
                                       uiox_ks_verify_report_t *report)
 {
     uiox_ks_verify_report_t local;
     uiox_ks_verify_report_t *r = report ? report : &local;
     vf_memset(r, 0, sizeof(*r));
 
     if (!ctx || !image || img_size == 0u) {
         r->result = UIOX_KS_ERR_INVAL;
         return r->result;
     }
 
     /* ── Step 1: Parse image header ─────────────────────────── */
     uiox_ks_img_hdr_t hdr;
     uiox_ks_err_t rc = uiox_ks_img_parse_hdr(image, img_size, &hdr);
     if (rc != UIOX_KS_OK) {
         r->result = rc;
         vf_strncpy(r->fail_reason, "bad image header", 127u);
         return rc;
     }
     r->header_ok       = true;
     r->kernel_version  = hdr.kernel_version;
 
     /* ── Step 2: Anti-rollback ───────────────────────────────── */
     rc = uiox_ks_img_check_version(&hdr, ctx->min_kernel_version);
     if (rc != UIOX_KS_OK) {
         r->result = rc;
         vf_strncpy(r->fail_reason, "rollback denied", 127u);
         return rc;
     }
     r->rollback_ok = true;
 
     /* ── Step 3: Hash payload ───────────────────────────────── */
     uint8_t actual_hash[UIOX_KS_SHA256_LEN];
     rc = uiox_ks_img_hash_payload(image, img_size, &hdr, actual_hash);
     if (rc != UIOX_KS_OK) {
         r->result = rc;
         vf_strncpy(r->fail_reason, "payload hash failed", 127u);
         return rc;
     }
 
     /* Compare against header hash */
     if (uiox_ks_ct_memcmp(actual_hash, hdr.payload_hash,
                             UIOX_KS_SHA256_LEN) != 0) {
         r->result = UIOX_KS_ERR_HASH;
         vf_strncpy(r->fail_reason, "SHA-256 payload mismatch", 127u);
         return r->result;
     }
     r->hash_ok = true;
     vf_memcpy(r->payload_hash, actual_hash, UIOX_KS_SHA256_LEN);
 
     /* ── Step 4: Locate .uiox_sig section ────────────────────── */
     const uiox_ks_sig_section_hdr_t *sig_sec = NULL;
     size_t sig_sec_size = 0u;
     rc = uiox_ks_img_get_sig_section(image, img_size, &hdr,
                                        &sig_sec, &sig_sec_size);
     if (rc != UIOX_KS_OK || !sig_sec) {
         if (ctx->sim_mode) {
             /* Simulation: accept unsigned image */
             r->sig_ok = true; r->cert_chain_ok = true; r->krl_ok = true;
             r->sigs_valid = 1u;
             goto accept;
         }
         r->result = UIOX_KS_ERR_SIG;
         vf_strncpy(r->fail_reason, "no .uiox_sig section", 127u);
         return r->result;
     }
 
     /* ── Step 5: Verify each signature entry ────────────────── */
     const uint8_t *entry_ptr =
         (const uint8_t *)sig_sec + sizeof(uiox_ks_sig_section_hdr_t);
 
     for (uint32_t i=0u; i<sig_sec->sig_count; i++) {
         const uiox_ks_sig_entry_t *entry =
             (const uiox_ks_sig_entry_t *)entry_ptr;
         r->sigs_checked++;
 
         /* 5a. Check key in store */
         uiox_ks_key_entry_t *key =
             uiox_ks_key_find_by_id(ctx->keystore, entry->signer_key_id);
         if (!key) { entry_ptr += sizeof(*entry); continue; }
 
         /* 5b. Check revocation */
         if (uiox_ks_key_is_revoked(ctx->keystore, entry->signer_key_id)) {
             r->result = UIOX_KS_ERR_REVOKED;
             vf_strncpy(r->fail_reason, "signing key revoked", 127u);
             return r->result;
         }
         r->krl_ok = true;
 
         /* 5c. Check expiry */
         if (!uiox_ks_key_is_valid(key, ctx->current_time_unix) &&
             !ctx->sim_mode) {
             entry_ptr += sizeof(*entry); continue;
         }
 
         /* 5d. Verify certificate chain */
         rc = uiox_ks_verify_cert_chain(ctx, entry);
         if (rc != UIOX_KS_OK && !ctx->sim_mode)
             { entry_ptr += sizeof(*entry); continue; }
         r->cert_chain_ok = true;
 
         /* 5e. Verify signature */
         rc = uiox_ks_verify_sig_entry(ctx, entry, actual_hash);
         if (rc == UIOX_KS_OK) {
             r->sigs_valid++;
             r->sig_ok = true;
             vf_memcpy(r->signing_key_id, entry->signer_key_id, 32u);
         }
         entry_ptr += sizeof(*entry);
     }
 
     if (!r->sig_ok && !ctx->sim_mode) {
         r->result = UIOX_KS_ERR_SIG;
         vf_strncpy(r->fail_reason, "no valid signature found", 127u);
         return r->result;
     }
 
 accept:
     r->result = UIOX_KS_OK;
     return UIOX_KS_OK;
 }
 
 uiox_ks_err_t uiox_ks_verify_sig_entry(uiox_ks_verify_ctx_t *ctx,
                                           const uiox_ks_sig_entry_t *entry,
                                           const uint8_t hash[UIOX_KS_SHA256_LEN])
 {
     if (ctx->sim_mode) return UIOX_KS_OK;
     uiox_ks_key_entry_t *key =
         uiox_ks_key_find_by_id(ctx->keystore, entry->signer_key_id);
     if (!key) return UIOX_KS_ERR_NOTFOUND;
     if (key->alg == UIOX_KS_ALG_RSA2048_SHA256 ||
         key->alg == UIOX_KS_ALG_RSA4096_SHA256)
         return uiox_ks_rsa_verify(&key->rsa, entry->sig, entry->sig_len, hash);
     if (key->alg == UIOX_KS_ALG_ECDSA_P256)
         return uiox_ks_ecdsa_verify(&key->ecdsa, entry->sig, hash);
     return UIOX_KS_ERR_UNSUP;
 }
 
 uiox_ks_err_t uiox_ks_verify_cert_chain(uiox_ks_verify_ctx_t *ctx,
                                            const uiox_ks_sig_entry_t *entry)
 {
     if (ctx->sim_mode) return UIOX_KS_OK;
     uiox_ks_key_entry_t *key =
         uiox_ks_key_find_by_id(ctx->keystore, entry->signer_key_id);
     if (!key) return UIOX_KS_ERR_NOTFOUND;
     return uiox_ks_key_verify_chain(ctx->keystore, key);
 }
 
 void uiox_ks_verify_print(const uiox_ks_verify_report_t *r)
 {
     if (!r) return;
     uiox_fw_printf("[ksign] Verify report:\n");
     uiox_fw_printf("  Result      : %s\n", uiox_ks_err_str(r->result));
     uiox_fw_printf("  Header OK   : %s\n", r->header_ok    ? "YES":"NO");
     uiox_fw_printf("  Hash OK     : %s\n", r->hash_ok      ? "YES":"NO");
     uiox_fw_printf("  Sig OK      : %s\n", r->sig_ok       ? "YES":"NO");
     uiox_fw_printf("  Cert chain  : %s\n", r->cert_chain_ok? "YES":"NO");
     uiox_fw_printf("  KRL OK      : %s\n", r->krl_ok       ? "YES":"NO");
     uiox_fw_printf("  Rollback OK : %s\n", r->rollback_ok  ? "YES":"NO");
     uiox_fw_printf("  Sigs check  : %u  valid=%u\n",
                     r->sigs_checked, r->sigs_valid);
     uiox_fw_printf("  Kernel ver  : %u\n", r->kernel_version);
     if (r->result != UIOX_KS_OK)
         uiox_fw_printf("  Fail reason : %s\n", r->fail_reason);
 }
 