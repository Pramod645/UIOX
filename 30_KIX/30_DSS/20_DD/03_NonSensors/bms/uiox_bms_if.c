/**
 * @file    uiox_bms_if.c
 * @brief   UIOX BMS interface driver implementation.
 * @date    2026-06-04
 */
// Layer 2
 #include "uiox_bms_if.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"
 
 int uiox_bms_if_config(uiox_bms_if_t *bif, uiox_bms_hw_t *hw)
 {
     if (!bif || !hw) return -EINVAL;
     memset(bif, 0, sizeof(*bif));
     bif->hw      = hw;
     bif->adc_gain= 365u;   /* default 365 µV/LSB for BQ76940 */
     bif->primed  = true;
     return 0;
 }
 
 int uiox_bms_if_start(uiox_bms_if_t *bif)
 {
     if (!bif || !bif->primed) return -EINVAL;
 
     /* Read device ID and calibration */
     const uiox_bms_hw_ops_t *ops =
         (const uiox_bms_hw_ops_t *)bif->hw->ops;
 
     if (ops && ops->reg_read)
         ops->reg_read(bif->hw, UIOX_REG_BMS_DEVICE_ID, &bif->device_id);
 
     /* Read ADC gain registers */
     uint8_t gain1 = 0, gain2 = 0, offset = 0;
     if (ops && ops->reg_read) {
         ops->reg_read(bif->hw, UIOX_REG_BMS_ADCGAIN1, &gain1);
         ops->reg_read(bif->hw, UIOX_REG_BMS_ADCGAIN2, &gain2);
         ops->reg_read(bif->hw, UIOX_REG_BMS_ADCOFFSET, &offset);
         /* BQ76940: GAIN = 365 + (((ADCGAIN1 & 0x0C) << 1) |
          *                        ((ADCGAIN2 & 0xE0) >> 5)) µV/LSB */
         bif->adc_gain = (uint16_t)(365u +
                         ((((uint16_t)gain1 & 0x0Cu) << 1u) |
                          (((uint16_t)gain2 & 0xE0u) >> 5u)));
         bif->adc_offset = (int8_t)offset;
     }
 
     /* Enable ADC and coulomb counter */
     if (ops && ops->reg_write) {
         ops->reg_write(bif->hw, UIOX_REG_BMS_SYS_CTRL1, 0x18u); /* ADC_EN */
         ops->reg_write(bif->hw, UIOX_REG_BMS_SYS_CTRL2, 0x41u); /* CC_EN + CHG + DSG */
     }
 
     /* Enable both FETs */
     uiox_bms_hw_set_chg_fet(bif->hw, true);
     uiox_bms_hw_set_dsg_fet(bif->hw, true);
     return 0;
 }
 
 void uiox_bms_if_stop(uiox_bms_if_t *bif)
 {
     if (!bif) return;
     uiox_bms_hw_set_chg_fet(bif->hw, false);
     uiox_bms_hw_set_dsg_fet(bif->hw, false);
 }
 
 int uiox_bms_if_measure(uiox_bms_if_t *bif)
 {
     if (!bif || !bif->primed) return -EINVAL;
     bif->stats.measurements++;
     int rc = uiox_bms_hw_measure_cells(bif->hw);
     if (rc < 0) { bif->stats.comm_errors++; return rc; }
     rc = uiox_bms_hw_measure_current(bif->hw);
     if (rc < 0) { bif->stats.comm_errors++; return rc; }
     rc = uiox_bms_hw_measure_temp(bif->hw);
     if (rc < 0) { bif->stats.comm_errors++; }
     return rc;
 }
 
 int uiox_bms_if_irq_handle(uiox_bms_if_t *bif, uint32_t now_ms)
 {
     if (!bif) return -EINVAL;
     bif->stats.irq_count++;
 
     uint32_t flags = 0;
     int rc = uiox_bms_hw_fault_status(bif->hw, &flags);
     if (rc < 0 || !flags) return rc;
 
     uiox_bms_hw_fault_clear(bif->hw, flags);
 
     static const struct { uint32_t bit; uiox_bms_ev_t ev; }
     fault_map[] = {
         { UIOX_BMS_FAULT_OVP,     UIOX_BMS_EV_OVP      },
         { UIOX_BMS_FAULT_UVP,     UIOX_BMS_EV_UVP      },
         { UIOX_BMS_FAULT_OCP_CHG, UIOX_BMS_EV_OCP_CHG  },
         { UIOX_BMS_FAULT_OCP_DSG, UIOX_BMS_EV_OCP_DSG  },
         { UIOX_BMS_FAULT_SCP,     UIOX_BMS_EV_SCP      },
         { UIOX_BMS_FAULT_OTP,     UIOX_BMS_EV_OTP      },
         { UIOX_BMS_FAULT_UTP,     UIOX_BMS_EV_UTP      },
     };
     for (size_t i = 0; i < sizeof(fault_map)/sizeof(fault_map[0]); i++) {
         if (flags & fault_map[i].bit) {
             uiox_bms_event_t ev = {
                 .type        = fault_map[i].ev,
                 .ts_ms       = now_ms,
                 .pack_mv     = bif->hw->pack_mv,
                 .current_ma  = bif->hw->current_ma,
                 .temp_dc     = bif->hw->temp_dc[0],
                 .fault_flags = flags,
                 .valid       = true,
             };
             uiox_bms_event_push(&ev);
             bif->stats.fault_count++;
         }
     }
     return (int)flags;
 }
 
 int uiox_bms_if_telemetry(uiox_bms_if_t *bif,
                            uiox_bms_telem_t *out,
                            uint32_t now_ms,
                            uint8_t soc_pct, uint8_t soh_pct,
                            int32_t remain_mah, int32_t full_mah)
 {
     if (!bif || !out) return -EINVAL;
     out->ts_ms      = now_ms;
     out->pack_mv    = bif->hw->pack_mv;
     out->current_ma = bif->hw->current_ma;
     out->soc_pct    = soc_pct;
     out->soh_pct    = soh_pct;
     out->remain_mah = remain_mah;
     out->full_mah   = full_mah;
     out->fault_flags= bif->hw->fault_flags;
     memcpy(out->cell_mv, bif->hw->cell_mv,
            bif->hw->num_cells * sizeof(uint32_t));
     memcpy(out->temp_dc, bif->hw->temp_dc,
            bif->hw->num_temps * sizeof(int16_t));
     return 0;
 }
 
 void uiox_bms_if_stats_get(const uiox_bms_if_t *bif,
                             uiox_bms_if_stats_t *out)
 { if (!bif || !out) return; memcpy(out, &bif->stats, sizeof(*out)); }
 
 void uiox_bms_if_stats_reset(uiox_bms_if_t *bif)
 { if (!bif) return; memset(&bif->stats, 0, sizeof(bif->stats)); }
 /* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int bms_class_init(uiox_dev_dev_t *dev)
{
    uiox_bms_if_t *bif = (uiox_bms_if_t *)dev->drv_priv;
    return bif ? uiox_bms_if_start(bif) : -EINVAL;
}

static void bms_class_deinit(uiox_dev_dev_t *dev)
{
    uiox_bms_if_t *bif = (uiox_bms_if_t *)dev->drv_priv;
    if (bif) uiox_bms_if_stop(bif);
}

/* No public reg_* wrappers for this class — the vtable is reached
 * directly, as uiox_therm_if.c and uiox_fan_if.c do. */
