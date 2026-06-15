/**
 * @file  uiox_nvme_subsys.c
 * @brief UIOX NVMe Subsystem — health, APST, event dispatch.
 * @date  2026-06-12
 */

 #include "uiox_nvme_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_nvme_subsys_t *sys, uiox_nvme_ev_t ev,
                  const uiox_nvme_evt_t *data)
 { if (sys->evt_cb) sys->evt_cb(ev, data, sys->evt_ctx); }
 
 int uiox_nvme_subsys_init(uiox_nvme_subsys_t *sys, uiox_nvme_hw_t *hw)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     int rc = uiox_nvme_if_config(&sys->nif, hw);
     if (rc < 0) return rc;
     rc = uiox_nvme_proto_init(&sys->proto, &sys->nif);
     if (rc < 0) return rc;
     sys->state          = UIOX_NVME_STATE_OFF;
     sys->health_poll_ms = 0u;
     return 0;
 }
 
 int uiox_nvme_subsys_start(uiox_nvme_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_NVME_STATE_INIT;
     int rc = uiox_nvme_proto_ctrl_init(&sys->proto);
     if (rc < 0) { sys->state = UIOX_NVME_STATE_ERROR; return rc; }
     sys->state = UIOX_NVME_STATE_READY;
     /* Fire READY event */
     uiox_nvme_evt_t *e = uiox_nvme_evt_alloc();
     if (e) {
         e->type = UIOX_NVME_EVT_READY;
         fire(sys, UIOX_NVME_EV_READY, e);
         uiox_nvme_evt_free(e);
     }
     return 0;
 }
 
 void uiox_nvme_subsys_stop(uiox_nvme_subsys_t *sys)
 {
     if (!sys) return;
     if (sys->state == UIOX_NVME_STATE_READY) {
         /* Flush all active namespaces before shutdown */
         for (uint32_t i = 0u; i < UIOX_NVME_MAX_NAMESPACES; i++) {
             if (sys->nif.hw->ns[i].active)
                 uiox_nvme_proto_flush(&sys->proto, i + 1u);
         }
         uiox_nvme_proto_shutdown(&sys->proto);
     }
     uiox_nvme_if_stop(&sys->nif);
     sys->state = UIOX_NVME_STATE_OFF;
 }
 
 void uiox_nvme_subsys_tick(uiox_nvme_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state != UIOX_NVME_STATE_READY) return;
     sys->tick_count++;
     sys->uptime_ms      += 10u;
     sys->health_poll_ms += 10u;
 
     /* Check for controller fatal status */
     uint32_t csts = uiox_nvme_hw_reg_read32(sys->nif.hw, NVME_REG_CSTS);
     if (csts & NVME_CSTS_CFS) {
         sys->state = UIOX_NVME_STATE_FATAL;
         sys->error_count++;
         uiox_nvme_evt_t *e = uiox_nvme_evt_alloc();
         if (e) {
            e->type   = UIOX_NVME_EVT_FATAL;
            e->status = -EIO;
            e->timestamp_ms = now_ms;
            fire(sys, UIOX_NVME_EV_FATAL, e);
            uiox_nvme_evt_free(e);
        }
        return;
    }

    /* Handle pending IRQ events */
    uiox_nvme_evt_t *e = uiox_nvme_if_irq_handle(&sys->nif, now_ms);
    if (e) {
        if (e->type == UIOX_NVME_EVT_ERROR) {
            sys->error_count++;
            sys->state = UIOX_NVME_STATE_ERROR;
            fire(sys, UIOX_NVME_EV_ERROR, e);
        }
        uiox_nvme_evt_free(e);
    }

    /* Periodic SMART / health check every 60 s */
    if (sys->health_poll_ms >= UIOX_NVME_HEALTH_INTERVAL_MS) {
        sys->health_poll_ms = 0u;
        static uint8_t smart_buf[512];
        if (uiox_nvme_proto_smart_log(&sys->proto, smart_buf) == 0) {
            /* Critical Warning byte 0: bit 0 = spare below threshold,
             * bit 1 = temperature, bit 2 = NVM subsystem reliability,
             * bit 3 = media read-only, bit 4 = volatile mem backup fail */
            uint8_t warn = smart_buf[0];
            if (warn & 0x01u) {
                uiox_nvme_evt_t *he = uiox_nvme_evt_alloc();
                if (he) {
                    he->type         = UIOX_NVME_EVT_HEALTH_WARN;
                    he->timestamp_ms = now_ms;
                    he->status       = (int)warn;
                    fire(sys, UIOX_NVME_EV_HEALTH_WARN, he);
                    uiox_nvme_evt_free(he);
                }
            }
            if (warn & 0x02u) {
                uiox_nvme_evt_t *te = uiox_nvme_evt_alloc();
                if (te) {
                    te->type         = UIOX_NVME_EVT_TEMP_WARN;
                    te->timestamp_ms = now_ms;
                    fire(sys, UIOX_NVME_EV_TEMP_WARN, te);
                    uiox_nvme_evt_free(te);
                }
            }
        }
    }
}

void uiox_nvme_subsys_set_cb(uiox_nvme_subsys_t *sys,
                               uiox_nvme_evt_cb_t cb, void *ctx)
{ if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }

int uiox_nvme_subsys_read(uiox_nvme_subsys_t *sys,
                           uint32_t nsid, uint64_t slba,
                           uint8_t *buf, uint32_t nlb)
{
    if (!sys || sys->state != UIOX_NVME_STATE_READY) return -ENODEV;
    return uiox_nvme_proto_read(&sys->proto, nsid, slba, buf, nlb);
}

int uiox_nvme_subsys_write(uiox_nvme_subsys_t *sys,
                            uint32_t nsid, uint64_t slba,
                            const uint8_t *buf, uint32_t nlb)
{
    if (!sys || sys->state != UIOX_NVME_STATE_READY) return -ENODEV;
    return uiox_nvme_proto_write(&sys->proto, nsid, slba, buf, nlb);
}

int uiox_nvme_subsys_flush(uiox_nvme_subsys_t *sys, uint32_t nsid)
{
    if (!sys || sys->state != UIOX_NVME_STATE_READY) return -ENODEV;
    return uiox_nvme_proto_flush(&sys->proto, nsid);
}

int uiox_nvme_subsys_trim(uiox_nvme_subsys_t *sys,
                           uint32_t nsid, uint64_t slba, uint32_t nlb)
{
    if (!sys || sys->state != UIOX_NVME_STATE_READY) return -ENODEV;
    return uiox_nvme_proto_trim(&sys->proto, nsid, slba, nlb);
}
