/**
 * @file  uiox_chg_if.h
 * @brief UIOX Charger interface driver — I²C access, ADC, IRQ dispatch.
 * @date  2026-06-11
 */

 #ifndef UIOX_CHG_IF_H
 #define UIOX_CHG_IF_H
 
 #include "uiox_chg_hw.h"
 #include "uiox_chg_buf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  plug_events;
     uint64_t  fault_events;
     uint64_t  pd_msgs_rx;
     uint64_t  pd_msgs_tx;
     uint32_t  adc_reads;
     uint32_t  wdog_kicks;
     uint32_t  errors;
     uint32_t  irq_count;
 } uiox_chg_if_stats_t;
 
 typedef struct {
     uiox_chg_hw_t      *hw;
     uiox_chg_if_stats_t stats;
     bool                primed;
     /* Cached last status */
     uiox_chg_chrg_t     last_chrg;
     uiox_chg_src_t      last_src;
     uint32_t            last_faults;
 } uiox_chg_if_t;
 
 int  uiox_chg_if_config      (uiox_chg_if_t *cif, uiox_chg_hw_t *hw);
 int  uiox_chg_if_start       (uiox_chg_if_t *cif);
 void uiox_chg_if_stop        (uiox_chg_if_t *cif);
 
 /* Status poll — reads status, ADC, faults; returns event or NULL */
 uiox_chg_evt_t *uiox_chg_if_poll        (uiox_chg_if_t *cif,
                                           uint32_t now_ms);
 
 /* IRQ handler — call from platform ISR or tick */
 uiox_chg_evt_t *uiox_chg_if_irq_handle  (uiox_chg_if_t *cif,
                                           uint32_t now_ms);
 
 /* Charge parameter setters (forwarded to HAL) */
 int  uiox_chg_if_set_ichg    (uiox_chg_if_t *cif, uint32_t ma);
 int  uiox_chg_if_set_vchg    (uiox_chg_if_t *cif, uint32_t mv);
 int  uiox_chg_if_set_iin_lim (uiox_chg_if_t *cif, uint32_t ma);
 int  uiox_chg_if_charge_en   (uiox_chg_if_t *cif, bool en);
 int  uiox_chg_if_otg_en      (uiox_chg_if_t *cif, bool en);
 
 /* PD messaging */
 int  uiox_chg_if_pd_tx       (uiox_chg_if_t *cif,
                                const uint8_t *buf, uint8_t len);
 int  uiox_chg_if_pd_rx       (uiox_chg_if_t *cif,
                                uint8_t *buf, uint8_t max_len);
 
 void uiox_chg_if_stats_get   (const uiox_chg_if_t *cif,
                                uiox_chg_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CHG_IF_H */
 