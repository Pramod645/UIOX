/**
 * @file    uiox_ram_buf.h
 * @brief   UIOX RAM memory region descriptor pool.
 *
 * Manages descriptors for memory regions (allocated blocks, DMA regions,
 * MPU-protected zones). Descriptors are separate from the actual memory
 * they describe — they live in a static pool in flash/SRAM.
 *
 * @date    2026-06-03
 */

 #ifndef UIOX_RAM_BUF_H
 #define UIOX_RAM_BUF_H
 
 #include "uiox_ram_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_RAM_REGION_POOL_SIZE   256
 
 /* =========================================================================
  * Memory region type
  * ====================================================================== */
 
 typedef enum {
     UIOX_RAM_REGION_FREE     = 0,
     UIOX_RAM_REGION_HEAP,        /**< General heap allocation             */
     UIOX_RAM_REGION_SLAB,        /**< Fixed-size slab allocation          */
     UIOX_RAM_REGION_DMA,         /**< DMA-coherent region                 */
     UIOX_RAM_REGION_STACK,       /**< Thread/task stack                   */
     UIOX_RAM_REGION_MPU,         /**< MPU-protected zone                  */
     UIOX_RAM_REGION_RESERVED,    /**< Reserved / firmware                 */
     UIOX_RAM_REGION_ECC_SCRUB,   /**< ECC scrub in progress              */
 } uiox_ram_region_type_t;
 
 /* =========================================================================
  * Memory region descriptor
  * ====================================================================== */
 
 typedef struct uiox_ram_region {
     uint64_t               phys_base;
     uint64_t               size;
     uiox_ram_region_type_t type;
     uint8_t                channel;    /**< DRAM channel (0..3)           */
     uint8_t                rank;       /**< DRAM rank (0..1)              */
     uint8_t                align_log2; /**< Alignment (2^n bytes)         */
     bool                   cached;
     bool                   executable;
     bool                   writable;
     uint8_t                in_use;
     struct uiox_ram_region *next;
 } uiox_ram_region_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void               uiox_ram_buf_init     (void);
 uiox_ram_region_t *uiox_ram_buf_alloc    (void);
 void               uiox_ram_buf_free     (uiox_ram_region_t *r);
 uint16_t           uiox_ram_buf_free_cnt (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RAM_BUF_H */
 