/* ── uiox_fw_usb.c ──────────────────────────────────────────── */
#include "../include/uiox_fw_usb.h"
#define OPS_USB(d) ((const uiox_usb_ops_t*)(d)->priv)
static inline void xw(uintptr_t b,uint32_t o,uint32_t v){*((volatile uint32_t*)(b+o))=v;}
static inline uint32_t xr(uintptr_t b,uint32_t o){return *((volatile uint32_t*)(b+o));}

static uiox_fw_err_t xhci_init(uiox_usb_hc_dev_t *d)
{
    /* Read CAPLENGTH */
    uint8_t caplen = (uint8_t)(xr(d->mmio_base, XHCI_CAPLENGTH) & 0xFFu);
    uintptr_t op = d->mmio_base + caplen;
    /* Reset controller */
    xw(op, XHCI_USBCMD, XHCI_USBCMD_HCRST);
    for(uint32_t i=0u;i<100000u;i++)
        if(!(xr(op,XHCI_USBCMD)&XHCI_USBCMD_HCRST))break;
    /* Wait halted */
    for(uint32_t i=0u;i<100000u;i++)
        if(xr(op,XHCI_USBSTS)&XHCI_USBSTS_HCH)break;
    /* Run */
    xw(op, XHCI_USBCMD, XHCI_USBCMD_RUN);
    d->num_ports=(uint8_t)((xr(d->mmio_base,XHCI_HCSPARAMS1)>>24u)&0xFFu);
    d->initialized=true;
    return UIOX_FW_OK;
}
static void xhci_deinit(uiox_usb_hc_dev_t *d)
{
    uint8_t caplen=(uint8_t)(xr(d->mmio_base,XHCI_CAPLENGTH)&0xFFu);
    xw(d->mmio_base+caplen, XHCI_USBCMD, 0u);
    d->initialized=false;
}
static uiox_fw_err_t xhci_port_reset(uiox_usb_hc_dev_t *d,uint8_t port)
{ (void)d;(void)port;return UIOX_FW_OK; }
static uiox_fw_err_t xhci_enumerate(uiox_usb_hc_dev_t *d)
{ d->num_devices=0u;return UIOX_FW_OK; }
static uiox_fw_err_t xhci_ctrl_xfer(uiox_usb_hc_dev_t *d,uint8_t addr,
    uint8_t bmReqType,uint8_t bRequest,uint16_t wValue,uint16_t wIndex,
    void *data,uint16_t wLength)
{ (void)d;(void)addr;(void)bmReqType;(void)bRequest;
  (void)wValue;(void)wIndex;(void)data;(void)wLength;return UIOX_FW_OK; }
static uiox_fw_err_t xhci_bulk_xfer(uiox_usb_hc_dev_t *d,uint8_t addr,
    uint8_t ep,void *buf,uint32_t len)
{ (void)d;(void)addr;(void)ep;(void)buf;(void)len;return UIOX_FW_OK; }
static void xhci_isr(uiox_usb_hc_dev_t *d){(void)d;}
static const uiox_usb_ops_t xhci_usb_ops={
    xhci_init,xhci_deinit,xhci_port_reset,xhci_enumerate,
    xhci_ctrl_xfer,xhci_bulk_xfer,xhci_isr
};
uiox_fw_err_t uiox_fw_usb_init(uiox_usb_hc_dev_t *d,const uiox_usb_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o;return o->init(d); }
void uiox_fw_usb_deinit(uiox_usb_hc_dev_t *d)
{ if(d&&d->priv&&OPS_USB(d)->deinit)OPS_USB(d)->deinit(d); }
uiox_fw_err_t uiox_fw_usb_enumerate(uiox_usb_hc_dev_t *d)
{ if(!d||!d->priv||!OPS_USB(d)->enumerate)return UIOX_FW_ERR_INVAL;
  return OPS_USB(d)->enumerate(d); }
uiox_fw_err_t uiox_fw_usb_ctrl_xfer(uiox_usb_hc_dev_t *d,uint8_t addr,
    uint8_t bmReqType,uint8_t bRequest,uint16_t wValue,uint16_t wIndex,
    void *data,uint16_t wLength)
{ if(!d||!d->priv||!OPS_USB(d)->ctrl_xfer)return UIOX_FW_ERR_INVAL;
  return OPS_USB(d)->ctrl_xfer(d,addr,bmReqType,bRequest,wValue,wIndex,data,wLength); }
uiox_fw_err_t uiox_fw_usb_init_xhci(uiox_usb_hc_dev_t *d,
                                       uintptr_t base,uint32_t irq)
{
    if(!d)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->mmio_base=base;d->irq=irq;d->hc_type=UIOX_USB_XHCI;
    return uiox_fw_usb_init(d,&xhci_usb_ops);
}
