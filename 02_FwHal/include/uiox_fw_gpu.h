/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_gpu.h — GPU / VirtIO-GPU framebuffer HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_GPU_H
#define UIOX_FW_GPU_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_GPU_VIRTIO=0, UIOX_GPU_MALI=1, UIOX_GPU_SIMPLE_FB=2 } uiox_gpu_type_t;

#define VIRTIO_GPU_CMD_RESOURCE_CREATE2D  0x0101u
#define VIRTIO_GPU_CMD_SET_SCANOUT        0x0103u
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH     0x0104u
#define VIRTIO_GPU_CMD_TRANSFER_TO_HOST2D 0x0105u
#define VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING 0x0106u
#define VIRTIO_GPU_CMD_GET_DISPLAY_INFO   0x0100u

typedef struct { uint32_t x,y,w,h; } uiox_rect_t;

typedef struct {
    uintptr_t     base;        /**< BAR0 / VirtIO MMIO               */
    uiox_gpu_type_t type;
    uint32_t      width, height;
    uint32_t      bpp;         /**< Bits per pixel (32)              */
    uintptr_t     fb_phys;     /**< Framebuffer physical address      */
    uint32_t      fb_size;
    bool          initialized;
    void         *priv;
} uiox_gpu_dev_t;

typedef struct {
    uiox_fw_err_t (*init)    (uiox_gpu_dev_t *dev, uint32_t w, uint32_t h);
    void          (*deinit)  (uiox_gpu_dev_t *dev);
    uiox_fw_err_t (*flush)   (uiox_gpu_dev_t *dev, const uiox_rect_t *r);
    uiox_fw_err_t (*set_mode)(uiox_gpu_dev_t *dev, uint32_t w, uint32_t h);
    void         *(*get_fb)  (uiox_gpu_dev_t *dev);
} uiox_gpu_ops_t;

uiox_fw_err_t uiox_fw_gpu_init       (uiox_gpu_dev_t *dev, const uiox_gpu_ops_t *ops,
                                        uint32_t w, uint32_t h);
void          uiox_fw_gpu_deinit     (uiox_gpu_dev_t *dev);
uiox_fw_err_t uiox_fw_gpu_flush      (uiox_gpu_dev_t *dev, const uiox_rect_t *r);
void         *uiox_fw_gpu_get_fb     (uiox_gpu_dev_t *dev);
uiox_fw_err_t uiox_fw_gpu_init_virtio(uiox_gpu_dev_t *dev, uintptr_t base);
uiox_fw_err_t uiox_fw_gpu_init_simplefb(uiox_gpu_dev_t *dev, uintptr_t fb_phys,
                                         uint32_t w, uint32_t h, uint32_t stride);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_GPU_H */
