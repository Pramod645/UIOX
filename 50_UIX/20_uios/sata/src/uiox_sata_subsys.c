/**
 * @file  uiox_sata_subsys.c
 * @brief UIOX SATA Subsystem — hotplug, power, event dispatch.
 * @date  2026-06-12
 */

 #include "uiox_sata_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_sata_subsys_t *sys, uiox_sata_ev_t ev,
                  const uiox_sata_evt_t *data)
 { if (sys->evt_cb) sys->evt_cb(ev, data, sys->evt_ctx); }
 
 int uiox_sata_subsys_init(uiox_sata_subsys_t *sys, uiox_sata_hw_t *hw)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     int rc = uiox_sata_if_config(&sys->sif, hw);
     if (rc < 0) return rc;
     rc = uiox_sata_proto_init(&sys->proto, &sys->sif);
     if (rc < 0) return rc;
     sys->state = UIOX_SATA_STATE_OFF;
     return 0;
 }
 
 int uiox_sata_subsys_start(uiox_sata_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_SATA_STATE_INIT;
     int rc = uiox_sata_if_start(&sys->sif);
     if (rc < 0) { sys->state = UIOX_SATA_STATE_ERROR; return rc; }
 
     /* Check if device is already attached */
     uint32_t ssts = uiox_sata_hw_px_read(sys->sif.hw,
                                            sys->sif.active_port,
                                            AHCI_PX_SSTS);
     uint8_t det = (uint8_t)(ssts & AHCI_PX_SSTS_DET_MASK);
     sys->sif.hw->dev_present = (det == AHCI_PX_SSTS_DET_COMM);
 
     if (sys->sif.hw->dev_present) {
         rc = uiox_sata_proto_dev_init(&sys->proto);
         sys->state = (rc == 0) ? UIOX_SATA_STATE_READY
                                 : UIOX_SATA_STATE_ERROR;
     } else {
         sys->state = UIOX_SATA_STATE_NO_DEV;
     }
     return 0;
 }
 
 void uiox_sata_subsys_stop(uiox_sata_subsys_t *sys)
 {
     if (!sys) return;
     /* Flush before stopping */
     if (sys->state == UIOX_SATA_STATE_READY)
         uiox_sata_proto_flush(&sys->proto);
     uiox_sata_if_stop(&sys->sif);
     sys->state = UIOX_SATA_STATE_OFF;
 }
 
 void uiox_sata_subsys_tick(uiox_sata_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_SATA_STATE_OFF) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 
     uiox_sata_evt_t *e = uiox_sata_if_irq_handle(&sys->sif, now_ms);
     if (!e) return;
 
     switch (e->type) {
     case UIOX_SATA_EVT_DEV_ATTACH:
         sys->attach_count++;
         sys->sif.hw->dev_present = true;
         sys->state = UIOX_SATA_STATE_INIT;
         fire(sys, UIOX_SATA_EV_DEV_ATTACH, e);
         if (uiox_sata_proto_dev_init(&sys->proto) == 0) {
             sys->state = UIOX_SATA_STATE_READY;
             fire(sys, UIOX_SATA_EV_DEV_READY, e);
         } else {
             sys->state = UIOX_SATA_STATE_ERROR;
             fire(sys, UIOX_SATA_EV_ERROR, e);
         }
         break;
 
     case UIOX_SATA_EVT_DEV_DETACH:
         sys->detach_count++;
         sys->sif.hw->dev_present = false;
         sys->proto.initialized   = false;
         sys->proto.init_state    = UIOX_SATA_INIT_IDLE;
         sys->state = UIOX_SATA_STATE_NO_DEV;
         fire(sys, UIOX_SATA_EV_DEV_DETACH, e);
         break;
 
     case UIOX_SATA_EVT_NCQ_DONE:
         /* Clear completed NCQ tags */
         sys->sif.hw->ncq_active = 0u;
         fire(sys, UIOX_SATA_EV_NCQ_DONE, e);
         break;
 
     case UIOX_SATA_EVT_ERROR:
         sys->error_count++;
         sys->state = UIOX_SATA_STATE_ERROR;
         fire(sys, UIOX_SATA_EV_ERROR, e);
         break;
 
     default:
         break;
     }
 
     uiox_sata_evt_free(e);
 }
 
 void uiox_sata_subsys_set_cb(uiox_sata_subsys_t *sys,
                                uiox_sata_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 
 int uiox_sata_subsys_read(uiox_sata_subsys_t *sys, uint64_t lba,
                            uint8_t *buf, uint32_t sectors)
 {
     if (!sys || sys->state != UIOX_SATA_STATE_READY) return -ENODEV;
     return uiox_sata_proto_read(&sys->proto, lba, buf, sectors);
 }
 
 int uiox_sata_subsys_write(uiox_sata_subsys_t *sys, uint64_t lba,
                             const uint8_t *buf, uint32_t sectors)
 {
     if (!sys || sys->state != UIOX_SATA_STATE_READY) return -ENODEV;
     return uiox_sata_proto_write(&sys->proto, lba, buf, sectors);
 }
 
 int uiox_sata_subsys_flush(uiox_sata_subsys_t *sys)
 {
     if (!sys || sys->state != UIOX_SATA_STATE_READY) return -ENODEV;
     return uiox_sata_proto_flush(&sys->proto);
 }
 
 int uiox_sata_subsys_trim(uiox_sata_subsys_t *sys,
                            uint64_t lba, uint32_t sectors)
 {
     if (!sys || sys->state != UIOX_SATA_STATE_READY) return -ENODEV;
     return uiox_sata_proto_trim(&sys->proto, lba, sectors);
 }
 