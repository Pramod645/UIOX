/**
 * @file  uiox_sd_if.h
 * @brief UIOX SD interface driver — CMD/DAT framing, CRC, bus width.
 * @date  2026-06-11
 */

 #ifndef UIOX_SD_IF_H
 #define UIOX_SD_IF_H
 
 #include "uiox_sd_hw.h"
 #include "uiox_sd_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  blocks_read;
     uint64_t  blocks_written;
     uint64_t  bytes_read;
     uint64_t  bytes_written;
     uint32_t  cmds_sent;
     uint32_t  crc_errors;
     uint32_t  timeout_errors;
     uint32_t  irq_count;
     uint32_t  errors;
 } uiox_sd_if_stats_t;
 
 typedef struct {
     uiox_sd_hw_t      *hw;
     uiox_sd_if_stats_t stats;
     bool               primed;
     uint8_t            bus_width;   /**< Current bus width (1 or 4)       */
 } uiox_sd_if_t;
 
 int  uiox_sd_if_config      (uiox_sd_if_t *sif, uiox_sd_hw_t *hw);
 int  uiox_sd_if_start       (uiox_sd_if_t *sif);
 void uiox_sd_if_stop        (uiox_sd_if_t *sif);
 
 /* Low-level command/response */
 int  uiox_sd_if_send_cmd    (uiox_sd_if_t *sif, uint8_t cmd,
                               uint32_t arg, uiox_sd_resp_t resp_type,
                               uint32_t *resp);
 
 /* Block transfers */
 int  uiox_sd_if_read        (uiox_sd_if_t *sif, uint32_t lba,
                               uint8_t *buf, uint32_t count);
 int  uiox_sd_if_write       (uiox_sd_if_t *sif, uint32_t lba,
                               const uint8_t *buf, uint32_t count);
 
 /* Bus configuration */
 int  uiox_sd_if_set_clock   (uiox_sd_if_t *sif, uint32_t hz);
 int  uiox_sd_if_set_bus_width(uiox_sd_if_t *sif, uint8_t width);
 
 /* IRQ handler */
 uiox_sd_evt_t *uiox_sd_if_irq_handle(uiox_sd_if_t *sif, uint32_t now_ms);
 
 void uiox_sd_if_stats_get   (const uiox_sd_if_t *sif,
                               uiox_sd_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SD_IF_H */
 