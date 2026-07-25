/**
 * @file  uiox_chg_if.c
 * @brief UIOX Charger interface driver — I²C, ADC, IRQ.
 * @date  2026-06-11
 */

 #include "uiox_chg_if.h"
 
 int uiox_chg_if_config(uiox_chg_if_t *cif, uiox_chg_hw_t *hw)
 {
     if (!cif || !hw) return -EINVAL;
     memset(cif, 0, sizeof(*cif));
     cif->hw     = hw;
     cif->primed = true;
     uiox_chg_buf_init();
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
     uiox_chg_src_t  src    = UIOX_CHG_SRC_NONE;
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
     } else if (src_changed && src != UIOX_CHG_SRC_NONE) {
         e->type = UIOX_CHG_EVT_PLUG_IN;
         cif->stats.plug_events++;
     } else if (src_changed && src == UIOX_CHG_SRC_NONE) {
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
 