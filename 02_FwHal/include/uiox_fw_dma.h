/**
 * @file  uiox_fw_dma.h
 * @brief UIOX Firmware HAL -- DMA controller (PL330 / software fallback).
 *
 * Generated from uiox_fw_dma.c usage analysis.
 * SELF-CONTAINED: no SoC or system headers required.
 * Drop into 02_FwHal/include/.
 *
 * @version 1.0.0
 * @date    2026-07-17
 */
#ifndef UIOX_FW_DMA_H
#define UIOX_FW_DMA_H

#include "uiox_fw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Constants
 * ====================================================================== */
#define UIOX_DMA_MAX_CHANNELS   8u

/* =========================================================================
 * Transfer direction
 * ====================================================================== */
typedef enum {
    UIOX_DMA_DIR_MEM_TO_MEM = 0,
    UIOX_DMA_DIR_MEM_TO_DEV = 1,
    UIOX_DMA_DIR_DEV_TO_MEM = 2,
    UIOX_DMA_DIR_DEV_TO_DEV = 3,
} uiox_dma_dir_t;

/* =========================================================================
 * Transfer data width
 * ====================================================================== */
typedef enum {
    UIOX_DMA_WIDTH_8  = 0,   /**< Byte transfers          */
    UIOX_DMA_WIDTH_16 = 1,   /**< Half-word transfers      */
    UIOX_DMA_WIDTH_32 = 2,   /**< Word transfers (default) */
    UIOX_DMA_WIDTH_64 = 3,   /**< Double-word transfers    */
} uiox_dma_width_t;

/* =========================================================================
 * Transfer completion callback
 * ====================================================================== */
typedef void (*uiox_dma_cb_t)(uint32_t chan, int status, void *priv);

/* =========================================================================
 * Transfer descriptor
 * ====================================================================== */
typedef struct {
    uintptr_t      src;          /**< Source address                  */
    uintptr_t      dst;          /**< Destination address             */
    uint32_t       len;          /**< Transfer length in bytes        */
    uiox_dma_dir_t dir;          /**< Transfer direction              */
    uiox_dma_width_t width;      /**< Bus width per beat              */
    bool           src_inc;      /**< Auto-increment source address   */
    bool           dst_inc;      /**< Auto-increment destination addr */
    uiox_dma_cb_t  complete_cb;  /**< Called on completion (or NULL)  */
    void          *cb_priv;      /**< Passed to complete_cb           */
} uiox_dma_xfer_t;

/* =========================================================================
 * DMA controller descriptor (forward declaration for ops vtable)
 * ====================================================================== */
typedef struct uiox_dma_ctrl uiox_dma_ctrl_t;

/* =========================================================================
 * Ops vtable
 * ====================================================================== */
typedef struct {
    uiox_fw_err_t (*init)      (uiox_dma_ctrl_t *ctrl);
    void          (*deinit)    (uiox_dma_ctrl_t *ctrl);
    uiox_fw_err_t (*transfer)  (uiox_dma_ctrl_t *ctrl, uint32_t chan,
                                 const uiox_dma_xfer_t *xfer);
    bool          (*busy)      (uiox_dma_ctrl_t *ctrl, uint32_t chan);
    uiox_fw_err_t (*wait)      (uiox_dma_ctrl_t *ctrl, uint32_t chan);
    void          (*abort)     (uiox_dma_ctrl_t *ctrl, uint32_t chan);
    int           (*alloc_chan)(uiox_dma_ctrl_t *ctrl);
    void          (*free_chan) (uiox_dma_ctrl_t *ctrl, uint32_t chan);
    void          (*isr)       (uiox_dma_ctrl_t *ctrl);
} uiox_dma_ops_t;

/* =========================================================================
 * Controller descriptor
 * ====================================================================== */
struct uiox_dma_ctrl {
    uint32_t  num_channels;
    bool      initialized;
    bool      chan_busy[UIOX_DMA_MAX_CHANNELS];
    void     *priv;   /**< Points to ops vtable (set by uiox_fw_dma_init) */
};

/* =========================================================================
 * Public API
 * ====================================================================== */
uiox_fw_err_t uiox_fw_dma_init      (uiox_dma_ctrl_t *ctrl,
                                       const uiox_dma_ops_t *ops);
void          uiox_fw_dma_deinit    (uiox_dma_ctrl_t *ctrl);
uiox_fw_err_t uiox_fw_dma_transfer  (uiox_dma_ctrl_t *ctrl, uint32_t chan,
                                       const uiox_dma_xfer_t *xfer);
bool          uiox_fw_dma_busy      (uiox_dma_ctrl_t *ctrl, uint32_t chan);
uiox_fw_err_t uiox_fw_dma_wait      (uiox_dma_ctrl_t *ctrl, uint32_t chan);
void          uiox_fw_dma_abort     (uiox_dma_ctrl_t *ctrl, uint32_t chan);
int           uiox_fw_dma_alloc_chan (uiox_dma_ctrl_t *ctrl);
void          uiox_fw_dma_free_chan  (uiox_dma_ctrl_t *ctrl, uint32_t chan);
uiox_fw_err_t uiox_fw_dma_memcpy    (uiox_dma_ctrl_t *ctrl,
                                       uintptr_t dst, uintptr_t src,
                                       uint32_t len);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_DMA_H */
