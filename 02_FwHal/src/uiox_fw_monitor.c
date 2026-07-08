/* ── Monitor ─── */
#include "../include/uiox_fw_monitor.h"
uiox_fw_err_t uiox_fw_monitor_init(uiox_monitor_dev_t *d,uiox_i2c_dev_t *ddc)
{ if(!d||!ddc)return UIOX_FW_ERR_INVAL;
  uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
  d->ddc=ddc;d->initialized=true;return UIOX_FW_OK; }
void uiox_fw_monitor_deinit(uiox_monitor_dev_t *d){if(d)d->initialized=false;}
uiox_fw_err_t uiox_fw_monitor_read_edid(uiox_monitor_dev_t *d)
{
    if(!d||!d->ddc)return UIOX_FW_ERR_NODEV;
    uiox_fw_err_t rc=uiox_fw_i2c_read_reg(d->ddc,EDID_ADDR,0u,d->edid.raw,128u);
    if(rc!=UIOX_FW_OK)return rc;
    /* Check EDID magic: bytes 0–7 = 00 FF FF FF FF FF FF 00 */
    d->edid.valid=(d->edid.raw[0]==0x00u && d->edid.raw[7]==0x00u &&
                   d->edid.raw[1]==0xFFu);
    d->connected=d->edid.valid;
    return UIOX_FW_OK;
}
void uiox_fw_monitor_parse_edid(uiox_monitor_dev_t *d)
{
    if(!d||!d->edid.valid)return;
    /* Manufacturer ID: bytes 8–9 */
    d->edid.manufacturer[0]=(char)(((d->edid.raw[8]>>2)&0x1Fu)+'A'-1u);
    d->edid.manufacturer[1]=(char)((((d->edid.raw[8]&3u)<<3u)|(d->edid.raw[9]>>5u))+'A'-1u);
    d->edid.manufacturer[2]=(char)((d->edid.raw[9]&0x1Fu)+'A'-1u);
    d->edid.manufacturer[3]='\0';
    /* Physical size: bytes 21–22 (cm) */
    d->edid.max_h_cm=d->edid.raw[21];
    d->edid.max_v_cm=d->edid.raw[22];
}
bool uiox_fw_monitor_connected(const uiox_monitor_dev_t *d)
{ return d&&d->connected; }
