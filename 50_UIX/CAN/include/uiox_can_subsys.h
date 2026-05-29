/**
 * @file    uiox_can_subsys.h
 * @brief   UIOX CAN subsystem — multi-bus management, routing, diagnostics.
 *
 * Top subsystem layer. Manages:
 *   - Up to UIOX_CAN_MAX_BUS CAN buses simultaneously
 *   - Message routing between buses (gateway mode)
 *   - Bus health monitoring and auto-recovery
 *   - Periodic statistics collection
 *   - Global RX dispatch to registered handlers
 *
 * @date    2026-05-26
 */
//Layer 4 — Subsystem
 #ifndef UIOX_CAN_SUBSYS_H
 #define UIOX_CAN_SUBSYS_H
 
 #include "uiox_can_proto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_CAN_MAX_BUS        4    /**< Maximum CAN buses                 */
 #define UIOX_CAN_MAX_RX_HANDLERS 8   /**< Global RX handler slots           */
 
 /* =========================================================================
  * Bus health state
  * ====================================================================== */
 
 typedef enum {
     UIOX_CAN_BUS_HEALTHY = 0,
     UIOX_CAN_BUS_WARNING,
     UIOX_CAN_BUS_ERROR_PASSIVE,
     UIOX_CAN_BUS_OFF,
     UIOX_CAN_BUS_DISABLED,
 } uiox_can_bus_health_t;
 
 /* =========================================================================
  * Per-bus slot
  * ====================================================================== */
 
 typedef struct {
     uiox_can_proto_t   proto;
     uiox_can_node_t    node;
     uiox_can_if_t      cif;
     uiox_can_bus_health_t health;
     bool               active;
     uint8_t            bus_idx;
 } uiox_can_bus_t;
 
 /* =========================================================================
  * Global RX handler
  * ====================================================================== */
 
 typedef void (*uiox_can_rx_handler_t)(uint8_t bus_idx,
                                        const uiox_can_msg_t *msg,
                                        void *ctx);
 
 typedef struct {
     uiox_can_rx_handler_t fn;
     void                 *ctx;
     uint32_t              id_filter;  /**< ID filter (0 = accept all)      */
     uint32_t              id_mask;
     bool                  active;
 } uiox_can_rx_entry_t;
 
 /* =========================================================================
  * Subsystem descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_can_bus_t      buses[UIOX_CAN_MAX_BUS];
     uint8_t             bus_count;
     uiox_can_rx_entry_t rx_handlers[UIOX_CAN_MAX_RX_HANDLERS];
     uint8_t             rx_handler_count;
     bool                gateway_enabled; /**< Route msgs between buses      */
     uint32_t            tick_ms;         /**< Last tick timestamp           */
 } uiox_can_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 void uiox_can_subsys_init(uiox_can_subsys_t *sys);
 
 int  uiox_can_subsys_add_bus(uiox_can_subsys_t       *sys,
                               uiox_can_hw_t           *hw,
                               const uiox_can_hw_ops_t *ops,
                               uint8_t                  node_id,
                               const char              *name,
                               bool                     fd_enabled,
                               uint32_t                 nom_bitrate,
                               uint32_t                 data_bitrate,
                               uint32_t                 heartbeat_ms,
                               const uiox_can_busoff_cfg_t *bo_cfg);
 
 int  uiox_can_subsys_start  (uiox_can_subsys_t *sys, uint8_t bus_idx);
 void uiox_can_subsys_stop   (uiox_can_subsys_t *sys, uint8_t bus_idx);
 
 int  uiox_can_subsys_tx     (uiox_can_subsys_t *sys,
                               uint8_t            bus_idx,
                               uint32_t           id,
                               const uint8_t     *data,
                               uint8_t            len,
                               bool               ext);
 
 int  uiox_can_subsys_register_rx(uiox_can_subsys_t     *sys,
                                   uiox_can_rx_handler_t  fn,
                                   void                  *ctx,
                                   uint32_t               id_filter,
                                   uint32_t               id_mask);
 
 void uiox_can_subsys_tick   (uiox_can_subsys_t *sys, uint32_t now_ms);
 void uiox_can_subsys_process(uiox_can_subsys_t *sys);
 
 uiox_can_bus_health_t uiox_can_subsys_health(const uiox_can_subsys_t *sys,
                                                uint8_t bus_idx);
 void uiox_can_subsys_stats  (const uiox_can_subsys_t *sys,
                               uint8_t bus_idx,
                               uiox_can_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CAN_SUBSYS_H */
 