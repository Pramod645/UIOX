/**
 * @file    uiox_can_device.c
 * @brief   UIOX CAN device API implementation.
 * @date    2026-05-26
 */

 #include "uiox_can_device.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_can_open(uiox_can_device_t *dev)
 {
     if (!dev) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     uiox_can_subsys_init(&dev->subsys);
     dev->open = true;
     return 0;
 }
 
 int uiox_can_add_bus(uiox_can_device_t          *dev,
                       const uiox_can_bus_params_t *p)
 {
     if (!dev || !p || !dev->open) return -EINVAL;
     return uiox_can_subsys_add_bus(&dev->subsys,
                                     p->hw,
                                     p->hw_ops,
                                     p->node_id,
                                     p->name,
                                     p->fd_enabled,
                                     p->nom_bitrate,
                                     p->data_bitrate,
                                     p->heartbeat_ms,
                                     &p->busoff);
 }
 
 int uiox_can_start_bus(uiox_can_device_t *dev, uint8_t bus_idx)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_can_subsys_start(&dev->subsys, bus_idx);
 }
 
 void uiox_can_stop_bus(uiox_can_device_t *dev, uint8_t bus_idx)
 {
     if (!dev || !dev->open) return;
     uiox_can_subsys_stop(&dev->subsys, bus_idx);
 }
 
 void uiox_can_close(uiox_can_device_t *dev)
 {
     if (!dev || !dev->open) return;
     for (uint8_t i = 0; i < dev->subsys.bus_count; i++) {
         uiox_can_subsys_stop(&dev->subsys, i);
         uiox_can_hw_deinit(dev->subsys.buses[i].cif.hw);
     }
     dev->open = false;
 }
 
 int uiox_can_tx(uiox_can_device_t *dev,
                  uint8_t            bus_idx,
                  uint32_t           id,
                  const uint8_t     *data,
                  uint8_t            len,
                  bool               ext)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_can_subsys_tx(&dev->subsys, bus_idx, id, data, len, ext);
 }
 
 int uiox_can_register_rx(uiox_can_device_t     *dev,
                           uiox_can_rx_handler_t  fn,
                           void                  *ctx,
                           uint32_t               id_filter,
                           uint32_t               id_mask)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_can_subsys_register_rx(&dev->subsys, fn, ctx,
                                         id_filter, id_mask);
 }
 
 int uiox_can_add_filter(uiox_can_device_t *dev,
                          uint8_t            bus_idx,
                          uint32_t           id,
                          uint32_t           mask,
                          bool               ext)
 {
     if (!dev || !dev->open) return -EINVAL;
     if (bus_idx >= dev->subsys.bus_count) return -EINVAL;
     return uiox_can_if_add_filter(&dev->subsys.buses[bus_idx].cif,
                                    id, mask, ext);
 }
 
 void uiox_can_clr_filters(uiox_can_device_t *dev, uint8_t bus_idx)
 {
     if (!dev || !dev->open) return;
     if (bus_idx >= dev->subsys.bus_count) return;
     uiox_can_if_clr_filters(&dev->subsys.buses[bus_idx].cif);
 }
 
 int uiox_can_add_mailbox(uiox_can_device_t        *dev,
                           uint8_t                   bus_idx,
                           const uiox_can_mailbox_t *mb)
 {
     if (!dev || !dev->open) return -EINVAL;
     if (bus_idx >= dev->subsys.bus_count) return -EINVAL;
     return uiox_can_node_add_mb(&dev->subsys.buses[bus_idx].node, mb);
 }
 
 int uiox_can_sdo_write(uiox_can_device_t *dev,
                         uint8_t  bus_idx,
                         uint8_t  node_id,
                         uint16_t index,
                         uint8_t  subindex,
                         const uint8_t *data,
                         uint8_t  len)
 {
     if (!dev || !dev->open) return -EINVAL;
     if (bus_idx >= dev->subsys.bus_count) return -EINVAL;
     return uiox_can_proto_sdo_write(
         &dev->subsys.buses[bus_idx].proto,
         node_id, index, subindex, data, len);
 }
 
 int uiox_can_sdo_read(uiox_can_device_t *dev,
                        uint8_t  bus_idx,
                        uint8_t  node_id,
                        uint16_t index,
                        uint8_t  subindex,
                        uint8_t *data_out,
                        uint8_t *len_out)
 {
     if (!dev || !dev->open) return -EINVAL;
     if (bus_idx >= dev->subsys.bus_count) return -EINVAL;
     return uiox_can_proto_sdo_read(
         &dev->subsys.buses[bus_idx].proto,
         node_id, index, subindex, data_out, len_out);
 }
 
 int uiox_can_nmt_cmd(uiox_can_device_t *dev,
                       uint8_t  bus_idx,
                       uint8_t  node_id,
                       uint8_t  cmd)
 {
     if (!dev || !dev->open) return -EINVAL;
     if (bus_idx >= dev->subsys.bus_count) return -EINVAL;
     return uiox_can_proto_nmt_cmd(
         &dev->subsys.buses[bus_idx].proto, node_id, cmd);
 }
 
 int uiox_can_emcy(uiox_can_device_t *dev,
                    uint8_t   bus_idx,
                    uint16_t  err_code,
                    uint8_t   err_reg)
 {
     if (!dev || !dev->open) return -EINVAL;
     if (bus_idx >= dev->subsys.bus_count) return -EINVAL;
     return uiox_can_proto_emcy(
         &dev->subsys.buses[bus_idx].proto,
         err_code, err_reg, NULL, 0);
 }
 
 void uiox_can_gateway(uiox_can_device_t *dev, bool enable)
 {
     if (!dev) return;
     dev->subsys.gateway_enabled = enable;
 }
 
 void uiox_can_tick(uiox_can_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_can_subsys_tick(&dev->subsys, now_ms);
 }
 
 void uiox_can_process(uiox_can_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_can_subsys_process(&dev->subsys);
 }
 
 uiox_can_bus_health_t uiox_can_health(const uiox_can_device_t *dev,
                                        uint8_t bus_idx)
 {
     if (!dev) return UIOX_CAN_BUS_DISABLED;
     return uiox_can_subsys_health(&dev->subsys, bus_idx);
 }
 
 void uiox_can_stats(const uiox_can_device_t *dev,
                      uint8_t bus_idx,
                      uiox_can_if_stats_t *out)
 {
     if (!dev || !out) return;
     uiox_can_subsys_stats(&dev->subsys, bus_idx, out);
 }
 
 const char *uiox_can_health_name(uiox_can_bus_health_t h)
 {
     switch (h) {
     case UIOX_CAN_BUS_HEALTHY:        return "HEALTHY";
     case UIOX_CAN_BUS_WARNING:        return "WARNING";
     case UIOX_CAN_BUS_ERROR_PASSIVE:  return "ERROR_PASSIVE";
     case UIOX_CAN_BUS_OFF:            return "BUS_OFF";
     case UIOX_CAN_BUS_DISABLED:       return "DISABLED";
     default:                          return "UNKNOWN";
     }
 }
 