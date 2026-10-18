/**
 * @file    uiox_fan_if.c
 * @brief   UIOX Fan Controller interface driver implementation.
 * @date    2026-06-05
 */

 #include "uiox_fan_if.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"
 
 int uiox_fan_if_config(uiox_fan_if_t *fif, uiox_fan_hw_t *hw)
 {
     if (!fif || !hw) return -EINVAL;
     memset(fif, 0, sizeof(*fif));
     fif->hw     = hw;
     fif->primed = true;
     return 0;
 }
 
 int uiox_fan_if_start(uiox_fan_if_t *fif)
 {
     if (!fif || !fif->primed) return -EINVAL;
 
     /* Read device/manufacturer IDs */
     const uiox_fan_hw_ops_t *ops =
         (const uiox_fan_hw_ops_t *)fif->hw->ops;
     if (ops && ops->reg_read) {
         ops->reg_read(fif->hw, UIOX_REG_DEVICE_ID, &fif->device_id);
         ops->reg_read(fif->hw, UIOX_REG_MFR_ID,    &fif->mfr_id);
     }
 
     /* Configure each fan channel */
     for (uint8_t i = 0; i < fif->hw->num_fans; i++) {
         uiox_fan_hw_chan_enable(fif->hw, i, true);
         /* Set minimum PWM (20%) at startup to prevent inrush */
         uiox_fan_hw_set_pwm(fif->hw, i, 51u);
     }
 
     /* Unmask stall + drive-fail interrupts */
     if (ops && ops->reg_write)
         ops->reg_write(fif->hw, UIOX_REG_INTERRUPT_ENABLE, 0x03u);
 
     return 0;
 }
 
 void uiox_fan_if_stop(uiox_fan_if_t *fif)
 {
     if (!fif) return;
     /* Ramp down fans before stopping */
     for (uint8_t i = 0; i < fif->hw->num_fans; i++)
         uiox_fan_hw_set_pwm(fif->hw, i, 0u);
 }
 
 int uiox_fan_if_measure(uiox_fan_if_t *fif)
 {
     if (!fif || !fif->primed) return -EINVAL;
     fif->stats.measurements++;
     int rc = 0;
 
     for (uint8_t i = 0; i < fif->hw->num_fans; i++) {
         uint16_t rpm = 0;
         int r = uiox_fan_hw_read_rpm(fif->hw, i, &rpm);
         if (r < 0) { fif->stats.comm_errors++; rc = r; }
     }
     for (uint8_t i = 0; i < fif->hw->num_temps; i++) {
         int16_t temp = 0;
         int r = uiox_fan_hw_read_temp(fif->hw, i, &temp);
         if (r < 0) fif->stats.comm_errors++;
     }
     return rc;
 }
 
 int uiox_fan_if_set_pwm(uiox_fan_if_t *fif, uint8_t ch,
                          uint8_t duty, uint32_t now_ms)
 {
     if (!fif || ch >= fif->hw->num_fans) return -EINVAL;
     uint8_t old = fif->hw->chan[ch].pwm_duty;
     int rc = uiox_fan_hw_set_pwm(fif->hw, ch, duty);
     if (rc == 0 && duty != old) {
         fif->stats.pwm_changes++;
         uiox_fan_event_t ev = {
             .type     = UIOX_FAN_EV_PWM_CHANGE,
             .fan_id   = ch,
             .pwm_duty = duty,
             .rpm      = fif->hw->chan[ch].rpm_measured,
             .ts_ms    = now_ms,
             .valid    = true,
         };
         uiox_fan_event_push(&ev);
     }
     return rc;
 }
 
 int uiox_fan_if_irq_handle(uiox_fan_if_t *fif, uint32_t now_ms)
 {
     if (!fif) return -EINVAL;
     fif->stats.irq_count++;
 
     uint32_t flags = 0;
     int rc = uiox_fan_hw_fault_status(fif->hw, &flags);
     if (rc < 0 || !flags) return rc;
 
     uiox_fan_hw_fault_clear(fif->hw, flags);
 
     static const struct { uint32_t bit; uiox_fan_ev_t ev; }
     fault_map[] = {
         { UIOX_FAN_FAULT_STALL,        UIOX_FAN_EV_STALL        },
         { UIOX_FAN_FAULT_DRIVE_FAIL,   UIOX_FAN_EV_FAULT        },
         { UIOX_FAN_FAULT_SPIN_UP_FAIL, UIOX_FAN_EV_SPIN_UP_FAIL },
         { UIOX_FAN_FAULT_OTP,          UIOX_FAN_EV_OVERHEAT     },
         { UIOX_FAN_FAULT_WATCHDOG,     UIOX_FAN_EV_WATCHDOG     },
     };
     for (size_t i = 0; i < sizeof(fault_map)/sizeof(fault_map[0]); i++) {
         if (flags & fault_map[i].bit) {
             uiox_fan_event_t ev = {
                 .type        = fault_map[i].ev,
                 .fan_id      = 0xFFu,
                 .ts_ms       = now_ms,
                 .fault_flags = flags,
                 .valid       = true,
             };
             uiox_fan_event_push(&ev);
             fif->stats.fault_count++;
             if (flags & UIOX_FAN_FAULT_STALL)
                 fif->stats.stall_count++;
         }
     }
     return (int)flags;
 }
 
 int uiox_fan_if_telemetry(uiox_fan_if_t *fif,
                            uiox_fan_telem_t *out, uint32_t now_ms)
 {
     if (!fif || !out) return -EINVAL;
     out->ts_ms       = now_ms;
     out->fault_flags = fif->hw->global_fault;
     for (uint8_t i = 0; i < fif->hw->num_fans; i++) {
         out->rpm[i] = fif->hw->chan[i].rpm_measured;
         out->pwm[i] = fif->hw->chan[i].pwm_duty;
     }
     for (uint8_t i = 0; i < fif->hw->num_temps; i++)
         out->temp_dc[i] = fif->hw->temp_dc[i];
     return 0;
 }
 
 void uiox_fan_if_stats_get(const uiox_fan_if_t *fif,
                             uiox_fan_if_stats_t *out)
 { if (!fif || !out) return; memcpy(out, &fif->stats, sizeof(*out)); }
 
 void uiox_fan_if_stats_reset(uiox_fan_if_t *fif)
 { if (!fif) return; memset(&fif->stats, 0, sizeof(fif->stats)); }
 /* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int fan_class_init(uiox_dev_dev_t *dev)
{
    uiox_fan_if_t *fif = (uiox_fan_if_t *)dev->drv_priv;
    return fif ? uiox_fan_if_start(fif) : -EINVAL;
}

static void fan_class_deinit(uiox_dev_dev_t *dev)
{
    uiox_fan_if_t *fif = (uiox_fan_if_t *)dev->drv_priv;
    if (fif) uiox_fan_if_stop(fif);
}

/* No public reg_* wrappers for this class — the vtable is reached
 * directly, as uiox_therm_if.c does. */
