/**
 * @file  uiox_chg_if.c
 * @brief UIOX Charger interface driver — I²C, ADC, IRQ.
 * @date  2026-06-11
 */

 #include "uiox_chg_if.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"
 
 int uiox_chg_if_config(uiox_chg_if_t *cif, uiox_chg_hw_t *hw)
 {
     if (!cif || !hw) return -EINVAL;
     memset(cif, 0, sizeof(*cif));
     cif->hw     = hw;
     cif->primed = true;
     return 0;
 }
 
 int uiox_chg_if_start(uiox_chg_if_t *cif)
 {
     if (!cif || !cif->primed) return -EINVAL;
 
     /* Enable ADC continuous conversion */
     int rc = uiox_chg_hw_reg_rmw(cif->hw, BQ25895_REG02,
                                   0x80u, 0x80u);  /* CONV_RATE = 1 */
     if (rc < 0) return rc;
 
     /* Enable charge */
     rc = uiox_chg_hw_charge_en(cif->hw, true);
     if (rc < 0) return rc;
 
     return 0;
 }
 
 void uiox_chg_if_stop(uiox_chg_if_t *cif)
 {
     if (!cif) return;
     uiox_chg_hw_charge_en(cif->hw, false);
     /* Disable ADC */
     uiox_chg_hw_reg_rmw(cif->hw, BQ25895_REG02, 0x80u, 0x00u);
 }
 
 int uiox_chg_if_set_ichg(uiox_chg_if_t *cif, uint32_t ma)
 {
     if (!cif) return -EINVAL;
     return uiox_chg_hw_set_ichg(cif->hw, ma);
 }
 
 int uiox_chg_if_set_vchg(uiox_chg_if_t *cif, uint32_t mv)
 {
     if (!cif) return -EINVAL;
     return uiox_chg_hw_set_vchg(cif->hw, mv);
 }
 
 int uiox_chg_if_set_iin_lim(uiox_chg_if_t *cif, uint32_t ma)
 {
     if (!cif) return -EINVAL;
     return uiox_chg_hw_set_iin_lim(cif->hw, ma);
 }
 
 int uiox_chg_if_charge_en(uiox_chg_if_t *cif, bool en)
 {
     if (!cif) return -EINVAL;
     return uiox_chg_hw_charge_en(cif->hw, en);
 }
 
 int uiox_chg_if_otg_en(uiox_chg_if_t *cif, bool en)
 {
     if (!cif) return -EINVAL;
     return uiox_chg_hw_otg_en(cif->hw, en);
 }
 
 int uiox_chg_if_pd_tx(uiox_chg_if_t *cif,
                        const uint8_t *buf, uint8_t len)
 {
     if (!cif || !buf) return -EINVAL;
     int rc = uiox_chg_hw_pd_tx(cif->hw, buf, len);
     if (rc == 0) cif->stats.pd_msgs_tx++;
     else         cif->stats.errors++;
     return rc;
 }
 
 int uiox_chg_if_pd_rx(uiox_chg_if_t *cif,
                        uint8_t *buf, uint8_t max_len)
 {
     if (!cif || !buf) return -EINVAL;
     int rc = uiox_chg_hw_pd_rx(cif->hw, buf, max_len);
     if (rc > 0) cif->stats.pd_msgs_rx++;
     return rc;
 }
 
 uiox_chg_evt_t *uiox_chg_if_poll(uiox_chg_if_t *cif, uint32_t now_ms)
 {
     if (!cif) return NULL;
 
     uiox_chg_chrg_t chrg   = UIOX_CHG_CHRG_IDLE;
     uiox_chg_hw_src_t  src    = UIOX_CHG_HW_SRC_NONE;
     uint32_t        faults = UIOX_CHG_FAULT_NONE;
 
     int rc = uiox_chg_hw_get_status(cif->hw, &chrg, &src, &faults);
     if (rc < 0) { cif->stats.errors++; return NULL; }
 
     /* Update ADC cache */
     int32_t v = 0;
     if (uiox_chg_hw_adc_read(cif->hw, UIOX_CHG_ADC_VBUS, &v) == 0)
         cif->stats.adc_reads++;
 
     /* Detect state transitions */
     bool src_changed   = (src    != cif->last_src);
     bool fault_changed = (faults != cif->last_faults);
     bool chrg_changed  = (chrg   != cif->last_chrg);
 
     cif->last_src    = src;
     cif->last_faults = faults;
     cif->last_chrg   = chrg;
 
     if (!src_changed && !fault_changed && !chrg_changed)
         return NULL;
 
     uiox_chg_evt_t *e = uiox_chg_evt_alloc();
     if (!e) { cif->stats.errors++; return NULL; }
 
     e->timestamp_ms = now_ms;
     e->src          = src;
     e->chrg         = chrg;
     e->fault_flags  = faults;
     e->vbus_mv      = cif->hw->adc_mv[UIOX_CHG_ADC_VBUS];
     e->ibat_ma      = cif->hw->adc_mv[UIOX_CHG_ADC_IBAT];
 
     if (fault_changed && faults != UIOX_CHG_FAULT_NONE) {
         e->type = UIOX_CHG_EVT_FAULT;
         cif->stats.fault_events++;
     } else if (fault_changed && faults == UIOX_CHG_FAULT_NONE) {
         e->type = UIOX_CHG_EVT_FAULT_CLEAR;
     } else if (src_changed && src != UIOX_CHG_HW_SRC_NONE) {
         e->type = UIOX_CHG_EVT_PLUG_IN;
         cif->stats.plug_events++;
     } else if (src_changed && src == UIOX_CHG_HW_SRC_NONE) {
         e->type = UIOX_CHG_EVT_PLUG_OUT;
         cif->stats.plug_events++;
     } else if (chrg_changed && chrg == UIOX_CHG_CHRG_DONE) {
         e->type = UIOX_CHG_EVT_CHRG_DONE;
     } else if (chrg_changed && chrg == UIOX_CHG_CHRG_FAST) {
         e->type = UIOX_CHG_EVT_CHRG_START;
     } else {
         e->type = UIOX_CHG_EVT_NONE;
     }
     return e;
 }
 
 uiox_chg_evt_t *uiox_chg_if_irq_handle(uiox_chg_if_t *cif, uint32_t now_ms)
 {
     if (!cif) return NULL;
     cif->stats.irq_count++;
 
     /* Consume pending IRQ flags */
     uint32_t irq = cif->hw->pending_irq;
     cif->hw->pending_irq = 0u;
 
     if (!irq) return NULL;
 
     /* PD message pending? */
     if (irq & UIOX_CHG_IRQ_PD_MSG) {
         /* Upper layer will call pd_rx — just mark count */
         cif->stats.pd_msgs_rx++;
     }
 
     return uiox_chg_if_poll(cif, now_ms);
 }
 
 void uiox_chg_if_stats_get(const uiox_chg_if_t *cif,
                              uiox_chg_if_stats_t *out)
 { if (!cif || !out) return; memcpy(out, &cif->stats, sizeof(*out)); }
 /* ═══════════════════════════ INSERTED ═══════════════════════════ */
