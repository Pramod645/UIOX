/**
 * @file    uiox_ram_if.c
 * @brief   UIOX RAM interface driver implementation.
 * @date    2026-06-03
 */

 #include "uiox_ram_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_ram_if_config(uiox_ram_if_t *rif, uiox_ram_hw_t *hw)
 {
     if (!rif || !hw) return -EINVAL;
     memset(rif, 0, sizeof(*rif));
     rif->hw     = hw;
     rif->primed = true;
     uiox_ram_buf_init();
     return 0;
 }
 
 int uiox_ram_if_start(uiox_ram_if_t *rif)
 {
     if (!rif || !rif->primed) return -EINVAL;
     int rc = uiox_ram_hw_train(rif->hw);
     if (rc < 0) return rc;
     if (rif->hw->caps & UIOX_RAM_CAP_ECC)
         uiox_ram_hw_ecc_enable(rif->hw, true);
     if (rif->hw->caps & UIOX_RAM_CAP_ZQ_CAL) {
         uiox_ram_hw_zq_cal(rif->hw);
         rif->stats.zq_cal_count++;
     }
     return 0;
 }
 
 void uiox_ram_if_stop(uiox_ram_if_t *rif)
 {
     if (!rif) return;
     uiox_ram_hw_set_power(rif->hw, UIOX_RAM_PWR_SELF_REFRESH);
     rif->stats.power_transitions++;
 }
 
 int uiox_ram_if_add_region(uiox_ram_if_t *rif,
                             uint64_t phys_base, uint64_t size,
                             uiox_ram_region_type_t type,
                             bool cached, bool writable, bool exec)
 {
     if (!rif || !size) return -EINVAL;
     uiox_ram_region_t *r = uiox_ram_buf_alloc();
     if (!r) return -ENOMEM;
     r->phys_base  = phys_base;
     r->size       = size;
     r->type       = type;
     r->cached     = cached;
     r->writable   = writable;
     r->executable = exec;
     r->next       = rif->region_list;
     rif->region_list = r;
     rif->num_regions++;
     return 0;
 }
 
 uiox_ram_region_t *uiox_ram_if_find_region(const uiox_ram_if_t *rif,
                                              uint64_t phys)
 {
     if (!rif) return NULL;
     for (uiox_ram_region_t *r = rif->region_list; r; r = r->next)
         if (phys >= r->phys_base && phys < r->phys_base + r->size)
             return r;
     return NULL;
 }
 
 static uint32_t s_last_zq_ms = 0;
 
 void uiox_ram_if_periodic(uiox_ram_if_t *rif, uint32_t now_ms)
 {
     if (!rif || !rif->hw) return;
     /* ZQ calibration every 1 second */
     if ((rif->hw->caps & UIOX_RAM_CAP_ZQ_CAL) &&
         (now_ms - s_last_zq_ms) >= 1000u) {
         uiox_ram_hw_zq_cal(rif->hw);
         rif->stats.zq_cal_count++;
         s_last_zq_ms = now_ms;
     }
     rif->stats.refresh_count++;
 }
 
 void uiox_ram_if_stats_get(const uiox_ram_if_t *rif,
                             uiox_ram_if_stats_t *out)
 { if (!rif || !out) return; memcpy(out, &rif->stats, sizeof(*out)); }
 
 void uiox_ram_if_stats_reset(uiox_ram_if_t *rif)
 { if (!rif) return; memset(&rif->stats, 0, sizeof(rif->stats)); }
 