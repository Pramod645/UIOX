/**
 * @file    uiox_therm_if.c
 * @brief   UIOX Thermal Sensor interface driver implementation.
 * @date    2026-06-05
 */

 #include "uiox_therm_if.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"
 
 int uiox_therm_if_config(uiox_therm_if_t *tif, uiox_therm_hw_t *hw)
 {
     if (!tif || !hw) return -EINVAL;
     memset(tif, 0, sizeof(*tif));
     tif->hw     = hw;
     tif->primed = true;
     return 0;
 }
 
 int uiox_therm_if_start(uiox_therm_if_t *tif)
 {
     if (!tif || !tif->primed) return -EINVAL;
 
     const uiox_therm_hw_ops_t *ops =
         (const uiox_therm_hw_ops_t *)tif->hw->ops;
 
     /* Set alert thresholds in hardware */
     if (tif->hw->t_high_dc)
         uiox_therm_hw_set_t_high(tif->hw, tif->hw->t_high_dc);
     if (tif->hw->t_hyst_dc)
         uiox_therm_hw_set_t_hyst(tif->hw, tif->hw->t_hyst_dc);
     if (tif->hw->t_crit_dc)
         uiox_therm_hw_set_t_crit(tif->hw, tif->hw->t_crit_dc);
 
     /* Configure alert as interrupt mode, fault queue = 2 */
     if (ops && ops->reg_write) {
         uint8_t conf = (uint8_t)(UIOX_THERM_CONF_CMP_INT | (1u << 3));
         ops->reg_write(tif->hw, UIOX_REG_THERM_CONF, conf);
     }
     return 0;
 }
 
 void uiox_therm_if_stop(uiox_therm_if_t *tif)
 {
     if (!tif) return;
     /* Put sensor into shutdown mode to save power */
     const uiox_therm_hw_ops_t *ops =
         (const uiox_therm_hw_ops_t *)tif->hw->ops;
     if (ops && ops->set_mode) ops->set_mode(tif->hw, true);
 }
 
 int uiox_therm_if_measure(uiox_therm_if_t *tif, uint32_t now_ms)
 {
     if (!tif || !tif->primed) return -EINVAL;
     tif->stats.measurements++;
     int rc = 0;
 
     for (uint8_t ch = 0; ch < tif->hw->num_channels; ch++) {
         int16_t temp = 0;
         int r = uiox_therm_hw_read_temp(tif->hw, ch, &temp);
         if (r < 0) {
             tif->stats.comm_errors++;
             tif->stats.error_count++;
             uiox_therm_event_t ev = {
                 .type      = UIOX_THERM_EV_SENSOR_ERROR,
                 .sensor_id = ch,
                 .ts_ms     = now_ms,
                 .valid     = true,
             };
             uiox_therm_event_push(&ev);
             rc = r;
             continue;
         }
 
         /* Check alert threshold */
         if (tif->hw->t_high_dc != 0 && temp >= tif->hw->t_high_dc) {
             if (!tif->hw->meas[ch].alert_active) {
                 tif->hw->meas[ch].alert_active = true;
                 tif->stats.alert_count++;
                 uiox_therm_event_t ev = {
                     .type         = UIOX_THERM_EV_ALERT_HIGH,
                     .sensor_id    = ch,
                     .temp_dc      = temp,
                     .threshold_dc = tif->hw->t_high_dc,
                     .ts_ms        = now_ms,
                     .valid        = true,
                 };
                 uiox_therm_event_push(&ev);
             }
         } else if (tif->hw->meas[ch].alert_active &&
                    temp < tif->hw->t_hyst_dc) {
             tif->hw->meas[ch].alert_active = false;
             uiox_therm_event_t ev = {
                 .type      = UIOX_THERM_EV_ALERT_CLEAR,
                 .sensor_id = ch,
                 .temp_dc   = temp,
                 .ts_ms     = now_ms,
                 .valid     = true,
             };
             uiox_therm_event_push(&ev);
         }
 
         /* Critical threshold */
         if (tif->hw->t_crit_dc != 0 && temp >= tif->hw->t_crit_dc) {
             uiox_therm_event_t ev = {
                 .type         = UIOX_THERM_EV_CRITICAL,
                 .sensor_id    = ch,
                 .temp_dc      = temp,
                 .threshold_dc = tif->hw->t_crit_dc,
                 .ts_ms        = now_ms,
                 .valid        = true,
             };
             uiox_therm_event_push(&ev);
         }
     }
     return rc;
 }
 
 int uiox_therm_if_irq_handle(uiox_therm_if_t *tif, uint32_t now_ms)
 {
     if (!tif) return -EINVAL;
     tif->stats.irq_count++;
     tif->stats.alert_count++;
 
     /* Read alert status */
     const uiox_therm_hw_ops_t *ops =
         (const uiox_therm_hw_ops_t *)tif->hw->ops;
     bool alert = false;
     if (ops && ops->alert_status) ops->alert_status(tif->hw, &alert);
 
     if (alert) {
         /* Measure all channels to find which is above threshold */
         uiox_therm_if_measure(tif, now_ms);
     }
     uiox_therm_hw_alert_clear(tif->hw);
     return alert ? 1 : 0;
 }
 
 int uiox_therm_if_telemetry(uiox_therm_if_t *tif,
                              uiox_therm_telem_t *out, uint32_t now_ms)
 {
     if (!tif || !out) return -EINVAL;
     out->ts_ms       = now_ms;
     out->num_channels= tif->hw->num_channels;
     for (uint8_t i = 0; i < tif->hw->num_channels; i++) {
         out->temp_dc[i] = tif->hw->meas[i].temp_dc;
         out->alert[i]   = tif->hw->meas[i].alert_active;
     }
     return 0;
 }
 
 int uiox_therm_if_set_alert(uiox_therm_if_t *tif,
                              int16_t t_high_dc, int16_t t_hyst_dc)
 {
     if (!tif) return -EINVAL;
     int rc = uiox_therm_hw_set_t_high(tif->hw, t_high_dc);
     if (rc == 0) rc = uiox_therm_hw_set_t_hyst(tif->hw, t_hyst_dc);
     return rc;
 }
 
 void uiox_therm_if_stats_get(const uiox_therm_if_t *tif,
                               uiox_therm_if_stats_t *out)
 { if (!tif || !out) return; memcpy(out, &tif->stats, sizeof(*out)); }
 
 void uiox_therm_if_stats_reset(uiox_therm_if_t *tif)
 { if (!tif) return; memset(&tif->stats, 0, sizeof(tif->stats)); }
 /* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int therm_class_init(uiox_dev_dev_t *dev)
{
    uiox_therm_if_t *tif = (uiox_therm_if_t *)dev->drv_priv;
    return tif ? uiox_therm_if_start(tif) : -EINVAL;
}

static void therm_class_deinit(uiox_dev_dev_t *dev)
{
    uiox_therm_if_t *tif = (uiox_therm_if_t *)dev->drv_priv;
    if (tif) uiox_therm_if_stop(tif);
}

/* The HAL's own driver reaches hw->ops directly (uiox_therm_if.c:16), so
 * there are no public reg_* wrappers for this class — unlike als. */
