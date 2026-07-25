/**
 * @file    uiox_fan_if.h
 * @brief   UIOX Fan Controller interface driver (register map, IRQ).
 * @date    2026-06-05
 */

 #ifndef UIOX_FAN_IF_H
 #define UIOX_FAN_IF_H
 
 #include "uiox_fan_hw.h"
 #include "uiox_fan_buf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * EMC2301-compatible register map (reference)
  * ====================================================================== */
 
 #define UIOX_REG_FAN_CONFIG1        0x20u
 #define UIOX_REG_FAN_CONFIG2        0x21u
 #define UIOX_REG_GAIN               0x22u
 #define UIOX_REG_SPIN_UP_CFG        0x23u
 #define UIOX_REG_MAX_STEP           0x24u
 #define UIOX_REG_MIN_DRIVE          0x25u
 #define UIOX_REG_VALID_TACH         0x26u
 #define UIOX_REG_DRIVE_FAIL_BAND_LO 0x27u
 #define UIOX_REG_DRIVE_FAIL_BAND_HI 0x28u
 #define UIOX_REG_TACH_TARGET_LO     0x29u
 #define UIOX_REG_TACH_TARGET_HI     0x2Au
 #define UIOX_REG_TACH_READING_HI    0x2Bu
 #define UIOX_REG_TACH_READING_LO    0x2Cu
 #define UIOX_REG_FAN_DRIVER         0x30u
 #define UIOX_REG_FAN_STATUS         0x24u
 #define UIOX_REG_FAN_STALL_STATUS   0x25u
 #define UIOX_REG_DRIVE_FAIL_STATUS  0x26u
 #define UIOX_REG_INTERRUPT_ENABLE   0x29u
 #define UIOX_REG_PWM_OUT            0x30u
 #define UIOX_REG_PWM_FREQ           0x31u
 #define UIOX_REG_PWM_FREQ_DIVIDE    0x32u
 #define UIOX_REG_DEVICE_ID          0xFDu
 #define UIOX_REG_MFR_ID             0xFEu
 #define UIOX_REG_REVISION           0xFFu
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  measurements;
     uint32_t  irq_count;
     uint32_t  fault_count;
     uint32_t  stall_count;
     uint32_t  comm_errors;
     uint32_t  pwm_changes;
 } uiox_fan_if_stats_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_fan_hw_t       *hw;
     uiox_fan_if_stats_t  stats;
     uint8_t              device_id;
     uint8_t              mfr_id;
     bool                 primed;
 } uiox_fan_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_fan_if_config    (uiox_fan_if_t *fif, uiox_fan_hw_t *hw);
 int  uiox_fan_if_start     (uiox_fan_if_t *fif);
 void uiox_fan_if_stop      (uiox_fan_if_t *fif);
 
 /** Full measurement: read all RPM + all temps. */
 int  uiox_fan_if_measure   (uiox_fan_if_t *fif);
 
 /** Set PWM on a channel (0..255), log event if changed. */
 int  uiox_fan_if_set_pwm   (uiox_fan_if_t *fif, uint8_t ch,
                              uint8_t duty, uint32_t now_ms);
 
 /** Handle fault IRQ — read faults, push events. */
 int  uiox_fan_if_irq_handle(uiox_fan_if_t *fif, uint32_t now_ms);
 
 /** Collect telemetry snapshot. */
 int  uiox_fan_if_telemetry (uiox_fan_if_t *fif,
                              uiox_fan_telem_t *out, uint32_t now_ms);
 
 void uiox_fan_if_stats_get  (const uiox_fan_if_t *fif,
                               uiox_fan_if_stats_t *out);
 void uiox_fan_if_stats_reset(uiox_fan_if_t *fif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FAN_IF_H */
 