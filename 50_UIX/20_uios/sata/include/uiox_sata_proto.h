/**
 * @file  uiox_sata_proto.h
 * @brief UIOX SATA protocol layer — IDENTIFY, ATA R/W, NCQ, SMART.
 * @date  2026-06-12
 */

 #ifndef UIOX_SATA_PROTO_H
 #define UIOX_SATA_PROTO_H
 
 #include "uiox_sata_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_SATA_INIT_IDLE       = 0,
     UIOX_SATA_INIT_RESET,
     UIOX_SATA_INIT_IDENTIFY,
     UIOX_SATA_INIT_FEATURES,
     UIOX_SATA_INIT_SMART_EN,
     UIOX_SATA_INIT_DONE,
     UIOX_SATA_INIT_ERROR,
 } uiox_sata_init_state_t;
 
 typedef struct {
     uiox_sata_if_t        *sif;
     uiox_sata_init_state_t init_state;
     bool                   initialized;
     bool                   ncq_enabled;
 } uiox_sata_proto_t;
 
 /* =========================================================================
  * Protocol API
  * ====================================================================== */
 
 int  uiox_sata_proto_init      (uiox_sata_proto_t *proto,
                                  uiox_sata_if_t *sif);
 
 /* Full device initialisation (reset → IDENTIFY → features) */
 int  uiox_sata_proto_dev_init  (uiox_sata_proto_t *proto);
 
 /* ATA IDENTIFY (populates hw->ident) */
 int  uiox_sata_proto_identify  (uiox_sata_proto_t *proto);
 
 /* Block I/O */
 int  uiox_sata_proto_read      (uiox_sata_proto_t *proto, uint64_t lba,
                                  uint8_t *buf, uint32_t sectors);
 int  uiox_sata_proto_write     (uiox_sata_proto_t *proto, uint64_t lba,
                                  const uint8_t *buf, uint32_t sectors);
 
 /* NCQ I/O */
 int  uiox_sata_proto_ncq_read  (uiox_sata_proto_t *proto, uint64_t lba,
                                  uint8_t *buf, uint32_t sectors);
 int  uiox_sata_proto_ncq_write (uiox_sata_proto_t *proto, uint64_t lba,
                                  const uint8_t *buf, uint32_t sectors);
 
 /* Flush write cache */
 int  uiox_sata_proto_flush     (uiox_sata_proto_t *proto);
 
 /* TRIM (Data Set Management) */
 int  uiox_sata_proto_trim      (uiox_sata_proto_t *proto,
                                  uint64_t lba_start, uint32_t sectors);
 
 /* SMART */
 int  uiox_sata_proto_smart_read(uiox_sata_proto_t *proto, uint8_t *buf);
 int  uiox_sata_proto_smart_en  (uiox_sata_proto_t *proto);
 
 /* Power management */
 int  uiox_sata_proto_standby   (uiox_sata_proto_t *proto);
 int  uiox_sata_proto_sleep     (uiox_sata_proto_t *proto);
 
 /* Capacity from IDENTIFY buffer */
 uint64_t uiox_sata_proto_capacity(const uiox_sata_ident_t *ident);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SATA_PROTO_H */
 