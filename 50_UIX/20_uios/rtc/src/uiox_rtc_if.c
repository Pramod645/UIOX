/**
 * @file  uiox_rtc_if.c
 * @brief UIOX RTC interface driver — register access, UIP guard, IRQ.
 * @date  2026-06-10
 */

 #include "uiox_rtc_if.h"
 #include <string.h>
 #include <errno.h>
 
 /* Spin until UIP clears; returns 0 on success, -ETIME on timeout */
 static int wait_uip_clear(uiox_rtc_if_t *rif)
 {
     for (unsigned i = 0u; i < 1000000u; i++) {
         uint8_t reg_a = uiox_rtc_hw_reg_read(rif->hw, RTC_REG_A);
         if (!(reg_a & RTC_REG_A_UIP))
             return 0;
     }
     rif->stats.uip_timeouts++;
     return -ETIME;
 }
 
 int uiox_rtc_if_config(uiox_rtc_if_t *rif, uiox_rtc_hw_t *hw)
 {
     if (!rif || !hw) return -EINVAL;
     memset(rif, 0, sizeof(*rif));
     rif->hw     = hw;
     rif->primed = true;
     uiox_rtc_buf_init();
     return 0;
 }
 
 int uiox_rtc_if_start(uiox_rtc_if_t *rif)
 {
     if (!rif || !rif->primed) return -EINVAL;
 
     /* Ensure oscillator is running */
     uint8_t reg_a = uiox_rtc_hw_reg_read(rif->hw, RTC_REG_A);
     if ((reg_a & RTC_REG_A_DV_MASK) != RTC_REG_A_DV_ON) {
         reg_a = (reg_a & ~RTC_REG_A_DV_MASK) | RTC_REG_A_DV_ON;
         uiox_rtc_hw_reg_write(rif->hw, RTC_REG_A, reg_a);
     }
 
     /* Enable update-ended IRQ (1 Hz heartbeat) */
     uint8_t reg_b = uiox_rtc_hw_reg_read(rif->hw, RTC_REG_B);
     reg_b |= RTC_REG_B_UIE;
     uiox_rtc_hw_reg_write(rif->hw, RTC_REG_B, reg_b);
 
     return 0;
 }
 
 void uiox_rtc_if_stop(uiox_rtc_if_t *rif)
 {
     if (!rif) return;
     /* Disable all interrupts */
     uint8_t reg_b = uiox_rtc_hw_reg_read(rif->hw, RTC_REG_B);
     reg_b &= (uint8_t)~(RTC_REG_B_PIE | RTC_REG_B_AIE | RTC_REG_B_UIE);
     uiox_rtc_hw_reg_write(rif->hw, RTC_REG_B, reg_b);
 }
 
 int uiox_rtc_if_time_read(uiox_rtc_if_t *rif,
                            uint8_t *s, uint8_t *m, uint8_t *h,
                            uint8_t *md, uint8_t *mo, uint16_t *yr)
 {
     if (!rif) return -EINVAL;
     int rc = wait_uip_clear(rif);
     if (rc < 0) return rc;
     return uiox_rtc_hw_time_read(rif->hw, s, m, h, md, mo, yr);
 }
 
 int uiox_rtc_if_time_write(uiox_rtc_if_t *rif,
                             uint8_t s, uint8_t m, uint8_t h,
                             uint8_t md, uint8_t mo, uint16_t yr)
 {
     if (!rif) return -EINVAL;
     return uiox_rtc_hw_time_write(rif->hw, s, m, h, md, mo, yr);
 }
 
 int uiox_rtc_if_alarm_read(uiox_rtc_if_t *rif,
                             uint8_t *s, uint8_t *m, uint8_t *h)
 {
     if (!rif) return -EINVAL;
     return uiox_rtc_hw_alarm_read(rif->hw, s, m, h);
 }
 
 int uiox_rtc_if_alarm_write(uiox_rtc_if_t *rif,
                              uint8_t s, uint8_t m, uint8_t h, bool en)
 {
     if (!rif) return -EINVAL;
     int rc = uiox_rtc_hw_alarm_write(rif->hw, s, m, h);
     if (rc < 0) return rc;
     return uiox_rtc_hw_alarm_enable(rif->hw, en);
 }
 
 uiox_rtc_evt_t *uiox_rtc_if_irq_handle(uiox_rtc_if_t *rif, uint32_t now_ms)
 {
     if (!rif) return NULL;
 
     /*
      * Reading Register C clears all interrupt flags — must happen
      * unconditionally to de-assert the IRQ line.
      */
     uint8_t reg_c = uiox_rtc_hw_reg_read(rif->hw, RTC_REG_C);
     if (!(reg_c & RTC_REG_C_IRQF))
         return NULL;
 
     /* Store pending bits for upper layers */
     rif->hw->pending_irq |= (uint32_t)(reg_c & 0x70u);
 
     uiox_rtc_evt_t *e = uiox_rtc_evt_alloc();
     if (!e) { rif->stats.errors++; return NULL; }
 
     e->flags        = reg_c;
     e->timestamp_ms = now_ms;
 
     if (reg_c & RTC_REG_C_AF) {
         e->type = UIOX_RTC_EVT_ALARM;
         rif->stats.irq_alarm++;
     } else if (reg_c & RTC_REG_C_PF) {
         e->type = UIOX_RTC_EVT_PERIODIC;
         rif->stats.irq_periodic++;
     } else {
         e->type = UIOX_RTC_EVT_UPDATE;
         rif->stats.irq_update++;
     }
     return e;
 }
 
 void uiox_rtc_if_stats_get(const uiox_rtc_if_t *rif,
                              uiox_rtc_if_stats_t *out)
 { if (!rif || !out) return; memcpy(out, &rif->stats, sizeof(*out)); }
 