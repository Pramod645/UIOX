/**
 * @file    uiox_bios_buf.h
 * @brief   UIOX BIOS flash page buffer pool.
 *
 * Provides statically-allocated page-sized buffers for:
 *   - Read-modify-write operations on flash sectors
 *   - Staging areas for firmware updates
 *   - NVRAM variable read/write staging
 *
 * @date    2026-06-04
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_BIOS_BUF_H
 #define UIOX_BIOS_BUF_H
 
 #include "uiox_bios_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_BIOS_PAGE_SIZE     256u    /**< SPI flash page (bytes)        */
 #define UIOX_BIOS_SECTOR_SIZE   4096u   /**< 4 KB sector                  */
 #define UIOX_BIOS_BUF_POOL_SIZE 4       /**< Staging buffer pool depth     */
 #define UIOX_BIOS_BUF_ALIGN     64
 
 typedef enum {
     UIOX_BIOS_BUF_FREE = 0,
     UIOX_BIOS_BUF_READ_STAGE,
     UIOX_BIOS_BUF_WRITE_STAGE,
     UIOX_BIOS_BUF_NVRAM_STAGE,
 } uiox_bios_buf_use_t;
 
 typedef struct uiox_bios_buf {
     uint8_t   data[UIOX_BIOS_SECTOR_SIZE + UIOX_BIOS_BUF_ALIGN];
     uint8_t  *aligned;       /**< Aligned pointer into data[]              */
     uint32_t  flash_offset;  /**< Flash offset this buffer represents      */
     uint32_t  valid_bytes;
     uiox_bios_buf_use_t use;
     uint8_t   in_use;
     struct uiox_bios_buf *next;
 } uiox_bios_buf_t;
 
 void             uiox_bios_buf_init  (void);
 uiox_bios_buf_t *uiox_bios_buf_alloc (uiox_bios_buf_use_t use);
 void             uiox_bios_buf_free  (uiox_bios_buf_t *b);
 uint8_t          uiox_bios_buf_free_count(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BIOS_BUF_H */
 