/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_mouse.h — PS/2 + USB HID mouse HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_MOUSE_H
#define UIOX_FW_MOUSE_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_MOUSE_PS2=0, UIOX_MOUSE_USB_HID=1 } uiox_mouse_type_t;

typedef struct { int32_t dx, dy, dz; uint8_t buttons; } uiox_mouse_event_t;
typedef void (*uiox_mouse_cb_t)(const uiox_mouse_event_t *e, void *p);

typedef struct {
    uiox_mouse_type_t type;
    uint32_t          irq;
    int32_t           x, y;     /**< Absolute position (accumulated) */
    uiox_mouse_cb_t   ev_cb;
    void             *ev_priv;
    bool              initialized;
    void             *priv;
} uiox_mouse_dev_t;

typedef struct {
    uiox_fw_err_t (*init)   (uiox_mouse_dev_t *dev);
    void          (*deinit) (uiox_mouse_dev_t *dev);
    void          (*isr)    (uiox_mouse_dev_t *dev);
} uiox_mouse_ops_t;

uiox_fw_err_t uiox_fw_mouse_init       (uiox_mouse_dev_t *dev,
                                          const uiox_mouse_ops_t *ops);
void          uiox_fw_mouse_deinit     (uiox_mouse_dev_t *dev);
void          uiox_fw_mouse_set_cb     (uiox_mouse_dev_t *dev,
                                          uiox_mouse_cb_t cb, void *p);
uiox_fw_err_t uiox_fw_mouse_init_ps2   (uiox_mouse_dev_t *dev, uint32_t irq);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_MOUSE_H */
