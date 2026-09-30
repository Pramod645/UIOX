/**
 * @file    uiox_tb4_hw.h
 * @brief   UIOX Thunderbolt 4 Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - Intel JHL8540 Thunderbolt 4 controller (Maple Ridge)
 *   - Intel GRL-USB4 (Goshen Ridge)
 *   - Titan Ridge (Thunderbolt 3 compatible)
 *   - USB4 v1.0 / v2.0 compliant controllers
 *
 * Owns:
 *   - NHI (Native Host Interface) MMIO register access
 *   - TX/RX ring DMA descriptor management
 *   - PCIe BAR0/BAR1 configuration space
 *   - Force power GPIO (FRC_PWR)
 *   - ICM (Internal Connection Manager) communication
 *   - Cable power control (USB-C VBus)
 *   - IRQ: ring complete, hotplug, error, ICM response
 *
 * @version 1.0.0
 * @date    2026-06-08
 */

 #ifndef UIOX_TB4_HW_H
 #define UIOX_TB4_HW_H
 
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Thunderbolt version
  * ====================================================================== */
 
 typedef enum {
     UIOX_TB4_VER_TB3   = 0,   /**< Thunderbolt 3 (40 Gbit/s)             */
     UIOX_TB4_VER_TB4,          /**< Thunderbolt 4 (40 Gbit/s guaranteed)  */
     UIOX_TB4_VER_USB4_V1,      /**< USB4 v1.0 (40 Gbit/s)                */
     UIOX_TB4_VER_USB4_V2,      /**< USB4 v2.0 (80 Gbit/s)                */
 } uiox_tb4_ver_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_TB4_CAP_40GBPS         (1u << 0)  /**< 40 Gbit/s bandwidth  */
 #define UIOX_TB4_CAP_80GBPS         (1u << 1)  /**< 80 Gbit/s (USB4 v2)  */
 #define UIOX_TB4_CAP_PCIE_TUNNEL    (1u << 2)  /**< PCIe tunneling        */
 #define UIOX_TB4_CAP_DP_TUNNEL      (1u << 3)  /**< DisplayPort tunneling */
 #define UIOX_TB4_CAP_USB3_TUNNEL    (1u << 4)  /**< USB 3.x tunneling     */
 #define UIOX_TB4_CAP_USB2_TUNNEL    (1u << 5)  /**< USB 2.0 tunneling     */
 #define UIOX_TB4_CAP_XDOMAIN        (1u << 6)  /**< XDomain P2P protocol  */
 #define UIOX_TB4_CAP_DMA_TUNNEL     (1u << 7)  /**< DMA tunneling (P2P)   */
 #define UIOX_TB4_CAP_DUAL_PORT      (1u << 8)  /**< Dual USB-C ports      */
 #define UIOX_TB4_CAP_DAISY_CHAIN    (1u << 9)  /**< Daisy-chaining (6 dev)*/
 #define UIOX_TB4_CAP_SECURITY       (1u << 10) /**< Security levels       */
 #define UIOX_TB4_CAP_WAKE_ON_CABLE  (1u << 11) /**< Wake from sleep on plug*/
 #define UIOX_TB4_CAP_ICM            (1u << 12) /**< Internal Connection Mgr*/
 #define UIOX_TB4_CAP_IOMMU          (1u << 13) /**< IOMMU-backed DMA      */
 
 /* =========================================================================
  * Security level
  * ====================================================================== */
 
 typedef enum {
     UIOX_TB4_SEC_NONE       = 0,  /**< No security (open)                 */
     UIOX_TB4_SEC_USER_AUTH,        /**< User authorisation required        */
     UIOX_TB4_SEC_SECURE_CONNECT,   /**< Secure Connect (challenge)         */
     UIOX_TB4_SEC_DP_ONLY,          /**< Display-only (no PCIe)            */
     UIOX_TB4_SEC_USB_ONLY,         /**< USB tunnels only                  */
 } uiox_tb4_sec_t;
 
 /* =========================================================================
  * NHI register offsets (Intel Maple Ridge / Goshen Ridge)
  * ====================================================================== */
 
 #define NHI_FREQS_VALID             0x0000u
 #define NHI_OUTMAILBOX              0x0004u
 #define NHI_INMAILBOX               0x0008u
 #define NHI_OUTMAILBOX_CMD          0x000Cu
 #define NHI_INMAILBOX_CMD           0x0010u
 #define NHI_DMABASE_LO              0x0018u
 #define NHI_DMABASE_HI              0x001Cu
 #define NHI_TX_RING_BASE_LO(n)      (0x0400u + (n)*0x10u)
 #define NHI_TX_RING_BASE_HI(n)      (0x0404u + (n)*0x10u)
 #define NHI_TX_RING_SIZE(n)         (0x0408u + (n)*0x10u)
 #define NHI_TX_RING_CONS(n)         (0x040Cu + (n)*0x10u)
 #define NHI_RX_RING_BASE_LO(n)      (0x0600u + (n)*0x10u)
 #define NHI_RX_RING_BASE_HI(n)      (0x0604u + (n)*0x10u)
 #define NHI_RX_RING_SIZE(n)         (0x0608u + (n)*0x10u)
 #define NHI_RX_RING_CONS(n)         (0x060Cu + (n)*0x10u)
 #define NHI_INTERRUPT_STATUS        0x0800u
 #define NHI_INTERRUPT_MASK          0x0804u
 #define NHI_INTERRUPT_MASK_SET      0x0808u
 #define NHI_INTERRUPT_MASK_CLR      0x080Cu
 #define NHI_ICM_PP                  0x0820u  /**< ICM producer pointer     */
 #define NHI_ICM_CP                  0x0824u  /**< ICM consumer pointer     */
 #define NHI_POWER_STATE             0x0880u
 
 /* NHI interrupt bits */
 #define NHI_INT_TX_RING(n)          (1u << (n))
 #define NHI_INT_RX_RING(n)          (1u << ((n) + 8u))
 #define NHI_INT_ICM                 (1u << 16u)
 #define NHI_INT_HOTPLUG             (1u << 17u)
 #define NHI_INT_ERROR               (1u << 18u)
 
 /* =========================================================================
  * DMA ring descriptor
  * ====================================================================== */
 
 #define UIOX_TB4_RING_DESC_ALIGN    16u
 #define UIOX_TB4_RING_SIZE          256
 typedef struct __attribute__((packed, aligned(UIOX_TB4_RING_DESC_ALIGN))) {
    uint32_t  buf_lo;       /**< Buffer physical address lo                */
    uint32_t  buf_hi;       /**< Buffer physical address hi                */
    uint32_t  len;          /**< Byte length of this descriptor            */
    uint32_t  flags;        /**< Ownership + status flags                  */
} uiox_tb4_desc_t;

#define TB4_DESC_OWN        (1u << 31)  /**< 1=owned by HW                */
#define TB4_DESC_DONE       (1u << 30)
#define TB4_DESC_EOF        (1u << 29)  /**< End-of-frame                  */
#define TB4_DESC_SOF        (1u << 28)  /**< Start-of-frame                */
#define TB4_DESC_ERROR      (1u << 27)

/* =========================================================================
 * Hardware device descriptor
 * ====================================================================== */

#define UIOX_TB4_MODEL_LEN      48
#define UIOX_TB4_UUID_LEN       16
#define UIOX_TB4_MAX_RINGS      12   /**< TX + RX rings per NHI           */
#define UIOX_TB4_MAX_PORTS      2    /**< USB-C physical ports             */
#define UIOX_TB4_MAX_ROUTERS    7    /**< Host + 6 downstream routers      */

typedef struct {
    uintptr_t           nhi_base;     /**< NHI MMIO BAR0 base              */
    uintptr_t           cfg_base;     /**< Router config space MMIO BAR1   */
    uint32_t            irq;          /**< MSI/MSI-X IRQ line              */
    uint32_t            caps;
    uiox_tb4_ver_t      version;
    uiox_tb4_sec_t      security;
    char                model[UIOX_TB4_MODEL_LEN];
    uint8_t             uuid[UIOX_TB4_UUID_LEN]; /**< Controller UUID      */
    uint8_t             num_ports;
    uint8_t             num_tx_rings;
    uint8_t             num_rx_rings;

    /* DMA ring descriptors (aligned, DMA-coherent) */
    uiox_tb4_desc_t    *tx_ring;      /**< TX descriptor ring              */
    uiox_tb4_desc_t    *rx_ring;      /**< RX descriptor ring              */
    uint16_t            tx_prod;      /**< TX producer index               */
    uint16_t            tx_cons;      /**< TX consumer index               */
    uint16_t            rx_prod;      /**< RX producer index               */
    uint16_t            rx_cons;      /**< RX consumer index               */

    /* GPIO */
    uint32_t            frc_pwr_pin;  /**< Force power GPIO pin            */
    uint32_t            plug_det_pin; /**< Cable detect GPIO               */

    /* State */
    bool                powered;
    bool                icm_ready;    /**< ICM firmware loaded             */
    volatile uint32_t   pending_irq;  /**< IRQ status word                 */

    void               *priv;
} uiox_tb4_hw_t;

