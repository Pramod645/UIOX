/**
 * @file    uiox_can_hw.h
 * @brief   UIOX CAN Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to CAN controller hardware. Owns:
 *   - MMIO register access to CAN controller (Bosch M_CAN / SJA1000 style)
 *   - Bit-timing configuration (nominal + data phase for CAN-FD)
 *   - TX/RX FIFO management
 *   - DMA engine for bulk message transfer
 *   - IRQ top-half handling (TX complete, RX ready, error, bus-off)
 *   - Clock and reset control
 *
 * Supports:
 *   - Classic CAN 2.0A (11-bit ID) / 2.0B (29-bit ID)
 *   - CAN-FD (ISO 11898-1:2015, up to 8 Mbit/s data phase)
 *
 * @version 1.0.0
 * @date    2026-05-26
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_CAN_HW_H
 #define UIOX_CAN_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * CAN frame constants
  * ====================================================================== */
 
 #define UIOX_CAN_MAX_DLC        8    /**< Classic CAN max data bytes        */
 #define UIOX_CANFD_MAX_DLC      64   /**< CAN-FD max data bytes             */
 #define UIOX_CAN_ID_STD_MASK    0x7FFu   /**< 11-bit standard ID mask       */
 #define UIOX_CAN_ID_EXT_MASK    0x1FFFFFFFu /**< 29-bit extended ID mask    */
 #define UIOX_CAN_ID_EXT_FLAG    (1u << 31)  /**< Extended frame flag        */
 #define UIOX_CAN_ID_RTR_FLAG    (1u << 30)  /**< RTR frame flag             */
 #define UIOX_CAN_ID_ERR_FLAG    (1u << 29)  /**< Error frame flag           */
 #define UIOX_CANFD_BRS_FLAG     (1u << 28)  /**< Bit-rate switch flag       */
 #define UIOX_CANFD_ESI_FLAG     (1u << 27)  /**< Error state indicator      */
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_CAN_CAP_FD         (1u << 0)  /**< CAN-FD capable             */
 #define UIOX_CAN_CAP_DMA        (1u << 1)  /**< DMA TX/RX                  */
 #define UIOX_CAN_CAP_HW_FILTER  (1u << 2)  /**< Hardware acceptance filter */
 #define UIOX_CAN_CAP_LOOPBACK   (1u << 3)  /**< Loopback test mode         */
 #define UIOX_CAN_CAP_LISTEN     (1u << 4)  /**< Listen-only mode           */
 #define UIOX_CAN_CAP_TIMESTAMP  (1u << 5)  /**< Hardware timestamping      */
 #define UIOX_CAN_CAP_BERR_RPT   (1u << 6)  /**< Bit error reporting        */
 #define UIOX_CAN_CAP_TDC        (1u << 7)  /**< Transmitter delay comp     */
 
 /* =========================================================================
  * Bit timing (Bosch M_CAN compatible)
  * ====================================================================== */
 
 typedef struct {
     uint32_t  brp;          /**< Baud rate prescaler                        */
     uint8_t   tseg1;        /**< Time segment 1 (prop + phase_seg1)        */
     uint8_t   tseg2;        /**< Time segment 2 (phase_seg2)               */
     uint8_t   sjw;          /**< Synchronisation jump width                 */
 } uiox_can_bittiming_t;
 
 /* =========================================================================
  * CAN controller operating mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_CAN_MODE_NORMAL = 0,   /**< Normal operation                      */
     UIOX_CAN_MODE_LOOPBACK,     /**< Internal loopback (test)              */
     UIOX_CAN_MODE_LISTEN,       /**< Listen-only (no ACK, no TX)           */
     UIOX_CAN_MODE_SLEEP,        /**< Low-power sleep mode                  */
 } uiox_can_mode_t;
 
 /* =========================================================================
  * CAN error state
  * ====================================================================== */
 
 typedef enum {
     UIOX_CAN_ERR_ACTIVE = 0,    /**< Error-active (TEC/REC < 128)         */
     UIOX_CAN_ERR_PASSIVE,       /**< Error-passive (TEC/REC >= 128)       */
     UIOX_CAN_ERR_BUS_OFF,       /**< Bus-off (TEC >= 256)                 */
 } uiox_can_err_state_t;
 
 /* =========================================================================
  * Error counters
  * ====================================================================== */
 
 typedef struct {
     uint16_t  tec;          /**< Transmit error counter                     */
     uint16_t  rec;          /**< Receive error counter                      */
     uint32_t  bus_errors;   /**< Total bus error count                      */
     uint32_t  rx_errors;    /**< RX error count                             */
     uint32_t  tx_errors;    /**< TX error count                             */
     uint32_t  overflows;    /**< RX FIFO overflow count                     */
     uint32_t  bus_off_count;/**< Number of bus-off events                   */
 } uiox_can_err_cnt_t;
 
 /* =========================================================================
  * DMA descriptor
  * ====================================================================== */
 
 #define UIOX_CAN_DMA_DESC_ALIGN  32
 
 typedef struct __attribute__((packed, aligned(UIOX_CAN_DMA_DESC_ALIGN))) {
     volatile uint32_t  status;      /**< OWN + done/error flags             */
     uint32_t           ctrl;        /**< Length + interrupt enable          */
     uint32_t           buf_lo;      /**< Physical address (lo 32)           */
     uint32_t           buf_hi;      /**< Physical address (hi 32)           */
     uint32_t           bytes_done;  /**< Written by HW on completion        */
     uint32_t           reserved[3];
 } uiox_can_dma_desc_t;
 
 #define UIOX_CAN_DESC_OWN   (1u << 31)
 #define UIOX_CAN_DESC_DONE  (1u << 1)
 #define UIOX_CAN_DESC_ERR   (1u << 0)
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t              base_addr;    /**< MMIO base of CAN controller   */
     uint32_t               irq;          /**< IRQ line                      */
     uint32_t               caps;         /**< UIOX_CAN_CAP_* bitmask       */
     uint32_t               clk_hz;       /**< CAN controller input clock    */
     uiox_can_mode_t        mode;         /**< Operating mode                */
     uiox_can_err_state_t   err_state;    /**< Current error state           */
     uiox_can_err_cnt_t     err_cnt;      /**< Error counters                */
     bool                   fd_enabled;   /**< CAN-FD enabled flag           */
 
     /* Bit timing */
     uiox_can_bittiming_t   nom_bt;       /**< Nominal bit timing            */
     uiox_can_bittiming_t   data_bt;      /**< Data phase bit timing (FD)    */
 
     /* DMA */
     uiox_can_dma_desc_t   *tx_ring;
     uiox_can_dma_desc_t   *rx_ring;
     uint16_t               tx_ring_sz;
     uint16_t               rx_ring_sz;
     uint16_t               tx_head;
     uint16_t               tx_tail;
     uint16_t               rx_head;
 
     void                  *priv;         /**< Driver-private data           */
 } uiox_can_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)         (uiox_can_hw_t *hw);
     void (*deinit)       (uiox_can_hw_t *hw);
     int  (*start)        (uiox_can_hw_t *hw);
     void (*stop)         (uiox_can_hw_t *hw);
     int  (*set_mode)     (uiox_can_hw_t *hw, uiox_can_mode_t mode);
     int  (*set_bittiming)(uiox_can_hw_t *hw,
                           const uiox_can_bittiming_t *nom,
                           const uiox_can_bittiming_t *data);
     int  (*tx_submit)    (uiox_can_hw_t *hw,
                           uintptr_t phys, uint32_t length);
     int  (*tx_done)      (uiox_can_hw_t *hw);
     int  (*rx_poll)      (uiox_can_hw_t *hw,
                           uintptr_t *phys_out, uint32_t *len_out);
     int  (*set_filter)   (uiox_can_hw_t *hw, uint8_t idx,
                           uint32_t id, uint32_t mask, bool ext);
     void (*clear_filters)(uiox_can_hw_t *hw);
     int  (*get_err_cnt)  (uiox_can_hw_t *hw, uiox_can_err_cnt_t *out);
     int  (*recover)      (uiox_can_hw_t *hw);
     void (*isr)          (uiox_can_hw_t *hw);
 } uiox_can_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_can_hw_init       (uiox_can_hw_t *hw,
                               const uiox_can_hw_ops_t *ops);
 int  uiox_can_hw_start      (uiox_can_hw_t *hw);
 void uiox_can_hw_stop       (uiox_can_hw_t *hw);
 void uiox_can_hw_deinit     (uiox_can_hw_t *hw);
 int  uiox_can_hw_set_mode   (uiox_can_hw_t *hw, uiox_can_mode_t mode);
 int  uiox_can_hw_set_filter (uiox_can_hw_t *hw, uint8_t idx,
                               uint32_t id, uint32_t mask, bool ext);
 int  uiox_can_hw_get_err_cnt(uiox_can_hw_t *hw,
                               uiox_can_err_cnt_t *out);
 int  uiox_can_hw_recover    (uiox_can_hw_t *hw);
 
 static inline uint32_t uiox_can_caps(const uiox_can_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CAN_HW_H */
 