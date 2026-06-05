/**
 * @file    uiox_pmic_if.h
 * @brief   UIOX PMIC interface driver (register map, IRQ dispatch).
 * @date    2026-06-04
 */

 #ifndef UIOX_PMIC_IF_H
 #define UIOX_PMIC_IF_H
 
 #include "uiox_pmic_hw.h"
 #include "uiox_pmic_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PMIC register map (DA9063-style as reference)
  * ====================================================================== */
 
 #define UIOX_REG_DEVICE_ID      0x000u
 #define UIOX_REG_VARIANT_ID     0x001u
 #define UIOX_REG_CUSTOMER_ID    0x002u
 #define UIOX_REG_CONFIG_ID      0x003u
 #define UIOX_REG_EVENT_A        0x010u  /**< Fault event register A       */
 #define UIOX_REG_EVENT_B        0x011u
 #define UIOX_REG_FAULT_LOG_A    0x012u  /**< Latched fault log            */
 #define UIOX_REG_FAULT_LOG_B    0x013u
 #define UIOX_REG_STATUS_A       0x01Du  /**< Real-time status             */
 #define UIOX_REG_STATUS_B       0x01Eu
 #define UIOX_REG_CONTROL_A      0x020u  /**< System control               */
 #define UIOX_REG_CONTROL_B      0x021u
 #define UIOX_REG_CONTROL_C      0x022u
 #define UIOX_REG_WDT_CTRL       0x041u  /**< Watchdog control             */
 #define UIOX_REG_BUCK1_SET      0x029u  /**< BUCK1 voltage set (first)    */
 #define UIOX_REG_LDO1_CONT      0x0A2u  /**< LDO1 control (first)        */
 #define UIOX_REG_ADC_MAN        0x0B0u  /**< Manual ADC trigger           */
 #define UIOX_REG_ADC_RES_L      0x0B1u  /**< ADC result low byte         */
 #define UIOX_REG_ADC_RES_H      0x0B2u  /**< ADC result high byte        */
 #define UIOX_REG_TEMP_A         0x0B4u  /**< Die temperature              */
 #define UIOX_REG_IRQ_MASK_A     0x016u  /**< IRQ mask register A         */
 #define UIOX_REG_IRQ_MASK_B     0x017u
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  reg_reads;
     uint64_t  reg_writes;
     uint32_t  irq_count;
     uint32_t  fault_count;
     uint32_t  wdt_kicks;
 } uiox_pmic_if_stats_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_pmic_hw_t      *hw;
     uiox_pmic_if_stats_t stats;
     uint8_t              device_id;
     uint8_t              variant_id;
     bool                 primed;
 } uiox_pmic_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_pmic_if_config  (uiox_pmic_if_t *pif, uiox_pmic_hw_t *hw);
 int  uiox_pmic_if_start   (uiox_pmic_if_t *pif);
 void uiox_pmic_if_stop    (uiox_pmic_if_t *pif);
 
 /** Read-modify-write a register field. */
 int  uiox_pmic_if_field_wr(uiox_pmic_if_t *pif,
                             uint16_t reg, uint8_t mask,
                             uint8_t shift, uint8_t val);
 
 /** Read and decode IRQ status; push events to log. */
 int  uiox_pmic_if_irq_handle(uiox_pmic_if_t *pif, uint32_t now_ms);
 
 /** Collect ADC telemetry snapshot. */
 int  uiox_pmic_if_telemetry(uiox_pmic_if_t *pif,
                              uiox_pmic_telem_t *out, uint32_t now_ms);
 
 void uiox_pmic_if_stats_get  (const uiox_pmic_if_t *pif,
                                uiox_pmic_if_stats_t *out);
 void uiox_pmic_if_stats_reset(uiox_pmic_if_t *pif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PMIC_IF_H */
 