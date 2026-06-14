/**
 * @file  uiox_sd_buf.h
 * @brief UIOX SD block and command queue buffer pool.
 * @date  2026-06-11
 */

 #ifndef UIOX_SD_BUF_H
 #define UIOX_SD_BUF_H
 
 #include "uiox_sd_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_SD_BLOCK_POOL_SIZE     8u    /**< Pre-allocated 512 B blocks  */
 #define UIOX_SD_CMD_POOL_SIZE       16u   /**< Command records             */
 #define UIOX_SD_EVT_POOL_SIZE       16u   /**< Event records               */
 
 /* =========================================================================
  * Block buffer
  * ====================================================================== */
 
 typedef struct {
     uint8_t   data[UIOX_SD_BLOCK_SIZE];
     uint32_t  lba;          /**< Block address this buffer was read from  */
     uint32_t  len;          /**< Valid bytes (always 512 for full blocks) */
     bool      dirty;        /**< True = needs write-back                  */
     uint8_t   in_use;
 } uiox_sd_block_t;
 
 /* =========================================================================
  * Command record
  * ====================================================================== */
 
 typedef struct {
     uint8_t         cmd_idx;
     uint32_t        arg;
     uiox_sd_resp_t  resp_type;
     uint32_t        resp[4];    /**< Up to 136-bit response (4 × 32-bit) */
     int             status;     /**< 0 = success, <0 = errno             */
     uint8_t         in_use;
 } uiox_sd_cmd_t;
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 typedef enum {
     UIOX_SD_EVT_NONE         = 0,
     UIOX_SD_EVT_CARD_INSERT,
     UIOX_SD_EVT_CARD_REMOVE,
     UIOX_SD_EVT_READ_DONE,
     UIOX_SD_EVT_WRITE_DONE,
     UIOX_SD_EVT_CMD_DONE,
     UIOX_SD_EVT_ERROR,
     UIOX_SD_EVT_WP_CHANGE,
 } uiox_sd_evt_type_t;
 
 typedef struct {
     uiox_sd_evt_type_t  type;
     uint32_t            timestamp_ms;
     uint32_t            lba;
     uint32_t            count;
     int                 status;
     uint8_t             in_use;
 } uiox_sd_evt_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void             uiox_sd_buf_init       (void);
 
 uiox_sd_block_t *uiox_sd_block_alloc    (void);
 void             uiox_sd_block_free     (uiox_sd_block_t *b);
 uint8_t          uiox_sd_block_free_cnt (void);
 
 uiox_sd_cmd_t   *uiox_sd_cmd_alloc      (void);
 void             uiox_sd_cmd_free       (uiox_sd_cmd_t *c);
 uint8_t          uiox_sd_cmd_free_cnt   (void);
 
 uiox_sd_evt_t   *uiox_sd_evt_alloc      (void);
 void             uiox_sd_evt_free       (uiox_sd_evt_t *e);
 uint8_t          uiox_sd_evt_free_cnt   (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SD_BUF_H */
 