/**
 * @file    uiox_tpwd_sec.h
 * @brief   UIOX Touch-Password security engine.
 *
 * Provides:
 *   - PBKDF2-HMAC-SHA256 credential hashing (4096 iterations)
 *   - Cryptographically random salt generation (TRNG or LFSR fallback)
 *   - Timing-safe memory comparison (prevents timing side-channel)
 *   - Credential storage and retrieval
 *   - Brute-force lockout counters
 *   - Secure memory zeroing
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_TPWD_SEC_H
 #define UIOX_TPWD_SEC_H
 
 #include "uiox_tpwd_buf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Security configuration
  * ====================================================================== */
 
 #define UIOX_TPWD_SEC_PBKDF2_ITER    4096u
 #define UIOX_TPWD_SEC_MAX_ATTEMPTS   5u      /**< Before lockout          */
 #define UIOX_TPWD_SEC_LOCKOUT_S      300u    /**< 5-minute lockout        */
 #define UIOX_TPWD_SEC_TOKEN_LEN      16u     /**< Session token bytes     */
 #define UIOX_TPWD_SEC_MAX_STORED     8u      /**< Max stored credentials  */
 
 /* =========================================================================
  * Stored credential record
  * ====================================================================== */
 
 #define UIOX_TPWD_SEC_ID_LEN         32u
 
 typedef struct {
     char     id[UIOX_TPWD_SEC_ID_LEN]; /**< User/slot identifier          */
     uint8_t  hash[UIOX_TPWD_HASH_LEN];
     uint8_t  salt[UIOX_TPWD_SALT_LEN];
     uint8_t  attempts;
     uint32_t lockout_until_s;           /**< 0 = not locked               */
     bool     valid;
 } uiox_tpwd_sec_record_t;
 
 /* =========================================================================
  * Security context
  * ====================================================================== */
 
 typedef struct {
     uiox_tpwd_sec_record_t records[UIOX_TPWD_SEC_MAX_STORED];
     uint8_t                record_count;
 
     /* TRNG / LFSR state for salt generation */
     uint32_t               rng_state;
 
     /* Session token (set after successful auth) */
     uint8_t                session_token[UIOX_TPWD_SEC_TOKEN_LEN];
     bool                   session_valid;
     uint32_t               session_expires_s;
 
     /* Global lockout (all slots) */
     uint8_t                global_failures;
     uint32_t               global_lockout_until_s;
 } uiox_tpwd_sec_t;
 
 /* =========================================================================
  * Security API
  * ====================================================================== */
 
 int  uiox_tpwd_sec_init       (uiox_tpwd_sec_t *sec, uint32_t rng_seed);
 
/** Generate cryptographically random salt (TRNG or LFSR fallback). */
void uiox_tpwd_sec_gen_salt   (uiox_tpwd_sec_t *sec,
    uint8_t *salt, uint8_t len);

/**
* @brief  Hash a credential (PIN digits, pattern sequence, or raw bytes).
*         Uses PBKDF2-HMAC-SHA256 with the provided salt.
*
* @param  data      Raw credential bytes (PIN string, pattern bytes, etc.)
* @param  data_len  Length of data.
* @param  salt      Salt bytes.
* @param  hash_out  Output buffer (UIOX_TPWD_HASH_LEN bytes).
*/
void uiox_tpwd_sec_hash       (const uint8_t *data, uint16_t data_len,
    const uint8_t *salt,
    uint8_t *hash_out);

/**
* @brief  Timing-safe compare of two hash buffers.
* @return true if equal, false otherwise.
*         Always takes the same time regardless of where mismatch occurs.
*/
bool uiox_tpwd_sec_compare    (const uint8_t *a, const uint8_t *b,
    uint16_t len);

/**
* @brief  Store a new credential record (enrol).
* @return 0 on success, -ENOSPC if full, -EINVAL on bad args.
*/
int  uiox_tpwd_sec_enrol      (uiox_tpwd_sec_t *sec,
    const char *id,
    const uint8_t *data, uint16_t data_len);

/**
* @brief  Verify a credential against stored record.
*
* @return  0 = match (auth success)
*         -EACCES = wrong credential
*         -EPERM  = account locked out
*         -ENOENT = unknown ID
*/
int  uiox_tpwd_sec_verify     (uiox_tpwd_sec_t *sec,
    const char *id,
    const uint8_t *data, uint16_t data_len,
    uint32_t now_s);

/** Delete a stored credential record. */
int  uiox_tpwd_sec_delete     (uiox_tpwd_sec_t *sec, const char *id);

/** Query lockout state for a given ID. */
bool uiox_tpwd_sec_is_locked  (const uiox_tpwd_sec_t *sec,
    const char *id, uint32_t now_s);

/** Generate a random session token after successful auth. */
void uiox_tpwd_sec_gen_token  (uiox_tpwd_sec_t *sec,
    uint32_t valid_for_s, uint32_t now_s);

/** Check whether the session token is still valid. */
bool uiox_tpwd_sec_token_valid(const uiox_tpwd_sec_t *sec, uint32_t now_s);

/** Invalidate the session token (logout). */
void uiox_tpwd_sec_logout     (uiox_tpwd_sec_t *sec);

/** Securely zero a memory region (not optimised away by compiler). */
void uiox_tpwd_sec_zero       (volatile void *buf, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_TPWD_SEC_H */
