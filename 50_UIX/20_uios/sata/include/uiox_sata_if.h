/**
 * @file  uiox_sata_if.h
 * @brief UIOX SATA interface driver — FIS framing, command issue, IRQ.
 * @date  2026-06-12
 */

 #ifndef UIOX_SATA_IF_H
 #define UIOX_SATA_IF_H
 
 #include "uiox_sata_hw.h"
 #include "uiox_sata_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  sectors_read;
     uint64_t  sectors_written;
     uint64_t  bytes_read;
     uint64_t  bytes_written;
     uint32_t  cmds_issued;
     uint32_t  ncq_cmds_issued;
     uint32_t  irq_count;
     uint32_t  errors;
     uint32_t  resets;
 } uiox_sata_if_stats_t;
 
 typedef struct {
     uiox_sata_hw_t      *hw;
     uiox_sata_if_stats_t stats;
     bool                 primed;
     uint8_t              active_port;
     /* Command slot occupancy bitmap */
     uint32_t             slot_bitmap;
 } uiox_sata_if_t;
 
 int  uiox_sata_if_config     (uiox_sata_if_t *sif, uiox_sata_hw_t *hw);
 int  uiox_sata_if_start      (uiox_sata_if_t *sif);
 void uiox_sata_if_stop       (uiox_sata_if_t *sif);
 
 /* Build and issue an H2D FIS command */
 int  uiox_sata_if_issue_cmd  (uiox_sata_if_t *sif,
                                uiox_sata_cmd_t *cmd);
 
 /* Block transfers */
 int  uiox_sata_if_read       (uiox_sata_if_t *sif, uint64_t lba,
                                uint8_t *buf, uint32_t sectors);
 int  uiox_sata_if_write      (uiox_sata_if_t *sif, uint64_t lba,
                                const uint8_t *buf, uint32_t sectors);
 
 /* NCQ */
 int  uiox_sata_if_ncq_read   (uiox_sata_if_t *sif, uint64_t lba,
                                uint8_t *buf, uint32_t sectors, uint8_t tag);
 int  uiox_sata_if_ncq_write  (uiox_sata_if_t *sif, uint64_t lba,
                                const uint8_t *buf, uint32_t sectors,
                                uint8_t tag);
 
 /* Find a free command slot */
 int  uiox_sata_if_alloc_slot (uiox_sata_if_t *sif);
 void uiox_sata_if_free_slot  (uiox_sata_if_t *sif, uint8_t slot);
 
 /* IRQ handler */
 uiox_sata_evt_t *uiox_sata_if_irq_handle(uiox_sata_if_t *sif,
                                           uint32_t now_ms);
 
 void uiox_sata_if_stats_get  (const uiox_sata_if_t *sif,
                                uiox_sata_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SATA_IF_H */
 