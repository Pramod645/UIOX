/**
 * @file  uiox_kix_ksign_image.h
 * @brief UIOX Signed Kernel — signed image format.
 *
 * Signed image layout (on-disk / in flash):
 *
 *   ┌─────────────────────────────────────┐
 *   │  uiox_ks_img_hdr_t   (512 bytes)    │ ← fixed-size header
 *   │  Kernel ELF binary   (variable)     │ ← signed payload
 *   │  .uiox_sig section   (appended)     │ ← signature + cert chain
 *   └─────────────────────────────────────┘
 *
 * The header overlaps with uiox_fw_secboot.h uiox_signed_img_hdr_t for
 * compatibility with the existing Stage 0d verification path.
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 *   The signature section has its own magic (UIOX_KS_SIG_MAGIC) as of
 *   format version 2.  Version 1 wrote UIOX_KS_IMG_MAGIC there, which is
 *   the same constant the image header carries — so the section check
 *   compared two structures that could not be told apart.
 *
 *   A _Static_assert pins the header to 512 bytes.  _pad is computed by
 *   hand and subtracts a literal 4 for sig_alg, an enum whose size is
 *   implementation-defined; a toolchain that sized it differently would
 *   shift every field after it and the signer and verifier would
 *   silently disagree about the format.
 *
 * @version 1.1.0
 * @date    2026-10-01
 */

 #ifndef UIOX_KIX_KSIGN_IMAGE_H
 #define UIOX_KIX_KSIGN_IMAGE_H
 
 #include "uiox_kix_ksign_key.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Signed image header (512-byte aligned)
  * ====================================================================== */
 
 #define UIOX_KS_IMG_HDR_SIZE    512u
 #define UIOX_KS_SIG_SECTION_MAX (4u * 1024u)  /**< Max 4 KB sig section */
 #define UIOX_KS_IMG_NAME_LEN    48u
 
 typedef struct __attribute__((packed)) {
     /* Identification */
     uint32_t  magic;                    /**< UIOX_KS_IMG_MAGIC           */
     uint32_t  format_version;           /**< UIOX_KS_FORMAT_VERSION      */
     char      name[UIOX_KS_IMG_NAME_LEN]; /**< "uiox-kernel-arm64"      */
 
     /* Target architecture */
     uint32_t  arch;                     /**< 0=ARM64, 1=ARM32, 2=x86_64 */
     uint32_t  min_kernel_version;       /**< Anti-rollback floor         */
     uint32_t  kernel_version;           /**< This kernel's version       */
 
     /* Payload addresses */
     uint64_t  load_addr;                /**< Physical load address        */
     uint64_t  entry_addr;               /**< Kernel entry point           */
 
     /* Hash coverage */
     uint64_t  payload_offset;           /**< Byte offset of kernel binary */
     uint64_t  payload_size;             /**< Size of kernel binary        */
     uint8_t   payload_hash[UIOX_KS_SHA256_LEN];  /**< SHA-256(payload)  */
     uint8_t   payload_hash384[UIOX_KS_SHA384_LEN];/**< SHA-384(payload) */
 
     /* Signature section location (appended after payload) */
     uint64_t  sig_section_offset;       /**< Offset of .uiox_sig data    */
     uint32_t  sig_section_size;
 
     /* Signing key reference */
     uint8_t   signing_key_id[UIOX_KS_KEY_ID_LEN]; /**< SHA-256(pubkey) */
     uiox_ks_alg_t sig_alg;
 
     /* Build info */
     uint64_t  build_time;               /**< Unix timestamp of signing   */
     char      build_id[32];             /**< Git commit or build UUID    */
 
     /* Flags */
     uint32_t  flags;
     /* Reserved / padding to 512 bytes */
     uint8_t   _pad[512u - 48u - 48u - 4u - 4u - 4u - 8u - 8u -
                    8u - 8u - 32u - 48u - 8u - 4u - 32u - 4u - 8u -
                    32u - 4u];
 } uiox_ks_img_hdr_t;
 
 /* The header MUST be exactly 512 bytes: the payload begins at
  * payload_offset and the signing tool writes this same struct, so the
  * signer and the verifier must agree on the layout byte for byte.
  *
  * _pad is computed by hand and subtracts a literal 4 for sig_alg — an
  * enum whose size is implementation-defined.  A toolchain that sized it
  * differently would shift every field after it and the two sides would
  * silently disagree about the format.  This turns that into a compile
  * error instead of a misparse at boot. */
 #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
 _Static_assert(sizeof(uiox_ks_img_hdr_t) == UIOX_KS_IMG_HDR_SIZE,
                "uiox_ks_img_hdr_t must be exactly 512 bytes");
 #endif
 
 /* Image flags */
 #define UIOX_KS_FLAG_DEBUG_ALLOWED  (1u << 0)  /**< Debug mode OK       */
 #define UIOX_KS_FLAG_TEST_SIGNED    (1u << 1)  /**< Test/dev key used   */
 #define UIOX_KS_FLAG_PRODUCTION     (1u << 2)  /**< Production key      */
 #define UIOX_KS_FLAG_HAS_INITRD     (1u << 3)  /**< initrd appended     */
 
 /* =========================================================================
  * Signature section (.uiox_sig) — appended after kernel binary
  *
  * magic is UIOX_KS_SIG_MAGIC in format version 2, and UIOX_KS_IMG_MAGIC
  * in version 1.  uiox_ks_img_get_sig_section accepts either according to
  * the header's format_version.
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint32_t  magic;                    /**< UIOX_KS_SIG_MAGIC (v2)      */
     uint32_t  sig_count;                /**< Number of signatures        */
     /* Followed by sig_count × uiox_ks_sig_entry_t */
 } uiox_ks_sig_section_hdr_t;
 
 typedef struct __attribute__((packed)) {
     uint8_t   signer_key_id[UIOX_KS_KEY_ID_LEN];
     uiox_ks_alg_t alg;
     uint32_t  sig_len;
     uint8_t   sig[UIOX_KS_RSA_SIG_MAX];   /**< Actual sig bytes        */
     /* Inline cert chain: signer cert + intermediates up to root */
     uint32_t  cert_chain_len;             /**< Total bytes of cert chain */
     /* cert chain bytes follow immediately (variable length) */
 } uiox_ks_sig_entry_t;
 
 /* =========================================================================
  * Image API
  * ====================================================================== */
 
 /** Parse and validate the image header at @buf.  Accepts format
  *  versions 1 and 2. */
 uiox_ks_err_t uiox_ks_img_parse_hdr (const void *buf, size_t buf_len,
                                         uiox_ks_img_hdr_t *out_hdr);
 
 /** Compute the hash of the kernel payload. */
 uiox_ks_err_t uiox_ks_img_hash_payload(const void *buf, size_t buf_len,
                                           const uiox_ks_img_hdr_t *hdr,
                                           uint8_t digest[UIOX_KS_SHA256_LEN]);
 
 /** Locate the .uiox_sig section in the image buffer. */
 uiox_ks_err_t uiox_ks_img_get_sig_section(const void *buf, size_t buf_len,
                                               const uiox_ks_img_hdr_t *hdr,
                                               const uiox_ks_sig_section_hdr_t **sec,
                                               size_t *sec_size);
 
 /** Anti-rollback: check kernel_version >= min_version stored in OTP/NVRAM. */
 uiox_ks_err_t uiox_ks_img_check_version(const uiox_ks_img_hdr_t *hdr,
                                            uint32_t min_version);
 
 /** Print image header to kernel console. */
 void          uiox_ks_img_print       (const uiox_ks_img_hdr_t *hdr);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KSIGN_IMAGE_H */
 