/**
 * @file  uiox_ksign_crypto.h
 * @brief UIOX Signed Kernel — cryptographic primitives.
 *
 * Provides SHA-256/SHA-384, RSA-PKCS#1 v1.5 verification stub,
 * and ECDSA-P256 verification stub. All with zero libc dependency.
 *
 * In production replace stubs with:
 *   - mbedTLS (embedded)
 *   - wolfSSL
 *   - TF-A crypto library
 *
 * @version 1.0.0
 */

 #ifndef UIOX_KSIGN_CRYPTO_H
 #define UIOX_KSIGN_CRYPTO_H
 
 #include "uiox_ksign_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * SHA-256 (reused from uiox_fw_secboot — identical implementation)
  * ====================================================================== */
 
 typedef struct {
     uint32_t state[8];
     uint64_t bit_count;
     uint32_t buf_len;
     uint8_t  buf[64];
 } uiox_ks_sha256_ctx_t;
 
 void uiox_ks_sha256_init  (uiox_ks_sha256_ctx_t *ctx);
 void uiox_ks_sha256_update(uiox_ks_sha256_ctx_t *ctx,
                              const uint8_t *data, size_t len);
 void uiox_ks_sha256_final (uiox_ks_sha256_ctx_t *ctx,
                              uint8_t digest[UIOX_KS_SHA256_LEN]);
 void uiox_ks_sha256        (const uint8_t *data, size_t len,
                              uint8_t digest[UIOX_KS_SHA256_LEN]);
 
 /* =========================================================================
  * SHA-384
  * ====================================================================== */
 
 typedef struct {
     uint64_t state[8];
     uint64_t bit_count_lo;
     uint64_t bit_count_hi;
     uint32_t buf_len;
     uint8_t  buf[128];
 } uiox_ks_sha384_ctx_t;
 
 void uiox_ks_sha384_init  (uiox_ks_sha384_ctx_t *ctx);
 void uiox_ks_sha384_update(uiox_ks_sha384_ctx_t *ctx,
                              const uint8_t *data, size_t len);
 void uiox_ks_sha384_final (uiox_ks_sha384_ctx_t *ctx,
                              uint8_t digest[UIOX_KS_SHA384_LEN]);
 void uiox_ks_sha384        (const uint8_t *data, size_t len,
                              uint8_t digest[UIOX_KS_SHA384_LEN]);
 
 /* =========================================================================
  * HMAC-SHA256 (for key derivation and MAC operations)
  * ====================================================================== */
 
 void uiox_ks_hmac_sha256(const uint8_t *key, size_t key_len,
                            const uint8_t *data, size_t data_len,
                            uint8_t mac[UIOX_KS_SHA256_LEN]);
 
 /* =========================================================================
  * RSA signature verification (PKCS#1 v1.5)
  * ====================================================================== */
 
 #define UIOX_KS_RSA2048_MOD_LEN    256u   /**< 2048 bits = 256 bytes    */
 #define UIOX_KS_RSA4096_MOD_LEN    512u   /**< 4096 bits = 512 bytes    */
 #define UIOX_KS_RSA_SIG_MAX        512u
 
 typedef struct {
     uint8_t  modulus[UIOX_KS_RSA4096_MOD_LEN];
     uint32_t modulus_len;
     uint32_t exponent;   /**< Usually 65537 (0x10001)                  */
 } uiox_ks_rsa_pubkey_t;
 
 /**
  * Verify RSA-PKCS#1 v1.5 SHA-256 signature.
  * @param key       Public key.
  * @param sig       Signature bytes.
  * @param sig_len   Signature length.
  * @param digest    SHA-256 digest of the signed data.
  * @return UIOX_KS_OK if valid, UIOX_KS_ERR_SIG otherwise.
  *
  * Production: replace body with real modular exponentiation.
  * Simulation: compares against a known-good test vector.
  */
 uiox_ks_err_t uiox_ks_rsa_verify(const uiox_ks_rsa_pubkey_t *key,
                                     const uint8_t *sig, uint32_t sig_len,
                                     const uint8_t digest[UIOX_KS_SHA256_LEN]);
 
 /* =========================================================================
  * ECDSA-P256 signature verification
  * ====================================================================== */
 
 #define UIOX_KS_ECDSA_P256_KEY_LEN  64u  /**< Uncompressed x,y coords  */
 #define UIOX_KS_ECDSA_SIG_LEN       64u  /**< r,s components           */
 
 typedef struct {
     uint8_t xy[UIOX_KS_ECDSA_P256_KEY_LEN];  /**< Uncompressed P256 pt */
 } uiox_ks_ecdsa_pubkey_t;
 
 /**
  * Verify ECDSA-P256 signature over SHA-256 digest.
  * Production: replace with real ECC point operations.
  */
 uiox_ks_err_t uiox_ks_ecdsa_verify(const uiox_ks_ecdsa_pubkey_t *key,
                                       const uint8_t sig[UIOX_KS_ECDSA_SIG_LEN],
                                       const uint8_t digest[UIOX_KS_SHA256_LEN]);
 
 /* =========================================================================
  * Constant-time memory compare (prevents timing side-channel)
  * ====================================================================== */
 
 int uiox_ks_ct_memcmp(const uint8_t *a, const uint8_t *b, size_t len);
 
 /* =========================================================================
  * Secure memory wipe (prevent dead-code elimination of key material)
  * ====================================================================== */
 
 void uiox_ks_memzero(volatile void *ptr, size_t len);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KSIGN_CRYPTO_H */
 