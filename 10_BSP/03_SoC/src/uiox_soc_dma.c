/**
 * @file    uiox_soc_dma.c
 * @brief   UIOX SoC HAL — DMA controller (software-fallback driver).
 *          Provides a working DMA API backed by CPU copy when no real
 *          DMA hardware is present (QEMU simulation mode).
 * @date    2026-07-07
 */

 #include "../include/uiox_soc_dma.h"

 #define OPS(c) ((const uiox_soc_dma_ops_t *)(c)->priv)
 
 /* =========================================================================
  * Software fallback DMA (CPU copy — no real DMA hardware needed)
  * ====================================================================== */
 
 static uiox_soc_err_t sw_dma_init(uiox_soc_dma_ctrl_t *ctrl)
 {
     ctrl->num_channels = UIOX_SOC_DMA_MAX_CHANNELS;
     ctrl->initialized  = true;
     return UIOX_SOC_OK;
 }
 
 static void sw_dma_deinit(uiox_soc_dma_ctrl_t *ctrl)
 { ctrl->initialized = false; }
 
 static uiox_soc_err_t sw_dma_transfer(uiox_soc_dma_ctrl_t *ctrl,
                                         uiox_uint32_t chan,
                                         const uiox_soc_dma_xfer_t *xfer)
 {
     if (chan >= UIOX_SOC_DMA_MAX_CHANNELS) return UIOX_SOC_ERR_INVAL;
     if (ctrl->chan_busy[chan])              return UIOX_SOC_ERR_BUSY;
     ctrl->chan_busy[chan] = true;
 
     /* Software copy respecting width and increment flags */
     if (xfer->width == UIOX_SOC_DMA_WIDTH_32) {
         volatile uiox_uint32_t *s = (volatile uiox_uint32_t *)xfer->src;
         volatile uiox_uint32_t *d = (volatile uiox_uint32_t *)xfer->dst;
         for (uiox_uint32_t i = 0u; i < xfer->len / 4u; i++) {
             *d = *s;
             if (xfer->src_inc) s++;
             if (xfer->dst_inc) d++;
         }
     } else if (xfer->width == UIOX_SOC_DMA_WIDTH_16) {
         volatile uiox_uint16_t *s = (volatile uiox_uint16_t *)xfer->src;
         volatile uiox_uint16_t *d = (volatile uiox_uint16_t *)xfer->dst;
         for (uiox_uint32_t i = 0u; i < xfer->len / 2u; i++) {
             *d = *s;
             if (xfer->src_inc) s++;
             if (xfer->dst_inc) d++;
         }
     } else {
         volatile uiox_uint8_t *s = (volatile uiox_uint8_t *)xfer->src;
         volatile uiox_uint8_t *d = (volatile uiox_uint8_t *)xfer->dst;
         for (uiox_uint32_t i = 0u; i < xfer->len; i++) {
             *d = *s;
             if (xfer->src_inc) s++;
             if (xfer->dst_inc) d++;
         }
     }
 
     ctrl->chan_busy[chan] = false;
     if (xfer->complete_cb)
         xfer->complete_cb(chan, 0, xfer->cb_priv);
     return UIOX_SOC_OK;
 }
 
 static uiox_bool_t sw_dma_busy(uiox_soc_dma_ctrl_t *ctrl, uiox_uint32_t chan)
 { return chan < UIOX_SOC_DMA_MAX_CHANNELS ? ctrl->chan_busy[chan] : false; }
 
 static uiox_soc_err_t sw_dma_wait(uiox_soc_dma_ctrl_t *ctrl,
                                     uiox_uint32_t chan)
 {
     /* Software DMA completes synchronously — already done */
     (void)ctrl; (void)chan;
     return UIOX_SOC_OK;
 }
 
 static void sw_dma_abort(uiox_soc_dma_ctrl_t *ctrl, uiox_uint32_t chan)
 { if (chan < UIOX_SOC_DMA_MAX_CHANNELS) ctrl->chan_busy[chan] = false; }
 
 static int sw_dma_alloc_chan(uiox_soc_dma_ctrl_t *ctrl)
 {
     for (uiox_uint32_t i = 0u; i < UIOX_SOC_DMA_MAX_CHANNELS; i++) {
         if (!ctrl->chan_busy[i]) return (int)i;
     }
     return -1;
 }
 
 static void sw_dma_free_chan(uiox_soc_dma_ctrl_t *ctrl, uiox_uint32_t chan)
 { if (chan < UIOX_SOC_DMA_MAX_CHANNELS) ctrl->chan_busy[chan] = false; }
 
 static void sw_dma_isr(uiox_soc_dma_ctrl_t *ctrl) { (void)ctrl; }
 
 static const uiox_soc_dma_ops_t sw_dma_ops = {
     .init       = sw_dma_init,
     .deinit     = sw_dma_deinit,
     .transfer   = sw_dma_transfer,
     .busy       = sw_dma_busy,
     .wait       = sw_dma_wait,
     .abort      = sw_dma_abort,
     .alloc_chan = sw_dma_alloc_chan,
     .free_chan  = sw_dma_free_chan,
     .isr        = sw_dma_isr,
 };
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_soc_err_t uiox_soc_dma_init(uiox_soc_dma_ctrl_t *ctrl,
                                    const uiox_soc_dma_ops_t *ops)
 {
     if (!ctrl) return UIOX_SOC_ERR_INVAL;
     /* If no real ops provided, use SW fallback */
     const uiox_soc_dma_ops_t *use = ops ? ops : &sw_dma_ops;
     ctrl->priv = (void *)use;
     return use->init(ctrl);
 }
 
 void uiox_soc_dma_deinit(uiox_soc_dma_ctrl_t *ctrl)
 {
     if (ctrl && ctrl->priv && OPS(ctrl)->deinit)
         OPS(ctrl)->deinit(ctrl);
 }
 
 uiox_soc_err_t uiox_soc_dma_transfer(uiox_soc_dma_ctrl_t *ctrl,
                                        uiox_uint32_t chan,
                                        const uiox_soc_dma_xfer_t *xfer)
 {
     if (!ctrl || !ctrl->priv || !xfer) return UIOX_SOC_ERR_INVAL;
     return OPS(ctrl)->transfer(ctrl, chan, xfer);
 }
 
 uiox_bool_t uiox_soc_dma_busy(uiox_soc_dma_ctrl_t *ctrl, uiox_uint32_t chan)
 {
     return ctrl && ctrl->priv ? OPS(ctrl)->busy(ctrl, chan) : false;
 }
 
 uiox_soc_err_t uiox_soc_dma_wait(uiox_soc_dma_ctrl_t *ctrl, uiox_uint32_t chan)
 {
     if (!ctrl || !ctrl->priv) return UIOX_SOC_ERR_INVAL;
     return OPS(ctrl)->wait(ctrl, chan);
 }
 
 void uiox_soc_dma_abort(uiox_soc_dma_ctrl_t *ctrl, uiox_uint32_t chan)
 {
     if (ctrl && ctrl->priv) OPS(ctrl)->abort(ctrl, chan);
 }
 
 int uiox_soc_dma_alloc_chan(uiox_soc_dma_ctrl_t *ctrl)
 {
     return ctrl && ctrl->priv ? OPS(ctrl)->alloc_chan(ctrl) : -1;
 }
 
 void uiox_soc_dma_free_chan(uiox_soc_dma_ctrl_t *ctrl, uiox_uint32_t chan)
 {
     if (ctrl && ctrl->priv) OPS(ctrl)->free_chan(ctrl, chan);
 }
 
 uiox_soc_err_t uiox_soc_dma_memcpy(uiox_soc_dma_ctrl_t *ctrl,
                                      uiox_uintptr_t dst,
                                      uiox_uintptr_t src,
                                      uiox_uint32_t  len)
 {
     int chan = uiox_soc_dma_alloc_chan(ctrl);
     if (chan < 0) return UIOX_SOC_ERR_BUSY;
 
     uiox_soc_dma_xfer_t x = {
         .src         = src,
         .dst         = dst,
         .len         = len,
         .dir         = UIOX_SOC_DMA_DIR_MEM_TO_MEM,
         .width       = UIOX_SOC_DMA_WIDTH_32,
         .src_inc     = true,
         .dst_inc     = true,
         .complete_cb = NULL,
     };
 
     uiox_soc_err_t rc = uiox_soc_dma_transfer(ctrl, (uiox_uint32_t)chan, &x);
     if (rc == UIOX_SOC_OK)
         uiox_soc_dma_wait(ctrl, (uiox_uint32_t)chan);
     uiox_soc_dma_free_chan(ctrl, (uiox_uint32_t)chan);
     return rc;
 }
 