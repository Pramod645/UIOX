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
 * ── what changed in 1.1.1 ─────────────────────────────────────────────
 *   The 512-byte padding is now DERIVED, not hand-computed.
 *
 *   The previous version put a fixed list of subtractions in _pad[...]:
 *
 *       512 - 48 - 48 - 4 - 4 - 4 - 8 - 8 - 8 - 8 - 32 - 48 - 8 - 4
 *           - 32 - 4 - 8 - 32 - 4
 *
 *   That list came to 212 bytes of padding when the fields before it
 *   actually occupied 260, so sizeof(uiox_ks_img_hdr_t) was 472 — and the
 *   _Static_assert caught it.  472 is not a cosmetic error: the signing
 *   tool and the verifier both write and read this struct, so a 40-byte
 *   disagreement is a format that cannot work.
 *
 *   offsetof() cannot fix it from inside the struct — the typedef does not
 *   exist until the closing brace.  So the field layout is declared once as
 *   uiox_ks_img_hdr_fields_t, its size is measured, and _pad is set to
 *   512 minus that.  The two declarations are kept identical by the assert
 *   below: if they diverge, the assert fails rather than the format.
 *
 * @version 1.1.1
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
 
 /* ── The field layout, WITHOUT padding ───────────────────────────────
  * Declared separately so the padding can be computed from it.  offsetof()
  * cannot be used inside the struct it refers to, because the typedef does
  * not exist until the closing brace — the first attempt at this fix failed
  * for exactly that reason.
  *
  * This shadow has the same fields in the same order, so its packed size is
  * the real struct's size minus its padding.  Keep the two in step: the
  * _Static_assert below enforces the total, so a field added to one and not
  * the other fails the build rather than shifting the format.
  *
  * Every field is packed, so no compiler alignment padding is inserted and
  * the measured size is the plain sum of the members. */
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
 } uiox_ks_img_hdr_fields_t;
 
 /* Reserved / padding to exactly 512 bytes.
  *
  * Derived from the shadow's size rather than a hand-written list of
  * subtractions: the previous list came to 212 bytes of padding when the
  * fields before it occupied 260, which made the struct 472.  This form
  * cannot drift — add a field to the shadow and the remainder adjusts.
  *
  * If this expression ever goes negative the build fails at the array
  * declaration, which is the correct failure: 512 bytes is a format
  * constraint, not a target. */
 #define UIOX_KS_IMG_PAD_LEN \
     (UIOX_KS_IMG_HDR_SIZE - (uint32_t)sizeof(uiox_ks_img_hdr_fields_t))
 
 /* ── The struct as it is written and read ────────────────────────────
  * Field-for-field the shadow above, plus the padding. */
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
 
     /* Reserved / padding to exactly 512 bytes */
     uint8_t   _pad[UIOX_KS_IMG_PAD_LEN];
 } uiox_ks_img_hdr_t;
 
 /* The header MUST be exactly 512 bytes: the payload begins at
  * payload_offset and the signing tool writes this same struct, so the
  * signer and the verifier must agree on the layout byte for byte.
  *
  * This is the check that caught the 472-byte version.  It stays because
  * the padding above is derived rather than constant, so adding a field to
  * the shadow changes _pad and this assert verifies the total still lands
  * on the format's fixed size. */
 #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
 _Static_assert(sizeof(uiox_ks_img_hdr_t) == UIOX_KS_IMG_HDR_SIZE,
                "uiox_ks_img_hdr_t must be exactly 512 bytes");
 _Static_assert(sizeof(uiox_ks_img_hdr_fields_t) <= UIOX_KS_IMG_HDR_SIZE,
                "uiox_ks_img_hdr_fields_t exceeds the 512-byte header");
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
 