static int therm_class_read_reg(uiox_dev_dev_t *dev, uint8_t reg,
                                void *val, uint16_t n)
{
    uiox_therm_if_t *tif = (uiox_therm_if_t *)dev->drv_priv;
    if (!tif || !val || !tif->hw || !tif->hw->ops) return -EINVAL;
    const uiox_therm_hw_ops_t *o =
        (const uiox_therm_hw_ops_t *)tif->hw->ops;
    switch (n) {
    case 1u: return o->reg_read   ? o->reg_read  (tif->hw, reg, (uint8_t  *)val) : -ENOSYS;
    case 2u: return o->reg_read16 ? o->reg_read16(tif->hw, reg, (uint16_t *)val) : -ENOSYS;
    default: return -EINVAL;
    }
}

static int therm_class_write_reg(uiox_dev_dev_t *dev, uint8_t reg,
                                 const void *val, uint16_t n)
{
    uiox_therm_if_t *tif = (uiox_therm_if_t *)dev->drv_priv;
    if (!tif || !val || !tif->hw || !tif->hw->ops) return -EINVAL;
    const uiox_therm_hw_ops_t *o =
        (const uiox_therm_hw_ops_t *)tif->hw->ops;
    switch (n) {
    case 1u: return o->reg_write   ? o->reg_write  (tif->hw, reg, *(const uint8_t  *)val) : -ENOSYS;
    case 2u: return o->reg_write16 ? o->reg_write16(tif->hw, reg, *(const uint16_t *)val) : -ENOSYS;
    default: return -EINVAL;
    }
}

