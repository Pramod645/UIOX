/**
 * @file  uiox_sd_device.h
 * @brief UIOX SD Card Reader application-facing API (Layer 5).
 * @date  2026-06-11
 */

 #ifndef UIOX_SD_DEVICE_H
 #define UIOX_SD_DEVICE_H
 
 #include "uiox_sd_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_sd_hw_t            *hw;
     const uiox_sd_hw_ops_t  *hw_ops;
     uiox_sd_evt_cb_t         evt_cb;
     void                    *evt_ctx;
 } uiox_sd_open_params_t;
 
 typedef struct {
     uiox_sd_subsys_t  subsys;
     uiox_sd_hw_t     *hw;
     bool              open;
 } uiox_sd_device_t;
 
 /* Lifecycle */
 int  uiox_sd_open        (uiox_sd_device_t *dev,
                            const uiox_sd_open_params_t *p);
 int  uiox_sd_start       (uiox_sd_device_t *dev);
 void uiox_sd_stop        (uiox_sd_device_t *dev);
 void uiox_sd_close       (uiox_sd_device_t *dev);
 void uiox_sd_tick        (uiox_sd_device_t *dev, uint32_t now_ms);
 
 /* Block I/O */
 int  uiox_sd_read        (uiox_sd_device_t *dev, uint32_t lba,
                            uint8_t *buf, uint32_t count);
 int  uiox_sd_write       (uiox_sd_device_t *dev, uint32_t lba,
                            const uint8_t *buf, uint32_t count);
 int  uiox_sd_erase       (uiox_sd_device_t *dev,
                            uint32_t lba_start, uint32_t lba_end);
 
 /* Card info */
 bool     uiox_sd_is_present     (const uiox_sd_device_t *dev);
 bool     uiox_sd_is_write_prot  (const uiox_sd_device_t *dev);
 uint64_t uiox_sd_capacity_bytes (const uiox_sd_device_t *dev);
 uint64_t uiox_sd_capacity_blocks(const uiox_sd_device_t *dev);
 const uiox_sd_card_t *uiox_sd_card_info(const uiox_sd_device_t *dev);
 
 /* Info / stats */
 void uiox_sd_print_info  (const uiox_sd_device_t *dev);
 void uiox_sd_print_stats (uiox_sd_device_t *dev);
 
 /* Name helpers */
 const char *uiox_sd_state_name    (uiox_sd_state_t s);
 const char *uiox_sd_ev_name       (uiox_sd_ev_t ev);
 const char *uiox_sd_card_type_name(uiox_sd_card_type_t t);
 const char *uiox_sd_bus_name      (uiox_sd_bus_t b);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SD_DEVICE_H */
 