/**
 * @file  uiox_fw_timer.c
 * @brief UIOX Firmware — SP804 / PIT / ARM Generic Timer driver.
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 /* Global tick timer used by uiox_fw_udelay / uiox_fw_mdelay */
 static uiox_fw_timer_t *s_tick_timer = NULL;
 
 /* ── SP804 (ARM) ──────────────────────────────────────────────────── */
 
 static void sp804_init(uiox_fw_timer_t *t)
 {
     uintptr_t b = t->base;
     /* Disable timer before configuring */
     fw_mmio_write32(b + SP804_TIMER1_CTRL, 0u);
     /* Load value: 1 MHz SP804 / desired Hz */
     uint32_t load = SP804_CLOCK_HZ / t->hz;
     fw_mmio_write32(b + SP804_TIMER1_LOAD, load);
     /* Clear any pending interrupt */
     fw_mmio_write32(b + SP804_TIMER1_INTCLR, 1u);
     /* Enable: 32-bit, periodic, interrupt enabled */
     fw_mmio_write32(b + SP804_TIMER1_CTRL,
                     SP804_CTRL_EN | SP804_CTRL_PERIODIC |
                     SP804_CTRL_IE | SP804_CTRL_32BIT);
 }
 
 static void sp804_stop(uiox_fw_timer_t *t)
 { fw_mmio_write32(t->base + SP804_TIMER1_CTRL, 0u); }
 
 static void sp804_ack(uiox_fw_timer_t *t)
 { fw_mmio_write32(t->base + SP804_TIMER1_INTCLR, 1u); }
 
 /* ── PIT 8254 (x86) ───────────────────────────────────────────────── */
 
 #if defined(__x86_64__) || defined(__i386__)
 #include <stdarg.h>
 static inline void _outb_t(uint16_t port, uint8_t v)
 { __asm__ volatile("outb %0,%1"::"a"(v),"dN"(port)); }
 
 static void pit_init(uiox_fw_timer_t *t)
 {
     uint32_t div = PIT_FREQ_HZ / t->hz;
     _outb_t(PIT_CMD, PIT_CMD_CH0_MODE3);
     _outb_t(PIT_CHANNEL0, (uint8_t)(div & 0xFFu));
     _outb_t(PIT_CHANNEL0, (uint8_t)(div >> 8u));
 }
 #endif
 
 /* ── ARM Generic Timer (AArch64) ──────────────────────────────────── */
 
 #if defined(__aarch64__)
 static void arm_gt_init(uiox_fw_timer_t *t)
 {
     uint64_t freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     uint64_t interval = freq / t->hz;
     __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(interval));
     __asm__ volatile("msr cntp_ctl_el0, %0"  :: "r"((uint64_t)ARM_GT_CTL_EN));
 }
 
 static void arm_gt_ack(uiox_fw_timer_t *t)
 {
     /* Reload counter */
     uint64_t freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     uint64_t interval = freq / t->hz;
     __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(interval));
 }
 
 static uint64_t arm_gt_read(void)
 {
     uint64_t v;
     __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(v) :: "memory");
     return v;
 }
 #endif
 
 /* ── Public API ─────────────────────────────────────────────────────── */
 
 uiox_fw_err_t uiox_fw_timer_init(uiox_fw_timer_t *t,
                                    uiox_fw_timer_type_t type,
                                    uintptr_t base, uint32_t irq,
                                    uint32_t hz)
 {
     if (!t || hz == 0u) return UIOX_FW_ERR_INVAL;
     t->type       = type;
     t->base       = base;
     t->irq        = irq;
     t->hz         = hz;
     t->cb         = NULL;
     t->cb_priv    = NULL;
     t->tick_count = 0u;
     t->uptime_ms  = 0u;
 
     switch (type) {
     case UIOX_FW_TIMER_SP804:
         sp804_init(t);
         break;
 #if defined(__x86_64__) || defined(__i386__)
     case UIOX_FW_TIMER_PIT:
         pit_init(t);
         break;
 #endif
 #if defined(__aarch64__)
     case UIOX_FW_TIMER_ARM_GT:
         arm_gt_init(t);
         break;
 #endif
     default:
         return UIOX_FW_ERR_UNSUP;
     }
 
     if (!s_tick_timer) s_tick_timer = t;
     return UIOX_FW_OK;
 }
 
 void uiox_fw_timer_start (uiox_fw_timer_t *t)
 {
     if (!t) return;
     if (t->type == UIOX_FW_TIMER_SP804) sp804_init(t);
 }
 
 void uiox_fw_timer_stop(uiox_fw_timer_t *t)
 {
     if (!t) return;
     if (t->type == UIOX_FW_TIMER_SP804) sp804_stop(t);
 }
 
 uint64_t uiox_fw_timer_ticks(const uiox_fw_timer_t *t)
 { return t ? t->tick_count : 0u; }
 
 uint64_t uiox_fw_timer_uptime_ms(const uiox_fw_timer_t *t)
 { return t ? t->uptime_ms : 0u; }
 
 void uiox_fw_timer_set_cb(uiox_fw_timer_t *t,
                            uiox_fw_timer_cb_t cb, void *priv)
 { if (t) { t->cb = cb; t->cb_priv = priv; } }
 
 void uiox_fw_timer_irq(uiox_fw_timer_t *t)
 {
     if (!t) return;
     t->tick_count++;
     t->uptime_ms += (1000u / t->hz);
     /* Acknowledge interrupt */
     if (t->type == UIOX_FW_TIMER_SP804) sp804_ack(t);
 #if defined(__aarch64__)
     else if (t->type == UIOX_FW_TIMER_ARM_GT) arm_gt_ack(t);
 #endif
     if (t->cb) t->cb(t->cb_priv);
 }
 
 void uiox_fw_udelay(uint32_t us)
 {
 #if defined(__aarch64__)
     uint64_t freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     uint64_t wait  = ((uint64_t)us * freq) / 1000000u;
     uint64_t start = arm_gt_read();
     while ((arm_gt_read() - start) < wait) ;
 #else
     /* Busy-wait fallback using tick counter */
     if (!s_tick_timer) return;
     uint64_t end_ms = s_tick_timer->uptime_ms + ((uint64_t)us / 1000u) + 1u;
     while (s_tick_timer->uptime_ms < end_ms) ;
 #endif
 }
 
 void uiox_fw_mdelay(uint32_t ms)
 { uiox_fw_udelay(ms * 1000u); }
 