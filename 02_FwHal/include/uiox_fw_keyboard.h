/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_keyboard.h — PS/2 + USB HID keyboard HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_KEYBOARD_H
#define UIOX_FW_KEYBOARD_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

#define PS2_DATA_PORT   0x60u
#define PS2_CMD_PORT    0x64u
#define PS2_STATUS_IBF  (1u << 1)
#define PS2_STATUS_OBF  (1u << 0)
#define KBD_SET_LEDS    0xEDu
#define KBD_RESET       0xFFu
#define KBD_ECHO        0xEEu
#define KBD_LED_SCROLL  (1u << 0)
#define KBD_LED_NUM     (1u << 1)
#define KBD_LED_CAPS    (1u << 2)

typedef enum { UIOX_KBD_PS2=0, UIOX_KBD_USB_HID=1 } uiox_kbd_type_t;

typedef void (*uiox_kbd_key_cb_t)(uint8_t scancode, bool pressed, void *p);

typedef struct {
    uiox_kbd_type_t type;
    uint32_t        irq;
    uint8_t         leds;
    uiox_kbd_key_cb_t key_cb;
    void           *key_priv;
    bool            initialized;
    void           *priv;
} uiox_kbd_dev_t;

typedef struct {
    uiox_fw_err_t (*init)     (uiox_kbd_dev_t *dev);
    void          (*deinit)   (uiox_kbd_dev_t *dev);
    uiox_fw_err_t (*set_leds) (uiox_kbd_dev_t *dev, uint8_t leds);
    void          (*isr)      (uiox_kbd_dev_t *dev);
} uiox_kbd_ops_t;

uiox_fw_err_t uiox_fw_kbd_init      (uiox_kbd_dev_t *dev, const uiox_kbd_ops_t *ops);
void          uiox_fw_kbd_deinit    (uiox_kbd_dev_t *dev);
uiox_fw_err_t uiox_fw_kbd_set_leds  (uiox_kbd_dev_t *dev, uint8_t leds);
void          uiox_fw_kbd_set_key_cb(uiox_kbd_dev_t *dev,
                                       uiox_kbd_key_cb_t cb, void *p);
uiox_fw_err_t uiox_fw_kbd_init_ps2  (uiox_kbd_dev_t *dev, uint32_t irq);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_KEYBOARD_H */
