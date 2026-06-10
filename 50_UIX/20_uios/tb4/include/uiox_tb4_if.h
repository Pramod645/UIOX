/**
 * @file    uiox_tb4_if.h
 * @brief   UIOX Thunderbolt 4 NHI interface driver.
 * @date    2026-06-08
 */

 #ifndef UIOX_TB4_IF_H
 #define UIOX_TB4_IF_H
 
 #include "uiox_tb4_hw.h"
 #include "uiox_tb4_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  frames_tx;
     uint64_t  frames_rx;
     uint64_t  bytes_tx;
     uint64_t  bytes_rx;
     uint32_t  icm_msgs_sent;
     uint32_t  icm_msgs_recv;
     uint32_t  hotplug_events;
     uint32_t  errors;
 } uiox_tb4_if_stats_t;
 
 typedef struct {
     uiox_tb4_hw_t       *hw;
     uiox_tb4_if_stats_t  stats;
     bool                 primed;
 } uiox_tb4_if_t;
 
 int  uiox_tb4_if_config   (uiox_tb4_if_t *tif, uiox_tb4_hw_t *hw);
 int  uiox_tb4_if_start    (uiox_tb4_if_t *tif);
 void uiox_tb4_if_stop     (uiox_tb4_if_t *tif);
 int  uiox_tb4_if_tx       (uiox_tb4_if_t *tif,
                             uiox_tb4_frame_t *frame);
 uiox_tb4_frame_t *uiox_tb4_if_rx(uiox_tb4_if_t *tif);
 int  uiox_tb4_if_icm_cmd  (uiox_tb4_if_t *tif,
                             const uint32_t *req, uint8_t req_dwords,
                             uint32_t *resp, uint8_t resp_max);
 void uiox_tb4_if_irq_handle(uiox_tb4_if_t *tif);
 void uiox_tb4_if_stats_get (const uiox_tb4_if_t *tif,
                              uiox_tb4_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_TB4_IF_H */
 