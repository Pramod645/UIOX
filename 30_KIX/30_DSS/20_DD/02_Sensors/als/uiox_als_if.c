/**
 * @file  uiox_als_if.c
 * @brief UIOX ALS interface driver — I²C, IRQ dispatch, integration guard.
 * @date  2026-06-11
 */

 #include "uiox_als_if.h"
 #include "uiox_klibc.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"      /* struct uiox_i2c_dev */
 
 int uiox_als_if_config(uiox_als_if_t *aif, uiox_als_hw_t *hw,
                         bool continuous)
 {
     if (!aif || !hw) return -EINVAL;
     memset(aif, 0, sizeof(*aif));
     aif->hw          = hw;
     aif->primed      = true;
     aif->continuous  = continuous;
     return 0;
 }
 
 int uiox_als_if_start(uiox_als_if_t *aif)
 {
     if (!aif || !aif->primed) return -EINVAL;
     int rc = uiox_als_hw_power_on(aif->hw);
     if (rc < 0) return rc;
     /* Enable threshold interrupt */
     rc = uiox_als_hw_int_enable(aif->hw, true);
     if (rc < 0) return rc;
     /* Set default integration-time window */
     aif->next_sample_ms = 0u;
     return 0;
 }
 
 void uiox_als_if_stop(uiox_als_if_t *aif)
 {
     if (!aif) return;
     uiox_als_hw_int_enable(aif->hw, false);
     uiox_als_hw_power_off(aif->hw);
 }
 
 int uiox_als_if_trigger(uiox_als_if_t *aif)
 {
     if (!aif || aif->continuous) return -EINVAL;
     return uiox_als_hw_trigger(aif->hw);
 }
 
 int uiox_als_if_fetch(uiox_als_if_t *aif, uint32_t now_ms,
                        uint16_t *als, uint16_t *white, uint16_t *ir)
 {
     if (!aif || !als || !white || !ir) return -EINVAL;
 
     /* Guard: do not read before integration completes */
     if (now_ms < aif->next_sample_ms) return 0;
 
     int rc = uiox_als_hw_read_als(aif->hw, als, white);
     if (rc < 0) { aif->stats.errors++; return rc; }
 
     /* IR channel optional */
     if (aif->hw->caps & UIOX_ALS_CAP_IR_CH) {
         rc = uiox_als_hw_read_ir(aif->hw, ir);
         if (rc < 0) *ir = 0u;
     } else {
         *ir = 0u;
     }
 
     /* Schedule next sample window */
     aif->next_sample_ms = now_ms +
         UIOX_ALS_HW_ITIME_MS[aif->hw->itime] + 5u; /* 5 ms margin */
 
     aif->stats.samples_read++;
     return 1;  /* data available */
 }
 
 int uiox_als_if_set_gain(uiox_als_if_t *aif, uiox_als_hw_gain_t g)
 {
     if (!aif) return -EINVAL;
     int rc = uiox_als_hw_set_gain(aif->hw, g);
     if (rc == 0) aif->stats.gain_changes++;
     return rc;
 }
 
 int uiox_als_if_set_itime(uiox_als_if_t *aif, uiox_als_hw_itime_t t)
 {
     if (!aif) return -EINVAL;
     return uiox_als_hw_set_itime(aif->hw, t);
 }
 
 int uiox_als_if_set_threshold(uiox_als_if_t *aif,
                                uint16_t low, uint16_t high)
 {
     if (!aif) return -EINVAL;
     return uiox_als_hw_set_threshold(aif->hw, low, high);
 }
 
 uiox_als_evt_t *uiox_als_if_irq_handle(uiox_als_if_t *aif,
                                          uint32_t now_ms)
 {
     if (!aif) return NULL;
     uint32_t irq = aif->hw->pending_irq;
     if (!irq) return NULL;
 
     aif->hw->pending_irq = 0u;
     aif->stats.irq_count++;
 
     uiox_als_evt_t *e = uiox_als_evt_alloc();
     if (!e) { aif->stats.errors++; return NULL; }
 
     e->timestamp_ms = now_ms;
     e->gain         = aif->hw->gain;
     e->itime        = aif->hw->itime;
 
     if (irq & UIOX_ALS_IRQ_THRESH_HIGH) {
         e->type = UIOX_ALS_EVT_THRESH_HIGH;
         aif->stats.irq_thresh_high++;
         uiox_als_hw_int_clear(aif->hw);
     } else if (irq & UIOX_ALS_IRQ_THRESH_LOW) {
         e->type = UIOX_ALS_EVT_THRESH_LOW;
         aif->stats.irq_thresh_low++;
         uiox_als_hw_int_clear(aif->hw);
     } else if (irq & UIOX_ALS_IRQ_DATA_READY) {
         e->type = UIOX_ALS_EVT_DATA_READY;
         aif->stats.irq_data_ready++;
     } else {
         e->type = UIOX_ALS_EVT_NONE;
     }
     return e;
 }
 
 void uiox_als_if_stats_get(const uiox_als_if_t *aif,
                              uiox_als_if_stats_t *out)
 { if (!aif || !out) return; memcpy(out, &aif->stats, sizeof(*out)); }
