/**
 * @file  uiox_emmc_if.h
 * @brief UIOX eMMC interface driver — CMD/DAT, CRC, bus width, IRQ.
 * @date  2026-06-12
 */

 #ifndef UIOX_EMMC_IF_H
 #define UIOX_EMMC_IF_H
 
 #include "uiox_emmc_hw.h"
 #include "uiox_emmc_buf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t blocks_read;
     uint64_t blocks_written;
     uint64_t bytes_read;
     uint64_t bytes_written;
     uint32_t cmds_sent;
     uint32_t crc_errors;
     uint32_t timeout_errors;
     uint32_t irq_count;
     uint32_t flush_count;
     uint32_t errors;
 } uiox_emmc_if_stats_t;
 
 typedef struct {
     uiox_emmc_hw_t      *hw;
     uiox_emmc_if_stats_t stats;
     bool                 primed;
     uint8_t              bus_width;
     uiox_emmc_speed_t    speed;
 } uiox_emmc_if_t;
 
 int  uiox_emmc_if_config      (uiox_emmc_if_t *eif, uiox_emmc_hw_t *hw);
 int  uiox_emmc_if_start       (uiox_emmc_if_t *eif);
 void uiox_emmc_if_stop        (uiox_emmc_if_t *eif);
 
 /* Command/response */
 int  uiox_emmc_if_send_cmd    (uiox_emmc_if_t *eif, uint8_t cmd,
                                 uint32_t arg, uiox_emmc_resp_t resp_type,
                                 uint32_t *resp);
 
 /* EXT_CSD SWITCH */
 int  uiox_emmc_if_switch      (uiox_emmc_if_t *eif,
                                 uint8_t access, uint8_t index,
                                 uint8_t val, uint8_t cmd_set);
 
 /* Block transfers */
 int  uiox_emmc_if_read        (uiox_emmc_if_t *eif, uint32_t lba,
                                 uint8_t *buf, uint32_t count);
 int  uiox_emmc_if_write       (uiox_emmc_if_t *eif, uint32_t lba,
                                 const uint8_t *buf, uint32_t count);
 
 /* EXT_CSD */
 int  uiox_emmc_if_read_ext_csd(uiox_emmc_if_t *eif, uint8_t *buf);
 
 /* Bus reconfiguration */
 int  uiox_emmc_if_set_clock   (uiox_emmc_if_t *eif, uint32_t hz);
 int  uiox_emmc_if_set_bus_width(uiox_emmc_if_t *eif, uint8_t width);
 int  uiox_emmc_if_set_speed   (uiox_emmc_if_t *eif,
                                 uiox_emmc_speed_t speed);
 
 /* Partition select */
 int  uiox_emmc_if_select_part (uiox_emmc_if_t *eif,
                                 uiox_emmc_part_t part);
 
 /* IRQ handler */
 uiox_emmc_evt_t *uiox_emmc_if_irq_handle(uiox_emmc_if_t *eif,
                                           uint32_t now_ms);
 
 void uiox_emmc_if_stats_get   (const uiox_emmc_if_t *eif,
                                 uiox_emmc_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_EMMC_IF_H */
 