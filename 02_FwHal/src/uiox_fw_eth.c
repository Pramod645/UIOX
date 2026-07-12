/* ── uiox_fw_eth.c ──────────────────────────────────────────── */
#include "../include/uiox_fw_eth.h"
#define OPS_ETH(d) ((const uiox_eth_ops_t *)(d)->priv)

/* VirtIO MMIO transport — reuses virtio constants from uiox_fw_net.c */
#define VIRTIO_ETH_MMIO_STATUS  0x070u
#define VIRTIO_ETH_MMIO_DEVICE  0x008u
#define VIRTIO_ETH_STATUS_DRIVER_OK 0x04u

static uiox_fw_err_t veth_init(uiox_eth_dev_t *dev)
{
    *((volatile uint32_t*)(dev->base + VIRTIO_ETH_MMIO_STATUS)) =
        VIRTIO_ETH_STATUS_DRIVER_OK;
    dev->mac[0]=0x52u; dev->mac[1]=0x54u; dev->mac[2]=0x00u;
    dev->mac[3]=0xABu; dev->mac[4]=0xCDu; dev->mac[5]=0xEFu;
    dev->link_up=true; dev->speed_mbps=1000u;
    return UIOX_FW_OK;
}
static void veth_deinit(uiox_eth_dev_t *dev) { (void)dev; }
static uiox_fw_err_t veth_send(uiox_eth_dev_t *dev,
                                 const uint8_t *f, uint32_t l)
{
    if (!dev->link_up || l > UIOX_ETH_MTU) return UIOX_FW_ERR_IO;
    /* Ring TX doorbell */
    *((volatile uint32_t*)(dev->base + 0x050u)) = 1u;
    dev->tx_bytes += l;
    return UIOX_FW_OK;
}
static void veth_isr(uiox_eth_dev_t *dev)
{
    uint32_t st = *((volatile uint32_t*)(dev->base + 0x060u));
    *((volatile uint32_t*)(dev->base + 0x064u)) = st;
    if ((st & 1u) && dev->rx_cb)
        dev->rx_cb(NULL, 0u, dev->rx_priv);
}
static const uiox_eth_ops_t veth_ops = { veth_init, veth_deinit,
                                           veth_send, veth_isr };
uiox_fw_err_t uiox_fw_eth_init(uiox_eth_dev_t *dev, const uiox_eth_ops_t *ops)
{ if(!dev||!ops||!ops->init) return UIOX_FW_ERR_INVAL;
  dev->priv=(void*)ops; return ops->init(dev); }
void uiox_fw_eth_deinit(uiox_eth_dev_t *dev)
{ if(dev&&dev->priv&&OPS_ETH(dev)->deinit) OPS_ETH(dev)->deinit(dev); }
uiox_fw_err_t uiox_fw_eth_send(uiox_eth_dev_t *dev,const uint8_t *f,uint32_t l)
{ if(!dev||!dev->priv||!OPS_ETH(dev)->send) return UIOX_FW_ERR_INVAL;
  return OPS_ETH(dev)->send(dev,f,l); }
void uiox_fw_eth_set_rx_cb(uiox_eth_dev_t *dev,uiox_eth_rx_cb_t cb,void *p)
{ if(dev){dev->rx_cb=cb;dev->rx_priv=p;} }
uiox_fw_err_t uiox_fw_eth_init_virtio(uiox_eth_dev_t *dev,
                                         uintptr_t base, uint32_t irq)
{
    if(!dev) return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)dev; for(size_t i=0u;i<sizeof(*dev);i++) p[i]=0u;
    dev->base=base; dev->irq=irq; dev->chip=UIOX_ETH_VIRTIO;
    return uiox_fw_eth_init(dev,&veth_ops);
}