#define UIOX_CHG_EVT_POOL_MAX   8u

static uiox_chg_evt_t  s_chg_evt_pool[UIOX_CHG_EVT_POOL_MAX];
static bool            s_chg_evt_used[UIOX_CHG_EVT_POOL_MAX];

uiox_chg_evt_t *uiox_chg_evt_alloc(void)
{
    for (uint32_t i = 0u; i < UIOX_CHG_EVT_POOL_MAX; i++) {
        if (!s_chg_evt_used[i]) {
            s_chg_evt_used[i] = true;
            memset(&s_chg_evt_pool[i], 0, sizeof(s_chg_evt_pool[i]));
            return &s_chg_evt_pool[i];
        }
    }
    return (uiox_chg_evt_t *)0;
}

void uiox_chg_evt_free(uiox_chg_evt_t *e)
{
    if (!e) return;
    if (e < s_chg_evt_pool ||
        e >= s_chg_evt_pool + UIOX_CHG_EVT_POOL_MAX)
        return;
    s_chg_evt_used[e - s_chg_evt_pool] = false;
}
/* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int chg_class_init(uiox_dev_dev_t *dev)
{
    uiox_chg_if_t *cif = (uiox_chg_if_t *)dev->drv_priv;
    return cif ? uiox_chg_if_start(cif) : -EINVAL;
}

static void chg_class_deinit(uiox_dev_dev_t *dev)
{
    uiox_chg_if_t *cif = (uiox_chg_if_t *)dev->drv_priv;
    if (cif) uiox_chg_if_stop(cif);
}

/* Byte-wide HAL (uiox_chg_hw.h:261-262). */
static int chg_class_read_reg(uiox_dev_dev_t *dev, uint8_t reg,
                              void *val, uint16_t n)
{
    uiox_chg_if_t *cif = (uiox_chg_if_t *)dev->drv_priv;
    if (!cif || !val || n != 1u) return -EINVAL;
    uint8_t v = 0u;
    int rc = uiox_chg_hw_reg_read(cif->hw, reg, &v);
    if (rc < 0) return rc;
    *(uint8_t *)val = v;
    return 0;
}

