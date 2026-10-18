/**
 * @file    uiox_pmic_if.c
 * @brief   UIOX PMIC interface driver implementation.
 * @date    2026-06-04
 */

 #include "uiox_pmic_if.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"
 
 int uiox_pmic_if_config(uiox_pmic_if_t *pif, uiox_pmic_hw_t *hw)
 {
     if (!pif || !hw) return -EINVAL;
     memset(pif, 0, sizeof(*pif));
     pif->hw     = hw;
     pif->primed = true;
     return 0;
 }
 
 int uiox_pmic_if_start(uiox_pmic_if_t *pif)
 {
     if (!pif || !pif->primed) return -EINVAL;
 
     /* Read device/variant IDs */
     uiox_pmic_hw_reg_read(pif->hw, UIOX_REG_DEVICE_ID,  &pif->device_id);
     uiox_pmic_hw_reg_read(pif->hw, UIOX_REG_VARIANT_ID, &pif->variant_id);
 
     /* Enable PMIC */
     int rc = uiox_pmic_hw_enable(pif->hw, true);
     if (rc < 0) return rc;
 
     /* Unmask critical IRQs (OTP, OCP, OVP, WDT) */
     uiox_pmic_hw_irq_clear(pif->hw, 0xFFFFFFFFu);
     const uiox_pmic_hw_ops_t *ops =
         (const uiox_pmic_hw_ops_t *)pif->hw->ops;
     if (ops && ops->irq_unmask)
         ops->irq_unmask(pif->hw,
                         UIOX_PMIC_FAULT_OTP | UIOX_PMIC_FAULT_OCP |
                         UIOX_PMIC_FAULT_OVP | UIOX_PMIC_FAULT_WDT);
 
     /* Enable watchdog */
     if (ops && ops->wdt_enable)
         ops->wdt_enable(pif->hw, true, 10000u); /* 10 s timeout */
 
     return 0;
 }
 
 void uiox_pmic_if_stop(uiox_pmic_if_t *pif)
 {
     if (!pif) return;
     uiox_pmic_hw_enable(pif->hw, false);
 }
 
 int uiox_pmic_if_field_wr(uiox_pmic_if_t *pif,
                            uint16_t reg, uint8_t mask,
                            uint8_t shift, uint8_t val)
 {
     if (!pif) return -EINVAL;
     uint8_t field = (uint8_t)(val << shift) & mask;
     int rc = uiox_pmic_hw_reg_update(pif->hw, reg, mask, field);
     if (rc == 0) pif->stats.reg_writes++;
     return rc;
 }
 
 int uiox_pmic_if_irq_handle(uiox_pmic_if_t *pif, uint32_t now_ms)
 {
     if (!pif) return -EINVAL;
     pif->stats.irq_count++;
 
     uint32_t flags = 0;
     int rc = uiox_pmic_hw_irq_status(pif->hw, &flags);
     if (rc < 0 || !flags) return rc;
 
     uiox_pmic_hw_irq_clear(pif->hw, flags);
 
     /* Push events for each active fault */
     static const struct { uint32_t bit; uiox_pmic_ev_t ev; }
     fault_map[] = {
         { UIOX_PMIC_FAULT_OTP,   UIOX_PMIC_EV_OTP   },
         { UIOX_PMIC_FAULT_OCP,   UIOX_PMIC_EV_OCP   },
         { UIOX_PMIC_FAULT_OVP,   UIOX_PMIC_EV_OVP   },
         { UIOX_PMIC_FAULT_UVP,   UIOX_PMIC_EV_UVP   },
         { UIOX_PMIC_FAULT_WDT,   UIOX_PMIC_EV_WDT   },
         { UIOX_PMIC_FAULT_PGOOD, UIOX_PMIC_EV_PGOOD_LOST },
     };
 
     for (size_t i = 0; i < sizeof(fault_map)/sizeof(fault_map[0]); i++) {
         if (flags & fault_map[i].bit) {
             uiox_pmic_event_t ev = {
                 .type        = fault_map[i].ev,
                 .rail_id     = 0xFFu,
                 .ts_ms       = now_ms,
                 .fault_flags = flags,
                 .valid       = true,
             };
             uiox_pmic_event_push(&ev);
             pif->stats.fault_count++;
             pif->hw->fault = true;
         }
     }
     return (int)flags;
 }
 
 int uiox_pmic_if_telemetry(uiox_pmic_if_t *pif,
                             uiox_pmic_telem_t *out, uint32_t now_ms)
 {
     if (!pif || !out) return -EINVAL;
     out->ts_ms = now_ms;
 
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_VSYS,     &out->vsys_mv);
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_VBAT,     &out->vbat_mv);
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_IBAT,     &out->ibat_ma);
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_VBUS,     &out->vbus_mv);
 
     uint32_t temp_raw = 0;
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_TEMP_DIE, &temp_raw);
     out->die_temp_c = (int8_t)(temp_raw & 0xFFu);
     pif->hw->die_temp_c = out->die_temp_c;
 
     uint32_t ntc_raw = 0;
     uiox_pmic_hw_adc_read(pif->hw, UIOX_PMIC_ADC_TEMP_NTC, &ntc_raw);
     out->ntc_temp_c = (int8_t)(ntc_raw & 0xFFu);
     return 0;
 }
 
 void uiox_pmic_if_stats_get(const uiox_pmic_if_t *pif,
                              uiox_pmic_if_stats_t *out)
 { if (!pif || !out) return; memcpy(out, &pif->stats, sizeof(*out)); }
 
 void uiox_pmic_if_stats_reset(uiox_pmic_if_t *pif)
 { if (!pif) return; memset(&pif->stats, 0, sizeof(pif->stats)); }
 /* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int pmic_class_init(uiox_dev_dev_t *dev)
{
    uiox_pmic_if_t *pif = (uiox_pmic_if_t *)dev->drv_priv;
    return pif ? uiox_pmic_if_start(pif) : -EINVAL;
}

static void pmic_class_deinit(uiox_dev_dev_t *dev)
{
    uiox_pmic_if_t *pif = (uiox_pmic_if_t *)dev->drv_priv;
    if (pif) uiox_pmic_if_stop(pif);
}

/* This HAL is word-wide (uiox_pmic_hw_reg_read takes uint16_t reg and
 * a uint16_t *val).  §8's bus path is 8-bit, so n must be 2 here — a
 * 1-byte request has no meaning on this device. */
