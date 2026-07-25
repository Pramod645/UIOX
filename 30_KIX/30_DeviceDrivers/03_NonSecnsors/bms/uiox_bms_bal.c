/**
 * @file    uiox_bms_bal.c
 * @brief   UIOX BMS cell balancing implementation.
 * @date    2026-06-04
 */

 #include "uiox_bms_bal.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_bms_bal_init(uiox_bms_bal_t *bal, uiox_bms_if_t *bif,
                        uiox_bms_bal_mode_t mode,
                        uint32_t delta_mv, uint32_t stop_mv)
 {
     if (!bal || !bif) return -EINVAL;
     memset(bal, 0, sizeof(*bal));
     bal->bif      = bif;
     bal->mode     = mode;
     bal->delta_mv = delta_mv ? delta_mv : UIOX_BMS_BAL_DELTA_MV_DEFAULT;
     bal->stop_mv  = stop_mv  ? stop_mv  : UIOX_BMS_BAL_STOP_MV_DEFAULT;
     bal->enabled  = true;
     return 0;
 }
 
 void uiox_bms_bal_tick(uiox_bms_bal_t *bal, uint32_t now_ms)
 {
     if (!bal || !bal->enabled || bal->mode == UIOX_BMS_BAL_MODE_OFF)
         return;
     (void)now_ms;
 
     uiox_bms_hw_t *hw = bal->bif->hw;
     uint8_t n = hw->num_cells;
     if (!n) return;
 
     /* Find min and max cell voltage */
     uint32_t vmin = hw->cell_mv[0];
     uint32_t vmax = hw->cell_mv[0];
     for (uint8_t i = 1; i < n; i++) {
         if (hw->cell_mv[i] < vmin) vmin = hw->cell_mv[i];
         if (hw->cell_mv[i] > vmax) vmax = hw->cell_mv[i];
     }
 
     uint32_t delta = vmax - vmin;
 
     if (delta < bal->stop_mv) {
         /* Within balance — stop any active balancing */
         if (bal->balance_mask) {
             bal->balance_mask = 0;
             uiox_bms_hw_set_balance(hw, 0u);
             uiox_bms_event_t ev = {
                 .type  = UIOX_BMS_EV_CELL_IMBALANCE,
                 .ts_ms = now_ms,
                 .pack_mv = hw->pack_mv,
                 .valid = true,
             };
             uiox_bms_event_push(&ev);
         }
         return;
     }
 
     if (delta < bal->delta_mv) return;
 
     /* Build balance mask: balance all cells above vmin + delta/2
      * up to UIOX_BMS_BAL_MAX_CELLS_SIMULT cells                     */
     uint16_t new_mask = 0;
     uint32_t threshold = vmin + delta / 2u;
     uint8_t  count = 0;
     for (uint8_t i = 0; i < n && count < UIOX_BMS_BAL_MAX_CELLS_SIMULT; i++) {
         if (hw->cell_mv[i] > threshold) {
             new_mask |= (uint16_t)(1u << i);
             count++;
         }
     }
 
     if (new_mask != bal->balance_mask) {
         bal->balance_mask = new_mask;
         uiox_bms_hw_set_balance(hw, new_mask);
         bal->bif->stats.balance_ops++;
         bal->bal_time_ms += 10u;  /* Approximate 10 ms per tick */
     }
 }
 
 void uiox_bms_bal_stop(uiox_bms_bal_t *bal)
 {
     if (!bal) return;
     bal->balance_mask = 0;
     uiox_bms_hw_set_balance(bal->bif->hw, 0u);
     bal->enabled = false;
 }
 
 bool uiox_bms_bal_active(const uiox_bms_bal_t *bal)
 { return bal ? (bal->balance_mask != 0u) : false; }
 