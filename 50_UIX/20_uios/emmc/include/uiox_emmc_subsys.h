/**
 * @file  uiox_emmc_subsys.h
 * @brief UIOX eMMC Subsystem — health monitor, cache, partition management.
 * @date  2026-06-12
 */

 #ifndef UIOX_EMMC_SUBSYS_H
 #define UIOX_EMMC_SUBSYS_H
 
 #include "uiox_emmc_proto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_EMMC_EV_READY          = 0,
     UIOX_EMMC_EV_READ_DONE,
     UIOX_EMMC_EV_WRITE_DONE,
     UIOX_EMMC_EV_FLUSH_DONE,
     UIOX_EMMC_EV_HEALTH_WARN,
     UIOX_EMMC_EV_EOL_WARN,
     UIOX_EMMC_EV_BKOPS_NEEDED,
     UIOX_EMMC_EV_PART_SWITCH,
     UIOX_EMMC_EV_ERROR,
 } uiox_emmc_ev_t;
 
 typedef void (*uiox_emmc_evt_cb_t)(uiox_emmc_ev_t ev,
                                     const uiox_emmc_evt_t *data,
                                     void *ctx);
 
 typedef enum {
     UIOX_EMMC_STATE_OFF      = 0,
     UIOX_EMMC_STATE_INIT,
     UIOX_EMMC_STATE_READY,
     UIOX_EMMC_STATE_ERROR,
 } uiox_emmc_state_t;
 
 /* Health poll interval */
 #define UIOX_EMMC_HEALTH_INTERVAL_MS    30000u  /**< Every 30 s           */
 
 typedef struct {
     uiox_emmc_if_t       eif;
     uiox_emmc_proto_t    proto;
     uiox_emmc_state_t    state;
     uiox_emmc_evt_cb_t   evt_cb;
     void                *evt_ctx;
     uint32_t             tick_count;
     uint64_t             uptime_ms;
     uint32_t             error_count;
     uint32_t             flush_count;
     uint32_t             health_poll_ms;  /**< ms since last health check */
 } uiox_emmc_subsys_t;
 
 int  uiox_emmc_subsys_init    (uiox_emmc_subsys_t *sys,
                                 uiox_emmc_hw_t *hw);
 int  uiox_emmc_subsys_start   (uiox_emmc_subsys_t *sys);
 void uiox_emmc_subsys_stop    (uiox_emmc_subsys_t *sys);
 void uiox_emmc_subsys_tick    (uiox_emmc_subsys_t *sys, uint32_t now_ms);
 void uiox_emmc_subsys_set_cb  (uiox_emmc_subsys_t *sys,
                                 uiox_emmc_evt_cb_t cb, void *ctx);
 
 int  uiox_emmc_subsys_read    (uiox_emmc_subsys_t *sys,
                                 uiox_emmc_part_t part, uint32_t lba,
                                 uint8_t *buf, uint32_t sectors);
 int  uiox_emmc_subsys_write   (uiox_emmc_subsys_t *sys,
                                 uiox_emmc_part_t part, uint32_t lba,
                                 const uint8_t *buf, uint32_t sectors);
 int  uiox_emmc_subsys_flush   (uiox_emmc_subsys_t *sys);
 int  uiox_emmc_subsys_trim    (uiox_emmc_subsys_t *sys,
                                 uint32_t lba, uint32_t sectors);
 int  uiox_emmc_subsys_bkops   (uiox_emmc_subsys_t *sys);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_EMMC_SUBSYS_H */
 