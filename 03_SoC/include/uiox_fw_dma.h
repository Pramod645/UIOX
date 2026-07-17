/**
 * @file  uiox_fw_dma.h
 * @brief UIOX Firmware HAL — DMA controller abstraction.
 *
 * Supports:
 *   ARM64/ARM32 — ARM PL080 / PL330 DMAC
 *   x86_64      — Intel 8237A compatible DMA
 *
 * Used by: storage (SD DMA), UART DMA TX/RX, network copy acceleration.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_DMA_H
 #define UIOX_FW_DMA_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_DMA_MAX_CHANNELS   8u
 
 typedef enum {
     UIOX_DMA_DIR_MEM_TO_MEM = 0,
     UIOX_DMA_DIR_MEM_TO_DEV = 1,
     UIOX_DMA_DIR_DEV_TO_MEM = 2,
     UIOX_DMA_DIR_DEV_TO_DEV = 3,
 } uiox_dma_dir_t;
 
 typedef enum {
     UIOX_DMA_WIDTH_8  = 0,
     UIOX_DMA_WIDTH_16 = 1,
     UIOX_DMA_WIDTH_32 = 2,
 } uiox_dma_width_t;
 
 typedef void (*uiox_dma_cb_t)(uint32_t chan, int status, void *priv);
 
 typedef struct {
     uintptr_t       src;
     uintptr_t       dst;
     uint32_t        len;
     uiox_dma_dir_t  dir;
     uiox_dma_width_t width;
     bool            src_inc;    /**< Increment source address           */
     bool            dst_inc;    /**< Increment dest address             */
     uiox_dma_cb_t   complete_cb;
     void           *cb_priv;
 } uiox_dma_xfer_t;
 
 typedef struct {
     uintptr_t  base;
     uint32_t   irq;
     uint32_t   num_channels;
     uint32_t   caps;
     bool       initialized;
     /* Per-channel state */
     uiox_dma_cb_t  chan_cb   [UIOX_DMA_MAX_CHANNELS];
     void          *chan_priv [UIOX_DMA_MAX_CHANNELS];
     bool           chan_busy [UIOX_DMA_MAX_CHANNELS];
     void          *priv;
 } uiox_dma_ctrl_t;
 
 typedef struct {
     uiox_fw_err_t (*init)     (uiox_dma_ctrl_t *ctrl);
     void          (*deinit)   (uiox_dma_ctrl_t *ctrl);
     uiox_fw_err_t (*transfer) (uiox_dma_ctrl_t *ctrl,
                                 uint32_t chan,
                                 const uiox_dma_xfer_t *xfer);
     bool          (*busy)     (uiox_dma_ctrl_t *ctrl, uint32_t chan);
     uiox_fw_err_t (*wait)     (uiox_dma_ctrl_t *ctrl, uint32_t chan);
     void          (*abort)    (uiox_dma_ctrl_t *ctrl, uint32_t chan);
     int           (*alloc_chan)(uiox_dma_ctrl_t *ctrl);
     void          (*free_chan) (uiox_dma_ctrl_t *ctrl, uint32_t chan);
     void          (*isr)      (uiox_dma_ctrl_t *ctrl);
 } uiox_dma_ops_t;
 
 uiox_fw_err_t uiox_fw_dma_init     (uiox_dma_ctrl_t *ctrl,
                                       const uiox_dma_ops_t *ops);
 void          uiox_fw_dma_deinit   (uiox_dma_ctrl_t *ctrl);
 uiox_fw_err_t uiox_fw_dma_transfer (uiox_dma_ctrl_t *ctrl,
                                       uint32_t chan,
                                       const uiox_dma_xfer_t *xfer);
 bool          uiox_fw_dma_busy     (uiox_dma_ctrl_t *ctrl, uint32_t chan);
 uiox_fw_err_t uiox_fw_dma_wait     (uiox_dma_ctrl_t *ctrl, uint32_t chan);
 void          uiox_fw_dma_abort    (uiox_dma_ctrl_t *ctrl, uint32_t chan);
 int           uiox_fw_dma_alloc_chan(uiox_dma_ctrl_t *ctrl);
 void          uiox_fw_dma_free_chan (uiox_dma_ctrl_t *ctrl, uint32_t chan);
 
 /* Simple synchronous memory copy via DMA (blocks until done) */
 uiox_fw_err_t uiox_fw_dma_memcpy   (uiox_dma_ctrl_t *ctrl,
                                       uintptr_t dst, uintptr_t src,
                                       uint32_t len);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_DMA_H */
 