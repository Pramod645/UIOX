/**
 * @file  uiox_fw_gpio.h
 * @brief UIOX Firmware — GPIO controller abstraction.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_GPIO_H
 #define UIOX_FW_GPIO_H
 
 #include "uiox_fw_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FW_GPIO_MAX_PINS   128u
 
 typedef enum {
     UIOX_FW_GPIO_IN     = 0,
     UIOX_FW_GPIO_OUT    = 1,
     UIOX_FW_GPIO_ALT    = 2,   /**< Alternate function (UART/SPI/I2C)  */
 } uiox_fw_gpio_dir_t;
 
 typedef enum {
     UIOX_FW_GPIO_PULL_NONE = 0,
     UIOX_FW_GPIO_PULL_UP,
     UIOX_FW_GPIO_PULL_DOWN,
 } uiox_fw_gpio_pull_t;
 
 typedef enum {
     UIOX_FW_GPIO_IRQ_NONE     = 0,
     UIOX_FW_GPIO_IRQ_RISING   = 1,
     UIOX_FW_GPIO_IRQ_FALLING  = 2,
     UIOX_FW_GPIO_IRQ_BOTH     = 3,
     UIOX_FW_GPIO_IRQ_HIGH     = 4,
     UIOX_FW_GPIO_IRQ_LOW      = 5,
 } uiox_fw_gpio_irq_mode_t;
 
 typedef void (*uiox_fw_gpio_cb_t)(uint32_t pin, bool level, void *priv);
 
 typedef struct {
     uintptr_t           base;
     uint32_t            irq;
     uint32_t            num_pins;
     uiox_fw_gpio_cb_t   cb[UIOX_FW_GPIO_MAX_PINS];
     void               *cb_priv[UIOX_FW_GPIO_MAX_PINS];
 } uiox_fw_gpio_t;
 
 /* API */
 uiox_fw_err_t uiox_fw_gpio_init     (uiox_fw_gpio_t *g,
                                        uintptr_t base, uint32_t irq,
                                        uint32_t num_pins);
 uiox_fw_err_t uiox_fw_gpio_set_dir  (uiox_fw_gpio_t *g, uint32_t pin,
                                        uiox_fw_gpio_dir_t dir);
 uiox_fw_err_t uiox_fw_gpio_set_pull (uiox_fw_gpio_t *g, uint32_t pin,
                                        uiox_fw_gpio_pull_t pull);
 void          uiox_fw_gpio_write    (uiox_fw_gpio_t *g, uint32_t pin,
                                        bool val);
 bool          uiox_fw_gpio_read     (const uiox_fw_gpio_t *g, uint32_t pin);
 uiox_fw_err_t uiox_fw_gpio_irq_en   (uiox_fw_gpio_t *g, uint32_t pin,
                                        uiox_fw_gpio_irq_mode_t mode,
                                        uiox_fw_gpio_cb_t cb, void *priv);
 void          uiox_fw_gpio_irq      (uiox_fw_gpio_t *g);  /* ISR entry */
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_GPIO_H */
 