static int therm_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_therm_if_t *tif = (uiox_therm_if_t *)dev->drv_priv;
    if (!tif || !arg) return -EINVAL;

    switch (cmd) {
    case UIOX_THERM_IOC_GET_TELEM: {
        int rc = uiox_therm_if_telemetry(tif, (uiox_therm_telem_t *)arg, 0u);
        return (rc < 0) ? rc : 0;
    }
    case UIOX_THERM_IOC_GET_TEMP: {
        uiox_therm_telem_t t;
        int rc = uiox_therm_if_telemetry(tif, &t, 0u);
        if (rc < 0) return rc;
        *(int16_t *)arg = (t.num_channels > 0u) ? t.temp_dc[0] : 0;
        return 0;
    }
    case UIOX_THERM_IOC_SET_ALERT: {
        const uiox_therm_alert_t *a = (const uiox_therm_alert_t *)arg;
        return uiox_therm_if_set_alert(tif, a->t_high_dc, a->t_hyst_dc);
    }
    case UIOX_THERM_IOC_GET_STATS:
        uiox_therm_if_stats_get(tif, (uiox_therm_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;
    }
}

const uiox_therm_ops_t uiox_therm_class_ops = {
    .head = { .name = "therm0", .probe = NULL,
              .init = therm_class_init, .deinit = therm_class_deinit,
              .ioctl = therm_class_ioctl },
    .bus  = { .init = therm_class_init, .deinit = therm_class_deinit,
              .read_reg = therm_class_read_reg,
              .write_reg = therm_class_write_reg },
};

static uiox_therm_hw_t       s_therm_hw;
static uiox_therm_if_t       s_therm_if;
static uiox_dev_dev_t        s_therm_dev;
static struct uiox_i2c_dev   s_therm_i2c;

int uiox_therm_probe(void)
{
    if (uiox_therm_fwbind(&s_therm_hw, &s_therm_i2c) < 0)      return -1;
    if (uiox_therm_if_config(&s_therm_if, &s_therm_hw) < 0)    return -1;  /* TWO args */

    s_therm_dev.cls  = UIOX_DEVCLASS_THERMAL;
    s_therm_dev.unit = 0u;
    s_therm_dev.bus  = (s_therm_hw.bus == UIOX_THERM_BUS_I2C)  ? UIOX_BUS_I2C
                     : (s_therm_hw.bus == UIOX_THERM_BUS_SPI)  ? UIOX_BUS_SPI
                     : (s_therm_hw.bus == UIOX_THERM_BUS_MMIO) ? UIOX_BUS_MMIO
                     : UIOX_BUS_NONE;
    s_therm_dev.base = (s_therm_hw.bus == UIOX_THERM_BUS_MMIO)
                     ? s_therm_hw.mmio_temp_reg : 0u;
    s_therm_dev.addr = s_therm_hw.i2c_addr;
    s_therm_dev.irq  = s_therm_hw.irq;
    s_therm_dev.name = "therm0";
    s_therm_dev.ops  = &uiox_therm_class_ops;
    s_therm_dev.drv_priv = &s_therm_if;

    return uiox_dev_bind(&s_therm_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */
