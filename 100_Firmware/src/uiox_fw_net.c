/**
 * @file  uiox_fw_net.c
 * @brief UIOX Firmware — Network MAC/PHY HAL implementation.
 *
 * Supports:
 *   VirtIO-net  — QEMU VirtIO network device (MMIO transport)
 *   Loopback    — Software loopback (test/simulation)
 *   Stub        — Any unsupported device (returns ERR_UNSUP)
 *
 * Matches:
 *   30_DeviceDrivers/04_NonSecnsors  (network in non-sensor device drivers)
 *   20_DriverInterfaces/03_NonSecnsors
 *   uiox_fw_net.h public API
 *
 * @version 1.0.0
 * @date    2026-06-25
 */
//uioxfwnet.c` | VirtIO-net MMIO init (reset → ACK → DRIVER → feature negotiation → FEATURESOK → DRIVEROK → MAC read from config space), send via TX queue notify, SMSC/e1000 stub (detection only, kernel finishes init), loopback driver (copies frame to internal buffer and immediately delivers via rx_cb), IRQ handler (reads + acks VirtIO IRQ status register) |
 #include "uiox_fw.h"


 /* =========================================================================
  * VirtIO-net minimal definitions (MMIO transport, legacy spec)
  * Ref: VirtIO 1.1 §4.2 — MMIO transport
  * ====================================================================== */
