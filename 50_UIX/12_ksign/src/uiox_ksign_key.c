/**
 * @file  uiox_ksign_key.c
 * @brief UIOX Signed Kernel — key store and KRL implementation.
 * @date  2026-07-07
 */

 #include "../include/uiox_ksign_key.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 static void km_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void km_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 static size_t km_strlen(const char *s){size_t n=0;while(*s++)n++;return n;}
 static void km_strncpy(char *d,const char *s,size_t n)
 { size_t i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]='\0'; }
 static int km_strncmp(const char *a,const char *b,size_t n)
 { while(n--&&*a&&*b){if(*a!=*b)return (int)(unsigned char)*a-(int)(unsigned char)*b;a++;b++;}return 0; }
 
 void uiox_ks_compute_key_id(const uiox_ks_key_entry_t *key,
                                uint8_t key_id[32])
 {
     uiox_ks_sha256_ctx_t ctx;
     uiox_ks_sha256_init(&ctx);
     if (key->alg == UIOX_KS_ALG_RSA2048_SHA256 ||
         key->alg == UIOX_KS_ALG_RSA4096_SHA256) {
         uiox_ks_sha256_update(&ctx, key->rsa.modulus, key->rsa.modulus_len);
     } else if (key->alg == UIOX_KS_ALG_ECDSA_P256) {
         uiox_ks_sha256_update(&ctx, key->ecdsa.xy, UIOX_KS_ECDSA_P256_KEY_LEN);
     }
     uiox_ks_sha256_final(&ctx, key_id);
 }
 
 uiox_ks_err_t uiox_ks_keystore_init(uiox_ks_keystore_t *ks,
                                        const uint8_t rot_hash[32])
 {
     if (!ks) return UIOX_KS_ERR_INVAL;
     km_memset(ks, 0, sizeof(*ks));
     if (rot_hash) km_memcpy(ks->rot_key_id, rot_hash, 32u);
     ks->initialized = true;
     return UIOX_KS_OK;
 }
 
 uiox_ks_err_t uiox_ks_key_add(uiox_ks_keystore_t *ks,
                                  const uiox_ks_key_entry_t *key)
 {
     if (!ks || !key) return UIOX_KS_ERR_INVAL;
     if (ks->key_count >= UIOX_KS_MAX_KEYS) return UIOX_KS_ERR_NOMEM;
     if (key->magic != UIOX_KS_KEY_MAGIC) return UIOX_KS_ERR_BADMAGIC;
     ks->keys[ks->key_count++] = *key;
     return UIOX_KS_OK;
 }
 
 uiox_ks_key_entry_t *uiox_ks_key_find_by_id(uiox_ks_keystore_t *ks,
                                                const uint8_t key_id[32])
 {
     if (!ks || !key_id) return NULL;
     for (uint32_t i=0u;i<ks->key_count;i++)
         if (uiox_ks_ct_memcmp(ks->keys[i].key_id, key_id, 32u) == 0)
             return &ks->keys[i];
     return NULL;
 }
 
 uiox_ks_key_entry_t *uiox_ks_key_find_by_name(uiox_ks_keystore_t *ks,
                                                  const char *name)
 {
     if (!ks || !name) return NULL;
     for (uint32_t i=0u;i<ks->key_count;i++)
         if (km_strncmp(ks->keys[i].name, name, UIOX_KS_KEY_NAME_LEN) == 0)
             return &ks->keys[i];
     return NULL;
 }
 
 uiox_ks_err_t uiox_ks_key_verify_chain(uiox_ks_keystore_t *ks,
                                           const uiox_ks_key_entry_t *key)
 {
     if (!ks || !key) return UIOX_KS_ERR_INVAL;
     if (key->is_root) {
         /* Root: verify self-signed against stored rot_key_id */
         return (uiox_ks_ct_memcmp(key->key_id,
                                     ks->rot_key_id, 32u) == 0)
                ? UIOX_KS_OK : UIOX_KS_ERR_CERT;
     }
     /* Walk up chain */
     const uiox_ks_key_entry_t *cur = key;
     for (int depth=0; depth < 4; depth++) {
         uiox_ks_key_entry_t *issuer =
             uiox_ks_key_find_by_id(ks, cur->issuer_key_id);
         if (!issuer) return UIOX_KS_ERR_CERT;
         /* Verify cur is signed by issuer */
         uint8_t cur_hash[UIOX_KS_SHA256_LEN];
         /* Hash the key public bytes */
         uiox_ks_sha256(cur->key_id, 32u, cur_hash);
         uiox_ks_err_t rc = UIOX_KS_ERR_SIG;
         if (issuer->alg == UIOX_KS_ALG_RSA2048_SHA256 ||
             issuer->alg == UIOX_KS_ALG_RSA4096_SHA256) {
             rc = uiox_ks_rsa_verify(&issuer->rsa,
                                      cur->issuer_sig, cur->issuer_sig_len,
                                      cur_hash);
         } else if (issuer->alg == UIOX_KS_ALG_ECDSA_P256) {
             rc = uiox_ks_ecdsa_verify(&issuer->ecdsa,
                                         cur->issuer_sig, cur_hash);
         }
         if (rc != UIOX_KS_OK) return UIOX_KS_ERR_CERT;
         if (issuer->is_root) return UIOX_KS_OK;
         cur = issuer;
     }
     return UIOX_KS_ERR_CERT;
 }
 
 bool uiox_ks_key_is_revoked(const uiox_ks_keystore_t *ks,
                                const uint8_t key_id[32])
 {
     if (!ks || !ks->krl) return false;
     for (uint32_t i=0u;i<ks->krl->entry_count;i++) {
         if (uiox_ks_ct_memcmp(ks->krl->revoked_key_ids[i],
                                 key_id, 32u) == 0)
             return true;
     }
     return false;
 }
 
 uiox_ks_err_t uiox_ks_krl_load(uiox_ks_keystore_t *ks, uiox_ks_krl_t *krl)
 {
     if (!ks || !krl) return UIOX_KS_ERR_INVAL;
     if (krl->magic != UIOX_KS_KRL_MAGIC) return UIOX_KS_ERR_BADMAGIC;
     /* Verify KRL signature against root CA key */
     uint8_t krl_hash[UIOX_KS_SHA256_LEN];
     uiox_ks_sha256((const uint8_t *)krl,
                     sizeof(*krl) - krl->krl_sig_len - UIOX_KS_SHA256_LEN,
                     krl_hash);
     if (uiox_ks_ct_memcmp(krl_hash, krl->krl_hash,
                             UIOX_KS_SHA256_LEN) != 0)
         return UIOX_KS_ERR_HASH;
     ks->krl = krl;
     return UIOX_KS_OK;
 }
 
 bool uiox_ks_key_is_valid(const uiox_ks_key_entry_t *key, uint64_t now_unix)
 {
     if (!key || !key->active) return false;
     if (now_unix < key->not_before) return false;
     if (key->not_after != 0u && now_unix > key->not_after) return false;
     return true;
 }
 
 void uiox_ks_keystore_print(const uiox_ks_keystore_t *ks)
 {
     if (!ks) return;
     uiox_fw_printf("[ksign] Key store (%u keys):\n", ks->key_count);
     for (uint32_t i=0u;i<ks->key_count;i++) {
         const uiox_ks_key_entry_t *k = &ks->keys[i];
         uiox_fw_printf("  [%u] %-32s  alg=%u  root=%d  active=%d\n",
                         i, k->name, (uint32_t)k->alg,
                         (int)k->is_root, (int)k->active);
     }
     if (ks->krl)
         uiox_fw_printf("  KRL: %u revoked keys\n", ks->krl->entry_count);
 }
 