static int chg_class_write_reg(uiox_dev_dev_t *dev, uint8_t reg,
                               const void *val, uint16_t n)
{
    uiox_chg_if_t *cif = (uiox_chg_if_t *)dev->drv_priv;
    if (!cif || !val || n != 1u) return -EINVAL;
    return uiox_chg_hw_reg_write(cif->hw, reg, *(const uint8_t *)val);
}

static int chg_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_chg_if_t *cif = (uiox_chg_if_t *)dev->drv_priv;
    if (!cif || !arg) return -EINVAL;

    switch (cmd) {
    case UIOX_CHG_IOC_GET_STATUS: {
        uiox_chg_status_t *s = (uiox_chg_status_t *)arg;
        /* Poll once so the cache is current, then read it — the
         * descriptor keeps last_chrg/last_src/last_faults precisely so
         * a caller can read status without a second poll. */
        (void)uiox_chg_if_poll(cif, 0u);
        s->chrg   = cif->last_chrg;
        s->src    = cif->last_src;
        s->faults = cif->last_faults;
        return 0;
    }
    case UIOX_CHG_IOC_SET_ICHG:
        return uiox_chg_if_set_ichg(cif, *(const uint32_t *)arg);
    case UIOX_CHG_IOC_SET_VCHG:
        return uiox_chg_if_set_vchg(cif, *(const uint32_t *)arg);
    case UIOX_CHG_IOC_SET_IIN:
        return uiox_chg_if_set_iin_lim(cif, *(const uint32_t *)arg);
    case UIOX_CHG_IOC_SET_ENABLE:
        return uiox_chg_if_charge_en(cif, *(const bool *)arg);
    case UIOX_CHG_IOC_GET_STATS:
        uiox_chg_if_stats_get(cif, (uiox_chg_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;
    }
}

const uiox_chg_ops_t uiox_chg_class_ops = {
    .head = { .name = "chg0", .probe = NULL,
              .init = chg_class_init, .deinit = chg_class_deinit,
              .ioctl = chg_class_ioctl },
    .bus  = { .init = chg_class_init, .deinit = chg_class_deinit,
              .read_reg = chg_class_read_reg,
              .write_reg = chg_class_write_reg },
};

/* STATIC — §9's uiox_dev_bind() stores the pointer it is given, not a
 * copy, so these must outlive the call. */
static uiox_chg_hw_t       s_chg_hw;
static uiox_chg_if_t       s_chg_if;
static uiox_dev_dev_t      s_chg_dev;
static struct uiox_i2c_dev s_chg_i2c;

int uiox_chg_probe(void)
{
    if (uiox_chg_fwbind(&s_chg_hw, &s_chg_i2c) < 0)      return -1;
    if (uiox_chg_if_config(&s_chg_if, &s_chg_hw) < 0)    return -1;

    s_chg_dev.cls  = UIOX_DEVCLASS_CHG;
    s_chg_dev.unit = 0u;
    s_chg_dev.bus  = UIOX_BUS_I2C;
    s_chg_dev.base = 0u;                 /* bus-attached: no MMIO   */
    s_chg_dev.addr = s_chg_hw.i2c_addr;  /* uiox_chg_hw.h:217       */
    s_chg_dev.irq  = s_chg_hw.irq;       /* uiox_chg_hw.h:220       */
    s_chg_dev.name = "chg0";
    s_chg_dev.ops  = &uiox_chg_class_ops;
    s_chg_dev.drv_priv = &s_chg_if;

    return uiox_dev_bind(&s_chg_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */
/* ═══════════════════════════ END INSERT ═════════════════════════ */
 