/* ═══════════════════════════ INSERTED ═══════════════════════════ */
/* ═════════════════════════════════════════════════════════════════════
 * Event pool
 *
 * The als_if_irq_handle() path borrows a slot and hands it to the caller,
 * who owns it until als_evt_free().  Static storage — there is no
 * free-store this early in boot.
 *
 * A foreign pointer passed to _free() is ignored rather than indexed
 * through: the subtraction would otherwise walk outside the array.
 * ═════════════════════════════════════════════════════════════════════ */
#define UIOX_ALS_EVT_POOL_MAX   8u

static uiox_als_evt_t  s_als_evt_pool[UIOX_ALS_EVT_POOL_MAX];
static bool            s_als_evt_used[UIOX_ALS_EVT_POOL_MAX];

uiox_als_evt_t *uiox_als_evt_alloc(void)
{
    for (uint32_t i = 0u; i < UIOX_ALS_EVT_POOL_MAX; i++) {
        if (!s_als_evt_used[i]) {
            s_als_evt_used[i] = true;
            memset(&s_als_evt_pool[i], 0, sizeof(s_als_evt_pool[i]));
            return &s_als_evt_pool[i];
        }
    }
    return (uiox_als_evt_t *)0;   /* pool exhausted — caller counts it */
}

void uiox_als_evt_free(uiox_als_evt_t *e)
{
    if (!e) return;
    if (e < s_als_evt_pool ||
        e >= s_als_evt_pool + UIOX_ALS_EVT_POOL_MAX)
        return;
    s_als_evt_used[e - s_als_evt_pool] = false;
}
/* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int als_class_init(uiox_dev_dev_t *dev)
{
    uiox_als_if_t *aif = (uiox_als_if_t *)dev->drv_priv;
    return aif ? uiox_als_if_start(aif) : -EINVAL;
}

static void als_class_deinit(uiox_dev_dev_t *dev)
{
    uiox_als_if_t *aif = (uiox_als_if_t *)dev->drv_priv;
    if (aif) uiox_als_if_stop(aif);
}

/* §8: the HAL stores 16-bit register values, so n selects the width.
 * The public wrappers exist for this class (uiox_als_hw_reg_read). */
static int als_class_read_reg(uiox_dev_dev_t *dev, uint8_t reg,
                              void *val, uint16_t n)
{
    uiox_als_if_t *aif = (uiox_als_if_t *)dev->drv_priv;
    if (!aif || !val || (n != 1u && n != 2u)) return -EINVAL;
    uint16_t v = 0u;
    int rc = uiox_als_hw_reg_read(aif->hw, reg, &v);
    if (rc < 0) return rc;
    if (n == 1u) *(uint8_t  *)val = (uint8_t)(v & 0xFFu);
    else         *(uint16_t *)val = v;
    return 0;
}