/* =========================================================================
 * Hardware operations vtable
 * ====================================================================== */

typedef struct {
    int  (*init)          (uiox_tb4_hw_t *hw);
    void (*deinit)        (uiox_tb4_hw_t *hw);
    int  (*power_on)      (uiox_tb4_hw_t *hw);
    void (*power_off)     (uiox_tb4_hw_t *hw);

    /* NHI MMIO */
    uint32_t (*nhi_read)  (uiox_tb4_hw_t *hw, uint32_t offset);
    void     (*nhi_write) (uiox_tb4_hw_t *hw, uint32_t offset,
                           uint32_t val);

    /* Router config space access (indirect MMIO) */
    int  (*cfg_read)      (uiox_tb4_hw_t *hw,
                           uint8_t route_hi, uint32_t route_lo,
                           uint32_t offset, uint32_t *val);
    int  (*cfg_write)     (uiox_tb4_hw_t *hw,
                           uint8_t route_hi, uint32_t route_lo,
                           uint32_t offset, uint32_t val);

    /* ICM (Internal Connection Manager) mailbox */
    int  (*icm_send)      (uiox_tb4_hw_t *hw,
                           const uint32_t *msg, uint8_t dwords);
    int  (*icm_recv)      (uiox_tb4_hw_t *hw,
                           uint32_t *msg, uint8_t max_dwords);

    /* TX / RX ring */
    int  (*tx_submit)     (uiox_tb4_hw_t *hw,
                           uintptr_t phys, uint32_t len, bool eof);
    int  (*rx_poll)       (uiox_tb4_hw_t *hw,
                           uintptr_t *phys_out, uint32_t *len_out);

    /* GPIO */
    void (*gpio_write)    (uiox_tb4_hw_t *hw, uint32_t pin, bool val);
    bool (*gpio_read)     (uiox_tb4_hw_t *hw, uint32_t pin);

    /* ISRs */
    void (*isr_ring)      (uiox_tb4_hw_t *hw);
    void (*isr_hotplug)   (uiox_tb4_hw_t *hw);
    void (*isr_icm)       (uiox_tb4_hw_t *hw);
    void (*isr_error)     (uiox_tb4_hw_t *hw);
} uiox_tb4_hw_ops_t;

/* =========================================================================
 * HAL public API
 * ====================================================================== */

int      uiox_tb4_hw_init      (uiox_tb4_hw_t *hw,
                                 const uiox_tb4_hw_ops_t *ops);
void     uiox_tb4_hw_deinit    (uiox_tb4_hw_t *hw);
int      uiox_tb4_hw_power_on  (uiox_tb4_hw_t *hw);
void     uiox_tb4_hw_power_off (uiox_tb4_hw_t *hw);
uint32_t uiox_tb4_hw_nhi_read  (uiox_tb4_hw_t *hw, uint32_t offset);
void     uiox_tb4_hw_nhi_write (uiox_tb4_hw_t *hw, uint32_t offset,
                                 uint32_t val);
int      uiox_tb4_hw_icm_send  (uiox_tb4_hw_t *hw,
                                 const uint32_t *msg, uint8_t dwords);
int      uiox_tb4_hw_icm_recv  (uiox_tb4_hw_t *hw,
                                 uint32_t *msg, uint8_t max_dwords);
int      uiox_tb4_hw_tx_submit (uiox_tb4_hw_t *hw,
                                 uintptr_t phys, uint32_t len, bool eof);
int      uiox_tb4_hw_rx_poll   (uiox_tb4_hw_t *hw,
                                 uintptr_t *phys_out, uint32_t *len_out);

static inline uint32_t uiox_tb4_caps(const uiox_tb4_hw_t *hw)
{ return hw ? hw->caps : 0u; }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_TB4_HW_H */
 