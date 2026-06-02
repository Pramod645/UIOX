/**
 * @file    uiox_cpu_subsys.c
 * @brief   UIOX CPU subsystem implementation.
 * @date    2026-06-02
 */

 #include "uiox_cpu_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_cpu_subsys_t *sys, uiox_cpu_evt_t evt, uint8_t core)
 {
     if (sys->evt_cb) sys->evt_cb(evt, core, sys->evt_ctx);
 }
 
 int uiox_cpu_subsys_init(uiox_cpu_subsys_t *sys,
                           uiox_cpu_hw_t     *hw,
                           uint32_t           timer_interval_ns)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     uiox_cpu_buf_init(hw->num_cores);
 
     int rc = uiox_cpu_if_init(&sys->cif, hw);
     if (rc < 0) return rc;
 
     rc = uiox_cpu_if_vectors_init(&sys->cif);
     if (rc < 0) return rc;
 
     rc = uiox_cpu_pm_init(&sys->pm, &sys->cif);
     if (rc < 0) return rc;
 
     rc = uiox_cpu_feat_init(&sys->feat, hw);
     if (rc < 0) return rc;
 
     rc = uiox_cpu_feat_detect(&sys->feat);
     if (rc < 0) return rc;
 
     rc = uiox_cpu_if_timer_init(&sys->cif, timer_interval_ns);
     if (rc < 0) return rc;
 
     /* Default OPPs (added before start) */
     uiox_cpu_pm_add_opp(&sys->pm, 600u,  800000u, 1000u);
     uiox_cpu_pm_add_opp(&sys->pm, 1200u, 900000u, 2500u);
     uiox_cpu_pm_add_opp(&sys->pm, 1800u,1000000u, 5000u);
     uiox_cpu_pm_add_opp(&sys->pm, 2400u,1100000u, 9000u);
     uiox_cpu_pm_add_opp(&sys->pm, 3000u,1200000u,15000u);
 
     /* Boot core 0 is online */
     hw->cores[0].state = UIOX_CPU_STATE_RUNNING;
     uiox_percpu[0].state = UIOX_CPU_STATE_RUNNING;
 
     sys->state   = UIOX_CPU_SUBSYS_BOOTING;
     sys->tick_ms = timer_interval_ns / 1000000u;
     if (sys->tick_ms == 0) sys->tick_ms = 1u;
     return 0;
 }
 
 int uiox_cpu_subsys_start(uiox_cpu_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_CPU_SUBSYS_RUNNING;
     fire(sys, UIOX_CPU_EVT_CORE_ONLINE, 0u);
     return 0;
 }
 
 void uiox_cpu_subsys_stop(uiox_cpu_subsys_t *sys)
 {
     if (!sys) return;
     sys->state = UIOX_CPU_SUBSYS_STOPPED;
     /* Offline all secondary cores */
     for (uint8_t i = 1; i < sys->cif.hw->num_cores; i++) {
         if (sys->cif.hw->cores[i].state != UIOX_CPU_STATE_OFFLINE) {
             const uiox_cpu_hw_ops_t *ops =
                 (const uiox_cpu_hw_ops_t *)sys->cif.hw->priv;
             if (ops && ops->core_powerdown)
                 ops->core_powerdown(sys->cif.hw, i);
             sys->cif.hw->cores[i].state = UIOX_CPU_STATE_OFFLINE;
             uiox_percpu[i].state        = UIOX_CPU_STATE_OFFLINE;
         }
     }
 }
 
 void uiox_cpu_subsys_tick(uiox_cpu_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state != UIOX_CPU_SUBSYS_RUNNING) return;
     sys->uptime_ms += sys->tick_ms;
     sys->timer_ticks++;
 
     /* Fire timer tick event */
     fire(sys, UIOX_CPU_EVT_TIMER_TICK, 0u);
 
     /* DVFS / thermal management tick */
     uiox_cpu_pm_tick(&sys->pm, now_ms);
 
     /* Check for thermal throttle event */
     if (sys->pm.thermal_limit_hit)
         fire(sys, UIOX_CPU_EVT_THERMAL_THROTTLE, 0u);
 
     /* Drain pending IPI messages on boot core */
     for (uint8_t src = 1; src < sys->cif.hw->num_cores; src++) {
         uiox_ipi_ring_t *ring = &uiox_percpu[0].ipi_in[src];
         uiox_ipi_msg_t msg;
         while (uiox_ipi_pop(ring, &msg)) {
             sys->ipi_count++;
             if (msg.fn) msg.fn(msg.fn_ctx);
             fire(sys, UIOX_CPU_EVT_IPI_RECEIVED, src);
         }
     }
 
     /* Execute deferred work items on boot core */
     uiox_cpu_work_t *w;
     while ((w = uiox_work_dequeue(0u)) != NULL) {
         if (w->fn) w->fn(w->ctx);
         uiox_work_free(w);
     }
 }
 
 int uiox_cpu_subsys_core_up(uiox_cpu_subsys_t *sys,
                              uint8_t core_id, uintptr_t entry)
 {
     if (!sys || core_id >= sys->cif.hw->num_cores) return -EINVAL;
     int rc = uiox_cpu_if_smp_boot(&sys->cif, core_id, entry);
     if (rc == 0) {
         uiox_percpu[core_id].state     = UIOX_CPU_STATE_RUNNING;
         sys->cif.hw->cores[core_id].state = UIOX_CPU_STATE_ONLINE;
         fire(sys, UIOX_CPU_EVT_CORE_ONLINE, core_id);
     }
     return rc;
 }
 
 int uiox_cpu_subsys_core_down(uiox_cpu_subsys_t *sys, uint8_t core_id)
 {
     if (!sys || core_id == 0) return -EINVAL; /* Can't offline boot core */
     const uiox_cpu_hw_ops_t *ops =
         (const uiox_cpu_hw_ops_t *)sys->cif.hw->priv;
     if (ops && ops->core_powerdown)
         ops->core_powerdown(sys->cif.hw, core_id);
     sys->cif.hw->cores[core_id].state = UIOX_CPU_STATE_OFFLINE;
     uiox_percpu[core_id].state        = UIOX_CPU_STATE_OFFLINE;
     fire(sys, UIOX_CPU_EVT_CORE_OFFLINE, core_id);
     return 0;
 }
 
 int uiox_cpu_subsys_ipi(uiox_cpu_subsys_t *sys,
                          uint8_t target_core,
                          uiox_ipi_type_t type,
                          uint64_t arg0)
 {
     if (!sys || target_core >= sys->cif.hw->num_cores) return -EINVAL;
     /* Push into target's IPI ring from boot core (0) */
     uiox_ipi_msg_t msg = { .type = type, .arg0 = arg0 };
     bool ok = uiox_ipi_push(&uiox_percpu[target_core].ipi_in[0], &msg);
     if (!ok) return -ENOSPC;
     return uiox_cpu_hw_ipi_send(sys->cif.hw, target_core, (uint8_t)type);
 }
 
 void uiox_cpu_subsys_set_cb(uiox_cpu_subsys_t *sys,
                               uiox_cpu_evt_cb_t cb, void *ctx)
 {
     if (!sys) return;
     sys->evt_cb  = cb;
     sys->evt_ctx = ctx;
 }
 
 void uiox_cpu_subsys_set_gov(uiox_cpu_subsys_t *sys,
                               uiox_cpu_governor_t gov)
 {
     if (!sys) return;
     uiox_cpu_pm_set_governor(&sys->pm, gov);
 }
 