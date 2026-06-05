/**
 * @file    uiox_bios_nvram.c
 * @brief   UIOX BIOS NVRAM implementation.
 * @date    2026-06-04
 */

 #include "uiox_bios_nvram.h"
 #include <string.h>
 #include <errno.h>
 
 /* Simple EFI variable store header (minimal subset) */
 #define NVRAM_MAGIC     0x55AA55AAu
 #define VAR_MAGIC_VALID 0xAAu
 #define VAR_MAGIC_DEL   0xFEu
 
 int uiox_bios_nvram_init(uiox_bios_nvram_t *nv,
                           uiox_bios_if_t    *bif,
                           uint32_t           store_offset)
 {
     if (!nv || !bif) return -EINVAL;
     memset(nv, 0, sizeof(*nv));
     nv->bif          = bif;
     nv->store_offset = store_offset;
     return 0;
 }
 
 int uiox_bios_nvram_load(uiox_bios_nvram_t *nv)
 {
     if (!nv) return -EINVAL;
     /* In a real implementation: parse EFI authenticated variable store
      * from flash. Here we stub with a default boot order variable.      */
     nv->var_count = 0;
     nv->dirty     = false;
 
     /* Pre-populate BootOrder variable */
     static const uiox_efi_guid_t global = UIOX_GUID_GLOBAL;
     uint16_t boot_order[] = { 0x0000u, 0x0001u, 0x0002u };
     return uiox_bios_nvram_set_var(nv, &global, "BootOrder",
                                     boot_order, sizeof(boot_order),
                                     UIOX_EFI_VAR_NV | UIOX_EFI_VAR_BS |
                                     UIOX_EFI_VAR_RT);
 }
 
 int uiox_bios_nvram_flush(uiox_bios_nvram_t *nv)
 {
     if (!nv || !nv->dirty) return 0;
     /* Serialise variable store to flash (stub: in production build the
      * full authenticated variable store format here)                    */
     nv->dirty = false;
     return 0;
 }
 
 static uiox_bios_var_t *find_var(uiox_bios_nvram_t    *nv,
                                    const uiox_efi_guid_t *guid,
                                    const char           *name)
 {
     for (uint8_t i = 0; i < nv->var_count; i++) {
         uiox_bios_var_t *v = &nv->vars[i];
         if (!v->valid || v->deleted) continue;
         if (memcmp(&v->guid, guid, sizeof(*guid)) == 0 &&
             strncmp(v->name, name, UIOX_BIOS_VAR_NAME_MAX) == 0)
             return v;
     }
     return NULL;
 }
 
 int uiox_bios_nvram_get_var(uiox_bios_nvram_t    *nv,
                              const uiox_efi_guid_t *guid,
                              const char           *name,
                              void                 *data_out,
                              uint32_t             *size_out,
                              uint32_t             *attrs_out)
 {
     if (!nv || !guid || !name || !data_out || !size_out) return -EINVAL;
     uiox_bios_var_t *v = find_var(nv, guid, name);
     if (!v) return -ENOENT;
     if (*size_out < v->data_size) { *size_out = v->data_size; return -ENOSPC; }
     memcpy(data_out, v->data, v->data_size);
     *size_out = v->data_size;
     if (attrs_out) *attrs_out = v->attributes;
     return 0;
 }
 
 int uiox_bios_nvram_set_var(uiox_bios_nvram_t    *nv,
                              const uiox_efi_guid_t *guid,
                              const char           *name,
                              const void           *data,
                              uint32_t              size,
                              uint32_t              attrs)
 {
     if (!nv || !guid || !name || !data || size > UIOX_BIOS_VAR_DATA_MAX)
         return -EINVAL;
 
     uiox_bios_var_t *v = find_var(nv, guid, name);
     if (!v) {
         if (nv->var_count >= UIOX_BIOS_MAX_VARS) return -ENOSPC;
         v = &nv->vars[nv->var_count++];
     }
     memcpy(&v->guid, guid, sizeof(*guid));
     strncpy(v->name, name, UIOX_BIOS_VAR_NAME_MAX - 1);
     memcpy(v->data, data, size);
     v->data_size  = size;
     v->attributes = attrs;
     v->valid      = true;
     v->deleted    = false;
     nv->dirty     = true;
     return 0;
 }
 
 int uiox_bios_nvram_del_var(uiox_bios_nvram_t    *nv,
                              const uiox_efi_guid_t *guid,
                              const char           *name)
 {
     if (!nv || !guid || !name) return -EINVAL;
     uiox_bios_var_t *v = find_var(nv, guid, name);
     if (!v) return -ENOENT;
     v->deleted = true;
     nv->dirty  = true;
     return 0;
 }
 
 uint8_t uiox_bios_nvram_cmos_get(uiox_bios_nvram_t *nv, uint8_t index)
 { return nv ? uiox_bios_hw_cmos_read(nv->bif->hw, index) : 0u; }
 
 void uiox_bios_nvram_cmos_set(uiox_bios_nvram_t *nv,
                                uint8_t index, uint8_t val)
 {
     if (!nv) return;
     uiox_bios_hw_cmos_write(nv->bif->hw, index, val);
     /* Update CMOS checksum (bytes 0x10..0x2D) */
     uint16_t cksum = 0;
     for (uint8_t i = 0x10u; i <= 0x2Du; i++)
         cksum += uiox_bios_hw_cmos_read(nv->bif->hw, i);
     uiox_bios_hw_cmos_write(nv->bif->hw,
                              UIOX_CMOS_CHECKSUM_HI, (uint8_t)(cksum >> 8u));
     uiox_bios_hw_cmos_write(nv->bif->hw,
                              UIOX_CMOS_CHECKSUM_LO, (uint8_t)(cksum & 0xFFu));
 }
 
 bool uiox_bios_nvram_cmos_valid(uiox_bios_nvram_t *nv)
 {
     if (!nv) return false;
     uint16_t cksum = 0;
     for (uint8_t i = 0x10u; i <= 0x2Du; i++)
         cksum += uiox_bios_hw_cmos_read(nv->bif->hw, i);
     uint16_t stored =
         (uint16_t)(uiox_bios_hw_cmos_read(nv->bif->hw, UIOX_CMOS_CHECKSUM_HI) << 8u) |
          uiox_bios_hw_cmos_read(nv->bif->hw, UIOX_CMOS_CHECKSUM_LO);
     return cksum == stored;
 }
 