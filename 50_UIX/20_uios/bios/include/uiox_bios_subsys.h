/**
 * @file    uiox_bios_subsys.h
 * @brief   UIOX BIOS subsystem — boot, setup, firmware update, events.
 * @date    2026-06-04
 */
//Layer 4 — Subsystem
 #ifndef UIOX_BIOS_SUBSYS_H
 #define UIOX_BIOS_SUBSYS_H
 
 #include "uiox_bios_svc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_BIOS_EVT_POST_DONE = 0,
     UIOX_BIOS_EVT_POST_ERROR,
     UIOX_BIOS_EVT_FLASH_WRITE_START,
     UIOX_BIOS_EVT_FLASH_WRITE_DONE,
     UIOX_BIOS_EVT_FLASH_WRITE_ERROR,
     UIOX_BIOS_EVT_WP_REMOVED,
     UIOX_BIOS_EVT_WP_RESTORED,
     UIOX_BIOS_EVT_VAR_SET,
     UIOX_BIOS_EVT_VAR_DEL,
     UIOX_BIOS_EVT_BOOT_SELECT,
     UIOX_BIOS_EVT_SECURE_BOOT,
 } uiox_bios_evt_t;
 
 typedef void (*uiox_bios_evt_cb_t)(uiox_bios_evt_t evt, void *ctx);
 
 typedef enum {
     UIOX_BIOS_SUBSYS_STOPPED = 0,
     UIOX_BIOS_SUBSYS_POST,
     UIOX_BIOS_SUBSYS_RUNTIME,
     UIOX_BIOS_SUBSYS_UPDATE,
     UIOX_BIOS_SUBSYS_ERROR,
 } uiox_bios_subsys_state_t;
 
 typedef struct {
     uiox_bios_if_t           bif;
     uiox_bios_nvram_t        nvram;
     uiox_bios_svc_t          svc;
     uiox_bios_subsys_state_t state;
     uiox_bios_evt_cb_t       evt_cb;
     void                    *evt_ctx;
     uint32_t                 tick_count;
     uint64_t                 uptime_ms;
 } uiox_bios_subsys_t;
 
 int  uiox_bios_subsys_init    (uiox_bios_subsys_t *sys,
                                 uiox_bios_hw_t     *hw,
                                 uint32_t            nvram_flash_offset);
 int  uiox_bios_subsys_start   (uiox_bios_subsys_t *sys);
 void uiox_bios_subsys_stop    (uiox_bios_subsys_t *sys);
 void uiox_bios_subsys_tick    (uiox_bios_subsys_t *sys, uint32_t now_ms);
 
 /** Firmware update: flash new BIOS image. */
 int  uiox_bios_subsys_update  (uiox_bios_subsys_t *sys,
                                 uint32_t flash_offset,
                                 const void *image, uint32_t image_size);
 
 /** Verify flashed image. */
 int  uiox_bios_subsys_verify  (uiox_bios_subsys_t *sys,
                                 uint32_t flash_offset,
                                 const void *image, uint32_t image_size);
 
 void uiox_bios_subsys_set_cb  (uiox_bios_subsys_t *sys,
                                 uiox_bios_evt_cb_t cb, void *ctx);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BIOS_SUBSYS_H */
 