/* This HAL writes bytes: uiox_pmic_hw_reg_read takes a uint8_t *val
 * (uiox_pmic_hw.h:179).  §8's bus path is byte-oriented, so n must be 1. */
/* This HAL is byte-wide on the value: uiox_pmic_hw_reg_read takes a
 * uint8_t *val (uiox_pmic_hw.h:179).  The REGISTER is wider (uint16_t),
 * which §8's 8-bit bus path absorbs by cast; anything needing the full
 * register goes through UIOX_PMIC_IOC_FIELD_WR. */
static int pmic_class_read_reg(uiox_dev_dev_t *dev, uint8_t reg,
    void *val, uint16_t n)
{
uiox_pmic_if_t *pif = (uiox_pmic_if_t *)dev->drv_priv;
if (!pif || !val || n != 1u) return -EINVAL;
uint8_t v = 0u;
int rc = uiox_pmic_hw_reg_read(pif->hw, (uint16_t)reg, &v);
if (rc < 0) return rc;
*(uint8_t *)val = v;
return 0;
}

static int pmic_class_write_reg(uiox_dev_dev_t *dev, uint8_t reg,
     const void *val, uint16_t n)
{
uiox_pmic_if_t *pif = (uiox_pmic_if_t *)dev->drv_priv;
if (!pif || !val || n != 1u) return -EINVAL;
return uiox_pmic_hw_reg_write(pif->hw, (uint16_t)reg,
       *(const uint8_t *)val);
}


static int pmic_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_pmic_if_t *pif = (uiox_pmic_if_t *)dev->drv_priv;
    if (!pif || !arg) return -EINVAL;

    switch (cmd) {
    case UIOX_PMIC_IOC_GET_TELEM:
        return uiox_pmic_if_telemetry(pif, (uiox_pmic_telem_t *)arg, 0u);
    case UIOX_PMIC_IOC_GET_VSYS: {
        uiox_pmic_telem_t t;
        int rc = uiox_pmic_if_telemetry(pif, &t, 0u);
        if (rc < 0) return rc;
        *(uint32_t *)arg = t.vsys_mv;
        return 0;
    }
    case UIOX_PMIC_IOC_FIELD_WR: {
        /* The wide-register path: _if_field_wr already takes the full
         * uint16_t, which is why §8's 8-bit bus path stays narrow. */
        const uiox_pmic_field_t *f = (const uiox_pmic_field_t *)arg;
        return uiox_pmic_if_field_wr(pif, f->reg, f->mask,
                                     f->shift, f->val);
    }
    case UIOX_PMIC_IOC_GET_STATS:
        uiox_pmic_if_stats_get(pif, (uiox_pmic_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;
    }
}

const uiox_pmic_ops_t uiox_pmic_class_ops = {
    .head = { .name = "pmic0", .probe = NULL,
              .init = pmic_class_init, .deinit = pmic_class_deinit,
              .ioctl = pmic_class_ioctl },
    .bus  = { .init = pmic_class_init, .deinit = pmic_class_deinit,
              .read_reg = pmic_class_read_reg,
              .write_reg = pmic_class_write_reg },
};

/* STATIC — §9's uiox_dev_bind() stores the pointer it is given, not a
 * copy, so these must outlive the call. */
static uiox_pmic_hw_t       s_pmic_hw;
static uiox_pmic_if_t       s_pmic_if;
static uiox_dev_dev_t       s_pmic_dev;
static struct uiox_i2c_dev  s_pmic_i2c;

int uiox_pmic_probe(void)
{
    if (uiox_pmic_fwbind(&s_pmic_hw, &s_pmic_i2c) < 0)      return -1;
    if (uiox_pmic_if_config(&s_pmic_if, &s_pmic_hw) < 0)    return -1;

    s_pmic_dev.cls  = UIOX_DEVCLASS_PMIC;
    s_pmic_dev.unit = 0u;
    s_pmic_dev.bus  = UIOX_BUS_I2C;
    s_pmic_dev.base = 0u;                   /* bus-attached: no MMIO  */
    s_pmic_dev.addr = s_pmic_hw.i2c_addr;   /* uiox_pmic_hw.h:100     */
    s_pmic_dev.irq  = s_pmic_hw.irq;        /* uiox_pmic_hw.h:101     */
    s_pmic_dev.name = "pmic0";
    s_pmic_dev.ops  = &uiox_pmic_class_ops;
    s_pmic_dev.drv_priv = &s_pmic_if;

    return uiox_dev_bind(&s_pmic_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */
