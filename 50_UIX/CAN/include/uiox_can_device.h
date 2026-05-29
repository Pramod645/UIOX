/**
 * @file    uiox_can_device.h
 * @brief   UIOX CAN top-level application-facing device API.
 *
 * Single include for application code. Wraps the entire CAN stack:
 * HAL → interface → node → protocol → subsystem.
 *
 * @date    2026-05-26
 */
//Layer 5 — Device API
 #ifndef UIOX_CAN_DEVICE_H
 #define UIOX_CAN_DEVICE_H
 
 #include "uiox_can_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Bus open parameters
  * ====================================================================== */
 
 typedef struct {
     uiox_can_hw_t             *hw;
     const uiox_can_hw_ops_t   *hw_ops;
     uint8_t                    node_id;
     const char                *name;
     bool                       fd_enabled;
     uint32_t                   nom_bitrate;   /**< e.g. 500000 = 500 kbit/s */
     uint32_t                   data_bitrate;  /**< e.g. 2000000 = 2 Mbit/s  */
     uint32_t                   heartbeat_ms;
     uiox_can_busoff_cfg_t      busoff;
 } uiox_can_bus_params_t;
 
/* =========================================================================
 * Device handle
 * ====================================================================== */

 typedef struct {
    uiox_can_subsys_t  subsys;
    bool               open;
} uiox_can_device_t;

/* =========================================================================
 * Application API
 * ====================================================================== */

/** Initialise device and subsystem. Call once at boot. */
int  uiox_can_open       (uiox_can_device_t *dev);

/**
 * @brief  Add a CAN bus to the device.
 * @return Bus index (≥ 0) on success, negative errno on failure.
 */
int  uiox_can_add_bus    (uiox_can_device_t          *dev,
                           const uiox_can_bus_params_t *p);

/** Start a bus (HAL start + NMT operational). */
int  uiox_can_start_bus  (uiox_can_device_t *dev, uint8_t bus_idx);

/** Stop a bus (NMT stopped + HAL stop). */
void uiox_can_stop_bus   (uiox_can_device_t *dev, uint8_t bus_idx);

/** Close all buses and release all resources. */
void uiox_can_close      (uiox_can_device_t *dev);

/**
 * @brief  Transmit a CAN frame on a bus.
 * @param  bus_idx   Bus index returned by uiox_can_add_bus().
 * @param  id        CAN identifier (add UIOX_CAN_ID_EXT_FLAG for 29-bit).
 * @param  data      Frame payload.
 * @param  len       Payload length (0..8 classic, 0..64 FD).
 * @param  ext       true = extended 29-bit frame.
 */
int  uiox_can_tx         (uiox_can_device_t *dev,
                           uint8_t            bus_idx,
                           uint32_t           id,
                           const uint8_t     *data,
                           uint8_t            len,
                           bool               ext);

/**
 * @brief  Register a global RX handler.
 * @param  id_filter  Accept frames where (id & id_mask) == id_filter.
 * @param  id_mask    0 = accept all frames.
 */
int  uiox_can_register_rx(uiox_can_device_t     *dev,
                           uiox_can_rx_handler_t  fn,
                           void                  *ctx,
                           uint32_t               id_filter,
                           uint32_t               id_mask);

/** Add an acceptance filter to a bus. */
int  uiox_can_add_filter (uiox_can_device_t *dev,
                           uint8_t            bus_idx,
                           uint32_t           id,
                           uint32_t           mask,
                           bool               ext);

/** Clear all acceptance filters on a bus. */
void uiox_can_clr_filters(uiox_can_device_t *dev, uint8_t bus_idx);

/** Add a mailbox (TX or RX message object) to a node. */
int  uiox_can_add_mailbox(uiox_can_device_t        *dev,
                           uint8_t                   bus_idx,
                           const uiox_can_mailbox_t *mb);

/** SDO expedited write (CANopen). */
int  uiox_can_sdo_write  (uiox_can_device_t *dev,
                           uint8_t  bus_idx,
                           uint8_t  node_id,
                           uint16_t index,
                           uint8_t  subindex,
                           const uint8_t *data,
                           uint8_t  len);

/** SDO expedited read (CANopen). */
int  uiox_can_sdo_read   (uiox_can_device_t *dev,
                           uint8_t  bus_idx,
                           uint8_t  node_id,
                           uint16_t index,
                           uint8_t  subindex,
                           uint8_t *data_out,
                           uint8_t *len_out);

/** Send NMT command (CANopen master). */
int  uiox_can_nmt_cmd    (uiox_can_device_t *dev,
                           uint8_t  bus_idx,
                           uint8_t  node_id,
                           uint8_t  cmd);

/** Send EMCY message. */
int  uiox_can_emcy       (uiox_can_device_t *dev,
                           uint8_t   bus_idx,
                           uint16_t  err_code,
                           uint8_t   err_reg);

/**
 * @brief  Enable CAN bus gateway (route all messages between buses).
 */
void uiox_can_gateway    (uiox_can_device_t *dev, bool enable);

/**
 * @brief  Periodic tick — drives heartbeat, SYNC, bus-off recovery.
 * @param  now_ms  Monotonic time in milliseconds.
 */
void uiox_can_tick       (uiox_can_device_t *dev, uint32_t now_ms);

/**
 * @brief  Process RX — drain all bus FIFOs and dispatch messages.
 *         Call from main loop or dedicated RX task.
 */
void uiox_can_process    (uiox_can_device_t *dev);

/** Get bus health status. */
uiox_can_bus_health_t uiox_can_health(const uiox_can_device_t *dev,
                                       uint8_t bus_idx);

/** Get bus interface statistics snapshot. */
void uiox_can_stats      (const uiox_can_device_t *dev,
                           uint8_t bus_idx,
                           uiox_can_if_stats_t *out);

/** Return human-readable health string. */
const char *uiox_can_health_name(uiox_can_bus_health_t h);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CAN_DEVICE_H */

 