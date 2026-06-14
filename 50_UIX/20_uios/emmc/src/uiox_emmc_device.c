/**
 * @file  uiox_emmc_device.c
 * @brief UIOX eMMC application device API.
 * @date  2026-06-12
 */

 #include "uiox_emmc_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_emmc_open(uiox_emmc_device_t *dev,
                     const uiox_emmc_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
     int rc  = uiox_emmc_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
     rc = uiox_emmc_subsys_init(&dev->subsys, p->hw);
     if (rc < 0) return rc;
     if (p->evt_cb)
         uiox_emmc_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int  uiox_emmc_start(uiox_emmc_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_emmc_subsys_start(&dev->subsys); }
 
 void uiox_emmc_stop(uiox_emmc_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_emmc_subsys_stop(&dev->subsys); }
 
 void uiox_emmc_close(uiox_emmc_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_emmc_stop(dev);
   uiox_emmc_hw_deinit(dev->hw);
   dev->open = false; }
 
 void uiox_emmc_tick(uiox_emmc_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return;
   uiox_emmc_subsys_tick(&dev->subsys, now_ms); }
 
 int uiox_emmc_read(uiox_emmc_device_t *dev, uiox_emmc_part_t part,
                     uint32_t lba, uint8_t *buf, uint32_t sectors)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_emmc_subsys_read(&dev->subsys, part, lba, buf, sectors); }
 
 int uiox_emmc_write(uiox_emmc_device_t *dev, uiox_emmc_part_t part,
                      uint32_t lba, const uint8_t *buf, uint32_t sectors)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_emmc_subsys_write(&dev->subsys, part, lba, buf, sectors); }
 
 int uiox_emmc_flush(uiox_emmc_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_emmc_subsys_flush(&dev->subsys); }
 
 int uiox_emmc_trim(uiox_emmc_device_t *dev,
                     uint32_t lba, uint32_t sectors)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_emmc_subsys_trim(&dev->subsys, lba, sectors); }
 
 int uiox_emmc_bkops(uiox_emmc_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_emmc_subsys_bkops(&dev->subsys); }
 
 int uiox_emmc_health(uiox_emmc_device_t *dev,
                       uint8_t *pre_eol,
                       uint8_t *life_a, uint8_t *life_b)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_emmc_proto_health_check(&dev->subsys.proto,
                                        pre_eol, life_a, life_b); }
 
 uint64_t uiox_emmc_capacity(const uiox_emmc_device_t *dev)
 { return (dev && dev->open)
          ? dev->hw->ident.capacity_bytes : 0u; }
 
 const uiox_emmc_ident_t *uiox_emmc_ident(const uiox_emmc_device_t *dev)
 { return (dev && dev->open) ? &dev->hw->ident : NULL; }
 
 void uiox_emmc_print_info(const uiox_emmc_device_t *dev)
 {
     if (!dev) return;
     const uiox_emmc_hw_t    *hw = dev->hw;
     const uiox_emmc_subsys_t *s = &dev->subsys;
     const uiox_emmc_ident_t  *id = &hw->ident;
     printf("  Controller     : %s\n", hw->model);
     printf("  MMIO base      : 0x%08lX\n", (unsigned long)hw->base);
     printf("  IRQ            : %u\n",   hw->irq);
     printf("  Capabilities   : 0x%08X\n", hw->caps);
     printf("  State          : %s\n",   uiox_emmc_state_name(s->state));
     printf("  Speed mode     : %s\n",
            uiox_emmc_speed_name(hw->speed));
     printf("  Bus width      : %u-bit\n", hw->bus_width);
     printf("  Clock          : %u Hz\n",  hw->clk_hz);
     printf("  Active part    : %s\n",
            uiox_emmc_part_name(hw->active_part));
     if (hw->dev_ready) {
         printf("  Product name   : %.6s\n", id->product_name);
         printf("  Capacity       : %llu GB\n",
                (unsigned long long)(id->capacity_bytes >> 30u));
         printf("  Cache          : %u KB  enabled=%s\n",
                id->cache_size_kb,
                hw->cache_enabled ? "YES" : "NO");
         printf("  TRIM           : %s\n",
                id->trim_supported ? "YES" : "NO");
         printf("  BKOPS          : %s\n",
                id->bkops_supported ? "YES" : "NO");
         printf("  Pre-EOL info   : %u (%s)\n",
                id->pre_eol_info,
                id->pre_eol_info == EXT_CSD_PRE_EOL_NORMAL  ? "NORMAL"  :
                id->pre_eol_info == EXT_CSD_PRE_EOL_WARNING ? "WARNING" :
                id->pre_eol_info == EXT_CSD_PRE_EOL_URGENT  ? "URGENT"  :
                "UNKNOWN");
         printf("  Life est A     : %u / 10\n", id->life_est_a);
         printf("  Life est B     : %u / 10\n", id->life_est_b);
         printf("  Partitions:\n");
         for (uint8_t i = 0u; i < UIOX_EMMC_MAX_PARTS; i++) {
             if (id->parts[i].enabled)
                 printf("    [%u] %-8s  %llu KB\n",
                        i, uiox_emmc_part_name(id->parts[i].id),
                        (unsigned long long)
                        (id->parts[i].size_bytes >> 10u));
         }
     }
 }
 
 void uiox_emmc_print_stats(uiox_emmc_device_t *dev)
 {
     if (!dev) return;
     const uiox_emmc_subsys_t *s = &dev->subsys;
     printf("  Uptime         : %llu ms\n",
            (unsigned long long)s->uptime_ms);
     printf("  Tick count     : %u\n", s->tick_count);
     printf("  Flush count    : %u\n", s->flush_count);
     printf("  Errors         : %u\n", s->error_count);
     uiox_emmc_if_stats_t is;
     uiox_emmc_if_stats_get(&dev->subsys.eif, &is);
     printf("  Blocks read    : %llu\n",
            (unsigned long long)is.blocks_read);
     printf("  Blocks written : %llu\n",
            (unsigned long long)is.blocks_written);
     printf("  Bytes read     : %llu\n",
            (unsigned long long)is.bytes_read);
     printf("  Bytes written  : %llu\n",
            (unsigned long long)is.bytes_written);
     printf("  CMDs sent      : %u\n", is.cmds_sent);
     printf("  CRC errors     : %u\n", is.crc_errors);
     printf("  IRQ count      : %u\n", is.irq_count);
     printf("  IF errors      : %u\n", is.errors);
     printf("  Blk pool free  : %u / %u\n",
            uiox_emmc_blk_free_cnt(), UIOX_EMMC_BLK_POOL_SIZE);
     printf("  Cmd pool free  : %u / %u\n",
            uiox_emmc_cmd_free_cnt(), UIOX_EMMC_CMD_POOL_SIZE);
     printf("  Evt pool free  : %u / %u\n",
            uiox_emmc_evt_free_cnt(), UIOX_EMMC_EVT_POOL_SIZE);
 }
 
 const char *uiox_emmc_state_name(uiox_emmc_state_t s)
 {
     switch (s) {
     case UIOX_EMMC_STATE_OFF:   return "OFF";
     case UIOX_EMMC_STATE_INIT:  return "INIT";
     case UIOX_EMMC_STATE_READY: return "READY";
     case UIOX_EMMC_STATE_ERROR: return "ERROR";
     default:                     return "UNKNOWN";
     }
 }
 
 const char *uiox_emmc_ev_name(uiox_emmc_ev_t ev)
 {
     switch (ev) {
     case UIOX_EMMC_EV_READY:        return "READY";
     case UIOX_EMMC_EV_READ_DONE:    return "READ_DONE";
     case UIOX_EMMC_EV_WRITE_DONE:   return "WRITE_DONE";
     case UIOX_EMMC_EV_FLUSH_DONE:   return "FLUSH_DONE";
     case UIOX_EMMC_EV_HEALTH_WARN:  return "HEALTH_WARN";
     case UIOX_EMMC_EV_EOL_WARN:     return "EOL_WARN";
     case UIOX_EMMC_EV_BKOPS_NEEDED: return "BKOPS_NEEDED";
     case UIOX_EMMC_EV_PART_SWITCH:  return "PART_SWITCH";
     case UIOX_EMMC_EV_ERROR:        return "ERROR";
     default:                          return "UNKNOWN";
     }
 }
 
 const char *uiox_emmc_speed_name(uiox_emmc_speed_t s)
 {
     switch (s) {
     case UIOX_EMMC_SPEED_IDENT:   return "IDENT (400 kHz)";
     case UIOX_EMMC_SPEED_DS:      return "DS (25 MHz)";
     case UIOX_EMMC_SPEED_HS52:    return "HS52 (52 MHz)";
     case UIOX_EMMC_SPEED_HS200:   return "HS200 (200 MHz SDR)";
     case UIOX_EMMC_SPEED_HS400:   return "HS400 (200 MHz DDR)";
     case UIOX_EMMC_SPEED_HS400ES: return "HS400ES (Enhanced Strobe)";
     default:                       return "Unknown";
     }
 }
 
 const char *uiox_emmc_part_name(uiox_emmc_part_t p)
 {
     switch (p) {
     case UIOX_EMMC_PART_USER:  return "USER";
     case UIOX_EMMC_PART_BOOT1: return "BOOT1";
     case UIOX_EMMC_PART_BOOT2: return "BOOT2";
     case UIOX_EMMC_PART_RPMB:  return "RPMB";
     case UIOX_EMMC_PART_GP1:   return "GP1";
     case UIOX_EMMC_PART_GP2:   return "GP2";
     case UIOX_EMMC_PART_GP3:   return "GP3";
     case UIOX_EMMC_PART_GP4:   return "GP4";
     default:                    return "UNKNOWN";
     }
 }
 