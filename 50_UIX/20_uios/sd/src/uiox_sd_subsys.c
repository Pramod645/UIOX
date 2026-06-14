/**
 * @file  uiox_sd_subsys.c
 * @brief UIOX SD Subsystem — hotplug, write-protect, event dispatch.
 * @date  2026-06-11
 */

 #include "uiox_sd_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_sd_subsys_t *sys, uiox_sd_ev_t ev,
                  const uiox_sd_evt_t *data)
 { if (sys->evt_cb) sys->evt_cb(ev, data, sys->evt_ctx); }
 
 int uiox_sd_subsys_init(uiox_sd_subsys_t *sys, uiox_sd_hw_t *hw)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     int rc = uiox_sd_if_config(&sys->sif, hw);
     if (rc < 0) return rc;
     rc = uiox_sd_proto_init(&sys->proto, &sys->sif);
     if (rc < 0) return rc;
     sys->state   = UIOX_SD_STATE_OFF;
     sys->wp_last = false;
     return 0;
 }
 
 int uiox_sd_subsys_start(uiox_sd_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_SD_STATE_INIT;
     int rc = uiox_sd_if_start(&sys->sif);
     if (rc < 0) { sys->state = UIOX_SD_STATE_ERROR; return rc; }
 
     /* Check if card is already inserted at startup */
     uint32_t ps = uiox_sd_hw_reg_read(sys->sif.hw, SDIO_REG_PRESENT_STATE);
     sys->sif.hw->card_present  = !!(ps & SDIO_PS_CARD_INSERTED);
     sys->sif.hw->write_protect = !!(ps & SDIO_PS_WRITE_PROTECT);
     sys->wp_last = sys->sif.hw->write_protect;
 
     if (sys->sif.hw->card_present) {
         rc = uiox_sd_proto_card_init(&sys->proto);
         sys->state = (rc == 0) ? UIOX_SD_STATE_READY
                                 : UIOX_SD_STATE_ERROR;
     } else {
         sys->state = UIOX_SD_STATE_NO_CARD;
     }
     return 0;
 }
 
 void uiox_sd_subsys_stop(uiox_sd_subsys_t *sys)
 {
     if (!sys) return;
     uiox_sd_if_stop(&sys->sif);
     sys->state = UIOX_SD_STATE_OFF;
 }
 
 void uiox_sd_subsys_tick(uiox_sd_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_SD_STATE_OFF) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 
     /* Poll for IRQ events */
     uiox_sd_evt_t *e = uiox_sd_if_irq_handle(&sys->sif, now_ms);
     if (e) {
         switch (e->type) {
         case UIOX_SD_EVT_CARD_INSERT:
             sys->insert_count++;
             sys->sif.hw->card_present = true;
             sys->state = UIOX_SD_STATE_INIT;
             fire(sys, UIOX_SD_EV_CARD_INSERT, e);
             /* Initialise card */
             if (uiox_sd_proto_card_init(&sys->proto) == 0) {
                 sys->state = UIOX_SD_STATE_READY;
                 fire(sys, UIOX_SD_EV_CARD_READY, e);
             } else {
                 sys->state = UIOX_SD_STATE_ERROR;
                 fire(sys, UIOX_SD_EV_ERROR, e);
             }
             break;
 
         case UIOX_SD_EVT_CARD_REMOVE:
             sys->remove_count++;
             sys->sif.hw->card_present = false;
             sys->proto.initialized    = false;
             sys->state = UIOX_SD_STATE_NO_CARD;
             fire(sys, UIOX_SD_EV_CARD_REMOVE, e);
             break;
 
         case UIOX_SD_EVT_ERROR:
             sys->error_count++;
             sys->state = UIOX_SD_STATE_ERROR;
             fire(sys, UIOX_SD_EV_ERROR, e);
             break;
 
         default:
             break;
         }
         uiox_sd_evt_free(e);
     }
 
     /* Poll write-protect change */
     if (sys->state == UIOX_SD_STATE_READY) {
         uint32_t ps = uiox_sd_hw_reg_read(sys->sif.hw,
                                             SDIO_REG_PRESENT_STATE);
         bool wp = !!(ps & SDIO_PS_WRITE_PROTECT);
         if (wp != sys->wp_last) {
             sys->wp_last = wp;
             sys->sif.hw->write_protect = wp;
             fire(sys, wp ? UIOX_SD_EV_WP_ACTIVE
                          : UIOX_SD_EV_WP_CLEAR, NULL);
         }
     }
 }
 
 void uiox_sd_subsys_set_cb(uiox_sd_subsys_t *sys,
                              uiox_sd_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 
 int uiox_sd_subsys_read(uiox_sd_subsys_t *sys, uint32_t lba,
                          uint8_t *buf, uint32_t count)
 {
     if (!sys || sys->state != UIOX_SD_STATE_READY) return -ENODEV;
     return uiox_sd_proto_read(&sys->proto, lba, buf, count);
 }
 
 int uiox_sd_subsys_write(uiox_sd_subsys_t *sys, uint32_t lba,
                           const uint8_t *buf, uint32_t count)
 {
     if (!sys || sys->state != UIOX_SD_STATE_READY) return -ENODEV;
     if (sys->sif.hw->write_protect) return -EROFS;
     return uiox_sd_proto_write(&sys->proto, lba, buf, count);
 }
 
 int uiox_sd_subsys_erase(uiox_sd_subsys_t *sys,
                           uint32_t lba_start, uint32_t lba_end)
 {
     if (!sys || sys->state != UIOX_SD_STATE_READY) return -ENODEV;
     if (sys->sif.hw->write_protect) return -EROFS;
     return uiox_sd_proto_erase(&sys->proto, lba_start, lba_end);
 }
 