/**
 * @file  uiox_fw_net.c
 * @brief UIOX Firmware — Network MAC/PHY HAL implementation.
 * @version 1.0.1  (fixed undeclared identifiers, removed unused helpers)
 * @date    2026-06-26
 */

 #include "uiox_fw.h"

 /* =========================================================================
  * VirtIO-net minimal MMIO definitions
  * ====================================================================== */
 
 #define VIRTIO_MMIO_MAGIC           0x74726976u
 #define VIRTIO_MMIO_VERSION         0x004u
 #define VIRTIO_MMIO_DEVICE_ID       0x008u
 #define VIRTIO_MMIO_VENDOR_ID       0x00Cu
 #define VIRTIO_MMIO_DEVICE_FEATURES 0x010u
 #define VIRTIO_MMIO_FEATURES_SEL    0x014u
 #define VIRTIO_MMIO_DRIVER_FEATURES 0x020u
 #define VIRTIO_MMIO_DRIVER_FEAT_SEL 0x024u
 #define VIRTIO_MMIO_QUEUE_SEL       0x030u
 #define VIRTIO_MMIO_QUEUE_NUM_MAX   0x034u
 #define VIRTIO_MMIO_QUEUE_NUM       0x038u
 #define VIRTIO_MMIO_QUEUE_READY     0x044u
 #define VIRTIO_MMIO_QUEUE_NOTIFY    0x050u
 #define VIRTIO_MMIO_IRQ_STATUS      0x060u
 #define VIRTIO_MMIO_IRQ_ACK         0x064u
 #define VIRTIO_MMIO_STATUS          0x070u
 #define VIRTIO_MMIO_QUEUE_DESC_LO   0x080u
 #define VIRTIO_MMIO_QUEUE_DESC_HI   0x084u
 #define VIRTIO_MMIO_QUEUE_AVAIL_LO  0x090u
 #define VIRTIO_MMIO_QUEUE_AVAIL_HI  0x094u
 #define VIRTIO_MMIO_QUEUE_USED_LO   0x0A0u
 #define VIRTIO_MMIO_QUEUE_USED_HI   0x0A4u
 #define VIRTIO_MMIO_CONFIG_SPACE    0x100u
 
 #define VIRTIO_STATUS_ACK           1u
 #define VIRTIO_STATUS_DRIVER        2u
 #define VIRTIO_STATUS_DRIVER_OK     4u
 #define VIRTIO_STATUS_FEATURES_OK   8u
 #define VIRTIO_STATUS_FAILED        128u
 #define VIRTIO_STATUS_RESET         0u
 
 #define VIRTIO_DEVICE_ID_NET        1u
 
 #define VIRTIO_NET_F_MAC            (1u << 5)
 #define VIRTIO_NET_F_STATUS         (1u << 16)
 
 #define VIRTIO_NET_CFG_MAC_OFFSET   0x00u
 #define VIRTIO_NET_CFG_STATUS       0x06u
 #define VIRTIO_NET_STATUS_LINK_UP   (1u << 0)
 
 #define VIRTIO_NET_Q_TX             1u
 
 /* =========================================================================
  * Loopback buffer
  * ====================================================================== */
 
 #define LOOPBACK_BUF_SIZE   UIOX_FW_NET_MTU
 
 static uint8_t  s_loopback_buf[LOOPBACK_BUF_SIZE];
 static uint32_t s_loopback_len = 0u;
 
 /* =========================================================================
  * VirtIO-net driver
  * ====================================================================== */
 
 static uiox_fw_err_t virtio_net_init(void *priv)
 {
     uiox_fw_net_dev_t *dev = (uiox_fw_net_dev_t *)priv;
     uintptr_t          b   = dev->base;
 
     if (b == 0u) return UIOX_FW_ERR_NODEV;
 
     /* ── Verify VirtIO magic ─────────────────────────────────────── */
     uint32_t magic = fw_mmio_read32(b);
     if (magic != VIRTIO_MMIO_MAGIC) {
         FW_ERR("VirtIO-net: bad magic 0x%08x at %p",
                magic, (void *)b);
         return UIOX_FW_ERR_BADMAGIC;   /* defined in uiox_fw_types.h */
     }
 
     /* ── Check device type ──────────────────────────────────────── */
     uint32_t dev_id = fw_mmio_read32(b + VIRTIO_MMIO_DEVICE_ID);
     if (dev_id != VIRTIO_DEVICE_ID_NET) {
         FW_ERR("VirtIO-net: not a net device (id=%u)", dev_id);
         return UIOX_FW_ERR_NODEV;
     }
 
     /* ── Reset ──────────────────────────────────────────────────── */
     fw_mmio_write32(b + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_RESET);
     uiox_fw_hw_barrier();    /* declared in uiox_fw_hw.h, defined in hw.c */
 
     /* ── ACK + DRIVER ────────────────────────────────────────────── */
     fw_mmio_write32(b + VIRTIO_MMIO_STATUS,
                      VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER);
 
     /* ── Negotiate features ─────────────────────────────────────── */
     fw_mmio_write32(b + VIRTIO_MMIO_FEATURES_SEL, 0u);
     uint32_t dev_features = fw_mmio_read32(b + VIRTIO_MMIO_DEVICE_FEATURES);
     uint32_t drv_features = dev_features &
                             (VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS);
 
     fw_mmio_write32(b + VIRTIO_MMIO_DRIVER_FEAT_SEL, 0u);
     fw_mmio_write32(b + VIRTIO_MMIO_DRIVER_FEATURES, drv_features);
 
     /* ── FEATURES_OK ─────────────────────────────────────────────── */
     uint32_t status = VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER |
                       VIRTIO_STATUS_FEATURES_OK;
     fw_mmio_write32(b + VIRTIO_MMIO_STATUS, status);
     uiox_fw_hw_barrier();
 
     if (!(fw_mmio_read32(b + VIRTIO_MMIO_STATUS) &
           VIRTIO_STATUS_FEATURES_OK)) {
         FW_ERR("VirtIO-net: features not accepted");
         fw_mmio_write32(b + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
         return UIOX_FW_ERR_IO;
     }
 
     /* ── DRIVER_OK ───────────────────────────────────────────────── */
     fw_mmio_write32(b + VIRTIO_MMIO_STATUS,
                      status | VIRTIO_STATUS_DRIVER_OK);
     uiox_fw_hw_barrier();
 
     /* ── Read MAC ────────────────────────────────────────────────── */
     if (drv_features & VIRTIO_NET_F_MAC) {
         for (uint32_t i = 0u; i < UIOX_FW_NET_MAC_LEN; i++) {
             dev->mac[i] = fw_mmio_read8(b + VIRTIO_MMIO_CONFIG_SPACE +
                                           VIRTIO_NET_CFG_MAC_OFFSET + i);
         }
     } else {
         dev->mac[0] = 0x52u;
         dev->mac[1] = 0x54u;
         dev->mac[2] = 0x00u;
         dev->mac[3] = 0x12u;
         dev->mac[4] = 0x34u;
         dev->mac[5] = 0x56u;
     }
 
     /* ── Link status ─────────────────────────────────────────────── */
     if (drv_features & VIRTIO_NET_F_STATUS) {
         uint16_t link =
             (uint16_t)fw_mmio_read32(b + VIRTIO_MMIO_CONFIG_SPACE +
                                        VIRTIO_NET_CFG_STATUS);
         dev->link_up = !!(link & VIRTIO_NET_STATUS_LINK_UP);
     } else {
         dev->link_up = true;
     }
     dev->speed_mbps = 1000u;
 
     FW_LOG("NET", "VirtIO-net OK  MAC=%02x:%02x:%02x:%02x:%02x:%02x  %s",
            dev->mac[0], dev->mac[1], dev->mac[2],
            dev->mac[3], dev->mac[4], dev->mac[5],
            dev->link_up ? "LINK UP" : "LINK DOWN");
 
     return UIOX_FW_OK;
 }
 
 static uiox_fw_err_t virtio_net_send(void *priv,
                                       const uint8_t *frame, uint32_t len)
 {
     uiox_fw_net_dev_t *dev = (uiox_fw_net_dev_t *)priv;
     if (!frame || len == 0u || len > UIOX_FW_NET_MTU)
         return UIOX_FW_ERR_INVAL;
     if (!dev->link_up) return UIOX_FW_ERR_IO;
 
     fw_mmio_write32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY,
                      VIRTIO_NET_Q_TX);
     uiox_fw_hw_barrier();
 
     dev->tx_frames++;
     dev->tx_bytes += len;
     return UIOX_FW_OK;
 }
 
 /* =========================================================================
  * Loopback driver
  * ====================================================================== */
 
 static uiox_fw_err_t loopback_init(void *priv)
 {
     uiox_fw_net_dev_t *dev = (uiox_fw_net_dev_t *)priv;
     dev->mac[0] = 0x02u;
     dev->mac[1] = 0x00u;
     dev->mac[2] = 0x00u;
     dev->mac[3] = 0x00u;
     dev->mac[4] = 0x00u;
     dev->mac[5] = 0x01u;
     dev->link_up    = true;
     dev->speed_mbps = 0u;
     s_loopback_len  = 0u;
     FW_LOG("NET", "loopback init OK");
     return UIOX_FW_OK;
 }
 
 static uiox_fw_err_t loopback_send(void *priv,
                                     const uint8_t *frame, uint32_t len)
 {
     uiox_fw_net_dev_t *dev = (uiox_fw_net_dev_t *)priv;
     if (!frame || len == 0u || len > LOOPBACK_BUF_SIZE)
         return UIOX_FW_ERR_INVAL;
 
     uiox_fw_memcpy(s_loopback_buf, frame, len);
     s_loopback_len = len;
 
     dev->tx_frames++;
     dev->tx_bytes += len;
     dev->rx_frames++;
     dev->rx_bytes += len;
 
     if (dev->rx_cb)
         dev->rx_cb(s_loopback_buf, s_loopback_len, dev->rx_priv);
 
     return UIOX_FW_OK;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_net_init(uiox_fw_net_dev_t *dev)
 {
     if (!dev) return UIOX_FW_ERR_INVAL;
 
     dev->tx_frames = 0u;
     dev->rx_frames = 0u;
     dev->tx_bytes  = 0u;
     dev->rx_bytes  = 0u;
     dev->errors    = 0u;
     dev->rx_cb     = NULL;
     dev->rx_priv   = NULL;
 
     uiox_fw_err_t rc;
 
     switch (dev->type) {
     case UIOX_FW_NET_VIRTIO:
         dev->priv = (void *)dev;
         dev->init = virtio_net_init;
         dev->send = virtio_net_send;
         rc = dev->init(dev->priv);
         break;
 
     case UIOX_FW_NET_LOOPBACK:
         dev->priv = (void *)dev;
         dev->init = loopback_init;
         dev->send = loopback_send;
         rc = dev->init(dev->priv);
         break;
 
     case UIOX_FW_NET_SMSC:
     case UIOX_FW_NET_E1000:
         dev->link_up    = false;
         dev->speed_mbps = 0u;
         FW_LOG("NET", "%s: detected (kernel driver will init)", dev->name);
         rc = UIOX_FW_OK;
         break;
 
     default:
         FW_ERR("NET: unknown device type %u", (uint32_t)dev->type);
         return UIOX_FW_ERR_UNSUP;
     }
 
     if (rc != UIOX_FW_OK) {
         dev->errors++;
         FW_ERR("NET: %s init failed (%d)", dev->name, (int)rc);
     }
     return rc;
 }
 
 uiox_fw_err_t uiox_fw_net_send(uiox_fw_net_dev_t *dev,
                                  const uint8_t *frame, uint32_t len)
 {
     if (!dev || !frame || len == 0u) return UIOX_FW_ERR_INVAL;
     if (!dev->send)                  return UIOX_FW_ERR_UNSUP;
     if (len > UIOX_FW_NET_MTU)       return UIOX_FW_ERR_OVERFLOW;
 
     uiox_fw_err_t rc = dev->send(dev->priv, frame, len);
     if (rc != UIOX_FW_OK) dev->errors++;
     return rc;
 }
 
 void uiox_fw_net_set_rx_cb(uiox_fw_net_dev_t *dev,
                              uiox_fw_net_rx_cb_t cb, void *priv)
 {
     if (!dev) return;
     dev->rx_cb   = cb;
     dev->rx_priv = priv;
 }
 
 void uiox_fw_net_irq(uiox_fw_net_dev_t *dev)
 {
     if (!dev) return;
 
     if (dev->type == UIOX_FW_NET_VIRTIO && dev->base != 0u) {
         uint32_t irq_status =
             fw_mmio_read32(dev->base + VIRTIO_MMIO_IRQ_STATUS);
         if (!irq_status) return;
 
         fw_mmio_write32(dev->base + VIRTIO_MMIO_IRQ_ACK, irq_status);
 
         if (irq_status & 0x01u) {
             dev->rx_frames++;
             if (dev->rx_cb)
                 dev->rx_cb(NULL, 0u, dev->rx_priv);
         }
     }
 }
 
 void uiox_fw_net_print(const uiox_fw_net_dev_t *dev)
 {
     if (!dev) return;
     static const char *type_names[] = {
         "virtio-net", "SMSC-LAN", "e1000", "loopback"
     };
     uint8_t t = (uint8_t)dev->type;
     uiox_fw_printf("[FW] NET  %-12s  type=%-12s  link=%-5s  %u Mbps\n",
                     dev->name,
                     t < 4u ? type_names[t] : "?",
                     dev->link_up ? "UP" : "DOWN",
                     dev->speed_mbps);
     uiox_fw_printf("         MAC=%02x:%02x:%02x:%02x:%02x:%02x"
                     "  tx=%llu/%llu B  rx=%llu/%llu B  err=%u\n",
                     dev->mac[0], dev->mac[1], dev->mac[2],
                     dev->mac[3], dev->mac[4], dev->mac[5],
                     (unsigned long long)dev->tx_frames,
                     (unsigned long long)dev->tx_bytes,
                     (unsigned long long)dev->rx_frames,
                     (unsigned long long)dev->rx_bytes,
                     dev->errors);
 }
 