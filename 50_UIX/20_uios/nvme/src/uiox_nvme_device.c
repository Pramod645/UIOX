/**
 * @file  uiox_nvme_device.c
 * @brief UIOX NVMe SSD application device API.
 * @date  2026-06-12
 */

 #include "uiox_nvme_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_nvme_open(uiox_nvme_device_t *dev,
                     const uiox_nvme_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
     int rc  = uiox_nvme_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
     rc = uiox_nvme_subsys_init(&dev->subsys, p->hw);
     if (rc < 0) return rc;
     if (p->evt_cb)
         uiox_nvme_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int  uiox_nvme_start(uiox_nvme_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_nvme_subsys_start(&dev->subsys); }
 
 void uiox_nvme_stop(uiox_nvme_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_nvme_subsys_stop(&dev->subsys); }
 
 void uiox_nvme_close(uiox_nvme_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_nvme_stop(dev);
   uiox_nvme_hw_deinit(dev->hw);
   dev->open = false; }
 
 void uiox_nvme_tick(uiox_nvme_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return;
   uiox_nvme_subsys_tick(&dev->subsys, now_ms); }
 
 int uiox_nvme_read(uiox_nvme_device_t *dev,
                     uint32_t nsid, uint64_t slba,
                     uint8_t *buf, uint32_t nlb)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_nvme_subsys_read(&dev->subsys, nsid, slba, buf, nlb); }
 
 int uiox_nvme_write(uiox_nvme_device_t *dev,
                      uint32_t nsid, uint64_t slba,
                      const uint8_t *buf, uint32_t nlb)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_nvme_subsys_write(&dev->subsys, nsid, slba, buf, nlb); }
 
 int uiox_nvme_flush(uiox_nvme_device_t *dev, uint32_t nsid)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_nvme_subsys_flush(&dev->subsys, nsid); }
 
 int uiox_nvme_trim(uiox_nvme_device_t *dev,
                     uint32_t nsid, uint64_t slba, uint32_t nlb)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_nvme_subsys_trim(&dev->subsys, nsid, slba, nlb); }
 
 int uiox_nvme_smart_log(uiox_nvme_device_t *dev, uint8_t *buf)
 { if (!dev || !dev->open || !buf) return -EINVAL;
   return uiox_nvme_proto_smart_log(&dev->subsys.proto, buf); }
 
 int uiox_nvme_format_ns(uiox_nvme_device_t *dev,
                          uint32_t nsid, uint8_t lbaf)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_nvme_proto_format_ns(&dev->subsys.proto, nsid, lbaf); }
 
 bool uiox_nvme_is_ready(const uiox_nvme_device_t *dev)
 { return dev && dev->open && dev->hw->ready; }
 
 uint64_t uiox_nvme_capacity(const uiox_nvme_device_t *dev, uint32_t nsid)
 {
     if (!dev || !dev->open || nsid == 0u ||
         nsid > UIOX_NVME_MAX_NAMESPACES) return 0u;
     const uiox_nvme_ns_t *ns = &dev->hw->ns[nsid - 1u];
     return ns->active ? (ns->nsze * ns->lba_size) : 0u;
 }
 
 const uiox_nvme_ctrl_id_t *uiox_nvme_ctrl_id(const uiox_nvme_device_t *dev)
 { return (dev && dev->open) ? &dev->hw->ctrl_id : NULL; }
 
 const uiox_nvme_ns_t *uiox_nvme_ns_info(const uiox_nvme_device_t *dev,
                                           uint32_t nsid)
 {
     if (!dev || !dev->open || nsid == 0u ||
         nsid > UIOX_NVME_MAX_NAMESPACES) return NULL;
     return &dev->hw->ns[nsid - 1u];
 }
 
 void uiox_nvme_print_info(const uiox_nvme_device_t *dev)
 {
     if (!dev) return;
     const uiox_nvme_hw_t    *hw = dev->hw;
     const uiox_nvme_subsys_t *s = &dev->subsys;
     const uiox_nvme_ctrl_id_t *id = &hw->ctrl_id;
     printf("  Controller     : %s\n", hw->model);
     printf("  PCIe gen       : %s\n", uiox_nvme_pcie_name(hw->pcie_gen));
     printf("  BAR0           : 0x%08lX\n", (unsigned long)hw->bar0);
     printf("  IRQ base       : %u  (%u vectors)\n",
            hw->irq_base, hw->num_irqs);
     printf("  Capabilities   : 0x%08X\n", hw->caps);
     printf("  State          : %s\n", uiox_nvme_state_name(s->state));
     printf("  Ready          : %s\n", hw->ready ? "YES" : "NO");
     if (hw->ready) {
         printf("  Model          : %.40s\n", id->model);
         printf("  Serial         : %.20s\n", id->serial);
         printf("  Firmware       : %.8s\n",  id->fw_rev);
         printf("  VID            : 0x%04X\n", id->vid);
         printf("  Namespaces     : %u\n", id->nn);
         printf("  IO queues      : %u\n", hw->num_io_queues);
         printf("  SQ depth       : %u\n", hw->sq_depth);
         printf("  TRIM           : %s\n",
                id->trim_supported ? "YES" : "NO");
         printf("  APST           : %s\n",
                id->apst_supported ? "YES" : "NO");
         printf("  Volatile WC    : %s  enabled=%s\n",
                id->volatile_wc ? "SUPPORTED" : "NO",
                hw->volatile_wc_enabled ? "YES" : "NO");
         printf("  Warn temp      : %u °C\n", id->warn_composite_temp);
         printf("  Crit temp      : %u °C\n", id->crit_composite_temp);
         /* Namespaces */
         for (uint32_t i = 0u; i < UIOX_NVME_MAX_NAMESPACES; i++) {
             const uiox_nvme_ns_t *ns = &hw->ns[i];
             if (!ns->active) continue;
             printf("  NS[%u]          : nsze=%llu  cap=%llu"
                    "  lba=%u B  → %llu GB\n",
                    ns->nsid,
                    (unsigned long long)ns->nsze,
                    (unsigned long long)ns->ncap,
                    ns->lba_size,
                    (unsigned long long)
                    ((ns->nsze * ns->lba_size) >> 30u));
         }
     }
 }
 
 void uiox_nvme_print_stats(uiox_nvme_device_t *dev)
 {
     if (!dev) return;
     const uiox_nvme_subsys_t *s = &dev->subsys;
     printf("  Uptime         : %llu ms\n",
            (unsigned long long)s->uptime_ms);
     printf("  Tick count     : %u\n",   s->tick_count);
     printf("  Errors         : %u\n",   s->error_count);
     uiox_nvme_if_stats_t is;
     uiox_nvme_if_stats_get(&dev->subsys.nif, &is);
     printf("  LBAs read      : %llu\n",
            (unsigned long long)is.lbas_read);
     printf("  LBAs written   : %llu\n",
            (unsigned long long)is.lbas_written);
     printf("  Bytes read     : %llu\n",
            (unsigned long long)is.bytes_read);
     printf("  Bytes written  : %llu\n",
            (unsigned long long)is.bytes_written);
     printf("  Admin CMDs     : %u\n",   is.admin_cmds);
     printf("  IO CMDs        : %u\n",   is.io_cmds);
     printf("  Flushes        : %u\n",   is.flushes);
     printf("  TRIMs          : %u\n",   is.trims);
     printf("  IRQ count      : %u\n",   is.irq_count);
     printf("  Retries        : %u\n",   is.retries);
     printf("  IF errors      : %u\n",   is.errors);
     printf("  Cmd pool free  : %u / %u\n",
            uiox_nvme_cmd_free_cnt(), UIOX_NVME_CMD_POOL_SIZE);
     printf("  Blk pool free  : %u / %u\n",
            uiox_nvme_blk_free_cnt(), UIOX_NVME_BLK_POOL_SIZE);
     printf("  Evt pool free  : %u / %u\n",
            uiox_nvme_evt_free_cnt(), UIOX_NVME_EVT_POOL_SIZE);
 }
 
 const char *uiox_nvme_state_name(uiox_nvme_state_t s)
 {
     switch (s) {
     case UIOX_NVME_STATE_OFF:   return "OFF";
     case UIOX_NVME_STATE_INIT:  return "INIT";
     case UIOX_NVME_STATE_READY: return "READY";
     case UIOX_NVME_STATE_ERROR: return "ERROR";
     case UIOX_NVME_STATE_FATAL: return "FATAL";
     default:                     return "UNKNOWN";
     }
 }
 
 const char *uiox_nvme_ev_name(uiox_nvme_ev_t ev)
 {
     switch (ev) {
     case UIOX_NVME_EV_READY:        return "READY";
     case UIOX_NVME_EV_READ_DONE:    return "READ_DONE";
     case UIOX_NVME_EV_WRITE_DONE:   return "WRITE_DONE";
     case UIOX_NVME_EV_FLUSH_DONE:   return "FLUSH_DONE";
     case UIOX_NVME_EV_TRIM_DONE:    return "TRIM_DONE";
     case UIOX_NVME_EV_HEALTH_WARN:  return "HEALTH_WARN";
     case UIOX_NVME_EV_TEMP_WARN:    return "TEMP_WARN";
     case UIOX_NVME_EV_MEDIA_ERROR:  return "MEDIA_ERROR";
     case UIOX_NVME_EV_FATAL:        return "FATAL";
     case UIOX_NVME_EV_ERROR:        return "ERROR";
     default:                          return "UNKNOWN";
     }
 }
 
 const char *uiox_nvme_pcie_name(uiox_nvme_pcie_gen_t g)
 {
     switch (g) {
     case UIOX_NVME_PCIE_GEN3: return "PCIe 3.0 x4 (32 GT/s)";
     case UIOX_NVME_PCIE_GEN4: return "PCIe 4.0 x4 (64 GT/s)";
     case UIOX_NVME_PCIE_GEN5: return "PCIe 5.0 x4 (128 GT/s)";
     default:                   return "Unknown";
     }
 }
 