/**
 * @file    uiox_ram_subsys.c
 * @brief   UIOX RAM subsystem implementation.
 * @date    2026-06-03
 */

 #include "uiox_ram_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_ram_subsys_t *sys, uiox_ram_evt_t evt)
 { if (sys->evt_cb) sys->evt_cb(evt, sys->evt_ctx); }
 
 int uiox_ram_subsys_init(uiox_ram_subsys_t *sys,
                           uiox_ram_hw_t     *hw,
                           void *heap_base,  size_t heap_size,
                           void *buddy_base, size_t buddy_size,
                           void *buddy_bmap, size_t buddy_bmap_size)
 {
     (void)buddy_bmap_size;
     if (!sys || !hw || !heap_base || !heap_size) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     int rc = uiox_ram_if_config(&sys->rif, hw);
     if (rc < 0) return rc;
 
     rc = uiox_ram_ecc_init(&sys->ecc, &sys->rif);
     if (rc < 0) return rc;
 
     rc = uiox_ram_mgr_init(&sys->mgr, &sys->ecc,
                             heap_base, heap_size,
                             buddy_base, buddy_size);
     if (rc < 0) return rc;
 
     /* Register main memory region */
     uiox_ram_if_add_region(&sys->rif, hw->base_phys, hw->total_bytes,
                             UIOX_RAM_REGION_HEAP, true, true, false);
 
     sys->state = UIOX_RAM_SUBSYS_STOPPED;
     return 0;
 }
 
 int uiox_ram_subsys_start(uiox_ram_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_ram_if_start(&sys->rif);
     if (rc < 0) return rc;
 
     /* Start ECC background scrub over entire RAM */
     uiox_ram_hw_t *hw = sys->rif.hw;
     uiox_ram_ecc_start_scrub(&sys->ecc,
                               hw->base_phys, hw->total_bytes,
                               64u * 1024u);
 
     sys->state = UIOX_RAM_SUBSYS_RUNNING;
     fire(sys, UIOX_RAM_EVT_INIT_DONE);
     return 0;
 }
 
 void uiox_ram_subsys_stop(uiox_ram_subsys_t *sys)
 {
     if (!sys) return;
     uiox_ram_if_stop(&sys->rif);
     sys->state = UIOX_RAM_SUBSYS_STOPPED;
     fire(sys, UIOX_RAM_EVT_POWER_CHANGE);
 }
 
 void uiox_ram_subsys_tick(uiox_ram_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_RAM_SUBSYS_STOPPED) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;  /* Assume 10ms tick */
 
     uiox_ram_if_periodic(&sys->rif, now_ms);
     uiox_ram_ecc_tick(&sys->ecc, now_ms);
 
     /* ECC events */
     if (sys->ecc.total_ce > 0 &&
         sys->ecc.total_ce != sys->rif.hw->ecc_ce_count) {
         sys->rif.hw->ecc_ce_count = sys->ecc.total_ce;
         fire(sys, UIOX_RAM_EVT_ECC_CE);
     }
     if (sys->ecc.total_ue > 0 &&
         sys->ecc.total_ue != sys->rif.hw->ecc_ue_count) {
         sys->rif.hw->ecc_ue_count = sys->ecc.total_ue;
         sys->state = UIOX_RAM_SUBSYS_ERROR;
         fire(sys, UIOX_RAM_EVT_ECC_UE);
     }
 
     /* ECC scrub done */
     if (!sys->ecc.scrub_running && sys->ecc.scrub_pos >= sys->ecc.scrub_end
         && sys->ecc.scrub_end > 0)
         fire(sys, UIOX_RAM_EVT_SCRUB_DONE);
 
     /* Low memory alert */
     size_t h_used = 0, h_free = 0;
     uiox_heap_stats(&sys->mgr.heap, &h_used, &h_free, NULL);
     uint64_t total = sys->rif.hw->total_bytes;
     if (total && (h_free * 100u / total) < UIOX_RAM_LOW_MEM_THRESHOLD_PCT) {
         sys->state = UIOX_RAM_SUBSYS_LOW_MEM;
         fire(sys, UIOX_RAM_EVT_LOW_MEM);
     }
 }
 
 void uiox_ram_subsys_set_cb(uiox_ram_subsys_t *sys,
                               uiox_ram_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 
 void *uiox_ram_subsys_alloc(uiox_ram_subsys_t *sys, size_t size)
 { return sys ? uiox_heap_alloc(&sys->mgr.heap, size) : NULL; }
 
 void *uiox_ram_subsys_calloc(uiox_ram_subsys_t *sys, size_t n, size_t sz)
 { return sys ? uiox_heap_calloc(&sys->mgr.heap, n, sz) : NULL; }
 
 void  uiox_ram_subsys_free(uiox_ram_subsys_t *sys, void *ptr)
 { if (sys && ptr) uiox_heap_free(&sys->mgr.heap, ptr); }
 
 void *uiox_ram_subsys_buddy(uiox_ram_subsys_t *sys, size_t size)
 { return sys ? uiox_buddy_alloc(&sys->mgr.buddy, size) : NULL; }
 
 void  uiox_ram_subsys_bfree(uiox_ram_subsys_t *sys, void *ptr, size_t size)
 { if (sys && ptr) uiox_buddy_free(&sys->mgr.buddy, ptr, size); }
 
 void uiox_ram_subsys_info(const uiox_ram_subsys_t *sys,
                            size_t *heap_used, size_t *heap_free,
                            size_t *buddy_used, size_t *buddy_free)
 {
     if (!sys) return;
     uiox_heap_stats(&sys->mgr.heap, heap_used, heap_free, NULL);
     uiox_buddy_stats(&sys->mgr.buddy, buddy_used, buddy_free);
 }
 