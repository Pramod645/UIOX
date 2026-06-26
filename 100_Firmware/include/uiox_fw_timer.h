/**
 * @file  uiox_fw_timer.h
 * @brief UIOX Firmware — Timer driver (SP804 / PIT 8254 / ARM Generic Timer).
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_TIMER_H
 #define UIOX_FW_TIMER_H
 
 #include "uiox_fw_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* SP804 dual-timer register offsets (ARM) */
 #define SP804_TIMER1_LOAD    0x000u
 #define SP804_TIMER1_VALUE   0x004u
 #define SP804_TIMER1_CTRL    0x008u
 #define SP804_TIMER1_INTCLR  0x00Cu
 #define SP804_TIMER1_RIS     0x010u
 #define SP804_TIMER1_MIS     0x014u
 #define SP804_TIMER2_LOAD    0x020u
 #define SP804_CTRL_EN        UIOX_FW_BIT(7)
 #define SP804_CTRL_PERIODIC  UIOX_FW_BIT(6)
 #define SP804_CTRL_IE        UIOX_FW_BIT(5)
 #define SP804_CTRL_32BIT     UIOX_FW_BIT(1)
 #define SP804_CTRL_ONESHOT   UIOX_FW_BIT(0)
 #define SP804_CLOCK_HZ       1000000u  /**< 1 MHz SP804 reference clock  */
 
 /* PIT 8254 (x86) */
 #define PIT_CHANNEL0         0x40u
 #define PIT_CHANNEL2         0x42u
 #define PIT_CMD              0x43u
 #define PIT_FREQ_HZ          1193182u
 #define PIT_CMD_CH0_MODE3    0x36u     /**< ch0, lobyte/hibyte, sq-wave  */
 
 /* ARM Generic Timer MSRs */
 #define ARM_GT_CNTPCT_EL0    "cntpct_el0"
 #define ARM_GT_CNTFRQ_EL0    "cntfrq_el0"
 #define ARM_GT_CNTP_TVAL_EL0 "cntp_tval_el0"
 #define ARM_GT_CNTP_CTL_EL0  "cntp_ctl_el0"
 #define ARM_GT_CTL_EN        UIOX_FW_BIT(0)
 #define ARM_GT_CTL_IMASK     UIOX_FW_BIT(1)
 
 /* Timer callback */
 typedef void (*uiox_fw_timer_cb_t)(void *priv);
 
 typedef enum {
     UIOX_FW_TIMER_SP804   = 0,   /**< ARM SP804 dual-timer              */
     UIOX_FW_TIMER_PIT     = 1,   /**< x86 PIT 8254                      */
     UIOX_FW_TIMER_ARM_GT  = 2,   /**< ARM Generic Timer                 */
 } uiox_fw_timer_type_t;
 
 typedef struct {
     uiox_fw_timer_type_t type;
     uintptr_t            base;   /**< MMIO base (SP804) or I/O port (PIT) */
     uint32_t             irq;
     uint32_t             hz;     /**< Tick frequency                    */
     uiox_fw_timer_cb_t   cb;
     void                *cb_priv;
     /* Stats */
     uint64_t             tick_count;
     uint64_t             uptime_ms;
 } uiox_fw_timer_t;
 
 /* API */
 uiox_fw_err_t uiox_fw_timer_init   (uiox_fw_timer_t *tmr,
                                       uiox_fw_timer_type_t type,
                                       uintptr_t base,
                                       uint32_t irq, uint32_t hz);
 void          uiox_fw_timer_start  (uiox_fw_timer_t *tmr);
 void          uiox_fw_timer_stop   (uiox_fw_timer_t *tmr);
 uint64_t      uiox_fw_timer_ticks  (const uiox_fw_timer_t *tmr);
 uint64_t      uiox_fw_timer_uptime_ms(const uiox_fw_timer_t *tmr);
 void          uiox_fw_timer_set_cb (uiox_fw_timer_t *tmr,
                                       uiox_fw_timer_cb_t cb, void *priv);
 void          uiox_fw_timer_irq    (uiox_fw_timer_t *tmr); /* ISR entry */
 void          uiox_fw_udelay       (uint32_t us);
 void          uiox_fw_mdelay       (uint32_t ms);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_TIMER_H */
 