static int bms_class_read_reg(uiox_dev_dev_t *dev, uint8_t reg,
                              void *val, uint16_t n)
{
    uiox_bms_if_t *bif = (uiox_bms_if_t *)dev->drv_priv;
    if (!bif || !val || !bif->hw->ops) return -EINVAL;
    const uiox_bms_hw_ops_t *o = (const uiox_bms_hw_ops_t *)bif->hw->ops;
    switch (n) {
    case 1u: return o->reg_read   ? o->reg_read  (bif->hw, reg, (uint8_t  *)val) : -ENOSYS;
    case 2u: return o->reg_read16 ? o->reg_read16(bif->hw, reg, (uint16_t *)val) : -ENOSYS;
    default: return -EINVAL;
    }
}

static int bms_class_write_reg(uiox_dev_dev_t *dev, uint8_t reg,
                               const void *val, uint16_t n)
{
    uiox_bms_if_t *bif = (uiox_bms_if_t *)dev->drv_priv;
    if (!bif || !val || !bif->hw->ops) return -EINVAL;
    const uiox_bms_hw_ops_t *o = (const uiox_bms_hw_ops_t *)bif->hw->ops;
    switch (n) {
    case 1u: return o->reg_write   ? o->reg_write  (bif->hw, reg, *(const uint8_t  *)val) : -ENOSYS;
    case 2u: return o->reg_write16 ? o->reg_write16(bif->hw, reg, *(const uint16_t *)val) : -ENOSYS;
    default: return -EINVAL;
    }
}

static int bms_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_bms_if_t *bif = (uiox_bms_if_t *)dev->drv_priv;
    if (!bif || !arg) return -EINVAL;

    switch (cmd) {
    case UIOX_BMS_IOC_GET_TELEM: {
        int rc = uiox_bms_if_measure(bif);
        if (rc < 0) return rc;
        /* telemetry() takes gauge results the DRIVER does not compute —
         * it reads cells, current and temperature.  Passing zero means
         * the struct reports only what was actually measured, rather
         * than inventing a fuel-gauge state. */
        return uiox_bms_if_telemetry(bif, (uiox_bms_telem_t *)arg,
                                     0u, 0u, 0u, 0, 0);
    }
    case UIOX_BMS_IOC_GET_PACK_MV: {
        uiox_bms_telem_t t;
        int rc = uiox_bms_if_measure(bif);
        if (rc < 0) return rc;
        rc = uiox_bms_if_telemetry(bif, &t, 0u, 0u, 0u, 0, 0);
        if (rc < 0) return rc;
        *(uint32_t *)arg = t.pack_mv;
        return 0;
    }
    case UIOX_BMS_IOC_GET_CELLS: {
        uiox_bms_telem_t t;
        int rc = uiox_bms_if_measure(bif);
        if (rc < 0) return rc;
        rc = uiox_bms_if_telemetry(bif, &t, 0u, 0u, 0u, 0, 0);
        if (rc < 0) return rc;
        memcpy(arg, t.cell_mv, sizeof(t.cell_mv));
        return 0;
    }
    case UIOX_BMS_IOC_GET_STATS:
        uiox_bms_if_stats_get(bif, (uiox_bms_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;
    }
}

const uiox_bms_ops_t uiox_bms_class_ops = {
    .head = { .name = "bms0", .probe = NULL,
              .init = bms_class_init, .deinit = bms_class_deinit,
              .ioctl = bms_class_ioctl },
    .bus  = { .init = bms_class_init, .deinit = bms_class_deinit,
              .read_reg = bms_class_read_reg,
              .write_reg = bms_class_write_reg },
};

/* STATIC — §9's uiox_dev_bind() stores the pointer it is given, not a
 * copy, so these must outlive the call. */
static uiox_bms_hw_t       s_bms_hw;
static uiox_bms_if_t       s_bms_if;
static uiox_dev_dev_t      s_bms_dev;
static struct uiox_i2c_dev s_bms_i2c;

int uiox_bms_probe(void)
{
    if (uiox_bms_fwbind(&s_bms_hw, &s_bms_i2c) < 0)      return -1;
    if (uiox_bms_if_config(&s_bms_if, &s_bms_hw) < 0)    return -1;

    s_bms_dev.cls  = UIOX_DEVCLASS_BMS;
    s_bms_dev.unit = 0u;
    s_bms_dev.bus  = UIOX_BUS_I2C;
    s_bms_dev.base = 0u;                  /* bus-attached: no MMIO   */
    s_bms_dev.addr = s_bms_hw.i2c_addr;   /* uiox_bms_hw.h:92        */
    s_bms_dev.irq  = s_bms_hw.irq;        /* uiox_bms_hw.h:93        */
    s_bms_dev.name = "bms0";
    s_bms_dev.ops  = &uiox_bms_class_ops;
    s_bms_dev.drv_priv = &s_bms_if;

    return uiox_dev_bind(&s_bms_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */
