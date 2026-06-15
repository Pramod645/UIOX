/**
 * @file  uiox_nvme_device.h
 * @brief UIOX NVMe SSD application-facing API (Layer 5).
 * @date  2026-06-12
 */

 #ifndef UIOX_NVME_DEVICE_H
 #define UIOX_NVME_DEVICE_H
 
 #include "uiox_nvme_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_nvme_hw_t           *hw;
     const uiox_nvme_hw_ops_t *hw_ops;
     uiox_nvme_evt_cb_t        evt_cb;
     void                     *evt_ctx;
 } uiox_nvme_open_params_t;
 
 typedef struct {
     uiox_nvme_subsys_t  subsys;
     uiox_nvme_hw_t     *hw;
     bool                open;
 } uiox_nvme_device_t;
 
 /* Lifecycle */
 int  uiox_nvme_open       (uiox_nvme_device_t *dev,
                             const uiox_nvme_open_params_t *p);
 int  uiox_nvme_start      (uiox_nvme_device_t *dev);
 void uiox_nvme_stop       (uiox_nvme_device_t *dev);
 void uiox_nvme_close      (uiox_nvme_device_t *dev);
 void uiox_nvme_tick       (uiox_nvme_device_t *dev, uint32_t now_ms);
 
 /* Block I/O */
 int  uiox_nvme_read       (uiox_nvme_device_t *dev,
                             uint32_t nsid, uint64_t slba,
                             uint8_t *buf, uint32_t nlb);
 int  uiox_nvme_write      (uiox_nvme_device_t *dev,
                             uint32_t nsid, uint64_t slba,
                             const uint8_t *buf, uint32_t nlb);
 int  uiox_nvme_flush      (uiox_nvme_device_t *dev, uint32_t nsid);
 int  uiox_nvme_trim       (uiox_nvme_device_t *dev,
                             uint32_t nsid, uint64_t slba, uint32_t nlb);
 
 /* Admin */
 int  uiox_nvme_smart_log  (uiox_nvme_device_t *dev, uint8_t *buf);
 int  uiox_nvme_format_ns  (uiox_nvme_device_t *dev,
                             uint32_t nsid, uint8_t lbaf);
 
 /* Device info */
 bool     uiox_nvme_is_ready    (const uiox_nvme_device_t *dev);
 uint64_t uiox_nvme_capacity    (const uiox_nvme_device_t *dev,
                                  uint32_t nsid);
 const uiox_nvme_ctrl_id_t *uiox_nvme_ctrl_id(
                                  const uiox_nvme_device_t *dev);
 const uiox_nvme_ns_t      *uiox_nvme_ns_info(
                                  const uiox_nvme_device_t *dev,
                                  uint32_t nsid);
 
 /* Info / stats */
 void uiox_nvme_print_info  (const uiox_nvme_device_t *dev);
 void uiox_nvme_print_stats (uiox_nvme_device_t *dev);
 
 /* Name helpers */
 const char *uiox_nvme_state_name(uiox_nvme_state_t s);
 const char *uiox_nvme_ev_name   (uiox_nvme_ev_t ev);
 const char *uiox_nvme_pcie_name (uiox_nvme_pcie_gen_t g);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NVME_DEVICE_H */
 