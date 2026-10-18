/**
 * @file  uiox_rtc_if.c
 * @brief UIOX RTC interface driver — register access, UIP guard, IRQ.
 * @date  2026-06-10
 */

 #include "uiox_rtc_if.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"
 
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
 /* ═══════════════════════════ INSERTED ═══════════════════════════ */
#define UIOX_RTC_EVT_POOL_MAX   8u

static uiox_rtc_evt_t  s_rtc_evt_pool[UIOX_RTC_EVT_POOL_MAX];
static bool            s_rtc_evt_used[UIOX_RTC_EVT_POOL_MAX];

uiox_rtc_evt_t *uiox_rtc_evt_alloc(void)
{
    for (uint32_t i = 0u; i < UIOX_RTC_EVT_POOL_MAX; i++) {
        if (!s_rtc_evt_used[i]) {
            s_rtc_evt_used[i] = true;
            memset(&s_rtc_evt_pool[i], 0, sizeof(s_rtc_evt_pool[i]));
            return &s_rtc_evt_pool[i];
        }
    }
    return (uiox_rtc_evt_t *)0;
}

void uiox_rtc_evt_free(uiox_rtc_evt_t *e)
{
    if (!e) return;
    if (e < s_rtc_evt_pool ||
        e >= s_rtc_evt_pool + UIOX_RTC_EVT_POOL_MAX)
        return;
    s_rtc_evt_used[e - s_rtc_evt_pool] = false;
}
/* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int rtc_class_init(uiox_dev_dev_t *dev)
{
    uiox_rtc_if_t *rif = (uiox_rtc_if_t *)dev->drv_priv;
    return rif ? uiox_rtc_if_start(rif) : -EINVAL;
}

static void rtc_class_deinit(uiox_dev_dev_t *dev)
{
    uiox_rtc_if_t *rif = (uiox_rtc_if_t *)dev->drv_priv;
    if (rif) uiox_rtc_if_stop(rif);
}

/* This HAL's accessors RETURN THE VALUE rather than a status:
 *     uint8_t uiox_rtc_hw_reg_read (hw, reg);
 *     void    uiox_rtc_hw_reg_write(hw, reg, val);
 * §8 wants int + a caller buffer, so the wrapper is synthesised here.
 * There is no error channel — a bus failure is not reportable through
 * this HAL, which is why the byte-only width is enforced. */
static int rtc_class_read_reg(uiox_dev_dev_t *dev, uint8_t reg,
                              void *val, uint16_t n)
{
    uiox_rtc_if_t *rif = (uiox_rtc_if_t *)dev->drv_priv;
    if (!rif || !val || n != 1u) return -EINVAL;
    *(uint8_t *)val = uiox_rtc_hw_reg_read(rif->hw, reg);
    return 0;
}

static int rtc_class_write_reg(uiox_dev_dev_t *dev, uint8_t reg,
                               const void *val, uint16_t n)
{
    uiox_rtc_if_t *rif = (uiox_rtc_if_t *)dev->drv_priv;
    if (!rif || !val || n != 1u) return -EINVAL;
    uiox_rtc_hw_reg_write(rif->hw, reg, *(const uint8_t *)val);
    return 0;
}

static int rtc_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_rtc_if_t *rif = (uiox_rtc_if_t *)dev->drv_priv;
    if (!rif || !arg) return -EINVAL;

    switch (cmd) {
    case UIOX_RTC_IOC_GET_TIME: {
        uiox_rtc_time_t *t = (uiox_rtc_time_t *)arg;
        return uiox_rtc_if_time_read(rif, &t->s, &t->m, &t->h,
                                     &t->md, &t->mo, &t->yr);
    }
    case UIOX_RTC_IOC_SET_TIME: {
        const uiox_rtc_time_t *t = (const uiox_rtc_time_t *)arg;
        return uiox_rtc_if_time_write(rif, t->s, t->m, t->h,
                                      t->md, t->mo, t->yr);
    }
    case UIOX_RTC_IOC_GET_ALARM: {
        uiox_rtc_alarm_t *a = (uiox_rtc_alarm_t *)arg;
        return uiox_rtc_if_alarm_read(rif, &a->s, &a->m, &a->h);
    }
    case UIOX_RTC_IOC_SET_ALARM: {
        const uiox_rtc_alarm_t *a = (const uiox_rtc_alarm_t *)arg;
        return uiox_rtc_if_alarm_write(rif, a->s, a->m, a->h, a->en);
    }
    case UIOX_RTC_IOC_GET_STATS:
        uiox_rtc_if_stats_get(rif, (uiox_rtc_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;
    }
}

const uiox_rtc_ops_t uiox_rtc_class_ops = {
    .head = { .name = "rtc0", .probe = NULL,
              .init = rtc_class_init, .deinit = rtc_class_deinit,
              .ioctl = rtc_class_ioctl },
    .bus  = { .init = rtc_class_init, .deinit = rtc_class_deinit,
              .read_reg = rtc_class_read_reg,
              .write_reg = rtc_class_write_reg },
};

static uiox_rtc_hw_t       s_rtc_hw;
static uiox_rtc_if_t       s_rtc_if;
static uiox_dev_dev_t      s_rtc_dev;
static struct uiox_i2c_dev s_rtc_i2c;

int uiox_rtc_probe(void)
{
    if (uiox_rtc_fwbind(&s_rtc_hw, &s_rtc_i2c) < 0)      return -1;
    if (uiox_rtc_if_config(&s_rtc_if, &s_rtc_hw) < 0)    return -1;

    s_rtc_dev.cls  = UIOX_DEVCLASS_RTC;
    s_rtc_dev.unit = 0u;
    s_rtc_dev.bus  = UIOX_BUS_I2C;
    s_rtc_dev.base = 0u;
    s_rtc_dev.addr = 0u;            /* TODO: no i2c_addr on this HAL */
    s_rtc_dev.irq  = s_rtc_hw.irq;
    s_rtc_dev.name = "rtc0";
    s_rtc_dev.ops  = &uiox_rtc_class_ops;
    s_rtc_dev.drv_priv = &s_rtc_if;

    return uiox_dev_bind(&s_rtc_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */
/* ═══════════════════════════ END INSERT ═════════════════════════ */