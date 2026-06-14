/**
 * @file  uiox_sata_device.h
 * @brief UIOX SATA Controller application-facing API (Layer 5).
 * @date  2026-06-12
 */

 #ifndef UIOX_SATA_DEVICE_H
 #define UIOX_SATA_DEVICE_H
 
 #include "uiox_sata_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_sata_hw_t           *hw;
     const uiox_sata_hw_ops_t *hw_ops;
     uiox_sata_evt_cb_t        evt_cb;
     void                     *evt_ctx;
 } uiox_sata_open_params_t;
 
 typedef struct {
     uiox_sata_subsys_t  subsys;
     uiox_sata_hw_t     *hw;
     bool                open;
 } uiox_sata_device_t;
 
 /* Lifecycle */
 int  uiox_sata_open       (uiox_sata_device_t *dev,
                             const uiox_sata_open_params_t *p);
 int  uiox_sata_start      (uiox_sata_device_t *dev);
 void uiox_sata_stop       (uiox_sata_device_t *dev);
 void uiox_sata_close      (uiox_sata_device_t *dev);
 void uiox_sata_tick       (uiox_sata_device_t *dev, uint32_t now_ms);
 
 /* Block I/O */
 int  uiox_sata_read       (uiox_sata_device_t *dev, uint64_t lba,
                             uint8_t *buf, uint32_t sectors);
 int  uiox_sata_write      (uiox_sata_device_t *dev, uint64_t lba,
                             const uint8_t *buf, uint32_t sectors);
 int  uiox_sata_flush      (uiox_sata_device_t *dev);
 int  uiox_sata_trim       (uiox_sata_device_t *dev,
                             uint64_t lba, uint32_t sectors);
 
 /* SMART */
 int  uiox_sata_smart_read (uiox_sata_device_t *dev, uint8_t *buf);
 
 /* Device info */
 bool     uiox_sata_is_present  (const uiox_sata_device_t *dev);
 bool     uiox_sata_is_ssd      (const uiox_sata_device_t *dev);
 uint64_t uiox_sata_capacity    (const uiox_sata_device_t *dev);
 const uiox_sata_ident_t *uiox_sata_ident(const uiox_sata_device_t *dev);
 
 /* Info / stats */
 void uiox_sata_print_info  (const uiox_sata_device_t *dev);
 void uiox_sata_print_stats (uiox_sata_device_t *dev);
 
 /* Name helpers */
 const char *uiox_sata_state_name  (uiox_sata_state_t s);
 const char *uiox_sata_ev_name     (uiox_sata_ev_t ev);
 const char *uiox_sata_dev_name    (uiox_sata_dev_t d);
 const char *uiox_sata_ctrl_name   (uiox_sata_ctrl_t c);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SATA_DEVICE_H */
 