static int fan_class_read_reg(uiox_dev_dev_t *dev, uint8_t reg,
                              void *val, uint16_t n)
{
    uiox_fan_if_t *fif = (uiox_fan_if_t *)dev->drv_priv;
    if (!fif || !val || !fif->hw->ops) return -EINVAL;
    const uiox_fan_hw_ops_t *o = (const uiox_fan_hw_ops_t *)fif->hw->ops;
    switch (n) {
    case 1u: return o->reg_read   ? o->reg_read  (fif->hw, reg, (uint8_t  *)val) : -ENOSYS;
    case 2u: return o->reg_read16 ? o->reg_read16(fif->hw, reg, (uint16_t *)val) : -ENOSYS;
    default: return -EINVAL;
    }
}

static int fan_class_write_reg(uiox_dev_dev_t *dev, uint8_t reg,
                               const void *val, uint16_t n)
{
    uiox_fan_if_t *fif = (uiox_fan_if_t *)dev->drv_priv;
    if (!fif || !val || !fif->hw->ops) return -EINVAL;
    const uiox_fan_hw_ops_t *o = (const uiox_fan_hw_ops_t *)fif->hw->ops;
    switch (n) {
    case 1u: return o->reg_write   ? o->reg_write  (fif->hw, reg, *(const uint8_t  *)val) : -ENOSYS;
    case 2u: return o->reg_write16 ? o->reg_write16(fif->hw, reg, *(const uint16_t *)val) : -ENOSYS;
    default: return -EINVAL;
    }
}

static int fan_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_fan_if_t *fif = (uiox_fan_if_t *)dev->drv_priv;
    if (!fif || !arg) return -EINVAL;

    switch (cmd) {
    case UIOX_FAN_IOC_GET_TELEM: {
        /* telemetry() only snapshots; measure() is what samples. */
        int rc = uiox_fan_if_measure(fif);
        if (rc < 0) return rc;
        rc = uiox_fan_if_telemetry(fif, (uiox_fan_telem_t *)arg, 0u);
        return (rc < 0) ? rc : 0;
    }
    case UIOX_FAN_IOC_GET_RPM: {
        uiox_fan_telem_t t;
        int rc = uiox_fan_if_measure(fif);
        if (rc < 0) return rc;
        rc = uiox_fan_if_telemetry(fif, &t, 0u);
        if (rc < 0) return rc;
        *(uint16_t *)arg = t.rpm[0];
        return 0;
    }
    case UIOX_FAN_IOC_SET_PWM: {
        const uiox_fan_pwm_t *p = (const uiox_fan_pwm_t *)arg;
        return uiox_fan_if_set_pwm(fif, p->fan_id, p->duty, p->now_ms);
    }
    case UIOX_FAN_IOC_GET_STATS:
        uiox_fan_if_stats_get(fif, (uiox_fan_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;
    }
}

const uiox_fan_ops_t uiox_fan_class_ops = {
    .head = { .name = "fan0", .probe = NULL,
              .init = fan_class_init, .deinit = fan_class_deinit,
              .ioctl = fan_class_ioctl },
    .bus  = { .init = fan_class_init, .deinit = fan_class_deinit,
              .read_reg = fan_class_read_reg,
              .write_reg = fan_class_write_reg },
};

static uiox_fan_hw_t       s_fan_hw;
static uiox_fan_if_t       s_fan_if;
static uiox_dev_dev_t      s_fan_dev;
static struct uiox_i2c_dev s_fan_i2c;

int uiox_fan_probe(void)
{
    if (uiox_fan_fwbind(&s_fan_hw, &s_fan_i2c) < 0)      return -1;
    if (uiox_fan_if_config(&s_fan_if, &s_fan_hw) < 0)    return -1;

    s_fan_dev.cls  = UIOX_DEVCLASS_FAN;
    s_fan_dev.unit = 0u;
    s_fan_dev.bus  = UIOX_BUS_I2C;
    s_fan_dev.base = 0u;
    s_fan_dev.addr = s_fan_hw.i2c_addr;
    s_fan_dev.irq  = s_fan_hw.irq;
    s_fan_dev.name = "fan0";
    s_fan_dev.ops  = &uiox_fan_class_ops;
    s_fan_dev.drv_priv = &s_fan_if;

    return uiox_dev_bind(&s_fan_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */
