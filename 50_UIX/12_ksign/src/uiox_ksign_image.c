/**
 * @file  uiox_ksign_image.c
 * @brief UIOX Signed Kernel — image header parsing and hash.
 * @date  2026-07-07
 */

 #include "../include/uiox_ksign_image.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 static void im_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void im_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 
 uiox_ks_err_t uiox_ks_img_parse_hdr(const void *buf, size_t buf_len,
                                        uiox_ks_img_hdr_t *out_hdr)
 {
     if (!buf || !out_hdr) return UIOX_KS_ERR_INVAL;
     if (buf_len < UIOX_KS_IMG_HDR_SIZE) return UIOX_KS_ERR_INVAL;
     const uiox_ks_img_hdr_t *h = (const uiox_ks_img_hdr_t *)buf;
     if (h->magic != UIOX_KS_IMG_MAGIC) return UIOX_KS_ERR_BADMAGIC;
     if (h->format_version != UIOX_KS_FORMAT_VERSION)
         return UIOX_KS_ERR_BADVERSION;
     im_memcpy(out_hdr, h, sizeof(*out_hdr));
     return UIOX_KS_OK;
 }
 
 uiox_ks_err_t uiox_ks_img_hash_payload(const void *buf, size_t buf_len,
                                           const uiox_ks_img_hdr_t *hdr,
                                           uint8_t digest[UIOX_KS_SHA256_LEN])
 {
     if (!buf || !hdr || !digest) return UIOX_KS_ERR_INVAL;
     if (hdr->payload_offset + hdr->payload_size > (uint64_t)buf_len)
         return UIOX_KS_ERR_INVAL;
     const uint8_t *payload =
         (const uint8_t *)buf + hdr->payload_offset;
     uiox_ks_sha256(payload, (size_t)hdr->payload_size, digest);
     return UIOX_KS_OK;
 }
 
 uiox_ks_err_t uiox_ks_img_get_sig_section(
                         const void *buf, size_t buf_len,
                         const uiox_ks_img_hdr_t *hdr,
                         const uiox_ks_sig_section_hdr_t **sec,
                         size_t *sec_size)
 {
     if (!buf || !hdr || !sec) return UIOX_KS_ERR_INVAL;
     if (hdr->sig_section_offset == 0u || hdr->sig_section_size == 0u)
         return UIOX_KS_ERR_NOTFOUND;
     if (hdr->sig_section_offset + hdr->sig_section_size > (uint64_t)buf_len)
         return UIOX_KS_ERR_INVAL;
     *sec = (const uiox_ks_sig_section_hdr_t *)
            ((const uint8_t *)buf + hdr->sig_section_offset);
     if ((*sec)->magic != UIOX_KS_IMG_MAGIC) return UIOX_KS_ERR_BADMAGIC;
     if (sec_size) *sec_size = hdr->sig_section_size;
     return UIOX_KS_OK;
 }
 
 uiox_ks_err_t uiox_ks_img_check_version(const uiox_ks_img_hdr_t *hdr,
                                            uint32_t min_version)
 {
     if (!hdr) return UIOX_KS_ERR_INVAL;
     return (hdr->kernel_version >= min_version)
            ? UIOX_KS_OK : UIOX_KS_ERR_ROLLBACK;
 }
 
 void uiox_ks_img_print(const uiox_ks_img_hdr_t *hdr)
 {
     if (!hdr) return;
     uiox_fw_printf("[ksign] Image header:\n");
     uiox_fw_printf("  name        : %.48s\n", hdr->name);
     uiox_fw_printf("  arch        : %u\n",  hdr->arch);
     uiox_fw_printf("  kernel_ver  : %u\n",  hdr->kernel_version);
     uiox_fw_printf("  load_addr   : 0x%016llx\n",
                     (unsigned long long)hdr->load_addr);
     uiox_fw_printf("  entry_addr  : 0x%016llx\n",
                     (unsigned long long)hdr->entry_addr);
     uiox_fw_printf("  payload_sz  : %llu B\n",
                     (unsigned long long)hdr->payload_size);
     uiox_fw_printf("  flags       : 0x%08x\n", hdr->flags);
     uiox_fw_printf("  build_id    : %.32s\n", hdr->build_id);
     uiox_fw_printf("  sig_section : offset=%llu  size=%u\n",
                     (unsigned long long)hdr->sig_section_offset,
                     hdr->sig_section_size);
 }
 