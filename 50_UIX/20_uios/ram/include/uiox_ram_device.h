/**
 * @file    uiox_ram_device.h
 * @brief   UIOX RAM top-level application-facing device API.
 * @date    2026-06-03
 */

 #ifndef UIOX_RAM_DEVICE_H
 #define UIOX_RAM_DEVICE_H
 
 #include "uiox_ram_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_ram_hw_t            *hw;
     const uiox_ram_hw_ops_t  *hw_ops;
     void                     *heap_base;
     size_t                    heap_size;
     void                     *buddy_base;
     size_t                    buddy_size;
     uiox_ram_evt_cb_t         evt_cb;
     void                     *evt_ctx;
 } uiox_ram_open_params_t;
 
 typedef struct {
     uiox_ram_subsys_t  subsys;
     uiox_ram_hw_t     *hw;
     bool               open;
 } uiox_ram_device_t;
 
 int   uiox_ram_open        (uiox_ram_device_t           *dev,
                              const uiox_ram_open_params_t *p);
 int   uiox_ram_start       (uiox_ram_device_t *dev);
 void  uiox_ram_stop        (uiox_ram_device_t *dev);
 void  uiox_ram_close       (uiox_ram_device_t *dev);
 void  uiox_ram_tick        (uiox_ram_device_t *dev, uint32_t now_ms);
 
 void *uiox_ram_alloc       (uiox_ram_device_t *dev, size_t size);
 void *uiox_ram_calloc      (uiox_ram_device_t *dev, size_t n, size_t sz);
 void *uiox_ram_realloc     (uiox_ram_device_t *dev,
                              void *ptr, size_t new_size);
 void  uiox_ram_free        (uiox_ram_device_t *dev, void *ptr);
 void *uiox_ram_buddy_alloc (uiox_ram_device_t *dev, size_t size);
 void  uiox_ram_buddy_free  (uiox_ram_device_t *dev,
                              void *ptr, size_t size);
 
 int   uiox_ram_slab_create (uiox_ram_device_t *dev, const char *name,
                              size_t obj_size, uint32_t count,
                              void *backing_mem);
 void *uiox_ram_slab_alloc  (uiox_ram_device_t *dev, size_t obj_size);
 void  uiox_ram_slab_free   (uiox_ram_device_t *dev,
                              void *ptr, size_t obj_size);
 
 int   uiox_ram_set_power   (uiox_ram_device_t *dev, uiox_ram_pwr_t state);
 int   uiox_ram_ecc_scrub   (uiox_ram_device_t *dev,
                              uint64_t phys_start, uint64_t size);
 
 void  uiox_ram_get_info    (uiox_ram_device_t *dev,
                              size_t *heap_used, size_t *heap_free,
                              size_t *buddy_used, size_t *buddy_free);
 void  uiox_ram_print_info  (const uiox_ram_device_t *dev);
 void  uiox_ram_print_stats (const uiox_ram_device_t *dev);
 
 const char *uiox_ram_state_name(uiox_ram_subsys_state_t s);
 const char *uiox_ram_evt_name  (uiox_ram_evt_t evt);
 const char *uiox_ram_type_name (uiox_ram_type_t t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RAM_DEVICE_H */
 