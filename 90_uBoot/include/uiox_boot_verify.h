#ifndef UIOX_BOOT_VERIFY_H
#define UIOX_BOOT_VERIFY_H
/*
 * uiox_boot_verify.h - Kernel image verification (SHA-256).
 */
#include "uiox_boot_types.h"

/* -- UIOX kernel image header (64 bytes) -------------------- */
typedef struct __attribute__((packed)) {
    uboot_u32_t magic;          /* UIOX_KIMG_MAGIC "UKRN"     */
    uboot_u32_t header_version; /* 1                           */
    uboot_u32_t arch;           /* UBOOT_ARCH_*                */
    uboot_u32_t flags;
    uboot_u64_t load_addr;      /* target physical load addr   */
    uboot_u64_t entry_point;    /* kernel entry point          */
    uboot_u64_t image_size;     /* total image bytes           */
    uboot_u64_t text_size;
    uboot_u8_t  sha256[32];     /* SHA-256 of image after hdr  */
} uiox_kimg_hdr_t;

/* -- SHA-256 context ---------------------------------------- */
typedef struct {
    uboot_u32_t state[8];
    uboot_u64_t count;
    uboot_u8_t  buf[64];
    uboot_u32_t buflen;
} uboot_sha256_ctx_t;

/* -- SHA-256 API -------------------------------------------- */
void uboot_sha256_init  (uboot_sha256_ctx_t *ctx);
void uboot_sha256_update(uboot_sha256_ctx_t *ctx,
                          const uboot_u8_t *data,
                          uboot_size_t len);
void uboot_sha256_final (uboot_sha256_ctx_t *ctx,
                          uboot_u8_t digest[32]);
void uboot_sha256        (const uboot_u8_t *data,
                          uboot_size_t len,
                          uboot_u8_t digest[32]);

/* -- Image verification ------------------------------------- */
int  uboot_verify_image (const void *img, uboot_size_t size);
void uboot_verify_print (const uiox_kimg_hdr_t *hdr, int ok);

#endif /* UIOX_BOOT_VERIFY_H */