static int als_class_write_reg(uiox_dev_dev_t *dev, uint8_t reg,
                               const void *val, uint16_t n)
{
    uiox_als_if_t *aif = (uiox_als_if_t *)dev->drv_priv;
    if (!aif || !val || (n != 1u && n != 2u)) return -EINVAL;
    uint16_t v = (n == 1u) ? (uint16_t)(*(const uint8_t *)val)
                           : *(const uint16_t *)val;
    return uiox_als_hw_reg_write(aif->hw, reg, v);
}

static int als_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_als_if_t *aif = (uiox_als_if_t *)dev->drv_priv;
    if (!aif || !arg) return -EINVAL;

    switch (cmd) {
    case UIOX_ALS_IOC_GET_RAW: {
        uiox_als_raw_t *o = (uiox_als_raw_t *)arg;
        //int rc = uiox_als_if_fetch(aif, uiox_als_if_now_ms(),
        //                           &o->als, &o->white, &o->ir);
        int rc = uiox_als_if_fetch(aif, aif->next_sample_ms,
                                   &o->als, &o->white, &o->ir);

        if (rc < 0) return rc;          /* device error            */
        if (rc == 0) return -EAGAIN;    /* integration not complete */
        return 0;
    }
    case UIOX_ALS_IOC_GET_WHITE:
        *(uint16_t *)arg = aif->hw->raw_white;  return 0;
    case UIOX_ALS_IOC_GET_IR:
        *(uint16_t *)arg = aif->hw->raw_ir;     return 0;
    case UIOX_ALS_IOC_SET_GAIN:
        return uiox_als_if_set_gain(aif,
                   (uiox_als_hw_gain_t)(*(const uint8_t *)arg));
    case UIOX_ALS_IOC_SET_ITIME:
        return uiox_als_if_set_itime(aif,
                   (uiox_als_hw_itime_t)(*(const uint8_t *)arg));
    case UIOX_ALS_IOC_SET_THRESH: {
        const uiox_als_thresh_t *t = (const uiox_als_thresh_t *)arg;
        return uiox_als_if_set_threshold(aif, t->low, t->high);
    }
    case UIOX_ALS_IOC_GET_STATS:
        uiox_als_if_stats_get(aif, (uiox_als_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;                 /* valid call, no body here */
    }
}

const uiox_als_ops_t uiox_als_class_ops = {
    .head = { .name = "als0", .probe = NULL,
              .init = als_class_init, .deinit = als_class_deinit,
              .ioctl = als_class_ioctl },
    .bus  = { .init = als_class_init, .deinit = als_class_deinit,
              .read_reg = als_class_read_reg,
              .write_reg = als_class_write_reg },
};

/* STATIC — §9's uiox_dev_bind() stores the pointer it is given, not a
 * copy, so these must outlive the call. */
static uiox_als_hw_t       s_als_hw;
static uiox_als_if_t       s_als_if;
static uiox_dev_dev_t      s_als_dev;
static struct uiox_i2c_dev s_als_i2c;

int uiox_als_probe(void)
{
    /* fwbind first: uiox_als_hw_reg_read() returns -EINVAL when
     * hw->ops is NULL, so a class without it would bind and then fail
     * every register access at runtime. */
    if (uiox_als_fwbind(&s_als_hw, &s_als_i2c) < 0)          return -1;
    if (uiox_als_if_config(&s_als_if, &s_als_hw, true) < 0)  return -1;

    s_als_dev.cls      = UIOX_DEVCLASS_ALS;
    s_als_dev.unit     = 0u;
    s_als_dev.bus      = UIOX_BUS_I2C;
    s_als_dev.base     = 0u;                 /* bus-attached: no MMIO   */
    s_als_dev.addr     = s_als_hw.i2c_addr;  /* 7-bit slave address     */
    s_als_dev.irq      = s_als_hw.irq;
    s_als_dev.name     = "als0";
    s_als_dev.ops      = &uiox_als_class_ops;
    s_als_dev.drv_priv = &s_als_if;

    return uiox_dev_bind(&s_als_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */ 