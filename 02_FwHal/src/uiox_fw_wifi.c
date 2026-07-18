/* ── uiox_fw_wifi.c ─────────────────────────────────────────── */
#include "uiox_fw_wifi.h"
#include "uiox_fw_eth.h"

#define OPS_WIFI(d) ((const uiox_wifi_ops_t*)(d)->priv)

static uiox_fw_err_t wifi_sdio_init(uiox_wifi_dev_t *d)
{
    d->mac[0]=0x02u;d->mac[1]=0x00u;d->mac[2]=0x00u;
    //d->mac[3]=0xWI;d->mac[4]=0xFI;d->mac[5]=0x00u;
    /* fixup non-ASCII */
    d->mac[3]=0x11u;d->mac[4]=0x22u;
    d->standard=UIOX_WIFI_802_11AX;
    d->associated=false;
    d->initialized=true;
    return UIOX_FW_OK;
}
static void wifi_sdio_deinit(uiox_wifi_dev_t *d){(void)d;}
static uiox_fw_err_t wifi_load_fw(uiox_wifi_dev_t *d,
                                     const uint8_t *fw,uint32_t len)
{ (void)fw;(void)len;d->fw_loaded=true;return UIOX_FW_OK; }
static uiox_fw_err_t wifi_scan(uiox_wifi_dev_t *d)
{
    /* Stub: add one fake AP */
    if(d->num_scan < UIOX_WIFI_MAX_SCAN){
        uiox_wifi_ap_t *ap=&d->scan_results[d->num_scan++];
        const char *s="UIOX-WiFi";
        for(int i=0;s[i]&&i<32;i++)ap->ssid[i]=s[i];
        ap->rssi_dbm=-55; ap->channel=6; ap->wpa2=true;
    }
    return UIOX_FW_OK;
}
static uiox_fw_err_t wifi_connect(uiox_wifi_dev_t *d,
                                    const char *ssid,const char *key)
{
    (void)key;
    for(int i=0;ssid[i]&&i<32;i++)d->ssid[i]=ssid[i];
    d->associated=true;
    d->rssi_dbm=-50;
    return UIOX_FW_OK;
}
static uiox_fw_err_t wifi_disconnect(uiox_wifi_dev_t *d)
{ d->associated=false;return UIOX_FW_OK; }
static uiox_fw_err_t wifi_send(uiox_wifi_dev_t *d,
                                  const uint8_t *frame,uint32_t len)
{
    if(!d->associated||len>UIOX_ETH_MTU)return UIOX_FW_ERR_IO;
    d->tx_bytes+=len;return UIOX_FW_OK;
}
static void wifi_isr(uiox_wifi_dev_t *d)
{
    if(d->rx_cb)d->rx_cb(NULL,0u,d->rx_priv);
}
static const uiox_wifi_ops_t wifi_sdio_ops={
    wifi_sdio_init,wifi_sdio_deinit,wifi_load_fw,
    wifi_scan,wifi_connect,wifi_disconnect,wifi_send,wifi_isr
};
uiox_fw_err_t uiox_fw_wifi_init(uiox_wifi_dev_t *d,const uiox_wifi_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o;return o->init(d); }
void uiox_fw_wifi_deinit(uiox_wifi_dev_t *d)
{ if(d&&d->priv&&OPS_WIFI(d)->deinit)OPS_WIFI(d)->deinit(d); }
uiox_fw_err_t uiox_fw_wifi_scan(uiox_wifi_dev_t *d)
{ if(!d||!d->priv||!OPS_WIFI(d)->scan)return UIOX_FW_ERR_INVAL;
  return OPS_WIFI(d)->scan(d); }
uiox_fw_err_t uiox_fw_wifi_connect(uiox_wifi_dev_t *d,
                                     const char *ssid,const char *key)
{ if(!d||!d->priv||!OPS_WIFI(d)->connect)return UIOX_FW_ERR_INVAL;
  return OPS_WIFI(d)->connect(d,ssid,key); }
uiox_fw_err_t uiox_fw_wifi_disconnect(uiox_wifi_dev_t *d)
{ if(!d||!d->priv||!OPS_WIFI(d)->disconnect)return UIOX_FW_ERR_INVAL;
  return OPS_WIFI(d)->disconnect(d); }
uiox_fw_err_t uiox_fw_wifi_send(uiox_wifi_dev_t *d,
                                   const uint8_t *frame,uint32_t len)
{ if(!d||!d->priv||!OPS_WIFI(d)->send)return UIOX_FW_ERR_INVAL;
  return OPS_WIFI(d)->send(d,frame,len); }
void uiox_fw_wifi_set_rx_cb(uiox_wifi_dev_t *d,uiox_wifi_rx_cb_t cb,void *p)
{ if(d){d->rx_cb=cb;d->rx_priv=p;} }
uiox_fw_err_t uiox_fw_wifi_init_sdio(uiox_wifi_dev_t *d,
                                        uintptr_t base,uint32_t irq)
{
    if(!d)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->base=base;d->irq=irq;d->bus=UIOX_WIFI_SDIO;
    return uiox_fw_wifi_init(d,&wifi_sdio_ops);
}
