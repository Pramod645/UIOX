/**
 * @file  uiox_emmc_subsys.c
 * @brief UIOX eMMC Subsystem — health, cache, partition, event dispatch.
 * @date  2026-06-12
 */

 #include "uiox_emmc_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_emmc_subsys_t *sys, uiox_emmc_ev_t ev,
                  const uiox_emmc_evt_t *data)
 { if (sys->evt_cb) sys->evt_cb(ev, data, sys->evt_ctx); }
 
 int uiox_emmc_subsys_init(uiox_emmc_subsys_t *sys, uiox_emmc_hw_t *hw)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     int rc = uiox_emmc_if_config(&sys->eif, hw);
     if (rc < 0) return rc;
     rc = uiox_emmc_proto_init(&sys->proto, &sys->eif);
     if (rc < 0) return rc;
     sys->state           = UIOX_EMMC_STATE_OFF;
     sys->health_poll_ms  = 0u;
     return 0;
 }
 
 int uiox_emmc_subsys_start(uiox_emmc_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_EMMC_STATE_INIT;
     int rc = uiox_emmc_if_start(&sys->eif);
     if (rc < 0) { sys->state = UIOX_EMMC_STATE_ERROR; return rc; }
     rc = uiox_emmc_proto_dev_init(&sys->proto);
     if (rc < 0) { sys->state = UIOX_EMMC_STATE_ERROR; return rc; }
     sys->state = UIOX_EMMC_STATE_READY;
     /* Fire READY event */
     uiox_emmc_evt_t *e = uiox_emmc_evt_alloc();
     if (e) {
         e->type = UIOX_EMMC_EVT_READY;
         fire(sys, UIOX_EMMC_EV_READY, e);
         uiox_emmc_evt_free(e);
     }
     return 0;
 }
 
 void uiox_emmc_subsys_stop(uiox_emmc_subsys_t *sys)
 {
     if (!sys) return;
     /* Power-off notification before shutdown */
     if (sys->state == UIOX_EMMC_STATE_READY) {
         uiox_emmc_proto_flush(&sys->proto);
         uiox_emmc_proto_pon(&sys->proto);
     }
     uiox_emmc_if_stop(&sys->eif);
     sys->state = UIOX_EMMC_STATE_OFF;
 }
 
 void uiox_emmc_subsys_tick(uiox_emmc_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state != UIOX_EMMC_STATE_READY) return;
     sys->tick_count++;
     sys->uptime_ms      += 10u;
     sys->health_poll_ms += 10u;
 
     /* Handle pending IRQ events */
     uiox_emmc_evt_t *e = uiox_emmc_if_irq_handle(&sys->eif, now_ms);
     if (e) {
         switch (e->type) {
         case UIOX_EMMC_EVT_ERROR:
             sys->error_count++;
             sys->state = UIOX_EMMC_STATE_ERROR;
             fire(sys, UIOX_EMMC_EV_ERROR, e);
             break;
         case UIOX_EMMC_EVT_FLUSH_DONE:
             fire(sys, UIOX_EMMC_EV_FLUSH_DONE, e);
             break;
         default:
             break;
         }
         uiox_emmc_evt_free(e);
     }
 
     /* Periodic health check */
     if (sys->health_poll_ms >= UIOX_EMMC_HEALTH_INTERVAL_MS) {
         sys->health_poll_ms = 0u;
         uint8_t pre_eol = 0u, life_a = 0u, life_b = 0u;
         if (uiox_emmc_proto_health_check(&sys->proto,
                                           &pre_eol,
                                           &life_a, &life_b) == 0) {
             uiox_emmc_evt_t *he = uiox_emmc_evt_alloc();
             if (he) {
                 he->type         = UIOX_EMMC_EVT_HEALTH_WARN;
                 he->timestamp_ms = now_ms;
                 if (pre_eol == EXT_CSD_PRE_EOL_WARNING ||
                     pre_eol == EXT_CSD_PRE_EOL_URGENT) {
                     fire(sys, UIOX_EMMC_EV_EOL_WARN, he);
                 } else if (life_a >= 8u || life_b >= 8u) {
                     fire(sys, UIOX_EMMC_EV_HEALTH_WARN, he);
                 }
                 uiox_emmc_evt_free(he);
             }
         }
     }
 }
 
 void uiox_emmc_subsys_set_cb(uiox_emmc_subsys_t *sys,
                                uiox_emmc_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 
 int uiox_emmc_subsys_read(uiox_emmc_subsys_t *sys,
                            uiox_emmc_part_t part, uint32_t lba,
                            uint8_t *buf, uint32_t sectors)
 {
     if (!sys || sys->state != UIOX_EMMC_STATE_READY) return -ENODEV;
     return uiox_emmc_proto_read(&sys->proto, part, lba, buf, sectors);
 }
 
 int uiox_emmc_subsys_write(uiox_emmc_subsys_t *sys,
                             uiox_emmc_part_t part, uint32_t lba,
                             const uint8_t *buf, uint32_t sectors)
 {
     if (!sys || sys->state != UIOX_EMMC_STATE_READY) return -ENODEV;
     return uiox_emmc_proto_write(&sys->proto, part, lba, buf, sectors);
 }
 
 int uiox_emmc_subsys_flush(uiox_emmc_subsys_t *sys)
 {
     if (!sys || sys->state != UIOX_EMMC_STATE_READY) return -ENODEV;
     int rc = uiox_emmc_proto_flush(&sys->proto);
     if (rc == 0) sys->flush_count++;
     return rc;
 }
 
 int uiox_emmc_subsys_trim(uiox_emmc_subsys_t *sys,
                            uint32_t lba, uint32_t sectors)
 {
     if (!sys || sys->state != UIOX_EMMC_STATE_READY) return -ENODEV;
     return uiox_emmc_proto_trim(&sys->proto, lba, sectors);
 }
 
 int uiox_emmc_subsys_bkops(uiox_emmc_subsys_t *sys)
 {
     if (!sys || sys->state != UIOX_EMMC_STATE_READY) return -ENODEV;
     return uiox_emmc_proto_bkops(&sys->proto);
 }
 