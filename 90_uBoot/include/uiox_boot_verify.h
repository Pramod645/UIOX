/**
 * @file  uiox_boot_verify.h
 * @brief UIOX Bootloader — SHA-256 (RFC 6234) and image header check.
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_BOOT_VERIFY_H
 #define UIOX_BOOT_VERIFY_H
 
 #include "uiox_boot_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * SHA-256 context (RFC 6234 compliant)
  * ====================================================================== */
 
 typedef struct {
     uint32_t state[8];
     uint64_t bit_count;
     uint32_t buf_len;
     uint8_t  buf[64];
 } uiox_sha256_ctx_t;
 
 void uiox_sha256_init  (uiox_sha256_ctx_t *ctx);
 void uiox_sha256_update(uiox_sha256_ctx_t *ctx,
                         const uint8_t *data, size_t len);
 void uiox_sha256_final (uiox_sha256_ctx_t *ctx, uint8_t digest[32]);
 
 /** One-shot SHA-256. */
 void uiox_sha256(const uint8_t *data, size_t len, uint8_t digest[32]);
 
 /* =========================================================================
  * Image verification
  * ====================================================================== */
 
 /**
  * Verify the UIOX image header magic, version, arch, and SHA-256 of
  * the payload bytes immediately following the header.
  *
  * @param hdr       Pointer to the uiox_image_hdr_t at the start of the
  *                  loaded image buffer.
  * @param payload   Pointer to the first byte after the header.
  * @param pay_len   Byte length of the payload (image_size from header).
  * @param expected_arch   uiox_arch_t to compare against header arch field.
  * @return UIOX_BOOT_OK on success, error code otherwise.
  */
 uiox_boot_err_t uiox_boot_verify_image(const uiox_image_hdr_t *hdr,
                                         const uint8_t *payload,
                                         size_t pay_len,
                                         uiox_arch_t expected_arch);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BOOT_VERIFY_H */
 