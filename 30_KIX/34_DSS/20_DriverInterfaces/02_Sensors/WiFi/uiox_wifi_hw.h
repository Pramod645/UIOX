/**
 * @file    uiox_wifi_hw.h
 * @brief   UIOX WiFi Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to WiFi MAC/PHY hardware. Owns:
 *   - MMIO register access to WiFi baseband / MAC controller
 *   - RF front-end and PLL/synthesiser programming
 *   - DMA engine for TX/RX frame transfer
 *   - IRQ handling (TX complete, RX ready, beacon, radar)
 *   - Channel/frequency programming
 *   - Transmit power control
 *   - Antenna diversity control
 *
 * Supports:
 *   - 802.11 b/g/n (2.4 GHz)
 *   - 802.11 a/n/ac (5 GHz)
 *   - 802.11ax (Wi-Fi 6 / 6E)
 *   - SPI / SDIO / PCIe attached chipsets
 *
 * @version 1.0.0
 * @date    2026-05-28
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_WIFI_HW_H
 #define UIOX_WIFI_HW_H
 
#include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_WIFI_CAP_2G4           (1u << 0)   /**< 2.4 GHz band          */
 #define UIOX_WIFI_CAP_5G            (1u << 1)   /**< 5 GHz band            */
 #define UIOX_WIFI_CAP_6G            (1u << 2)   /**< 6 GHz band (Wi-Fi 6E) */
 #define UIOX_WIFI_CAP_11N           (1u << 3)   /**< 802.11n HT            */
 #define UIOX_WIFI_CAP_11AC          (1u << 4)   /**< 802.11ac VHT          */
 #define UIOX_WIFI_CAP_11AX          (1u << 5)   /**< 802.11ax HE           */
 #define UIOX_WIFI_CAP_DMA           (1u << 6)   /**< DMA TX/RX             */
 #define UIOX_WIFI_CAP_HW_CRYPT     (1u << 7)   /**< HW AES/CCMP engine    */
 #define UIOX_WIFI_CAP_AMPDU_TX      (1u << 8)   /**< TX A-MPDU aggregation */
 #define UIOX_WIFI_CAP_AMPDU_RX      (1u << 9)   /**< RX A-MPDU de-aggr.   */
 #define UIOX_WIFI_CAP_MIMO_2T2R     (1u << 10)  /**< 2×2 MIMO              */
 #define UIOX_WIFI_CAP_MIMO_4T4R     (1u << 11)  /**< 4×4 MIMO              */
 #define UIOX_WIFI_CAP_MU_MIMO       (1u << 12)  /**< MU-MIMO               */
 #define UIOX_WIFI_CAP_BEAMFORMING   (1u << 13)  /**< Tx beamforming        */
 #define UIOX_WIFI_CAP_WMM           (1u << 14)  /**< WMM QoS               */
 #define UIOX_WIFI_CAP_WPS           (1u << 15)  /**< WPS push-button       */
 #define UIOX_WIFI_CAP_P2P           (1u << 16)  /**< Wi-Fi Direct P2P      */
 #define UIOX_WIFI_CAP_MONITOR       (1u << 17)  /**< Monitor/sniffer mode  */
 #define UIOX_WIFI_CAP_STA_AP        (1u << 18)  /**< Concurrent STA+AP     */
 #define UIOX_WIFI_CAP_WOWLAN        (1u << 19)  /**< Wake-on-WLAN          */
 
 /* =========================================================================
  * Bus interface types
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_BUS_SDIO = 0,
     UIOX_WIFI_BUS_SPI,
     UIOX_WIFI_BUS_PCIE,
     UIOX_WIFI_BUS_USB,
     UIOX_WIFI_BUS_AHB,    /**< On-chip (directly memory-mapped)           */
 } uiox_wifi_bus_t;
 
 /* =========================================================================
  * Operating mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_MODE_STA = 0,   /**< Station (client)                       */
     UIOX_WIFI_MODE_AP,         /**< Access point (soft-AP)                 */
     UIOX_WIFI_MODE_MONITOR,    /**< Monitor / passive sniffer              */
     UIOX_WIFI_MODE_P2P_GO,     /**< P2P Group Owner                        */
     UIOX_WIFI_MODE_P2P_CLIENT, /**< P2P Client                             */
 } uiox_wifi_mode_t;
 
 /* =========================================================================
  * Channel bandwidth
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_BW_20MHZ = 0,
     UIOX_WIFI_BW_40MHZ,
     UIOX_WIFI_BW_80MHZ,
     UIOX_WIFI_BW_160MHZ,
 } uiox_wifi_bw_t;
 
 /* =========================================================================
  * MAC address
  * ====================================================================== */
 
 #define UIOX_WIFI_MAC_LEN       6
 typedef uint8_t uiox_wifi_mac_t[UIOX_WIFI_MAC_LEN];
 
 /* =========================================================================
  * DMA descriptor
  * ====================================================================== */
 
 #define UIOX_WIFI_DMA_DESC_ALIGN  64
 
 typedef struct __attribute__((packed, aligned(UIOX_WIFI_DMA_DESC_ALIGN))) {
     volatile uint32_t  status;      /**< OWN + done/error flags            */
     uint32_t           ctrl;        /**< Length + interrupt + rate info    */
     uint32_t           buf_lo;      /**< Physical address lo               */
     uint32_t           buf_hi;      /**< Physical address hi               */
     uint32_t           tsf_lo;      /**< Timestamp (TSF low 32 bits)       */
     uint32_t           tsf_hi;
     uint32_t           reserved[2];
 } uiox_wifi_dma_desc_t;
 
 #define UIOX_WIFI_DESC_OWN    (1u << 31)
 #define UIOX_WIFI_DESC_DONE   (1u << 1)
 #define UIOX_WIFI_DESC_ERR    (1u << 0)
 #define UIOX_WIFI_DESC_EOR    (1u << 30)  /**< End of ring                 */
 
 /* =========================================================================
  * RF/Channel configuration
  * ====================================================================== */
 
 typedef struct {
     uint8_t       channel;        /**< 802.11 channel number               */
     uint32_t      freq_mhz;       /**< Centre frequency (MHz)              */
     uiox_wifi_bw_t bw;            /**< Channel bandwidth                   */
     int8_t        tx_power_dbm;   /**< TX power (dBm); −1 = regulatory max*/
     bool          is_5ghz;
     bool          is_6ghz;
 } uiox_wifi_channel_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t           base_addr;    /**< MMIO base                       */
     uint32_t            irq;          /**< IRQ line                        */
     uint32_t            caps;         /**< UIOX_WIFI_CAP_* bitmask        */
     uiox_wifi_bus_t     bus;
     uiox_wifi_mode_t    mode;
     uiox_wifi_mac_t     mac_addr;
     uiox_wifi_channel_t channel;
     uint32_t            clk_hz;
 
     /* DMA rings */
     uiox_wifi_dma_desc_t *tx_ring;
     uiox_wifi_dma_desc_t *rx_ring;
     uint16_t              tx_ring_sz;
     uint16_t              rx_ring_sz;
     uint16_t              tx_head;
     uint16_t              tx_tail;
     uint16_t              rx_head;
 
     /* RF state */
     int8_t    rssi_dbm;         /**< Last measured RSSI                    */
     int8_t    noise_floor_dbm;  /**< Noise floor estimate                  */
     uint8_t   tx_rate_idx;      /**< Current TX rate index                 */
     bool      associated;
 
     void     *priv;
 } uiox_wifi_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_wifi_hw_t *hw);
     void (*deinit)        (uiox_wifi_hw_t *hw);
     int  (*start)         (uiox_wifi_hw_t *hw);
     void (*stop)          (uiox_wifi_hw_t *hw);
     int  (*set_mode)      (uiox_wifi_hw_t *hw, uiox_wifi_mode_t mode);
     int  (*set_channel)   (uiox_wifi_hw_t *hw,
                            const uiox_wifi_channel_t *ch);
     int  (*set_mac)       (uiox_wifi_hw_t *hw,
                            const uiox_wifi_mac_t mac);
     int  (*set_tx_power)  (uiox_wifi_hw_t *hw, int8_t dbm);
 
     /* DMA TX/RX */
     int  (*tx_submit)     (uiox_wifi_hw_t *hw,
                            uintptr_t phys, uint32_t length,
                            uint8_t rate_idx, bool ampdu);
     int  (*tx_reclaim)    (uiox_wifi_hw_t *hw);
     int  (*rx_poll)       (uiox_wifi_hw_t *hw,
                            uintptr_t *phys_out,
                            uint32_t  *len_out,
                            int8_t    *rssi_out);
 
     /* Hardware crypto */
     int  (*hw_key_install)(uiox_wifi_hw_t *hw, uint8_t key_idx,
                            const uint8_t *key, uint8_t key_len,
                            const uiox_wifi_mac_t peer,
                            bool is_group, uint8_t cipher);
     void (*hw_key_delete) (uiox_wifi_hw_t *hw, uint8_t key_idx);
 
     /* RF measurements */
     int  (*get_rssi)      (uiox_wifi_hw_t *hw, int8_t *rssi_dbm);
     int  (*get_noise)     (uiox_wifi_hw_t *hw, int8_t *noise_dbm);
 
     /* ISRs */
     void (*isr_tx)        (uiox_wifi_hw_t *hw);
     void (*isr_rx)        (uiox_wifi_hw_t *hw);
     void (*isr_bcn)       (uiox_wifi_hw_t *hw);
     void (*isr_err)       (uiox_wifi_hw_t *hw);
 
     /* SPI/SDIO bus ops (for external chipsets) */
     int  (*bus_read)      (uiox_wifi_hw_t *hw,
                            uint32_t addr, void *buf, uint16_t len);
     int  (*bus_write)     (uiox_wifi_hw_t *hw,
                            uint32_t addr, const void *buf, uint16_t len);
 } uiox_wifi_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_wifi_hw_init       (uiox_wifi_hw_t *hw,
                                const uiox_wifi_hw_ops_t *ops);
 void uiox_wifi_hw_deinit     (uiox_wifi_hw_t *hw);
 int  uiox_wifi_hw_start      (uiox_wifi_hw_t *hw);
 void uiox_wifi_hw_stop       (uiox_wifi_hw_t *hw);
 int  uiox_wifi_hw_set_channel(uiox_wifi_hw_t *hw,
                                const uiox_wifi_channel_t *ch);
 int  uiox_wifi_hw_set_mode   (uiox_wifi_hw_t *hw, uiox_wifi_mode_t mode);
 int  uiox_wifi_hw_get_rssi   (uiox_wifi_hw_t *hw, int8_t *rssi_dbm);
 
 static inline uint32_t uiox_wifi_caps(const uiox_wifi_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_WIFI_HW_H */
 