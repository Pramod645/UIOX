/**
 * @file    uiox_soc_secboot.h
 * @brief   UIOX SoC — Secure Boot verification.
 *
 * Implements a minimal Chain of Trust:
 *   Root of Trust (RoT) key
 *     └── signs the SoC Verification Certificate (SVC)
 *           └── signs the Kernel Image Header
 *
 * @version 1.0.2
 * @date    2026-07-07
 */

 #ifndef UIOX_SOC_SECBOOT_H
 #define UIOX_SOC_SECBOOT_H
 
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── Secure boot result codes ───────────────────────────── */
 typedef enum {
     UIOX_SOC_SECBOOT_OK              =  0,
     UIOX_SOC_SECBOOT_ERR_BAD_MAGIC   = -1,
     UIOX_SOC_SECBOOT_ERR_HASH        = -2,
     UIOX_SOC_SECBOOT_ERR_SIG         = -3,
     UIOX_SOC_SECBOOT_ERR_CERT        = -4,
     UIOX_SOC_SECBOOT_ERR_ROLLBACK    = -5,
     UIOX_SOC_SECBOOT_ERR_KEY         = -6,
     UIOX_SOC_SECBOOT_ERR_REVOKED     = -7,
     UIOX_SOC_SECBOOT_ERR_INVAL       = -8,
 } uiox_soc_secboot_result_t;
 
 /* ── Signature algorithm IDs ────────────────────────────── */
 typedef enum {
     UIOX_SOC_SIG_NONE       = 0,
     UIOX_SOC_SIG_RSA2048    = 1,
     UIOX_SOC_SIG_ED25519    = 2,
     UIOX_SOC_SIG_ECDSA_P256 = 3,
 } uiox_soc_sig_algo_t;
 
 /* ── Constants ──────────────────────────────────────────── */
 #define UIOX_SOC_SECBOOT_MAGIC        0x55534543u  /**< "USEC"            */
 #define UIOX_SOC_SECBOOT_SHA256_LEN   32u
 #define UIOX_SOC_SECBOOT_SIG_MAX_LEN  512u
 #define UIOX_SOC_SECBOOT_KEY_MAX_LEN  512u
 #define UIOX_SOC_SECBOOT_CERT_NAMELEN 32u
 #define UIOX_SOC_SECBOOT_MAX_CHAIN    4u
 
 /* ── Root of Trust descriptor ───────────────────────────── */
 typedef struct {
     uiox_uint8_t           pubkey_hash[UIOX_SOC_SECBOOT_SHA256_LEN];
     uiox_soc_sig_algo_t algo;
     uiox_bool_t              sim_mode;
 } uiox_soc_rot_t;
 
 /* ── SoC Verification Certificate ──────────────────────── */
 typedef struct __attribute__((packed)) {
     uiox_uint32_t magic;
     uiox_uint32_t version;
     uiox_uint32_t flags;
     uiox_uint8_t   subject_pubkey[UIOX_SOC_SECBOOT_KEY_MAX_LEN];
     uiox_uint32_t subject_pubkey_len;
     uiox_uint8_t   issuer_sig[UIOX_SOC_SECBOOT_SIG_MAX_LEN];
     uiox_uint32_t issuer_sig_len;
     char     subject_name[UIOX_SOC_SECBOOT_CERT_NAMELEN];
     char     issuer_name [UIOX_SOC_SECBOOT_CERT_NAMELEN];
     uiox_uint8_t  _pad[32];
 } uiox_soc_svc_t;   /**< SoC Verification Certificate (was FVC) */
 
 /* Certificate flags */
 #define UIOX_SOC_SVC_FLAG_KERNEL_SIGN   (1u <<  0)
 #define UIOX_SOC_SVC_FLAG_SOC_SIGN      (1u <<  1)
 #define UIOX_SOC_SVC_FLAG_DEBUG_UNLOCK  (1u << 31)
 
 /* ── Signed image header ────────────────────────────────── */
 typedef struct __attribute__((packed)) {
     uiox_uint32_t magic;
     uiox_uint32_t format_version;
     uiox_uint32_t image_type;
     uiox_uint32_t load_addr_lo;
     uiox_uint32_t load_addr_hi;
     uiox_uint32_t entry_addr_lo;
     uiox_uint32_t entry_addr_hi;
     uiox_uint32_t image_size;
     uiox_uint32_t version;
     uiox_soc_sig_algo_t sig_algo;
     uiox_uint8_t  image_hash[UIOX_SOC_SECBOOT_SHA256_LEN];
     uiox_uint8_t  signature[UIOX_SOC_SECBOOT_SIG_MAX_LEN];
     uiox_uint32_t signature_len;
     uiox_uint8_t  _pad[60];
 } uiox_soc_signed_img_hdr_t;
 
 #define UIOX_SOC_IMG_TYPE_FIRMWARE  0u
 #define UIOX_SOC_IMG_TYPE_KERNEL    1u
 #define UIOX_SOC_IMG_TYPE_DTB       2u
 
 /* ── Secure boot context ────────────────────────────────── */
 typedef struct {
     uiox_soc_rot_t  rot;
     uiox_soc_svc_t *svc;           /**< SoC Verification Certificate    */
     uiox_uint32_t        min_version;
     uiox_bool_t            debug_mode;
 } uiox_soc_secboot_ctx_t;
 
 /* ── Secure boot report ─────────────────────────────────── */
 typedef struct {
     uiox_soc_secboot_result_t result;
     uiox_bool_t                      hash_ok;
     uiox_bool_t                      sig_ok;
     uiox_bool_t                      cert_ok;
     uiox_bool_t                      rollback_ok;
     uiox_uint32_t                  image_version;
     uiox_uint8_t                   measured_hash[UIOX_SOC_SECBOOT_SHA256_LEN];
     char                      fail_reason[128];
 } uiox_soc_secboot_report_t;
 
 /* ── Secure Boot API ────────────────────────────────────── */
 uiox_soc_err_t uiox_soc_secboot_init         (uiox_soc_secboot_ctx_t *ctx,
                                                const uiox_uint8_t rot_hash[32],
                                                uiox_soc_sig_algo_t algo,
                                                uiox_bool_t sim_mode);
 
 uiox_soc_secboot_result_t
                uiox_soc_secboot_verify_cert  (uiox_soc_secboot_ctx_t *ctx,
                                                const uiox_soc_svc_t *svc);
 
 uiox_soc_secboot_result_t
                uiox_soc_secboot_verify_image (uiox_soc_secboot_ctx_t *ctx,
                                                const uiox_soc_signed_img_hdr_t *hdr,
                                                const uiox_uint8_t *payload,
                                                uiox_size_t len,
                                                uiox_soc_secboot_report_t *report);
 
 void           uiox_soc_secboot_extend_pcr   (uiox_uint32_t pcr_index,
                                                const uiox_uint8_t measurement[32]);
 void           uiox_soc_secboot_read_pcr     (uiox_uint32_t pcr_index,
    uiox_uint8_t out[32]);
 void           uiox_soc_secboot_print        (const uiox_soc_secboot_report_t *r);
 
 /* ── SHA-256 ─────────────────────────────────────────────── */
 typedef struct {
     uiox_uint32_t state[8];
     uiox_uint64_t bit_count;
     uiox_uint32_t buf_len;
     uiox_uint8_t  buf[64];
 } uiox_soc_sha256_ctx_t;
 
 void uiox_soc_sha256_init  (uiox_soc_sha256_ctx_t *ctx);
 void uiox_soc_sha256_update(uiox_soc_sha256_ctx_t *ctx,
                               const uiox_uint8_t *d, uiox_size_t l);
 void uiox_soc_sha256_final (uiox_soc_sha256_ctx_t *ctx,
    uiox_uint8_t digest[32]);
 void uiox_soc_sha256       (const uiox_uint8_t *d, uiox_size_t l,
    uiox_uint8_t digest[32]);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_SECBOOT_H */
 