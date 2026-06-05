/**
 * @file    uiox_ram_device.c
 * @brief   UIOX RAM device API implementation.
 * @date    2026-06-03
 */

 #include "uiox_ram_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_ram_open(uiox_ram_device_t *dev, const uiox_ram_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_ram_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     /* Buddy bitmap: 2 bits per min-block */
     static uint8_t s_bmap[4096];
     memset(s_bmap, 0, sizeof(s_bmap));
 
     rc = uiox_ram_subsys_init(&dev->subsys, p->hw,
                                p->heap_base,  p->heap_size,
                                p->buddy_base, p->buddy_size,
                                s_bmap, sizeof(s_bmap));
     if (rc < 0) return rc;
 
     if (p->evt_cb)
         uiox_ram_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
 
     dev->open = true;
     return 0;
 }
 
 int  uiox_ram_start(uiox_ram_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_ram_subsys_start(&dev->subsys); }
 
 void uiox_ram_stop(uiox_ram_device_t *dev)
 { if (!dev || !dev->open) return; uiox_ram_subsys_stop(&dev->subsys); }
 
 void uiox_ram_close(uiox_ram_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_ram_stop(dev);
     uiox_ram_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 void uiox_ram_tick(uiox_ram_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return; uiox_ram_subsys_tick(&dev->subsys, now_ms); }
 
 void *uiox_ram_alloc(uiox_ram_device_t *dev, size_t size)
 { return dev && dev->open ? uiox_ram_subsys_alloc(&dev->subsys, size) : NULL; }
 
 void *uiox_ram_calloc(uiox_ram_device_t *dev, size_t n, size_t sz)
 { return dev && dev->open ? uiox_ram_subsys_calloc(&dev->subsys, n, sz) : NULL; }
 
 void *uiox_ram_realloc(uiox_ram_device_t *dev, void *ptr, size_t new_size)
 {
     if (!dev || !dev->open) return NULL;
     return uiox_heap_realloc(&dev->subsys.mgr.heap, ptr, new_size);
 }
 
 void uiox_ram_free(uiox_ram_device_t *dev, void *ptr)
 { if (dev && dev->open && ptr) uiox_ram_subsys_free(&dev->subsys, ptr); }
 
 void *uiox_ram_buddy_alloc(uiox_ram_device_t *dev, size_t size)
 { return dev && dev->open ? uiox_ram_subsys_buddy(&dev->subsys, size) : NULL; }
 
 void uiox_ram_buddy_free(uiox_ram_device_t *dev, void *ptr, size_t size)
 { if (dev && dev->open) uiox_ram_subsys_bfree(&dev->subsys, ptr, size); }
 
 int uiox_ram_slab_create(uiox_ram_device_t *dev, const char *name,
                           size_t obj_size, uint32_t count,
                           void *backing_mem)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_slab_create(&dev->subsys.mgr, name, obj_size, count, backing_mem);
 }
 
 void *uiox_ram_slab_alloc(uiox_ram_device_t *dev, size_t obj_size)
 { return dev && dev->open ? uiox_slab_alloc(&dev->subsys.mgr, obj_size) : NULL; }
 
 void uiox_ram_slab_free(uiox_ram_device_t *dev, void *ptr, size_t obj_size)
 { if (dev && dev->open) uiox_slab_free(&dev->subsys.mgr, ptr, obj_size); }
 
 int uiox_ram_set_power(uiox_ram_device_t *dev, uiox_ram_pwr_t state)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_ram_hw_set_power(dev->hw, state); }
 
 int uiox_ram_ecc_scrub(uiox_ram_device_t *dev,
                         uint64_t phys_start, uint64_t size)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_ram_ecc_start_scrub(&dev->subsys.ecc,
                                      phys_start, size, 64u*1024u);
 }
 
 void uiox_ram_get_info(uiox_ram_device_t *dev,
                         size_t *heap_used, size_t *heap_free,
                         size_t *buddy_used, size_t *buddy_free)
 {
     if (!dev || !dev->open) return;
     uiox_ram_subsys_info(&dev->subsys,
                           heap_used, heap_free, buddy_used, buddy_free);
 }
 
 void uiox_ram_print_info(const uiox_ram_device_t *dev)
 {
     if (!dev) return;
     const uiox_ram_hw_t *hw = dev->hw;
     printf("  RAM type       : %s\n", uiox_ram_type_name(hw->type));
     printf("  Model          : %s\n", hw->model);
     printf("  Speed          : %u MT/s\n", hw->speed_mtps);
     printf("  Channels       : %u  × %u-bit\n", hw->num_channels, hw->bus_width);
     printf("  Ranks          : %u\n", hw->num_ranks);
     printf("  Total size     : %llu MB\n",
            (unsigned long long)(hw->total_bytes / (1024*1024)));
     printf("  Physical base  : 0x%016llX\n",
            (unsigned long long)hw->base_phys);
     printf("  Capabilities   : 0x%08X\n", hw->caps);
     printf("  ECC            : %s\n",
            (hw->caps & UIOX_RAM_CAP_ECC) ? "yes" : "no");
     printf("  Regions        : %u\n", dev->subsys.rif.num_regions);
 }
 
 void uiox_ram_print_stats(const uiox_ram_device_t *dev)
 {
     if (!dev) return;
     const uiox_ram_subsys_t *s = &dev->subsys;
     printf("  State          : %s\n", uiox_ram_state_name(s->state));
     printf("  Uptime         : %llu ms\n", (unsigned long long)s->uptime_ms);
     printf("  Tick count     : %u\n", s->tick_count);
     size_t hu=0,hf=0,bu=0,bf=0;
     uiox_ram_subsys_info(s, &hu, &hf, &bu, &bf);
     printf("  Heap used      : %zu KB  free=%zu KB\n", hu/1024, hf/1024);
     printf("  Buddy used     : %zu KB  free=%zu KB\n", bu/1024, bf/1024);
     size_t hp=0;
     uiox_heap_stats(&s->mgr.heap, NULL, NULL, &hp);
     printf("  Heap peak      : %zu KB\n", hp/1024);
     printf("  Heap allocs    : %u  frees=%u\n",
            s->mgr.heap.alloc_count, s->mgr.heap.free_count);
     printf("  Buddy allocs   : %u  frees=%u\n",
            s->mgr.buddy.alloc_count, s->mgr.buddy.free_count);
     printf("  ECC CE         : %u\n", s->ecc.total_ce);
     printf("  ECC UE         : %u\n", s->ecc.total_ue);
     printf("  Scrub running  : %s\n", s->ecc.scrub_running ? "yes" : "no");
     printf("  ZQ cal count   : %u\n", s->rif.stats.zq_cal_count);
     printf("  Region descs   : %u free / %u\n",
            uiox_ram_buf_free_cnt(), UIOX_RAM_REGION_POOL_SIZE);
     uiox_ram_ecc_print_log(&s->ecc);
 }
 
 const char *uiox_ram_state_name(uiox_ram_subsys_state_t s)
 {
     switch (s) {
     case UIOX_RAM_SUBSYS_STOPPED: return "STOPPED";
     case UIOX_RAM_SUBSYS_RUNNING: return "RUNNING";
     case UIOX_RAM_SUBSYS_LOW_MEM: return "LOW_MEM";
     case UIOX_RAM_SUBSYS_ERROR:   return "ERROR";
     default:                       return "UNKNOWN";
     }
 }
 
 const char *uiox_ram_evt_name(uiox_ram_evt_t evt)
 {
     switch (evt) {
     case UIOX_RAM_EVT_INIT_DONE:    return "INIT_DONE";
     case UIOX_RAM_EVT_ECC_CE:       return "ECC_CE";
     case UIOX_RAM_EVT_ECC_UE:       return "ECC_UE";
     case UIOX_RAM_EVT_LOW_MEM:      return "LOW_MEM";
     case UIOX_RAM_EVT_SCRUB_DONE:   return "SCRUB_DONE";
     case UIOX_RAM_EVT_POWER_CHANGE: return "POWER_CHANGE";
     default:                         return "UNKNOWN";
     }
 }
 
 const char *uiox_ram_type_name(uiox_ram_type_t t)
 {
     switch (t) {
     case UIOX_RAM_TYPE_LPDDR5:  return "LPDDR5";
     case UIOX_RAM_TYPE_LPDDR4X: return "LPDDR4X";
     case UIOX_RAM_TYPE_DDR4:    return "DDR4";
     case UIOX_RAM_TYPE_DDR5:    return "DDR5";
     case UIOX_RAM_TYPE_SRAM:    return "SRAM";
     case UIOX_RAM_TYPE_PSRAM:   return "PSRAM";
     default:                     return "UNKNOWN";
     }
 }
 