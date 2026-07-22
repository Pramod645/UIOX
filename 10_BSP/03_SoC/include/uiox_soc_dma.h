/**
 * @file    uiox_soc_dma.h
 * @brief   UIOX SoC HAL — DMA controller abstraction.
 *
 * Supports:
 *   ARM64/ARM32 — ARM PL080 / PL330 DMAC
 *   x86_64      — Intel 8237A compatible DMA
 *
 * Used by: storage (SD DMA), UART DMA TX/RX,
 *          network copy acceleration.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_SOC_DMA_H
 #define UIOX_SOC_DMA_H
 
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_SOC_DMA_MAX_CHANNELS   8u
 
 typedef enum {
     UIOX_SOC_DMA_DIR_MEM_TO_MEM = 0,
     UIOX_SOC_DMA_DIR_MEM_TO_DEV = 1,
     UIOX_SOC_DMA_DIR_DEV_TO_MEM = 2,
     UIOX_SOC_DMA_DIR_DEV_TO_DEV = 3,
 } uiox_soc_dma_dir_t;
 
 typedef enum {
     UIOX_SOC_DMA_WIDTH_8  = 0,
     UIOX_SOC_DMA_WIDTH_16 = 1,
     UIOX_SOC_DMA_WIDTH_32 = 2,
 } uiox_soc_dma_width_t;
 
 typedef void (*uiox_soc_dma_cb_t)(uiox_uint32_t chan,
                                     int status, void *priv);
 
 typedef struct {
    uiox_uintptr_t            src;
    uiox_uintptr_t            dst;
     uiox_uint32_t             len;
     uiox_soc_dma_dir_t   dir;
     uiox_soc_dma_width_t width;
     uiox_bool_t                 src_inc;   /**< Increment source address        */
     uiox_bool_t                 dst_inc;   /**< Increment dest  address         */
     uiox_soc_dma_cb_t    complete_cb;
     void                *cb_priv;
 } uiox_soc_dma_xfer_t;
 
 typedef struct {
    uiox_uintptr_t          base;
     uiox_uint32_t           irq;
     uiox_uint32_t           num_channels;
     uiox_uint32_t           caps;
     uiox_bool_t               initialized;
     /* Per-channel state */
     uiox_soc_dma_cb_t  chan_cb   [UIOX_SOC_DMA_MAX_CHANNELS];
     void              *chan_priv [UIOX_SOC_DMA_MAX_CHANNELS];
     uiox_bool_t               chan_busy [UIOX_SOC_DMA_MAX_CHANNELS];
     void              *priv;
 } uiox_soc_dma_ctrl_t;
 
 typedef struct {
     uiox_soc_err_t (*init)      (uiox_soc_dma_ctrl_t *ctrl);
     void           (*deinit)    (uiox_soc_dma_ctrl_t *ctrl);
     uiox_soc_err_t (*transfer)  (uiox_soc_dma_ctrl_t *ctrl,
                                   uiox_uint32_t chan,
                                   const uiox_soc_dma_xfer_t *xfer);
     uiox_bool_t           (*busy)      (uiox_soc_dma_ctrl_t *ctrl,
                                   uiox_uint32_t chan);
     uiox_soc_err_t (*wait)      (uiox_soc_dma_ctrl_t *ctrl,
                                   uiox_uint32_t chan);
     void           (*abort)     (uiox_soc_dma_ctrl_t *ctrl,
                                   uiox_uint32_t chan);
     int            (*alloc_chan)(uiox_soc_dma_ctrl_t *ctrl);
     void           (*free_chan) (uiox_soc_dma_ctrl_t *ctrl,
                                   uiox_uint32_t chan);
     void           (*isr)       (uiox_soc_dma_ctrl_t *ctrl);
 } uiox_soc_dma_ops_t;
 
 /* ── DMA API ────────────────────────────────────────────── */
 uiox_soc_err_t uiox_soc_dma_init      (uiox_soc_dma_ctrl_t *ctrl,
                                         const uiox_soc_dma_ops_t *ops);
 void           uiox_soc_dma_deinit    (uiox_soc_dma_ctrl_t *ctrl);
 uiox_soc_err_t uiox_soc_dma_transfer  (uiox_soc_dma_ctrl_t *ctrl,
                                         uiox_uint32_t chan,
                                         const uiox_soc_dma_xfer_t *xfer);
 uiox_bool_t           uiox_soc_dma_busy      (uiox_soc_dma_ctrl_t *ctrl,
                                         uiox_uint32_t chan);
 uiox_soc_err_t uiox_soc_dma_wait      (uiox_soc_dma_ctrl_t *ctrl,
                                         uiox_uint32_t chan);
 void           uiox_soc_dma_abort     (uiox_soc_dma_ctrl_t *ctrl,
                                         uiox_uint32_t chan);
 int            uiox_soc_dma_alloc_chan (uiox_soc_dma_ctrl_t *ctrl);
 void           uiox_soc_dma_free_chan  (uiox_soc_dma_ctrl_t *ctrl,
                                         uiox_uint32_t chan);
 
 /** Simple synchronous memory copy via DMA (blocks until done). */
 uiox_soc_err_t uiox_soc_dma_memcpy   (uiox_soc_dma_ctrl_t *ctrl,
                                         uiox_uintptr_t dst,
                                         uiox_uintptr_t src,
                                         uiox_uint32_t  len);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_DMA_H */
 