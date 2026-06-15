/**
 * @file  uiox_nvme_buf.h
 * @brief UIOX NVMe SQ/CQ descriptor / PRP / event buffer pool.
 * @date  2026-06-12
 */

 #ifndef UIOX_NVME_BUF_H
 #define UIOX_NVME_BUF_H
 
 #include "uiox_nvme_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_NVME_CMD_POOL_SIZE     64u    /**< In-flight command records  */
 #define UIOX_NVME_EVT_POOL_SIZE     16u
 #define UIOX_NVME_BLK_POOL_SIZE     8u    /**< 4 KB data buffers          */
 #define UIOX_NVME_SECTORS_PER_BLK   8u    /**< 8 × 512 B = 4 KB           */
 
 /* =========================================================================
  * In-flight command record
  * ====================================================================== */
 
 typedef enum {
     UIOX_NVME_CMD_FREE    = 0,
     UIOX_NVME_CMD_PENDING,
     UIOX_NVME_CMD_DONE,
     UIOX_NVME_CMD_ERROR,
 } uiox_nvme_cmd_state_t;
 
 typedef struct {
     uiox_nvme_sqe_t       sqe;
     uiox_nvme_cqe_t       cqe;
     uint16_t              qid;         /**< Queue this command was sent to */
     uiox_nvme_cmd_state_t state;
     int                   status;
     uint8_t               in_use;
 } uiox_nvme_cmd_t;
 
 /* =========================================================================
  * Data block buffer (4 KB)
  * ====================================================================== */
 
 typedef struct {
     uint8_t  data[UIOX_NVME_SECTORS_PER_BLK * UIOX_NVME_LBA_SIZE];
     uint64_t slba;       /**< Starting LBA                               */
     uint32_t nlb;        /**< Number of LBAs                             */
     uint32_t nsid;
     bool     dirty;
     uint8_t  in_use;
 } uiox_nvme_blk_t;
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 typedef enum {
     UIOX_NVME_EVT_NONE          = 0,
     UIOX_NVME_EVT_READY,
     UIOX_NVME_EVT_READ_DONE,
     UIOX_NVME_EVT_WRITE_DONE,
     UIOX_NVME_EVT_FLUSH_DONE,
     UIOX_NVME_EVT_TRIM_DONE,
     UIOX_NVME_EVT_CMD_DONE,
     UIOX_NVME_EVT_HEALTH_WARN,
     UIOX_NVME_EVT_TEMP_WARN,
     UIOX_NVME_EVT_ERROR,
     UIOX_NVME_EVT_FATAL,
 } uiox_nvme_evt_type_t;
 
 typedef struct {
     uiox_nvme_evt_type_t type;
     uint32_t             timestamp_ms;
     uint32_t             nsid;
     uint64_t             slba;
     uint32_t             nlb;
     int                  status;
     uint16_t             cid;
     uint8_t              in_use;
 } uiox_nvme_evt_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void               uiox_nvme_buf_init      (void);
 
 uiox_nvme_cmd_t   *uiox_nvme_cmd_alloc    (void);
 void               uiox_nvme_cmd_free      (uiox_nvme_cmd_t *c);
 uint8_t            uiox_nvme_cmd_free_cnt  (void);
 
 uiox_nvme_blk_t   *uiox_nvme_blk_alloc    (void);
 void               uiox_nvme_blk_free      (uiox_nvme_blk_t *b);
 uint8_t            uiox_nvme_blk_free_cnt  (void);
 
 uiox_nvme_evt_t   *uiox_nvme_evt_alloc    (void);
 void               uiox_nvme_evt_free      (uiox_nvme_evt_t *e);
 uint8_t            uiox_nvme_evt_free_cnt  (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NVME_BUF_H */
 