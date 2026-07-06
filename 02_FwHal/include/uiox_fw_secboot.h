/**
 * @file    uiox_fw_secboot.h
 * @brief   UIOX Firmware — Secure Boot verification.
 *
 * Provides SHA-256 image integrity checks and Ed25519 signature
 * verification for kernel images before the firmware hands off.
 *
 * Chain of trust:
 *   Root-of-Trust key (fused into OTP / TPM)
 *       └── Firmware signing key certificate
 *               └── Kernel image signature
 *
 * Security levels:
 *   SECBOOT_LEVEL_OFF      — no verification (development only)
 *   SECBOOT_LEVEL_HASH     — SHA-256 integrity only (no signature)
 *   SECBOOT_LEVEL_SIGN     — full Ed25519 signature verification
 *   SECBOOT_LEVEL_MEASURED — signature + TPM PCR measurement
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
 * Constants
 * ====================================================================== */
#define UIOX_SECBOOT_HASH_LEN      32u   /**< SHA-256 digest bytes       */
#define UIOX_SECBOOT_SIG_LEN       64u   /**< Ed25519 signature bytes    */
#define UIOX_SECBOOT_PUBKEY_LEN    32u   /**< Ed25519 public key bytes   */
#define UIOX_SECBOOT_MAX_KEYS       4u   /**< trusted keys in keyring    */
#define UIOX_SECBOOT_PCR_INDEX      8u   /**< TPM PCR for kernel image   */
#define UIOX_SECBOOT_IMG_MAGIC  0x554B524Eu  /**< "UKRN" image magic     */

/* =========================================================================
 * Security levels
 * ====================================================================== */
typedef enum {
    UIOX_SECBOOT_LEVEL_OFF      = 0,  /**< no verification (dev only)  */
    UIOX_SECBOOT_LEVEL_HASH     = 1,  /**< SHA-256 only                */
    UIOX_SECBOOT_LEVEL_SIGN     = 2,  /**< Ed25519 signature           */
    UIOX_SECBOOT_LEVEL_MEASURED = 3,  /**< signature + TPM PCR extend  */
} uiox_secboot_level_t;

/* =========================================================================
 * Kernel image header (embedded at offset 0 of the image)
 * ====================================================================== */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /**< UIOX_SECBOOT_IMG_MAGIC "UKRN"     */
    uint32_t header_version;    /**< must be 1                          */
    uint32_t arch;              /**< UIOX_ARCH_* from uiox_fw_types.h  */
    uint32_t flags;
    uint64_t load_addr;         /**< target physical load address       */
    uint64_t entry_point;       /**< kernel entry point                 */
    uint64_t image_size;        /**< total image bytes (incl. header)   */
    uint64_t text_size;         /**< .text section bytes                */
    uint8_t  sha256[UIOX_SECBOOT_HASH_LEN];  /**< digest of image body */
    uint8_t  signature[UIOX_SECBOOT_SIG_LEN];/**< Ed25519 over sha256  */
    uint8_t  signer_key[UIOX_SECBOOT_PUBKEY_LEN]; /**< signer pub key  */
    uint32_t reserved[4];
} uiox_fw_img_hdr_t;

/* =========================================================================
 * Trusted key record
 * ====================================================================== */
typedef struct {
    uint8_t  pubkey[UIOX_SECBOOT_PUBKEY_LEN];
    char     name[32];
    bool     valid;
} uiox_fw_trusted_key_t;

/* =========================================================================
 * Secure boot context
 * ====================================================================== */
typedef struct {
    uiox_secboot_level_t   level;
    uiox_fw_trusted_key_t  keys[UIOX_SECBOOT_MAX_KEYS];
    uint8_t                num_keys;
    bool                   verified;       /**< last verify() succeeded  */
    char                   fail_reason[64];
    uint8_t                measured_pcr[UIOX_SECBOOT_HASH_LEN];
} uiox_fw_secboot_ctx_t;

/* =========================================================================
 * SHA-256 context (self-contained, no libc)
 * ====================================================================== */
typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buf[64];
    uint32_t buflen;
} uiox_fw_sha256_ctx_t;

/* =========================================================================
 * API
 * ====================================================================== */

/** Initialise secure boot context with the given security level. */
uiox_fw_err_t uiox_fw_secboot_init    (uiox_fw_secboot_ctx_t *ctx,
                                         uiox_secboot_level_t   level);

/** Add a trusted public key to the in-memory keyring. */
uiox_fw_err_t uiox_fw_secboot_add_key (uiox_fw_secboot_ctx_t *ctx,
                                         const uint8_t pubkey[UIOX_SECBOOT_PUBKEY_LEN],
                                         const char   *name);

/**
 * Verify a loaded kernel image.
 *  - Checks magic and header version
 *  - Computes SHA-256 over image body and compares with header field
 *  - If level >= SIGN: verifies Ed25519 signature with keyring
 *  - If level >= MEASURED: extends TPM PCR[8] with the image hash
 *
 * @param ctx    secure boot context (must have been init'd)
 * @param img    pointer to image in RAM (starts with uiox_fw_img_hdr_t)
 * @param size   total bytes of image in RAM
 * @return UIOX_FW_OK on success
 */
uiox_fw_err_t uiox_fw_secboot_verify  (uiox_fw_secboot_ctx_t *ctx,
                                         const void *img, size_t size);

/** Print verification status and key list. */
void          uiox_fw_secboot_print   (const uiox_fw_secboot_ctx_t *ctx);

/* SHA-256 primitives (used internally and by POST crypto test) */
void uiox_fw_sha256_init   (uiox_fw_sha256_ctx_t *ctx);
void uiox_fw_sha256_update (uiox_fw_sha256_ctx_t *ctx,
                              const uint8_t *data, size_t len);
void uiox_fw_sha256_final  (uiox_fw_sha256_ctx_t *ctx, uint8_t digest[32]);
void uiox_fw_sha256        (const uint8_t *data, size_t len,
                              uint8_t digest[32]);
void uiox_fw_sha256_hex    (const uint8_t *data, size_t len,
                              char hex_out[65]);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_SECBOOT_H */
