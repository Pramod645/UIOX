/**
 * @file    uiox_bms_if.h
 * @brief   UIOX BMS interface driver (AFE register map, IRQ).
 * @date    2026-06-04
 */

 #ifndef UIOX_BMS_IF_H
 #define UIOX_BMS_IF_H
 
 #include "uiox_bms_hw.h"
 #include "uiox_bms_buf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * BQ76940-compatible register map (reference)
  * ====================================================================== */
 
 #define UIOX_REG_BMS_SYS_STAT       0x00u
 #define UIOX_REG_BMS_CELLBAL1       0x01u
 #define UIOX_REG_BMS_CELLBAL2       0x02u
 #define UIOX_REG_BMS_CELLBAL3       0x03u
 #define UIOX_REG_BMS_SYS_CTRL1      0x04u
 #define UIOX_REG_BMS_SYS_CTRL2      0x05u
 #define UIOX_REG_BMS_PROTECT1       0x06u
 #define UIOX_REG_BMS_PROTECT2       0x07u
 #define UIOX_REG_BMS_PROTECT3       0x08u
 #define UIOX_REG_BMS_OV_TRIP        0x09u
 #define UIOX_REG_BMS_UV_TRIP        0x0Au
 #define UIOX_REG_BMS_CC_CFG         0x0Bu
 #define UIOX_REG_BMS_VC1_HI         0x0Cu
 #define UIOX_REG_BMS_VC1_LO         0x0Du
 #define UIOX_REG_BMS_BAT_HI         0x2Au
 #define UIOX_REG_BMS_BAT_LO         0x2Bu
 #define UIOX_REG_BMS_TS1_HI         0x2Cu
 #define UIOX_REG_BMS_TS1_LO         0x2Du
 #define UIOX_REG_BMS_CC_HI          0x32u
 #define UIOX_REG_BMS_CC_LO          0x33u
 #define UIOX_REG_BMS_ADCGAIN1       0x50u
 #define UIOX_REG_BMS_ADCOFFSET      0x51u
 #define UIOX_REG_BMS_ADCGAIN2       0x59u
 #define UIOX_REG_BMS_DEVICE_ID      0x60u
 
 /* SYS_STAT bits */
 #define UIOX_BMS_STAT_OCD           (1u << 0)
 #define UIOX_BMS_STAT_SCD           (1u << 1)
 #define UIOX_BMS_STAT_OV            (1u << 2)
 #define UIOX_BMS_STAT_UV            (1u << 3)
 #define UIOX_BMS_STAT_OVRD_ALERT    (1u << 4)
 #define UIOX_BMS_STAT_DEVICE_XREADY (1u << 5)
 #define UIOX_BMS_STAT_CC_READY      (1u << 7)
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  measurements;
     uint32_t  irq_count;
     uint32_t  fault_count;
     uint32_t  comm_errors;
     uint32_t  balance_ops;
 } uiox_bms_if_stats_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_bms_hw_t       *hw;
     uiox_bms_if_stats_t  stats;
     uint8_t              device_id;
     uint16_t             adc_gain;   /**< ADC gain (µV/LSB)              */
     int8_t               adc_offset; /**< ADC offset (mV)                */
     bool                 primed;
 } uiox_bms_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_bms_if_config   (uiox_bms_if_t *bif, uiox_bms_hw_t *hw);
 int  uiox_bms_if_start    (uiox_bms_if_t *bif);
 void uiox_bms_if_stop     (uiox_bms_if_t *bif);
 
 /** Full measurement cycle: cells + current + temperature. */
 int  uiox_bms_if_measure  (uiox_bms_if_t *bif);
 
 /** Handle alert IRQ — read faults, push events. */
 int  uiox_bms_if_irq_handle(uiox_bms_if_t *bif, uint32_t now_ms);
 
 /** Collect telemetry snapshot. */
 int  uiox_bms_if_telemetry(uiox_bms_if_t *bif,
                             uiox_bms_telem_t *out,
                             uint32_t now_ms,
                             uint8_t soc_pct, uint8_t soh_pct,
                             int32_t remain_mah, int32_t full_mah);
 
 void uiox_bms_if_stats_get  (const uiox_bms_if_t *bif,
                               uiox_bms_if_stats_t *out);
 void uiox_bms_if_stats_reset(uiox_bms_if_t *bif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BMS_IF_H */
 