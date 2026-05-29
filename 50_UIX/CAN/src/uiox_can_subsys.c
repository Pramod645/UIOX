/**
 * @file    uiox_can_subsys.c
 * @brief   UIOX CAN subsystem implementation.
 * @date    2026-05-26
 */

 #include "uiox_can_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 void uiox_can_subsys_init(uiox_can_subsys_t *sys)
 {
     if (!sys) return;
     memset(sys, 0, sizeof(*sys));
 }
 
 int uiox_can_subsys_add_bus(uiox_can_subsys_t       *sys,
                              uiox_can_hw_t           *hw,
                              const uiox_can_hw_ops_t *ops,
                              uint8_t                  node_id,
                              const char              *name,
                              bool                     fd_enabled,
                              uint32_t                 nom_bitrate,
                              uint32_t                 data_bitrate,
                              uint32_t                 heartbeat_ms,
                              const uiox_can_busoff_cfg_t *bo_cfg)
 {
     if (!sys || !hw || !ops || !bo_cfg) return -EINVAL;
     if (sys->bus_count >= UIOX_CAN_MAX_BUS)  return -ENOSPC;
 
     uint8_t idx       = sys->bus_count;
     uiox_can_bus_t *b = &sys->buses[idx];
     memset(b, 0, sizeof(*b));
     b->bus_idx = idx;
 
     /* 1. HAL init */
     int rc = uiox_can_hw_init(hw, ops);
     if (rc < 0) return rc;
 
     /* 2. IF config */
     rc = uiox_can_if_config(&b->cif, hw, idx,
                              fd_enabled, nom_bitrate, data_bitrate);
     if (rc < 0) return rc;
 
     /* 3. Node init */
     rc = uiox_can_node_init(&b->node, &b->cif,
                              node_id, name, heartbeat_ms);
     if (rc < 0) return rc;
 
     /* 4. Protocol init */
     rc = uiox_can_proto_init(&b->proto, &b->node, bo_cfg, 0u);
     if (rc < 0) return rc;
 
     b->health = UIOX_CAN_BUS_HEALTHY;
     b->active = false;
     sys->bus_count++;
     return (int)idx;
 }
 
 int uiox_can_subsys_start(uiox_can_subsys_t *sys, uint8_t bus_idx)
 {
     if (!sys || bus_idx >= sys->bus_count) return -EINVAL;
     uiox_can_bus_t *b = &sys->buses[bus_idx];
     int rc = uiox_can_hw_start(b->cif.hw);
     if (rc < 0) return rc;
     uiox_can_node_nmt(&b->node, UIOX_CAN_NMT_OPERATIONAL);
     b->active = true;
     return 0;
 }
 
 void uiox_can_subsys_stop(uiox_can_subsys_t *sys, uint8_t bus_idx)
 {
     if (!sys || bus_idx >= sys->bus_count) return;
     uiox_can_bus_t *b = &sys->buses[bus_idx];
     uiox_can_node_nmt(&b->node, UIOX_CAN_NMT_STOPPED);
     uiox_can_hw_stop(b->cif.hw);
     b->active = false;
 }
 
 int uiox_can_subsys_tx(uiox_can_subsys_t *sys,
                         uint8_t            bus_idx,
                         uint32_t           id,
                         const uint8_t     *data,
                         uint8_t            len,
                         bool               ext)
 {
     if (!sys || bus_idx >= sys->bus_count) return -EINVAL;
     uiox_can_bus_t *b = &sys->buses[bus_idx];
     if (!b->active) return -ENETDOWN;
     return uiox_can_node_tx(&b->node, id, data, len, ext);
 }
 
 int uiox_can_subsys_register_rx(uiox_can_subsys_t     *sys,
                                  uiox_can_rx_handler_t  fn,
                                  void                  *ctx,
                                  uint32_t               id_filter,
                                  uint32_t               id_mask)
 {
     if (!sys || !fn) return -EINVAL;
     if (sys->rx_handler_count >= UIOX_CAN_MAX_RX_HANDLERS)
         return -ENOSPC;
     uiox_can_rx_entry_t *e = &sys->rx_handlers[sys->rx_handler_count++];
     e->fn        = fn;
     e->ctx       = ctx;
     e->id_filter = id_filter;
     e->id_mask   = id_mask;
     e->active    = true;
     return 0;
 }
 
 /* =========================================================================
  * Update bus health from error counters
  * ====================================================================== */
 
 static void update_health(uiox_can_bus_t *b)
 {
     uiox_can_err_cnt_t ec;
     if (uiox_can_hw_get_err_cnt(b->cif.hw, &ec) < 0) return;
 
     if (b->cif.hw->err_state == UIOX_CAN_ERR_BUS_OFF)
         b->health = UIOX_CAN_BUS_OFF;
     else if (b->cif.hw->err_state == UIOX_CAN_ERR_PASSIVE)
         b->health = UIOX_CAN_BUS_ERROR_PASSIVE;
     else if (ec.tec > 96u || ec.rec > 96u)
         b->health = UIOX_CAN_BUS_WARNING;
     else
         b->health = UIOX_CAN_BUS_HEALTHY;
 }
 
 /* =========================================================================
  * Tick — drives protocol timers and health monitoring
  * ====================================================================== */
 
 void uiox_can_subsys_tick(uiox_can_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys) return;
     for (uint8_t i = 0; i < sys->bus_count; i++) {
         uiox_can_bus_t *b = &sys->buses[i];
         if (!b->active) continue;
         uiox_can_proto_tick(&b->proto, now_ms);
         update_health(b);
     }
     sys->tick_ms = now_ms;
 }
 
 /* =========================================================================
  * Process — drain RX FIFOs, dispatch to handlers and gateway
  * ====================================================================== */
 
 void uiox_can_subsys_process(uiox_can_subsys_t *sys)
 {
     if (!sys) return;
 
     for (uint8_t bi = 0; bi < sys->bus_count; bi++) {
         uiox_can_bus_t *b = &sys->buses[bi];
         if (!b->active) continue;
 
         uiox_can_msg_t *msg;
         while ((msg = uiox_can_if_rx(&b->cif)) != NULL) {
             uint32_t id = msg->id & UIOX_CAN_ID_EXT_MASK;
 
             /* Global RX handlers */
             bool consumed = false;
             for (uint8_t hi = 0; hi < sys->rx_handler_count; hi++) {
                 uiox_can_rx_entry_t *e = &sys->rx_handlers[hi];
                 if (!e->active) continue;
                 if ((id & e->id_mask) == (e->id_filter & e->id_mask)) {
                     e->fn(bi, msg, e->ctx);
                     consumed = true;
                 }
             }
 
             /* Gateway: forward to other buses */
             if (sys->gateway_enabled) {
                 for (uint8_t oi = 0; oi < sys->bus_count; oi++) {
                     if (oi == bi) continue;
                     if (!sys->buses[oi].active) continue;
                     uiox_can_buf_ref(msg);
                     uiox_can_if_tx(&sys->buses[oi].cif, msg);
                 }
             }
 
             /* Protocol dispatch */
             if (!consumed)
                 uiox_can_proto_rx(&b->proto, msg);
             else
                 uiox_can_buf_free(msg);
         }
     }
 }
 
 uiox_can_bus_health_t uiox_can_subsys_health(const uiox_can_subsys_t *sys,
                                                uint8_t bus_idx)
 {
     if (!sys || bus_idx >= sys->bus_count)
         return UIOX_CAN_BUS_DISABLED;
     return sys->buses[bus_idx].health;
 }
 
 void uiox_can_subsys_stats(const uiox_can_subsys_t *sys,
                             uint8_t bus_idx,
                             uiox_can_if_stats_t *out)
 {
     if (!sys || bus_idx >= sys->bus_count || !out) return;
     uiox_can_if_stats_get(&sys->buses[bus_idx].cif, out);
 }
 