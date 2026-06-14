/**
 * @file  uiox_sata_device.c
 * @brief UIOX SATA application device API.
 * @date  2026-06-12
 */

 #include "uiox_sata_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_sata_open(uiox_sata_device_t *dev,
                     const uiox_sata_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
     int rc  = uiox_sata_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
     rc = uiox_sata_subsys_init(&dev->subsys, p->hw);
     if (rc < 0) return rc;
     if (p->evt_cb)
         uiox_sata_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int  uiox_sata_start(uiox_sata_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sata_subsys_start(&dev->subsys); }
 
 void uiox_sata_stop(uiox_sata_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_sata_subsys_stop(&dev->subsys); }
 
 void uiox_sata_close(uiox_sata_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_sata_stop(dev);
   uiox_sata_hw_deinit(dev->hw);
   dev->open = false; }
 
 void uiox_sata_tick(uiox_sata_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return;
   uiox_sata_subsys_tick(&dev->subsys, now_ms); }
 
 int uiox_sata_read(uiox_sata_device_t *dev, uint64_t lba,
                     uint8_t *buf, uint32_t sectors)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sata_subsys_read(&dev->subsys, lba, buf, sectors); }
 
 int uiox_sata_write(uiox_sata_device_t *dev, uint64_t lba,
                      const uint8_t *buf, uint32_t sectors)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sata_subsys_write(&dev->subsys, lba, buf, sectors); }
 
 int uiox_sata_flush(uiox_sata_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sata_subsys_flush(&dev->subsys); }
 
 int uiox_sata_trim(uiox_sata_device_t *dev,
                     uint64_t lba, uint32_t sectors)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_sata_subsys_trim(&dev->subsys, lba, sectors); }
 
 int uiox_sata_smart_read(uiox_sata_device_t *dev, uint8_t *buf)
 { if (!dev || !dev->open || !buf) return -EINVAL;
   return uiox_sata_proto_smart_read(&dev->subsys.proto, buf); }
 
 bool uiox_sata_is_present(const uiox_sata_device_t *dev)
 { return dev && dev->open && dev->hw->dev_present; }
 
 bool uiox_sata_is_ssd(const uiox_sata_device_t *dev)
 { return dev && dev->open && dev->hw->ident.is_ssd; }
 
 uint64_t uiox_sata_capacity(const uiox_sata_device_t *dev)
 { return (dev && dev->open) ? dev->hw->ident.capacity_bytes : 0u; }
 
 const uiox_sata_ident_t *uiox_sata_ident(const uiox_sata_device_t *dev)
 { return (dev && dev->open) ? &dev->hw->ident : NULL; }
 
 void uiox_sata_print_info(const uiox_sata_device_t *dev)
 {
     if (!dev) return;
     const uiox_sata_hw_t    *hw = dev->hw;
     const uiox_sata_subsys_t *s = &dev->subsys;
     printf("  Controller     : %s\n", hw->model);
     printf("  Ctrl type      : %s\n", uiox_sata_ctrl_name(hw->ctrl_type));
     printf("  BAR5           : 0x%08lX\n", (unsigned long)hw->bar5);
     printf("  IRQ            : %u\n", hw->irq);
     printf("  Port           : %u\n", hw->port);
     printf("  Capabilities   : 0x%08X\n", hw->caps);
     printf("  State          : %s\n", uiox_sata_state_name(s->state));
     printf("  Dev present    : %s\n", hw->dev_present ? "YES" : "NO");
     if (hw->dev_present) {
         const uiox_sata_ident_t *id = &hw->ident;
         printf("  Model          : %.40s\n", id->model_str);
         printf("  Serial         : %.20s\n", id->serial_str);
         printf("  Firmware       : %.8s\n",  id->fw_str);
         printf("  Capacity       : %llu GB\n",
                (unsigned long long)(id->capacity_bytes >> 30u));
         printf("  Type           : %s\n",
                id->is_ssd ? "SSD" : "HDD");
         printf("  RPM            : %u\n", id->rpm);
         printf("  NCQ depth      : %u\n", id->ncq_depth);
         printf("  NCQ enabled    : %s\n",
                s->proto.ncq_enabled ? "YES" : "NO");
         printf("  TRIM           : %s\n",
                id->trim_supported ? "YES" : "NO");
         printf("  SMART          : %s\n",
                id->smart_supported ? "YES" : "NO");
     }
 }
 
 void uiox_sata_print_stats(uiox_sata_device_t *dev)
 {
     if (!dev) return;
     const uiox_sata_subsys_t *s = &dev->subsys;
     printf("  Uptime         : %llu ms\n",
            (unsigned long long)s->uptime_ms);
     printf("  Tick count     : %u\n",  s->tick_count);
     printf("  Dev attaches   : %u\n",  s->attach_count);
     printf("  Dev detaches   : %u\n",  s->detach_count);
     printf("  Errors         : %u\n",  s->error_count);
     uiox_sata_if_stats_t is;
     uiox_sata_if_stats_get(&dev->subsys.sif, &is);
     printf("  Sectors read   : %llu\n",
            (unsigned long long)is.sectors_read);
     printf("  Sectors written: %llu\n",
            (unsigned long long)is.sectors_written);
     printf("  Bytes read     : %llu\n",
            (unsigned long long)is.bytes_read);
     printf("  Bytes written  : %llu\n",
            (unsigned long long)is.bytes_written);
     printf("  CMDs issued    : %u\n",  is.cmds_issued);
     printf("  NCQ CMDs issued: %u\n",  is.ncq_cmds_issued);
     printf("  IRQ count      : %u\n",  is.irq_count);
     printf("  Resets         : %u\n",  is.resets);
     printf("  IF errors      : %u\n",  is.errors);
     printf("  Cmd pool free  : %u / %u\n",
            uiox_sata_cmd_free_cnt(), UIOX_SATA_CMD_POOL_SIZE);
     printf("  Blk pool free  : %u / %u\n",
            uiox_sata_blk_free_cnt(), UIOX_SATA_BLK_POOL_SIZE);
     printf("  Evt pool free  : %u / %u\n",
            uiox_sata_evt_free_cnt(), UIOX_SATA_EVT_POOL_SIZE);
 }
 
 const char *uiox_sata_state_name(uiox_sata_state_t s)
 {
     switch (s) {
     case UIOX_SATA_STATE_OFF:    return "OFF";
     case UIOX_SATA_STATE_INIT:   return "INIT";
     case UIOX_SATA_STATE_NO_DEV: return "NO_DEV";
     case UIOX_SATA_STATE_READY:  return "READY";
     case UIOX_SATA_STATE_ERROR:  return "ERROR";
     default:                      return "UNKNOWN";
     }
 }
 
 const char *uiox_sata_ev_name(uiox_sata_ev_t ev)
 {
     switch (ev) {
     case UIOX_SATA_EV_DEV_ATTACH:  return "DEV_ATTACH";
     case UIOX_SATA_EV_DEV_DETACH:  return "DEV_DETACH";
     case UIOX_SATA_EV_DEV_READY:   return "DEV_READY";
     case UIOX_SATA_EV_READ_DONE:   return "READ_DONE";
     case UIOX_SATA_EV_WRITE_DONE:  return "WRITE_DONE";
     case UIOX_SATA_EV_NCQ_DONE:    return "NCQ_DONE";
     case UIOX_SATA_EV_SMART_WARN:  return "SMART_WARN";
     case UIOX_SATA_EV_ERROR:       return "ERROR";
     default:                         return "UNKNOWN";
     }
 }
 
 const char *uiox_sata_dev_name(uiox_sata_dev_t d)
 {
     switch (d) {
     case UIOX_SATA_DEV_NONE:  return "None";
     case UIOX_SATA_DEV_ATA:   return "ATA (HDD/SSD)";
     case UIOX_SATA_DEV_ATAPI: return "ATAPI";
     case UIOX_SATA_DEV_SEMB:  return "SEMB";
     case UIOX_SATA_DEV_PM:    return "Port Multiplier";
     default:                   return "Unknown";
     }
 }
 
 const char *uiox_sata_ctrl_name(uiox_sata_ctrl_t c)
 {
     switch (c) {
     case UIOX_SATA_CTRL_AHCI:   return "AHCI 1.3.1";
     case UIOX_SATA_CTRL_LEGACY: return "Legacy IDE";
     case UIOX_SATA_CTRL_ASPM:   return "AHCI + ASPM";
     default:                     return "Unknown";
     }
 }
 