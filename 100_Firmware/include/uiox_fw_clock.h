/**
 * @file  uiox_fw_clock.h
 * @brief UIOX Firmware — Clock / PLL management.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_CLOCK_H
 #define UIOX_FW_CLOCK_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FW_CLOCK_MAX   16u
 
 typedef enum {
     UIOX_FW_CLK_CPU   = 0,
     UIOX_FW_CLK_BUS,
     UIOX_FW_CLK_UART0,
     UIOX_FW_CLK_UART1,
     UIOX_FW_CLK_TIMER0,
     UIOX_FW_CLK_GPIO,
     UIOX_FW_CLK_I2C,
     UIOX_FW_CLK_SPI,
     UIOX_FW_CLK_ETH,
     UIOX_FW_CLK_STORAGE,
     UIOX_FW_CLK_MAX,
 } uiox_fw_clk_id_t;
 
 typedef struct {
     uiox_fw_clk_id_t id;
     uint32_t         freq_hz;
     bool             enabled;
     char             name[16];
 } uiox_fw_clock_t;
 
 /* API */
 uiox_fw_err_t uiox_fw_clock_init      (void);
 uiox_fw_err_t uiox_fw_clock_enable    (uiox_fw_clk_id_t id);
 uiox_fw_err_t uiox_fw_clock_disable   (uiox_fw_clk_id_t id);
 uint32_t      uiox_fw_clock_get_hz    (uiox_fw_clk_id_t id);
 uiox_fw_err_t uiox_fw_clock_set_hz    (uiox_fw_clk_id_t id, uint32_t hz);
 void          uiox_fw_clock_print     (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_CLOCK_H */
 