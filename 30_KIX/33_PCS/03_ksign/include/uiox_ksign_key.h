/**
 * @file  uiox_ksign_key.h
 * @brief UIOX Signed Kernel — key store, KRL, key lifecycle.
 *
 * Key hierarchy:
 *   Root CA (burned into OTP / ROM)
 *     └── Intermediate CA (stored in read-only flash)
 *           └── Kernel Signing Key (used to sign each kernel build)
 *
 * @version 1.0.0
 */

 #ifndef UIOX_KSIGN_KEY_H
 #define UIOX_KSIGN_KEY_H
 
 #include "uiox_ksign_crypto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Key entry
  * ====================================================================== */
 
 #define UIOX_KS_KEY_NAME_LEN    48u
 #define UIOX_KS_KEY_ID_LEN      32u   /**< SHA-256 of public key bytes  */
 #define UIOX_KS_MAX_KEYS        16u
 
 typedef struct {
     uint32_t         magic;           /**< UIOX_KS_KEY_MAGIC            */
     uint32_t         version;
     char             name[UIOX_KS_KEY_NAME_LEN];
     uint8_t          key_id[UIOX_KS_KEY_ID_LEN]; /**< SHA-256(pubkey)   */
     uiox_ks_alg_t    alg;
     uint32_t         usage;           /**< UIOX_KS_USAGE_* bitmask      */
     uint64_t         not_before;      /**< Unix timestamp               */
     uint64_t         not_after;       /**< 0 = no expiry                */
     uint32_t         serial;          /**< Monotonic serial number      */
     /* Public key payload (union based on alg) */
     uiox_ks_rsa_pubkey_t    rsa;
     uiox_ks_ecdsa_pubkey_t  ecdsa;
     /* Issuer (for cert chain) */
     uint8_t          issuer_key_id[UIOX_KS_KEY_ID_LEN];
     uint8_t          issuer_sig[UIOX_KS_RSA_SIG_MAX];
     uint32_t         issuer_sig_len;
     bool             is_root;         /**< True = self-signed root CA   */
     bool             active;
 } uiox_ks_key_entry_t;
 
 /* =========================================================================
  * Key Revocation List (KRL)
  * ====================================================================== */
 
 #define UIOX_KS_KRL_MAX_ENTRIES  64u
 
 typedef struct __attribute__((packed)) {
     uint32_t magic;                       /**< UIOX_KS_KRL_MAGIC         */
     uint32_t version;
     uint32_t entry_count;
     uint64_t issued_at;                   /**< Unix timestamp            */
     uint8_t  revoked_key_ids
              [UIOX_KS_KRL_MAX_ENTRIES]
              [UIOX_KS_KEY_ID_LEN];
     uint32_t revoked_serials[UIOX_KS_KRL_MAX_ENTRIES];
     /* KRL itself is signed by the root CA */
     uint8_t  krl_sig[UIOX_KS_RSA_SIG_MAX];
     uint32_t krl_sig_len;
     uint8_t  krl_hash[UIOX_KS_SHA256_LEN];
 } uiox_ks_krl_t;
 
 /* =========================================================================
  * Key store context
  * ====================================================================== */
 
 typedef struct {
     uiox_ks_key_entry_t  keys[UIOX_KS_MAX_KEYS];
     uint32_t             key_count;
     uiox_ks_krl_t       *krl;          /**< Pointer to active KRL       */
     /* Root of trust key ID (from OTP) */
     uint8_t              rot_key_id[UIOX_KS_KEY_ID_LEN];
     bool                 initialized;
 } uiox_ks_keystore_t;
 
 /* =========================================================================
  * Key store API
  * ====================================================================== */
 
 uiox_ks_err_t        uiox_ks_keystore_init    (uiox_ks_keystore_t *ks,
                                                   const uint8_t rot_hash[32]);
 uiox_ks_err_t        uiox_ks_key_add          (uiox_ks_keystore_t *ks,
                                                   const uiox_ks_key_entry_t *key);
 uiox_ks_key_entry_t *uiox_ks_key_find_by_id   (uiox_ks_keystore_t *ks,
                                                   const uint8_t key_id[32]);
 uiox_ks_key_entry_t *uiox_ks_key_find_by_name (uiox_ks_keystore_t *ks,
                                                   const char *name);
 
 /** Verify a key's certificate chain up to the root CA. */
 uiox_ks_err_t        uiox_ks_key_verify_chain (uiox_ks_keystore_t *ks,
                                                   const uiox_ks_key_entry_t *key);
 
 /** Check if a key_id is revoked in the active KRL. */
 bool                 uiox_ks_key_is_revoked   (const uiox_ks_keystore_t *ks,
                                                   const uint8_t key_id[32]);
 
 /** Load and validate a KRL (must be signed by root CA). */
 uiox_ks_err_t        uiox_ks_krl_load         (uiox_ks_keystore_t *ks,
                                                   uiox_ks_krl_t *krl);
 
 /** Check key expiry against current time @now_unix. */
 bool                 uiox_ks_key_is_valid      (const uiox_ks_key_entry_t *key,
                                                   uint64_t now_unix);
 
 /** Compute key_id = SHA-256(public_key_bytes). */
 void                 uiox_ks_compute_key_id   (const uiox_ks_key_entry_t *key,
                                                   uint8_t key_id[32]);
 
 void                 uiox_ks_keystore_print   (const uiox_ks_keystore_t *ks);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KSIGN_KEY_H */
 