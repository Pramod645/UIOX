/**
 * @file  uiox_sd_subsys.h
 * @brief UIOX SD Subsystem — hotplug, write-protect, events.
 * @date  2026-06-11
 */

 #ifndef UIOX_SD_SUBSYS_H
 #define UIOX_SD_SUBSYS_H
 
 #include "uiox_sd_proto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Subsystem events
  * ====================================================================== */
 
 typedef enum {
     UIOX_SD_EV_CARD_INSERT    = 0,
     UIOX_SD_EV_CARD_REMOVE,
     UIOX_SD_EV_CARD_READY,
     UIOX_SD_EV_READ_DONE,
     UIOX_SD_EV_WRITE_DONE,
     UIOX_SD_EV_WP_ACTIVE,
     UIOX_SD_EV_WP_CLEAR,
     UIOX_SD_EV_ERROR,
 } uiox_sd_ev_t;
 
 typedef void (*uiox_sd_evt_cb_t)(uiox_sd_ev_t ev,
                                   const uiox_sd_evt_t *data,
                                   void *ctx);
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_SD_STATE_OFF      = 0,
     UIOX_SD_STATE_INIT,
     UIOX_SD_STATE_NO_CARD,
     UIOX_SD_STATE_READY,
     UIOX_SD_STATE_ERROR,
 } uiox_sd_state_t;
 
 typedef struct {
     uiox_sd_if_t       sif;
     uiox_sd_proto_t    proto;
     uiox_sd_state_t    state;
     uiox_sd_evt_cb_t   evt_cb;
     void              *evt_ctx;
     /* Statistics */
     uint32_t           tick_count;
     uint64_t           uptime_ms;
     uint32_t           insert_count;
     uint32_t           remove_count;
     uint32_t           error_count;
     /* WP tracking */
     bool               wp_last;
 } uiox_sd_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_sd_subsys_init     (uiox_sd_subsys_t *sys, uiox_sd_hw_t *hw);
 int  uiox_sd_subsys_start    (uiox_sd_subsys_t *sys);
 void uiox_sd_subsys_stop     (uiox_sd_subsys_t *sys);
 void uiox_sd_subsys_tick     (uiox_sd_subsys_t *sys, uint32_t now_ms);
 void uiox_sd_subsys_set_cb   (uiox_sd_subsys_t *sys,
                                uiox_sd_evt_cb_t cb, void *ctx);
 int  uiox_sd_subsys_read     (uiox_sd_subsys_t *sys, uint32_t lba,
                                uint8_t *buf, uint32_t count);
 int  uiox_sd_subsys_write    (uiox_sd_subsys_t *sys, uint32_t lba,
                                const uint8_t *buf, uint32_t count);
 int  uiox_sd_subsys_erase    (uiox_sd_subsys_t *sys,
                                uint32_t lba_start, uint32_t lba_end);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SD_SUBSYS_H */
 