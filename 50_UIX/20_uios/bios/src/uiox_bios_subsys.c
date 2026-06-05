/**
 * @file    uiox_bios_subsys.c
 * @brief   UIOX BIOS subsystem implementation.
 * @date    2026-06-04
 */

 #include "uiox_bios_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_bios_subsys_t *sys, uiox_bios_evt_t evt)
 { if (sys->evt_cb) sys->evt_cb(evt, sys->evt_ctx); }
 
 int uiox_bios_subsys_init(uiox_bios_subsys_t *sys,
                            uiox_bios_hw_t     *hw,
                            uint32_t            nvram_flash_offset)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     int rc = uiox_bios_if_config(&sys->bif, hw);
     if (rc < 0) return rc;
 
     rc = uiox_bios_nvram_init(&sys->nvram, &sys->bif, nvram_flash_offset);
     if (rc < 0) return rc;
 
     rc = uiox_bios_svc_init(&sys->svc, &sys->nvram);
     if (rc < 0) return rc;
 
     sys->state = UIOX_BIOS_SUBSYS_STOPPED;
     return 0;
 }
 
 int uiox_bios_subsys_start(uiox_bios_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_BIOS_SUBSYS_POST;
 
     int rc = uiox_bios_svc_post(&sys->svc);
     if (rc < 0) {
         sys->state = UIOX_BIOS_SUBSYS_ERROR;
         fire(sys, UIOX_BIOS_EVT_POST_ERROR);
         return rc;
     }
 
     uiox_bios_svc_build_memmap(&sys->svc, 0x100000ULL,
                                 (uint64_t)sys->svc.total_ram_mb * 1024ULL * 1024ULL - 0x100000ULL);
 
     sys->state = UIOX_BIOS_SUBSYS_RUNTIME;
     fire(sys, UIOX_BIOS_EVT_POST_DONE);
     fire(sys, UIOX_BIOS_EVT_BOOT_SELECT);
     return 0;
 }
 
 void uiox_bios_subsys_stop(uiox_bios_subsys_t *sys)
 {
     if (!sys) return;
     uiox_bios_nvram_flush(&sys->nvram);
     sys->state = UIOX_BIOS_SUBSYS_STOPPED;
 }
 
 void uiox_bios_subsys_tick(uiox_bios_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_BIOS_SUBSYS_STOPPED) return;
     (void)now_ms;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 }
 
 int uiox_bios_subsys_update(uiox_bios_subsys_t *sys,
                              uint32_t flash_offset,
                              const void *image, uint32_t image_size)
 {
     if (!sys || !image || !image_size) return -EINVAL;
     sys->state = UIOX_BIOS_SUBSYS_UPDATE;
     fire(sys, UIOX_BIOS_EVT_FLASH_WRITE_START);
 
     int rc = uiox_bios_if_write(&sys->bif, flash_offset, image, image_size);
     if (rc < 0) {
         sys->state = UIOX_BIOS_SUBSYS_ERROR;
         fire(sys, UIOX_BIOS_EVT_FLASH_WRITE_ERROR);
         return rc;
     }
 
     sys->state = UIOX_BIOS_SUBSYS_RUNTIME;
     fire(sys, UIOX_BIOS_EVT_FLASH_WRITE_DONE);
     return 0;
 }
 
 int uiox_bios_subsys_verify(uiox_bios_subsys_t *sys,
                              uint32_t flash_offset,
                              const void *image, uint32_t image_size)
 {
     if (!sys || !image) return -EINVAL;
     return uiox_bios_if_verify(&sys->bif, flash_offset, image, image_size);
 }
 
 void uiox_bios_subsys_set_cb(uiox_bios_subsys_t *sys,
                                uiox_bios_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 