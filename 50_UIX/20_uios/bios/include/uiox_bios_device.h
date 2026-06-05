/**
 * @file    uiox_bios_device.h
 * @brief   UIOX BIOS top-level application-facing device API.
 * @date    2026-06-04
 */
//Layer 5 — Device API
 #ifndef UIOX_BIOS_DEVICE_H
 #define UIOX_BIOS_DEVICE_H
 
 #include "uiox_bios_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_bios_hw_t            *hw;
     const uiox_bios_hw_ops_t  *hw_ops;
     uint32_t                   nvram_flash_offset;
     uiox_bios_evt_cb_t         evt_cb;
     void                      *evt_ctx;
 } uiox_bios_open_params_t;
 
 typedef struct {
     uiox_bios_subsys_t  subsys;
     uiox_bios_hw_t     *hw;
     bool                open;
 } uiox_bios_device_t;
 
 int  uiox_bios_open         (uiox_bios_device_t           *dev,
                               const uiox_bios_open_params_t *p);
 int  uiox_bios_start        (uiox_bios_device_t *dev);
 void uiox_bios_stop         (uiox_bios_device_t *dev);
 void uiox_bios_close        (uiox_bios_device_t *dev);
 void uiox_bios_tick         (uiox_bios_device_t *dev, uint32_t now_ms);
 
 /* Flash access */
 int  uiox_bios_flash_read   (uiox_bios_device_t *dev,
                               uint32_t offset, void *buf, uint32_t len);
 int  uiox_bios_flash_write  (uiox_bios_device_t *dev,
                               uint32_t offset,
                               const void *buf, uint32_t len);
 int  uiox_bios_flash_update (uiox_bios_device_t *dev,
                               uint32_t offset,
                               const void *image, uint32_t size);
 int  uiox_bios_flash_verify (uiox_bios_device_t *dev,
                               uint32_t offset,
                               const void *image, uint32_t size);
 
 /* Write-protect */
 int  uiox_bios_set_wp       (uiox_bios_device_t *dev, bool protect);
 bool uiox_bios_get_wp       (const uiox_bios_device_t *dev);
 
 /* NVRAM / EFI variables */
 int  uiox_bios_var_get      (uiox_bios_device_t    *dev,
                               const uiox_efi_guid_t *guid,
                               const char *name,
                               void *data, uint32_t *size,
                               uint32_t *attrs);
 int  uiox_bios_var_set      (uiox_bios_device_t    *dev,
                               const uiox_efi_guid_t *guid,
                               const char *name,
                               const void *data, uint32_t size,
                               uint32_t attrs);
 int  uiox_bios_var_del      (uiox_bios_device_t    *dev,
                               const uiox_efi_guid_t *guid,
                               const char *name);
 
 /* CMOS */
 uint8_t uiox_bios_cmos_get  (uiox_bios_device_t *dev, uint8_t index);
 void    uiox_bios_cmos_set  (uiox_bios_device_t *dev,
                               uint8_t index, uint8_t val);
 
 /* TPM */
 int  uiox_bios_tpm_send     (uiox_bios_device_t *dev,
                               const uint8_t *cmd, uint16_t cmd_len,
                               uint8_t *resp, uint16_t *resp_len);
 
 /* Info */
 void uiox_bios_print_info   (const uiox_bios_device_t *dev);
 void uiox_bios_print_stats  (const uiox_bios_device_t *dev);
 
 const char *uiox_bios_state_name(uiox_bios_subsys_state_t s);
 const char *uiox_bios_evt_name  (uiox_bios_evt_t evt);
 const char *uiox_bios_type_name (uiox_bios_type_t t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BIOS_DEVICE_H */
 