/**
 * @file  uiox_fw_secboot.h
 * @brief UIOX Firmware — Secure Boot verification.
 *
 * Implements a minimal Chain of Trust:
 *
 *   Root of Trust (RoT) key — burned into OTP / read from secure storage
 *     └── signs the Firmware Verification Certificate (FVC)
 *           └── signs the Kernel Image Header
 *
 * Verification steps:
 *   1. Validate the FVC signature against the RoT public key.
 *   2. Extract the firmware/kernel public key from the FVC.
 *   3. Compute SHA-256 of the kernel image payload.
 *   4. Verify the image signature (RSA-PKCS#1 v1.5 or Ed25519 stub).
 *   5. Check version anti-rollback counter (monotonic fuse counter).
 *
 * In simulation mode (UIOX_SECBOOT_SIM) all cryptographic checks are
 * replaced with deterministic stubs so the build works without an HSM.
 *
 * Integrates with: uiox_fw_post.h (secboot result fed into POST report)
 *                  uiox_boot_verify.h (SHA-256 from bootloader layer)
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_SECBOOT_H
 #define UIOX_FW_SECBOOT_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Secure boot result codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_SECBOOT_OK              =  0,
     UIOX_SECBOOT_ERR_BAD_MAGIC   = -1,  /**< Image header magic wrong   */
     UIOX_SECBOOT_ERR_HASH        = -2,  /**< SHA-256 mismatch           */
     UIOX_SECBOOT_ERR_SIG         = -3,  /**< Signature verification fail*/
     UIOX_SECBOOT_ERR_CERT        = -4,  /**< Certificate chain broken   */
     UIOX_SECBOOT_ERR_ROLLBACK    = -5,  /**< Anti-rollback check failed */
     UIOX_SECBOOT_ERR_KEY         = -6,  /**< Key not found / invalid    */
     UIOX_SECBOOT_ERR_REVOKED     = -7,  /**< Key revoked in OTP         */
     UIOX_SECBOOT_ERR_INVAL       = -8,
 } uiox_secboot_result_t;
 
 /* =========================================================================
  * Signature algorithm IDs
  * ====================================================================== */
 
 typedef enum {
     UIOX_SIG_NONE      = 0,  /**< No signature (development mode)       */
     UIOX_SIG_RSA2048   = 1,  /**< RSA-2048 PKCS#1 v1.5, SHA-256        */
     UIOX_SIG_ED25519   = 2,  /**< Ed25519 (Edwards-curve DSA)           */
     UIOX_SIG_ECDSA_P256= 3,  /**< ECDSA P-256, SHA-256                  */
 } uiox_sig_algo_t;
 
 /* =========================================================================
  * Constants
  * ====================================================================== */
 
 #define UIOX_SECBOOT_MAGIC          0x55534543u  /**< "USEC"             */
 #define UIOX_SECBOOT_SHA256_LEN     32u
 #define UIOX_SECBOOT_SIG_MAX_LEN    512u  /**< max RSA-4096 sig bytes    */
 #define UIOX_SECBOOT_KEY_MAX_LEN    512u
 #define UIOX_SECBOOT_CERT_NAME_LEN  32u
 #define UIOX_SECBOOT_MAX_CHAIN      4u    /**< Max chain depth           */
 
 /* =========================================================================
  * Root of Trust descriptor
  *
  * In real hardware: public key hash burned into OTP fuses.
  * In simulation:    hardcoded test vector.
  * ====================================================================== */
 
 typedef struct {
     uint8_t       pubkey_hash[UIOX_SECBOOT_SHA256_LEN]; /**< SHA-256(pubkey)*/
     uiox_sig_algo_t algo;
     bool          sim_mode;   /**< true = skip real crypto (dev/QEMU)   */
 } uiox_rot_t;
 
 /* =========================================================================
  * Firmware Verification Certificate (FVC)
  *
  * Stored in read-only flash immediately before the firmware image.
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint32_t  magic;               /**< UIOX_SECBOOT_MAGIC              */
     uint32_t  version;             /**< Anti-rollback version           */
     uint32_t  flags;               /**< Feature flags                   */
     uint8_t   subject_pubkey[UIOX_SECBOOT_KEY_MAX_LEN];
     uint32_t  subject_pubkey_len;
     uint8_t   issuer_sig[UIOX_SECBOOT_SIG_MAX_LEN];
     uint32_t  issuer_sig_len;
     char      subject_name[UIOX_SECBOOT_CERT_NAME_LEN];
     char      issuer_name [UIOX_SECBOOT_CERT_NAME_LEN];
     uint8_t   _pad[32];
 } uiox_fvc_t;
 
 /* Certificate flags */
 #define UIOX_FVC_FLAG_KERNEL_SIGN   (1u << 0)  /**< Signs kernel images */
 #define UIOX_FVC_FLAG_FW_SIGN       (1u << 1)  /**< Signs firmware      */
 #define UIOX_FVC_FLAG_DEBUG_UNLOCK  (1u << 31) /**< Dev/debug unlock    */
 
 /* =========================================================================
  * Signed image header
  *
  * Prepended to any binary verified by the secure boot chain.
  * Compatible with uiox_boot_types.h uiox_image_hdr_t fields.
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint32_t  magic;               /**< UIOX_SECBOOT_MAGIC              */
     uint32_t  format_version;
     uint32_t  image_type;          /**< 0=firmware, 1=kernel, 2=dtb     */
     uint32_t  load_addr_lo;
     uint32_t  load_addr_hi;
     uint32_t  entry_addr_lo;
     uint32_t  entry_addr_hi;
     uint32_t  image_size;
     uint32_t  version;             /**< Anti-rollback version           */
     uiox_sig_algo_t sig_algo;
     uint8_t   image_hash[UIOX_SECBOOT_SHA256_LEN];
     uint8_t   signature[UIOX_SECBOOT_SIG_MAX_LEN];
     uint32_t  signature_len;
     uint8_t   _pad[60];
 } uiox_signed_img_hdr_t;
 
 #define UIOX_IMG_TYPE_FIRMWARE  0u
 #define UIOX_IMG_TYPE_KERNEL    1u
 #define UIOX_IMG_TYPE_DTB       2u
 
 /* =========================================================================
  * Secure boot context
  * ====================================================================== */
 
 typedef struct {
     uiox_rot_t            rot;           /**< Root of Trust              */
     uiox_fvc_t           *fvc;           /**< Firmware Verification Cert */
     uint32_t              min_version;   /**< Anti-rollback floor        */
     bool                  debug_mode;    /**< Skip sig (dev boards)      */
 } uiox_secboot_ctx_t;
 
 /* =========================================================================
  * Secure boot verification report
  * ====================================================================== */
 
 typedef struct {
     uiox_secboot_result_t result;
     bool                  hash_ok;
     bool                  sig_ok;
     bool                  cert_ok;
     bool                  rollback_ok;
     uint32_t              image_version;
     uint8_t               measured_hash[UIOX_SECBOOT_SHA256_LEN];
     char                  fail_reason[128];
 } uiox_secboot_report_t;
 
 /* =========================================================================
  * Secure Boot API
  * ====================================================================== */
 
 /**
  * Initialise the secure boot context.
  * @param ctx        Output context (caller-allocated).
  * @param rot_hash   SHA-256 of Root of Trust public key (from OTP / const).
  * @param algo       Signature algorithm the RoT uses.
  * @param sim_mode   true = QEMU / dev mode (skip real crypto).
  */
 uiox_fw_err_t uiox_fw_secboot_init   (uiox_secboot_ctx_t *ctx,
                                         const uint8_t rot_hash[32],
                                         uiox_sig_algo_t algo,
                                         bool sim_mode);
 
 /**
  * Verify the Firmware Verification Certificate against the RoT.
  * Must be called before uiox_fw_secboot_verify_image().
  */
 uiox_secboot_result_t
               uiox_fw_secboot_verify_cert (uiox_secboot_ctx_t *ctx,
                                             const uiox_fvc_t *fvc);
 
 /**
  * Verify a signed image (kernel / DTB / firmware update).
  * @param ctx    Secure boot context with validated FVC.
  * @param hdr    Signed image header at start of image buffer.
  * @param payload Pointer to image bytes (immediately after header).
  * @param len    Length of payload in bytes.
  * @param report Output verification report (may be NULL).
  */
 uiox_secboot_result_t
               uiox_fw_secboot_verify_image(uiox_secboot_ctx_t *ctx,
                                             const uiox_signed_img_hdr_t *hdr,
                                             const uint8_t *payload,
                                             size_t len,
                                             uiox_secboot_report_t *report);
 
 /**
  * Extend the Platform Configuration Register (PCR) with @measurement.
  * Used to build a TPM-style measurement log for attestation.
  */
 void          uiox_fw_secboot_extend_pcr (uint32_t pcr_index,
                                            const uint8_t measurement[32]);
 
 /**
  * Read back a PCR value (for attestation or debug).
  */
 void          uiox_fw_secboot_read_pcr   (uint32_t pcr_index,
                                            uint8_t out[32]);
 
 /** Print the secure boot report to the debug UART. */
 void          uiox_fw_secboot_print      (const uiox_secboot_report_t *r);
 
 /* =========================================================================
  * SHA-256 (reused from bootloader — declared here for convenience)
  * ====================================================================== */
 
 typedef struct {
     uint32_t state[8];
     uint64_t bit_count;
     uint32_t buf_len;
     uint8_t  buf[64];
 } uiox_sha256_ctx_t;
 
 void uiox_sha256_init  (uiox_sha256_ctx_t *ctx);
 void uiox_sha256_update(uiox_sha256_ctx_t *ctx, const uint8_t *d, size_t l);
 void uiox_sha256_final (uiox_sha256_ctx_t *ctx, uint8_t digest[32]);
 void uiox_sha256       (const uint8_t *d, size_t l, uint8_t digest[32]);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_SECBOOT_H */
 