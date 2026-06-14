/**
 * @file  uiox_sata_subsys.h
 * @brief UIOX SATA Subsystem — hotplug, power management, events.
 * @date  2026-06-12
 */

 #ifndef UIOX_SATA_SUBSYS_H
 #define UIOX_SATA_SUBSYS_H
 
 #include "uiox_sata_proto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_SATA_EV_DEV_ATTACH   = 0,
     UIOX_SATA_EV_DEV_DETACH,
     UIOX_SATA_EV_DEV_READY,
     UIOX_SATA_EV_READ_DONE,
     UIOX_SATA_EV_WRITE_DONE,
     UIOX_SATA_EV_NCQ_DONE,
     UIOX_SATA_EV_SMART_WARN,
     UIOX_SATA_EV_ERROR,
 } uiox_sata_ev_t;
 
 typedef void (*uiox_sata_evt_cb_t)(uiox_sata_ev_t ev,
                                     const uiox_sata_evt_t *data,
                                     void *ctx);
 
 typedef enum {
     UIOX_SATA_STATE_OFF      = 0,
     UIOX_SATA_STATE_INIT,
     UIOX_SATA_STATE_NO_DEV,
     UIOX_SATA_STATE_READY,
     UIOX_SATA_STATE_ERROR,
 } uiox_sata_state_t;
 
 typedef struct {
     uiox_sata_if_t       sif;
     uiox_sata_proto_t    proto;
     uiox_sata_state_t    state;
     uiox_sata_evt_cb_t   evt_cb;
     void                *evt_ctx;
     uint32_t             tick_count;
     uint64_t             uptime_ms;
     uint32_t             attach_count;
     uint32_t             detach_count;
     uint32_t             error_count;
 } uiox_sata_subsys_t;
 
 int  uiox_sata_subsys_init    (uiox_sata_subsys_t *sys,
                                 uiox_sata_hw_t *hw);
 int  uiox_sata_subsys_start   (uiox_sata_subsys_t *sys);
 void uiox_sata_subsys_stop    (uiox_sata_subsys_t *sys);
 void uiox_sata_subsys_tick    (uiox_sata_subsys_t *sys, uint32_t now_ms);
 void uiox_sata_subsys_set_cb  (uiox_sata_subsys_t *sys,
                                 uiox_sata_evt_cb_t cb, void *ctx);
 int  uiox_sata_subsys_read    (uiox_sata_subsys_t *sys, uint64_t lba,
                                 uint8_t *buf, uint32_t sectors);
 int  uiox_sata_subsys_write   (uiox_sata_subsys_t *sys, uint64_t lba,
                                 const uint8_t *buf, uint32_t sectors);
 int  uiox_sata_subsys_flush   (uiox_sata_subsys_t *sys);
 int  uiox_sata_subsys_trim    (uiox_sata_subsys_t *sys,
                                 uint64_t lba, uint32_t sectors);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SATA_SUBSYS_H */
 