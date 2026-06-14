/**
 * @file  uiox_sd_proto.h
 * @brief UIOX SD protocol layer — card init, CSD/CID decode, R/W.
 * @date  2026-06-11
 */

 #ifndef UIOX_SD_PROTO_H
 #define UIOX_SD_PROTO_H
 
 #include "uiox_sd_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* SD initialisation states */
 typedef enum {
     UIOX_SD_INIT_IDLE        = 0,
     UIOX_SD_INIT_VOLTAGE_CHK,
     UIOX_SD_INIT_ACMD41,
     UIOX_SD_INIT_SEND_CID,
     UIOX_SD_INIT_SEND_RCA,
     UIOX_SD_INIT_SEND_CSD,
     UIOX_SD_INIT_SELECT_CARD,
     UIOX_SD_INIT_SET_BLKLEN,
     UIOX_SD_INIT_SET_BUS_WIDTH,
     UIOX_SD_INIT_SEND_SCR,
     UIOX_SD_INIT_DONE,
     UIOX_SD_INIT_ERROR,
 } uiox_sd_init_state_t;
 
 typedef struct {
     uiox_sd_if_t        *sif;
     uiox_sd_init_state_t init_state;
     uint32_t             init_retry;
     bool                 initialized;
 } uiox_sd_proto_t;
 
 /* =========================================================================
  * Protocol API
  * ====================================================================== */
 
 int  uiox_sd_proto_init        (uiox_sd_proto_t *proto,
                                  uiox_sd_if_t *sif);
 
 /* Full card initialisation sequence (CMD0→ACMD41→CID→RCA→CSD→SELECT) */
 int  uiox_sd_proto_card_init   (uiox_sd_proto_t *proto);
 
 /* Block I/O (calls CMD17/18/24/25 as appropriate) */
 int  uiox_sd_proto_read        (uiox_sd_proto_t *proto, uint32_t lba,
                                  uint8_t *buf, uint32_t count);
 int  uiox_sd_proto_write       (uiox_sd_proto_t *proto, uint32_t lba,
                                  const uint8_t *buf, uint32_t count);
 
 /* Card status */
 int  uiox_sd_proto_get_status  (uiox_sd_proto_t *proto, uint32_t *status);
 
 /* Erase */
 int  uiox_sd_proto_erase       (uiox_sd_proto_t *proto,
                                  uint32_t lba_start, uint32_t lba_end);
 
 /* Switch to high-speed (CMD6) */
 int  uiox_sd_proto_switch_hs   (uiox_sd_proto_t *proto);
 
 /* Decode capacity from CSD */
 uint64_t uiox_sd_proto_capacity(const uiox_sd_card_t *card);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SD_PROTO_H */
 