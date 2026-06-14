/**
 * @file  uiox_sd_device.c
 * @brief UIOX SD Card Reader application device API.
 * @date  2026-06-11
 */

 #include "uiox_sd_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_sd_open(uiox_sd_device_t *dev, const uiox_sd_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
     int rc  = uiox_sd_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
     rc = uiox_sd_subsys_init(&dev->subsys, p->hw);
     if (rc < 0) return rc;
     if (p->evt_cb)
         uiox_sd_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int  uiox_sd_start(uiox_sd_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sd_subsys_start(&dev->subsys); }
 
 void uiox_sd_stop(uiox_sd_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_sd_subsys_stop(&dev->subsys); }
 
 void uiox_sd_close(uiox_sd_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_sd_stop(dev);
   uiox_sd_hw_deinit(dev->hw);
   dev->open = false; }
 
 void uiox_sd_tick(uiox_sd_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return;
   uiox_sd_subsys_tick(&dev->subsys, now_ms); }
 
 int uiox_sd_read(uiox_sd_device_t *dev, uint32_t lba,
                   uint8_t *buf, uint32_t count)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sd_subsys_read(&dev->subsys, lba, buf, count); }
 
 int uiox_sd_write(uiox_sd_device_t *dev, uint32_t lba,
                    const uint8_t *buf, uint32_t count)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sd_subsys_write(&dev->subsys, lba, buf, count); }
 
 int uiox_sd_erase(uiox_sd_device_t *dev,
                    uint32_t lba_start, uint32_t lba_end)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sd_subsys_erase(&dev->subsys, lba_start, lba_end); }
 
 bool uiox_sd_is_present(const uiox_sd_device_t *dev)
 { return dev && dev->open && dev->hw->card_present; }
 
 bool uiox_sd_is_write_prot(const uiox_sd_device_t *dev)
 { return dev && dev->open && dev->hw->write_protect; }
 
 uint64_t uiox_sd_capacity_bytes(const uiox_sd_device_t *dev)
 { return (dev && dev->open) ? dev->hw->card.capacity_bytes : 0u; }
 
 uint64_t uiox_sd_capacity_blocks(const uiox_sd_device_t *dev)
 { return (dev && dev->open) ? dev->hw->card.capacity_blocks : 0u; }
 
 const uiox_sd_card_t *uiox_sd_card_info(const uiox_sd_device_t *dev)
 { return (dev && dev->open) ? &dev->hw->card : NULL; }
 
 void uiox_sd_print_info(const uiox_sd_device_t *dev)
 {
     if (!dev) return;
     const uiox_sd_hw_t    *hw = dev->hw;
     const uiox_sd_subsys_t *s = &dev->subsys;
     printf("  Model          : %s\n", hw->model);
     printf("  Bus type       : %s\n", uiox_sd_bus_name(hw->bus_type));
     printf("  MMIO base      : 0x%08lX\n", (unsigned long)hw->base);
     printf("  IRQ            : %u\n", hw->irq);
     printf("  Capabilities   : 0x%08X\n", hw->caps);
     printf("  State          : %s\n", uiox_sd_state_name(s->state));
     printf("  Card present   : %s\n", hw->card_present ? "YES" : "NO");
     printf("  Write protect  : %s\n", hw->write_protect ? "YES" : "NO");
     if (hw->card_present) {
         printf("  Card type      : %s\n",
                uiox_sd_card_type_name(hw->card.card_type));
         printf("  Capacity       : %llu MB\n",
                (unsigned long long)(hw->card.capacity_bytes >> 20u));
         printf("  Blocks         : %llu\n",
                (unsigned long long) hw->card.capacity_blocks);
         printf("  RCA            : 0x%04X\n", hw->card.rca);
         printf("  Bus width      : %u-bit\n", s->sif.bus_width);
         printf("  Transfer clock : %u Hz\n", hw->clk_xfer_hz);
     }
 }
 
 void uiox_sd_print_stats(uiox_sd_device_t *dev)
 {
     if (!dev) return;
     const uiox_sd_subsys_t *s = &dev->subsys;
     printf("  Uptime         : %llu ms\n", (unsigned long long)s->uptime_ms);
     printf("  Tick count     : %u\n",  s->tick_count);
     printf("  Card inserts   : %u\n",  s->insert_count);
     printf("  Card removes   : %u\n",  s->remove_count);
     printf("  Errors         : %u\n",  s->error_count);
     uiox_sd_if_stats_t is;
     uiox_sd_if_stats_get(&dev->subsys.sif, &is);
     printf("  Blocks read    : %llu\n", (unsigned long long)is.blocks_read);
     printf("  Blocks written : %llu\n", (unsigned long long)is.blocks_written);
     printf("  Bytes read     : %llu\n", (unsigned long long)is.bytes_read);
     printf("  Bytes written  : %llu\n", (unsigned long long)is.bytes_written);
     printf("  CMDs sent      : %u\n",   is.cmds_sent);
     printf("  CRC errors     : %u\n",   is.crc_errors);
     printf("  Timeouts       : %u\n",   is.timeout_errors);
     printf("  IRQ count      : %u\n",   is.irq_count);
     printf("  IF errors      : %u\n",   is.errors);
     printf("  Block pool free: %u / %u\n",
            uiox_sd_block_free_cnt(), UIOX_SD_BLOCK_POOL_SIZE);
     printf("  Cmd pool free  : %u / %u\n",
            uiox_sd_cmd_free_cnt(), UIOX_SD_CMD_POOL_SIZE);
     printf("  Evt pool free  : %u / %u\n",
            uiox_sd_evt_free_cnt(), UIOX_SD_EVT_POOL_SIZE);
 }
 
 const char *uiox_sd_state_name(uiox_sd_state_t s)
 {
     switch(s){
     case UIOX_SD_STATE_OFF:     return "OFF";
     case UIOX_SD_STATE_INIT:    return "INIT";
     case UIOX_SD_STATE_NO_CARD: return "NO_CARD";
     case UIOX_SD_STATE_READY:   return "READY";
     case UIOX_SD_STATE_ERROR:   return "ERROR";
     default:                     return "UNKNOWN";
     }
 }
 
 const char *uiox_sd_ev_name(uiox_sd_ev_t ev)
 {
     switch(ev){
     case UIOX_SD_EV_CARD_INSERT: return "CARD_INSERT";
     case UIOX_SD_EV_CARD_REMOVE: return "CARD_REMOVE";
     case UIOX_SD_EV_CARD_READY:  return "CARD_READY";
     case UIOX_SD_EV_READ_DONE:   return "READ_DONE";
     case UIOX_SD_EV_WRITE_DONE:  return "WRITE_DONE";
     case UIOX_SD_EV_WP_ACTIVE:   return "WP_ACTIVE";
     case UIOX_SD_EV_WP_CLEAR:    return "WP_CLEAR";
     case UIOX_SD_EV_ERROR:       return "ERROR";
     default:                      return "UNKNOWN";
     }
 }
 
 const char *uiox_sd_card_type_name(uiox_sd_card_type_t t)
 {
     switch(t){
     case UIOX_SD_CARD_NONE:    return "None";
     case UIOX_SD_CARD_SDSC:    return "SDSC (≤2 GB)";
     case UIOX_SD_CARD_SDHC:    return "SDHC (4–32 GB)";
     case UIOX_SD_CARD_SDXC:    return "SDXC (64 GB–2 TB)";
     case UIOX_SD_CARD_UNKNOWN: return "Unknown";
     default:                    return "?";
     }
 }
 
 const char *uiox_sd_bus_name(uiox_sd_bus_t b)
 {
     switch(b){
     case UIOX_SD_BUS_SDIO_1BIT: return "SDIO 1-bit";
     case UIOX_SD_BUS_SDIO_4BIT: return "SDIO 4-bit";
     case UIOX_SD_BUS_SPI:       return "SPI";
     default:                     return "Unknown";
     }
 }
 