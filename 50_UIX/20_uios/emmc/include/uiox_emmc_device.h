/**
 * @file  uiox_emmc_device.h
 * @brief UIOX eMMC application-facing API (Layer 5).
 * @date  2026-06-12
 */

 #ifndef UIOX_EMMC_DEVICE_H
 #define UIOX_EMMC_DEVICE_H
 
 #include "uiox_emmc_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_emmc_hw_t           *hw;
     const uiox_emmc_hw_ops_t *hw_ops;
     uiox_emmc_evt_cb_t        evt_cb;
     void                     *evt_ctx;
 } uiox_emmc_open_params_t;
 
 typedef struct {
     uiox_emmc_subsys_t  subsys;
     uiox_emmc_hw_t     *hw;
     bool                open;
 } uiox_emmc_device_t;
 
 /* Lifecycle */
 int  uiox_emmc_open       (uiox_emmc_device_t *dev,
                             const uiox_emmc_open_params_t *p);
 int  uiox_emmc_start      (uiox_emmc_device_t *dev);
 void uiox_emmc_stop       (uiox_emmc_device_t *dev);
 void uiox_emmc_close      (uiox_emmc_device_t *dev);
 void uiox_emmc_tick       (uiox_emmc_device_t *dev, uint32_t now_ms);
 
 /* Block I/O */
 int  uiox_emmc_read       (uiox_emmc_device_t *dev,
                             uiox_emmc_part_t part, uint32_t lba,
                             uint8_t *buf, uint32_t sectors);
 int  uiox_emmc_write      (uiox_emmc_device_t *dev,
                             uiox_emmc_part_t part, uint32_t lba,
                             const uint8_t *buf, uint32_t sectors);
 int  uiox_emmc_flush      (uiox_emmc_device_t *dev);
 int  uiox_emmc_trim       (uiox_emmc_device_t *dev,
                             uint32_t lba, uint32_t sectors);
 
 /* Maintenance */
 int  uiox_emmc_bkops      (uiox_emmc_device_t *dev);
 int  uiox_emmc_health     (uiox_emmc_device_t *dev,
                             uint8_t *pre_eol,
                             uint8_t *life_a, uint8_t *life_b);
 
 /* Device info */
 uint64_t uiox_emmc_capacity     (const uiox_emmc_device_t *dev);
 const uiox_emmc_ident_t *uiox_emmc_ident(const uiox_emmc_device_t *dev);
 
 /* Info / stats */
 void uiox_emmc_print_info  (const uiox_emmc_device_t *dev);
 void uiox_emmc_print_stats (uiox_emmc_device_t *dev);
 
 /* Name helpers */
 const char *uiox_emmc_state_name(uiox_emmc_state_t s);
 const char *uiox_emmc_ev_name   (uiox_emmc_ev_t ev);
 const char *uiox_emmc_speed_name(uiox_emmc_speed_t s);
 const char *uiox_emmc_part_name (uiox_emmc_part_t p);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_EMMC_DEVICE_H */
 