/**
 * @file  uiox_sata_buf.h
 * @brief UIOX SATA PRD / command table / event buffer pool.
 * @date  2026-06-12
 */

 #ifndef UIOX_SATA_BUF_H
 #define UIOX_SATA_BUF_H
 
 #include "uiox_sata_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_SATA_CMD_POOL_SIZE     32u   /**< Matches NCQ depth          */
 #define UIOX_SATA_EVT_POOL_SIZE     16u
 #define UIOX_SATA_BLK_POOL_SIZE     8u    /**< 4 KB sector buffers        */
 #define UIOX_SATA_SECTORS_PER_BLK   8u    /**< 8 × 512 B = 4 KB per buf  */
 
 /* =========================================================================
  * Command slot record
  * ====================================================================== */
 
 typedef enum {
     UIOX_SATA_CMD_FREE    = 0,
     UIOX_SATA_CMD_PENDING,
     UIOX_SATA_CMD_DONE,
     UIOX_SATA_CMD_ERROR,
 } uiox_sata_cmd_state_t;
 
 typedef struct {
     uiox_sata_fis_h2d_t   fis;
     uiox_sata_prd_t       prdt[UIOX_SATA_PRD_PER_CMD];
     uint8_t               slot;
     uint8_t               tag;         /**< NCQ tag                       */
     uint64_t              lba;
     uint32_t              sector_count;
     bool                  write;
     uiox_sata_cmd_state_t state;
     int                   status;
     uint8_t               in_use;
 } uiox_sata_cmd_t;
 
 /* =========================================================================
  * Sector buffer
  * ====================================================================== */
 
 typedef struct {
     uint8_t  data[UIOX_SATA_SECTORS_PER_BLK * UIOX_SATA_SECTOR_SIZE];
     uint64_t lba;
     uint32_t sectors;
     bool     dirty;
     uint8_t  in_use;
 } uiox_sata_blk_t;
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 typedef enum {
     UIOX_SATA_EVT_NONE        = 0,
     UIOX_SATA_EVT_DEV_ATTACH,
     UIOX_SATA_EVT_DEV_DETACH,
     UIOX_SATA_EVT_READ_DONE,
     UIOX_SATA_EVT_WRITE_DONE,
     UIOX_SATA_EVT_CMD_DONE,
     UIOX_SATA_EVT_NCQ_DONE,
     UIOX_SATA_EVT_ERROR,
     UIOX_SATA_EVT_SMART_WARN,
 } uiox_sata_evt_type_t;
 
 typedef struct {
     uiox_sata_evt_type_t type;
     uint32_t             timestamp_ms;
     uint64_t             lba;
     uint32_t             sector_count;
     int                  status;
     uint32_t             error_reg;
     uint8_t              in_use;
 } uiox_sata_evt_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void               uiox_sata_buf_init       (void);
 
 uiox_sata_cmd_t   *uiox_sata_cmd_alloc      (void);
 void               uiox_sata_cmd_free       (uiox_sata_cmd_t *c);
 uint8_t            uiox_sata_cmd_free_cnt   (void);
 
 uiox_sata_blk_t   *uiox_sata_blk_alloc      (void);
 void               uiox_sata_blk_free       (uiox_sata_blk_t *b);
 uint8_t            uiox_sata_blk_free_cnt   (void);
 
 uiox_sata_evt_t   *uiox_sata_evt_alloc      (void);
 void               uiox_sata_evt_free       (uiox_sata_evt_t *e);
 uint8_t            uiox_sata_evt_free_cnt   (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SATA_BUF_H */
 