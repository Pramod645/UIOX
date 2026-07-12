/**
 * @file  uiox_fw_wdt.h
 * @brief UIOX Firmware HAL — Watchdog timer (SP805 / x86 iTCO).
 *
 * The watchdog must be kicked periodically during firmware init.
 * On timeout: hardware reset.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_WDT_H
 #define UIOX_FW_WDT_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * SP805 Watchdog register offsets (ARM AMBA)
  * ====================================================================== */
 
 #define SP805_WDT_LOAD          0x000u  /**< Load register               */
 #define SP805_WDT_VALUE         0x004u  /**< Current value (RO)          */
 #define SP805_WDT_CTRL          0x008u  /**< Control register            */
 #define SP805_WDT_INTCLR        0x00Cu  /**< Interrupt clear             */
 #define SP805_WDT_RIS           0x010u  /**< Raw interrupt status        */
 #define SP805_WDT_MIS           0x014u  /**< Masked interrupt status     */
 #define SP805_WDT_LOCK          0xC00u  /**< Lock register               */
 
 /* Control bits */
 #define SP805_CTRL_INTEN        (1u << 0)  /**< Interrupt enable         */
 #define SP805_CTRL_RESEN        (1u << 1)  /**< Reset enable             */
 
 /* Lock / unlock */
 #define SP805_UNLOCK_MAGIC      0x1ACCECAu
 #define SP805_LOCK_MAGIC        0x00000001u
 
 /* QEMU virt base */
 #define UIOX_WDT_ARM64_BASE     0x09030000u
 #define UIOX_WDT_ARM32_BASE     0x101E1000u  /**< versatilepb SP805      */
 #define UIOX_WDT_CLOCK_HZ       1000000u     /**< 1 MHz WDT clock        */
 
 /* =========================================================================
  * Watchdog context
  * ====================================================================== */
 
 typedef struct {
     uintptr_t base;
     uint32_t  clk_hz;
     uint32_t  timeout_ms;
     bool      initialized;
     void     *priv;
 } uiox_wdt_dev_t;
 
 /* HAL ops vtable */
 typedef struct {
     uiox_fw_err_t (*init)    (uiox_wdt_dev_t *dev, uint32_t timeout_ms);
     void          (*kick)    (uiox_wdt_dev_t *dev);
     void          (*stop)    (uiox_wdt_dev_t *dev);
     void          (*start)   (uiox_wdt_dev_t *dev);
     uint32_t      (*remaining)(uiox_wdt_dev_t *dev); /**< ms remaining  */
 } uiox_wdt_ops_t;
 
 /* API */
 uiox_fw_err_t uiox_fw_wdt_init    (uiox_wdt_dev_t *dev,
                                      const uiox_wdt_ops_t *ops,
                                      uint32_t timeout_ms);
 void          uiox_fw_wdt_kick    (uiox_wdt_dev_t *dev);
 void          uiox_fw_wdt_stop    (uiox_wdt_dev_t *dev);
 void          uiox_fw_wdt_start   (uiox_wdt_dev_t *dev);
 uint32_t      uiox_fw_wdt_remaining(uiox_wdt_dev_t *dev);
 uiox_fw_err_t uiox_fw_wdt_init_sp805(uiox_wdt_dev_t *dev,
                                        uintptr_t base, uint32_t clk_hz,
                                        uint32_t timeout_ms);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_WDT_H */
 