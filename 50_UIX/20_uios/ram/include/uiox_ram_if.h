/**
 * @file    uiox_ram_if.h
 * @brief   UIOX RAM interface driver (DRAM controller, timing, channels).
 * @date    2026-06-03
 */

 #ifndef UIOX_RAM_IF_H
 #define UIOX_RAM_IF_H
 
 #include "uiox_ram_hw.h"
 #include "uiox_ram_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  bytes_read;
     uint64_t  bytes_written;
     uint64_t  read_ops;
     uint64_t  write_ops;
     uint64_t  refresh_count;
     uint32_t  zq_cal_count;
     uint32_t  power_transitions;
 } uiox_ram_if_stats_t;
 
 typedef struct {
     uiox_ram_hw_t       *hw;
     uiox_ram_region_t   *region_list; /**< Linked list of managed regions  */
     uint8_t              num_regions;
     uiox_ram_if_stats_t  stats;
     bool                 primed;
 } uiox_ram_if_t;
 
 int  uiox_ram_if_config    (uiox_ram_if_t *rif, uiox_ram_hw_t *hw);
 int  uiox_ram_if_start     (uiox_ram_if_t *rif);
 void uiox_ram_if_stop      (uiox_ram_if_t *rif);
 
 /** Register a memory region with the interface. */
 int  uiox_ram_if_add_region(uiox_ram_if_t    *rif,
                              uint64_t          phys_base,
                              uint64_t          size,
                              uiox_ram_region_type_t type,
                              bool cached, bool writable, bool exec);
 
 /** Find region containing a physical address. */
 uiox_ram_region_t *uiox_ram_if_find_region(const uiox_ram_if_t *rif,
                                              uint64_t phys);
 
 /** Perform periodic refresh / ZQ calibration. */
 void uiox_ram_if_periodic  (uiox_ram_if_t *rif, uint32_t now_ms);
 
 void uiox_ram_if_stats_get  (const uiox_ram_if_t *rif,
                               uiox_ram_if_stats_t *out);
 void uiox_ram_if_stats_reset(uiox_ram_if_t *rif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RAM_IF_H */
 