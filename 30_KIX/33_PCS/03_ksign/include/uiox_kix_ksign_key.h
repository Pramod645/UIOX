/**
 * @file  uiox_kix_ksign_key.h
 * @brief UIOX Signed Kernel — key store, KRL, key lifecycle.
 *
 * Key hierarchy:
 *   Root CA (burned into OTP / ROM)
 *     └── Intermediate CA (stored in read-only flash)
 *           └── Kernel Signing Key (used to sign each kernel build)
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 * uiox_ks_verify_cert_chain used to be declared here AND in
 * uiox_kix_ksign_verify.h, with two different signatures — one taking a
 * verify context and a signature entry, the other taking a keystore, a
 * leaf key id, a KRL and a timestamp.  Two functions cannot share one
 * name in one link unit.
 *
 * The chain walk belongs to THIS module: it reads the keystore and
 * nothing else.  It is named uiox_ks_key_walk_chain() here, and
 * uiox_kix_ksign_verify.c has a thin wrapper for the pipeline.
 *
 * @version 1.1.0
 * @date    2026-10-01
 */

 #ifndef UIOX_KIX_KSIGN_KEY_H
 #define UIOX_KIX_KSIGN_KEY_H
 
 #include "uiox_kix_ksign_crypto.h"
 
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
     /* Public key payload (one or the other, by alg) */
     uiox_ks_rsa_pubkey_t    rsa;
     uiox_ks_ecdsa_pubkey_t  ecdsa;
     /* Issuer, for the cert chain */
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
     /* Root of trust key ID, read from OTP */
     uint8_t              rot_key_id[UIOX_KS_KEY_ID_LEN];
     bool                 initialized;
 } uiox_ks_keystore_t;
 
 /* =========================================================================
  * Key store API
  * ====================================================================== */
 
 /** Seed the store with the root-of-trust key id read from OTP. */
 uiox_ks_err_t uiox_ks_keystore_init(uiox_ks_keystore_t *ks,
                                       const uint8_t rot_key_id[UIOX_KS_KEY_ID_LEN]);
 
 /** Add a key entry.  Refuses a bad magic and a full store. */
 uiox_ks_err_t uiox_ks_keystore_add(uiox_ks_keystore_t        *ks,
                                      const uiox_ks_key_entry_t *entry);
 
 /** Find a key by key_id (SHA-256 of its public key bytes). */
 const uiox_ks_key_entry_t *uiox_ks_keystore_find(
         const uiox_ks_keystore_t *ks,
         const uint8_t             key_id[UIOX_KS_KEY_ID_LEN]);
 
 /**
  * Check a key's time window and revocation status.
  *
  * ── the no-clock case, made explicit ───────────────────────────────────
  * now_sec of 0 does NOT mean "check and pass" — it means there is no
  * wall clock, which is the state at boot before timekeeping_init().  The
  * time window is then NOT evaluated, and that is a weaker check than one
  * that ran and passed.  The caller is told which happened via
  * expiry_checked, so a report can say "expiry not evaluated" rather than
  * leave a zero to imply a verdict.
  *
  * Refusing every key for want of a clock would make boot verification
  * impossible; skipping silently would make a report lie.  This is the
  * third option: skip, and say so.
  *
  * @param expiry_checked  Optional out-flag.  true when the window was
  *                        evaluated, false when now_sec was 0.
  */
 uiox_ks_err_t uiox_ks_key_check_valid(const uiox_ks_key_entry_t *key,
                                         const uiox_ks_krl_t        *krl,
                                         uint64_t                    now_sec,
                                         bool                       *expiry_checked);
 
 /**
  * Walk a certificate chain from a leaf key id up to the Root-of-Trust.
  *
  * The walk terminates only when it reaches a key whose is_root flag is
  * set AND whose key_id equals the OTP-anchored rot_key_id.  A self-signed
  * root that does not match the OTP anchor is a broken chain, not a
  * successful walk — which is what makes the anchor meaningful.
  *
  * Bounded by UIOX_KS_MAX_KEYS so a cycle cannot loop forever.
  */
 uiox_ks_err_t uiox_ks_key_walk_chain(const uiox_ks_keystore_t *ks,
                                        const uint8_t             leaf_key_id[UIOX_KS_KEY_ID_LEN],
                                        const uiox_ks_krl_t      *krl,
                                        uint64_t                  now_sec,
                                        bool                     *expiry_checked);
 
 /** Compute key_id = SHA-256(public key bytes), per algorithm. */
 void uiox_ks_compute_key_id(const uiox_ks_key_entry_t *key,
                               uint8_t                    key_id[UIOX_KS_KEY_ID_LEN]);
 
 /** Attach a KRL as the store's active revocation list. */
 uiox_ks_err_t uiox_ks_keystore_set_krl(uiox_ks_keystore_t *ks,
                                          uiox_ks_krl_t      *krl);
 
 void uiox_ks_keystore_print(const uiox_ks_keystore_t *ks);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KSIGN_KEY_H */
 