/**
 * @file  uiox_nvme_subsys.h
 * @brief UIOX NVMe Subsystem — health, APST, NS management, events.
 * @date  2026-06-12
 */

 #ifndef UIOX_NVME_SUBSYS_H
 #define UIOX_NVME_SUBSYS_H
 
 #include "uiox_nvme_proto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_NVME_EV_READY         = 0,
     UIOX_NVME_EV_READ_DONE,
     UIOX_NVME_EV_WRITE_DONE,
     UIOX_NVME_EV_FLUSH_DONE,
     UIOX_NVME_EV_TRIM_DONE,
     UIOX_NVME_EV_HEALTH_WARN,
     UIOX_NVME_EV_TEMP_WARN,
     UIOX_NVME_EV_MEDIA_ERROR,
     UIOX_NVME_EV_FATAL,
     UIOX_NVME_EV_ERROR,
 } uiox_nvme_ev_t;
 
 typedef void (*uiox_nvme_evt_cb_t)(uiox_nvme_ev_t ev,
                                     const uiox_nvme_evt_t *data,
                                     void *ctx);
 
 typedef enum {
     UIOX_NVME_STATE_OFF     = 0,
     UIOX_NVME_STATE_INIT,
     UIOX_NVME_STATE_READY,
     UIOX_NVME_STATE_ERROR,
     UIOX_NVME_STATE_FATAL,
 } uiox_nvme_state_t;
 
 #define UIOX_NVME_HEALTH_INTERVAL_MS    60000u  /**< Health poll every 60s */
 
 typedef struct {
     uiox_nvme_if_t       nif;
     uiox_nvme_proto_t    proto;
     uiox_nvme_state_t    state;
     uiox_nvme_evt_cb_t   evt_cb;
     void                *evt_ctx;
     uint32_t             tick_count;
     uint64_t             uptime_ms;
     uint32_t             error_count;
     uint32_t             health_poll_ms;
 } uiox_nvme_subsys_t;
 
 int  uiox_nvme_subsys_init    (uiox_nvme_subsys_t *sys,
                                 uiox_nvme_hw_t *hw);
 int  uiox_nvme_subsys_start   (uiox_nvme_subsys_t *sys);
 void uiox_nvme_subsys_stop    (uiox_nvme_subsys_t *sys);
 void uiox_nvme_subsys_tick    (uiox_nvme_subsys_t *sys, uint32_t now_ms);
 void uiox_nvme_subsys_set_cb  (uiox_nvme_subsys_t *sys,
                                 uiox_nvme_evt_cb_t cb, void *ctx);
 
 int  uiox_nvme_subsys_read    (uiox_nvme_subsys_t *sys,
                                 uint32_t nsid, uint64_t slba,
                                 uint8_t *buf, uint32_t nlb);
 int  uiox_nvme_subsys_write   (uiox_nvme_subsys_t *sys,
                                 uint32_t nsid, uint64_t slba,
                                 const uint8_t *buf, uint32_t nlb);
 int  uiox_nvme_subsys_flush   (uiox_nvme_subsys_t *sys, uint32_t nsid);
 int  uiox_nvme_subsys_trim    (uiox_nvme_subsys_t *sys,
                                 uint32_t nsid, uint64_t slba,
                                 uint32_t nlb);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NVME_SUBSYS_H */
 