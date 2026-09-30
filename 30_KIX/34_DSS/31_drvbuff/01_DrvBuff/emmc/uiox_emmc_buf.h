/**
 * @file  uiox_emmc_buf.h
 * @brief UIOX eMMC block / command / event buffer pool.
 * @date  2026-06-12
 */

 #ifndef UIOX_EMMC_BUF_H
 #define UIOX_EMMC_BUF_H
 
 #include "uiox_emmc_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_EMMC_BLK_POOL_SIZE    8u    /**< 4 KB block buffers (8×512)  */
 #define UIOX_EMMC_CMD_POOL_SIZE    16u
 #define UIOX_EMMC_EVT_POOL_SIZE    16u
 #define UIOX_EMMC_SECTORS_PER_BLK 8u    /**< 8 sectors = 4 KB            */
 
 /* =========================================================================
  * Block buffer
  * ====================================================================== */
 
 typedef struct {
     uint8_t          data[UIOX_EMMC_SECTORS_PER_BLK * UIOX_EMMC_BLOCK_SIZE];
     uint32_t         lba;
     uint32_t         sectors;
     uiox_emmc_part_t part;
     bool             dirty;
     uint8_t          in_use;
 } uiox_emmc_blk_t;
 
 /* =========================================================================
  * Command record
  * ====================================================================== */
 
 typedef struct {
     uint8_t           cmd_idx;
     uint32_t          arg;
     uiox_emmc_resp_t  resp_type;
     uint32_t          resp[4];
     int               status;
     uint8_t           in_use;
 } uiox_emmc_cmd_t;
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 typedef enum {
     UIOX_EMMC_EVT_NONE          = 0,
     UIOX_EMMC_EVT_READY,
     UIOX_EMMC_EVT_READ_DONE,
     UIOX_EMMC_EVT_WRITE_DONE,
     UIOX_EMMC_EVT_FLUSH_DONE,
     UIOX_EMMC_EVT_CMD_DONE,
     UIOX_EMMC_EVT_HEALTH_WARN,
     UIOX_EMMC_EVT_EOL_WARN,
     UIOX_EMMC_EVT_BKOPS_NEEDED,
     UIOX_EMMC_EVT_ERROR,
 } uiox_emmc_evt_type_t;
 
 typedef struct {
     uiox_emmc_evt_type_t type;
     uint32_t             timestamp_ms;
     uint32_t             lba;
     uint32_t             sectors;
     uiox_emmc_part_t     part;
     int                  status;
     uint8_t              in_use;
 } uiox_emmc_evt_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void               uiox_emmc_buf_init      (void);
 
 uiox_emmc_blk_t   *uiox_emmc_blk_alloc    (void);
 void               uiox_emmc_blk_free      (uiox_emmc_blk_t *b);
 uint8_t            uiox_emmc_blk_free_cnt  (void);
 
 uiox_emmc_cmd_t   *uiox_emmc_cmd_alloc     (void);
 void               uiox_emmc_cmd_free      (uiox_emmc_cmd_t *c);
 uint8_t            uiox_emmc_cmd_free_cnt  (void);
 
 uiox_emmc_evt_t   *uiox_emmc_evt_alloc     (void);
 void               uiox_emmc_evt_free      (uiox_emmc_evt_t *e);
 uint8_t            uiox_emmc_evt_free_cnt  (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_EMMC_BUF_H */
 