/**
 * @file  uiox_emmc_proto.h
 * @brief UIOX eMMC protocol — init, EXT_CSD, HS400, R/W, health.
 * @date  2026-06-12
 */

 #ifndef UIOX_EMMC_PROTO_H
 #define UIOX_EMMC_PROTO_H
 
 #include "uiox_emmc_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_EMMC_INIT_IDLE        = 0,
     UIOX_EMMC_INIT_RESET,
     UIOX_EMMC_INIT_CMD1,
     UIOX_EMMC_INIT_CID,
     UIOX_EMMC_INIT_RCA,
     UIOX_EMMC_INIT_CSD,
     UIOX_EMMC_INIT_SELECT,
     UIOX_EMMC_INIT_EXT_CSD,
     UIOX_EMMC_INIT_BUS_WIDTH,
     UIOX_EMMC_INIT_SPEED,
     UIOX_EMMC_INIT_CACHE_EN,
     UIOX_EMMC_INIT_DONE,
     UIOX_EMMC_INIT_ERROR,
 } uiox_emmc_init_state_t;
 
 typedef struct {
     uiox_emmc_if_t        *eif;
     uiox_emmc_init_state_t init_state;
     bool                   initialized;
     bool                   cache_enabled;
     uiox_emmc_speed_t      negotiated_speed;
     uint8_t                negotiated_width;
 } uiox_emmc_proto_t;
 
 int  uiox_emmc_proto_init      (uiox_emmc_proto_t *proto,
                                  uiox_emmc_if_t *eif);
 int  uiox_emmc_proto_dev_init  (uiox_emmc_proto_t *proto);
 
 int  uiox_emmc_proto_read      (uiox_emmc_proto_t *proto,
                                  uiox_emmc_part_t part, uint32_t lba,
                                  uint8_t *buf, uint32_t sectors);
 int  uiox_emmc_proto_write     (uiox_emmc_proto_t *proto,
                                  uiox_emmc_part_t part, uint32_t lba,
                                  const uint8_t *buf, uint32_t sectors);
 
 int  uiox_emmc_proto_flush     (uiox_emmc_proto_t *proto);
 int  uiox_emmc_proto_trim      (uiox_emmc_proto_t *proto,
                                  uint32_t lba, uint32_t sectors);
 int  uiox_emmc_proto_cache_ctrl(uiox_emmc_proto_t *proto, bool enable);
 int  uiox_emmc_proto_bkops     (uiox_emmc_proto_t *proto);
 int  uiox_emmc_proto_pon       (uiox_emmc_proto_t *proto);
 
 uint64_t uiox_emmc_proto_capacity(const uiox_emmc_ident_t *id);
 void     uiox_emmc_proto_parse_ext_csd(uiox_emmc_ident_t *id,
                                         const uint8_t *ext_csd);
 int      uiox_emmc_proto_health_check(uiox_emmc_proto_t *proto,
                                        uint8_t *pre_eol,
                                        uint8_t *life_a,
                                        uint8_t *life_b);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_EMMC_PROTO_H */
 