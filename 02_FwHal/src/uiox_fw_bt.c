#include "../include/uiox_fw_bt.h"
#define OPS_BT(d) ((const uiox_bt_ops_t *)(d)->priv)

static uiox_fw_err_t bt_uart_init(uiox_bt_dev_t *dev)
{
    /* Pulse reset GPIO: low then high */
    if (dev->gpio_reset) {
        /* Platform GPIO would be called here */
        volatile uint32_t n = 100000u; while (n--) ;  /* ~1 ms */
    }
    return UIOX_FW_OK;
}

static void bt_uart_deinit(uiox_bt_dev_t *dev) { (void)dev; }

static uiox_fw_err_t bt_uart_reset(uiox_bt_dev_t *dev)
{
    /* HCI Reset command: 0x01 0x03 0x0C 0x00 */
    uint8_t cmd[4] = { HCI_CMD_PKT, 0x03u, 0x0Cu, 0x00u };
    return OPS_BT(dev)->send_hci(dev, cmd, 4u);
}

static uiox_fw_err_t bt_uart_send(uiox_bt_dev_t *dev,
                                    const uint8_t *pkt, uint32_t len)
{
    if (!dev->uart) return UIOX_FW_ERR_NODEV;
    uiox_uart_hw_t *hw = dev->uart;
    for (uint32_t i = 0u; i < len; i++)
        uiox_uart_hw_putc(hw, (char)pkt[i]);
    return UIOX_FW_OK;
}

static int32_t bt_uart_recv(uiox_bt_dev_t *dev, uint8_t *buf, uint32_t max)
{
    if (!dev->uart) return -1;
    uint32_t n = 0u;
    while (n < max && uiox_uart_hw_rx_ready(dev->uart)) {
        int c = uiox_uart_hw_getc(dev->uart);
        if (c < 0) break;
        buf[n++] = (uint8_t)c;
    }
    return (int32_t)n;
}

static uiox_fw_err_t bt_uart_load_fw(uiox_bt_dev_t *dev,
                                       const uint8_t *fw, uint32_t len)
{
    /* Vendor-specific HCI commands to upload firmware patch */
    (void)fw; (void)len;
    dev->fw_loaded = true;
    return UIOX_FW_OK;
}

static void bt_uart_isr(uiox_bt_dev_t *dev) { (void)dev; }

static const uiox_bt_ops_t bt_uart_ops = {
    .init     = bt_uart_init,
    .deinit   = bt_uart_deinit,
    .reset    = bt_uart_reset,
    .send_hci = bt_uart_send,
    .recv_hci = bt_uart_recv,
    .load_fw  = bt_uart_load_fw,
    .isr      = bt_uart_isr,
};

uiox_fw_err_t uiox_fw_bt_init(uiox_bt_dev_t *dev, const uiox_bt_ops_t *ops)
{
    if (!dev || !ops || !ops->init) return UIOX_FW_ERR_INVAL;
    dev->priv = (void *)ops;
    uiox_fw_err_t rc = ops->init(dev);
    if (rc == UIOX_FW_OK) dev->initialized = true;
    return rc;
}
void uiox_fw_bt_deinit(uiox_bt_dev_t *dev)
{ if (dev && dev->priv && OPS_BT(dev)->deinit) OPS_BT(dev)->deinit(dev); }

uiox_fw_err_t uiox_fw_bt_reset(uiox_bt_dev_t *dev)
{ if (!dev || !dev->priv || !OPS_BT(dev)->reset) return UIOX_FW_ERR_INVAL;
  return OPS_BT(dev)->reset(dev); }

uiox_fw_err_t uiox_fw_bt_send_hci(uiox_bt_dev_t *dev,
                                     const uint8_t *pkt, uint32_t len)
{ if (!dev || !dev->priv || !OPS_BT(dev)->send_hci) return UIOX_FW_ERR_INVAL;
  return OPS_BT(dev)->send_hci(dev, pkt, len); }

int32_t uiox_fw_bt_recv_hci(uiox_bt_dev_t *dev,
                               uint8_t *buf, uint32_t max_len)
{ if (!dev || !dev->priv || !OPS_BT(dev)->recv_hci) return -1;
  return OPS_BT(dev)->recv_hci(dev, buf, max_len); }

uiox_fw_err_t uiox_fw_bt_init_uart(uiox_bt_dev_t *dev,
                                     uiox_uart_hw_t *uart,
                                     uint32_t gpio_reset,
                                     uint32_t gpio_pwren)
{
    if (!dev || !uart) return UIOX_FW_ERR_INVAL;
    uint8_t *p = (uint8_t *)dev;
    for (size_t i=0u;i<sizeof(*dev);i++) p[i]=0u;
    dev->uart       = uart;
    dev->transport  = UIOX_BT_TRANSPORT_UART;
    dev->gpio_reset = gpio_reset;
    dev->gpio_pwren = gpio_pwren;
    return uiox_fw_bt_init(dev, &bt_uart_ops);
}
