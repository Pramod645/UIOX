/**
 * @file    uiox_ram_subsys.h
 * @brief   UIOX RAM subsystem — pools, MPU, power, events.
 * @date    2026-06-03
 */

 #ifndef UIOX_RAM_SUBSYS_H
 #define UIOX_RAM_SUBSYS_H
 
 #include "uiox_ram_mgr.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_RAM_EVT_INIT_DONE = 0,
     UIOX_RAM_EVT_ECC_CE,
     UIOX_RAM_EVT_ECC_UE,
     UIOX_RAM_EVT_LOW_MEM,
     UIOX_RAM_EVT_SCRUB_DONE,
     UIOX_RAM_EVT_POWER_CHANGE,
 } uiox_ram_evt_t;
 
 typedef void (*uiox_ram_evt_cb_t)(uiox_ram_evt_t evt, void *ctx);
 
 typedef enum {
     UIOX_RAM_SUBSYS_STOPPED = 0,
     UIOX_RAM_SUBSYS_RUNNING,
     UIOX_RAM_SUBSYS_LOW_MEM,
     UIOX_RAM_SUBSYS_ERROR,
 } uiox_ram_subsys_state_t;
 
 #define UIOX_RAM_LOW_MEM_THRESHOLD_PCT  10u   /**< Alert below 10% free   */
 
 typedef struct {
     uiox_ram_if_t            rif;
     uiox_ram_ecc_t           ecc;
     uiox_ram_mgr_t           mgr;
     uiox_ram_subsys_state_t  state;
     uiox_ram_evt_cb_t        evt_cb;
     void                    *evt_ctx;
     uint32_t                 tick_count;
     uint64_t                 uptime_ms;
 } uiox_ram_subsys_t;
 
 int  uiox_ram_subsys_init    (uiox_ram_subsys_t *sys,
                                uiox_ram_hw_t     *hw,
                                void *heap_base,  size_t heap_size,
                                void *buddy_base, size_t buddy_size,
                                void *buddy_bmap, size_t buddy_bmap_size);
 int  uiox_ram_subsys_start   (uiox_ram_subsys_t *sys);
 void uiox_ram_subsys_stop    (uiox_ram_subsys_t *sys);
 void uiox_ram_subsys_tick    (uiox_ram_subsys_t *sys, uint32_t now_ms);
 void uiox_ram_subsys_set_cb  (uiox_ram_subsys_t *sys,
                                uiox_ram_evt_cb_t cb, void *ctx);
 
 /* Allocation wrappers */
 void *uiox_ram_subsys_alloc  (uiox_ram_subsys_t *sys, size_t size);
 void *uiox_ram_subsys_calloc (uiox_ram_subsys_t *sys,
                                size_t n, size_t sz);
 void  uiox_ram_subsys_free   (uiox_ram_subsys_t *sys, void *ptr);
 void *uiox_ram_subsys_buddy  (uiox_ram_subsys_t *sys, size_t size);
 void  uiox_ram_subsys_bfree  (uiox_ram_subsys_t *sys,
                                void *ptr, size_t size);
 
 /* Memory info */
 void  uiox_ram_subsys_info   (const uiox_ram_subsys_t *sys,
                                size_t *heap_used, size_t *heap_free,
                                size_t *buddy_used, size_t *buddy_free);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RAM_SUBSYS_H */
 