/**
 * @file    uiox_fw_secboot.h
 * @brief   UIOX Firmware — Secure Boot verification.
 *
 * Implements the UIOX secure boot chain:
 *   1. Verify firmware image SHA-256 against stored golden hash
 *   2. Verify firmware Ed25519 signature against fused public key
 *   3. Verify kernel image signature before handoff
 *   4. Enforce rollback protection via monotonic counter (NV store)
 *   5. Record measurement into TPM PCR-style software log
 *
 * On ARM64 the fused keys live in "ROM" at a platform-defined address.
 * On x86-64 they live in a simulated flash region.
 *
 * @version 1.0.0
 * @date    2026-07-06
 */
#ifndef UIOX_FW_SECBOOT_H
#define UIOX_FW_SECBOOT_H

#include "uiox_fw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Secure boot configuration flags
 * ====================================================================== */
#define UIOX_SECBOOT_F_ENFORCE_SIG    (1u << 0) /**< Fail on bad sig     */
#define UIOX_SECBOOT_F_ENFORCE_HASH   (1u << 1) /**< Fail on bad hash    */
#define UIOX_SECBOOT_F_ROLLBACK_CHK   (1u << 2) /**< Check ver counter   */
#define UIOX_SECBOOT_F_MEASURE        (1u << 3) /**< Extend PCR log      */
#define UIOX_SECBOOT_F_LOCK_DEBUG     (1u << 4) /**< Disable JTAG/debug  */
#define UIOX_SECBOOT_F_ALL \
    (UIOX_SECBOOT_F_ENFORCE_SIG  | UIOX_SECBOOT_F_ENFORCE_HASH | \
     UIOX_SECBOOT_F_ROLLBACK_CHK | UIOX_SECBOOT_F_MEASURE      | \
     UIOX_SECBOOT_F_LOCK_DEBUG)

/* =========================================================================
 * Image descriptor — describes any signed binary (firmware or kernel)
 * ====================================================================== */
#define UIOX_SB_MAGIC        0x55494F58u  /* "UIOX"                      */
#define UIOX_SB_MAX_HASH_LEN 32u          /* SHA-256 = 32 bytes          */
#define UIOX_SB_SIG_LEN      64u          /* Ed25519 signature = 64 bytes*/
#define UIOX_SB_PUBKEY_LEN   32u          /* Ed25519 public key = 32 bytes*/

typedef struct __attribute__((packed)) {
    uint32_t magic;                       /**< UIOX_SB_MAGIC              */
    uint32_t version;                     /**< monotonic version counter  */
    uint32_t image_size;                  /**< size of image body bytes   */
    uint32_t flags;
    uint8_t  sha256[UIOX_SB_MAX_HASH_LEN];/**< SHA-256 of image body     */
    uint8_t  signature[UIOX_SB_SIG_LEN]; /**< Ed25519 over sha256 field  */
    char     name[32];                   /**< human name ("fw", "kernel")*/
    uint32_t reserved[4];
} uiox_sb_header_t;

/* =========================================================================
 * Measurement log (PCR-style software record)
 * ====================================================================== */
#define UIOX_SB_MAX_MEASUREMENTS  8u

typedef struct {
    char    component[32];               /**< "firmware", "kernel", …    */
    uint8_t hash[UIOX_SB_MAX_HASH_LEN]; /**< SHA-256 of measured data   */
    uint64_t timestamp_us;
} uiox_sb_measurement_t;

/* =========================================================================
 * Secure boot context
 * ====================================================================== */
typedef struct {
    uint32_t              flags;         /**< UIOX_SECBOOT_F_* bitmask   */
    uint8_t               root_pubkey[UIOX_SB_PUBKEY_LEN];
    uint32_t              min_fw_version;/**< rollback floor for firmware*/
    uint32_t              min_kn_version;/**< rollback floor for kernel  */
    uiox_sb_measurement_t log[UIOX_SB_MAX_MEASUREMENTS];
    uint8_t               log_count;
    bool                  boot_verified;
    bool                  kernel_verified;
    bool                  debug_locked;
} uiox_fw_secboot_ctx_t;

/* =========================================================================
 * Secure Boot API
 * ====================================================================== */

/** Initialise context with platform root key and policy flags.          */
uiox_fw_err_t uiox_fw_secboot_init (uiox_fw_secboot_ctx_t *ctx,
                                      const uint8_t root_pubkey[32],
                                      uint32_t      flags);

/** Verify firmware image at @p img_base of @p img_size bytes.
 *  Checks magic, SHA-256, Ed25519 sig, and version counter.
 *  @return UIOX_FW_OK on success, UIOX_FW_ERR_SECURITY on failure.    */
uiox_fw_err_t uiox_fw_secboot_verify_fw    (uiox_fw_secboot_ctx_t *ctx,
                                              const void *img_base,
                                              uint32_t    img_size);

/** Verify kernel image before handoff.                                  */
uiox_fw_err_t uiox_fw_secboot_verify_kernel(uiox_fw_secboot_ctx_t *ctx,
                                              const void *img_base,
                                              uint32_t    img_size);

/** Extend the measurement log with a new component hash.               */
uiox_fw_err_t uiox_fw_secboot_measure      (uiox_fw_secboot_ctx_t *ctx,
                                              const char   *component,
                                              const void   *data,
                                              uint32_t      size);

/** Lock debug ports (JTAG / SWD / serial console).                    */
void          uiox_fw_secboot_lock_debug   (uiox_fw_secboot_ctx_t *ctx);

/** Check monotonic rollback counter stored in OTP / NV.                */
uiox_fw_err_t uiox_fw_secboot_check_version(const uiox_fw_secboot_ctx_t *ctx,
                                              uint32_t image_version,
                                              uint32_t min_version);

/** Print current secure boot state via firmware UART.                  */
void          uiox_fw_secboot_print        (const uiox_fw_secboot_ctx_t *ctx);

/* =========================================================================
 * SHA-256 (self-contained, no libgcc dependency)
 * ====================================================================== */
typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buf[64];
    uint32_t buflen;
} uiox_sb_sha256_ctx_t;

void uiox_sb_sha256_init  (uiox_sb_sha256_ctx_t *ctx);
void uiox_sb_sha256_update(uiox_sb_sha256_ctx_t *ctx,
                             const uint8_t *data, uint32_t len);
void uiox_sb_sha256_final (uiox_sb_sha256_ctx_t *ctx, uint8_t digest[32]);
void uiox_sb_sha256       (const uint8_t *data, uint32_t len,
                             uint8_t digest[32]);

/* Ed25519 verify — stub uses SHA-256 HMAC fallback when crypto unavail */
uiox_fw_err_t uiox_sb_ed25519_verify(const uint8_t *msg,  uint32_t msg_len,
                                       const uint8_t  sig[64],
                                       const uint8_t  pubkey[32]);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_SECBOOT_H */
