/* ── uiox_fw_camera.c ───────────────────────────────────────── */
#include "../include/uiox_fw_camera.h"
#define OPS_CAM(d) ((const uiox_cam_ops_t *)(d)->priv)
static uiox_fw_err_t ov5640_write(uiox_cam_dev_t *dev,
                                    uint16_t reg, uint8_t val)
{
    uint8_t buf[3] = { (uint8_t)(reg>>8u), (uint8_t)reg, val };
    /* 16-bit register address: send as 3-byte write */
    uiox_i2c_xfer_t x = { .addr=dev->addr, .tx_buf=buf, .tx_len=3u,
                            .rx_buf=NULL,   .rx_len=0u };
    return uiox_fw_i2c_transfer(dev->i2c, &x);
}
static uiox_fw_err_t ov5640_read(uiox_cam_dev_t *dev,
                                   uint16_t reg, uint8_t *val)
{
    uint8_t addr_buf[2] = { (uint8_t)(reg>>8u), (uint8_t)reg };
    uiox_i2c_xfer_t x = { .addr=dev->addr, .tx_buf=addr_buf, .tx_len=2u,
                            .rx_buf=val,    .rx_len=1u,
                            .repeated_start=true };
    return uiox_fw_i2c_transfer(dev->i2c, &x);
}
static uiox_fw_err_t ov5640_init(uiox_cam_dev_t *dev)
{
    /* Power-on sequence */
    uint8_t chip_h, chip_l;
    ov5640_read(dev, OV5640_REG_CHIP_ID_H, &chip_h);
    ov5640_read(dev, OV5640_REG_CHIP_ID_L, &chip_l);
    uint16_t chip_id = (uint16_t)((uint16_t)chip_h<<8u | chip_l);
    if (chip_id != OV5640_CHIP_ID) return UIOX_FW_ERR_NODEV;
    /* Software reset then wakeup */
    ov5640_write(dev, OV5640_REG_SYS_CTRL0, 0x82u);
    volatile uint32_t n = 200000u; while (n--) ;
    ov5640_write(dev, OV5640_REG_SYS_CTRL0, 0x02u);
    dev->initialized = true;
    return UIOX_FW_OK;
}
static void ov5640_deinit(uiox_cam_dev_t *dev)
{ ov5640_write(dev, OV5640_REG_SYS_CTRL0, 0x42u); }  /* standby */
static uiox_fw_err_t ov5640_set_res(uiox_cam_dev_t *dev, uiox_cam_res_t res)
{ dev->resolution = res; return UIOX_FW_OK; }
static uiox_fw_err_t ov5640_stream_on(uiox_cam_dev_t *dev)
{ dev->streaming = true;  return ov5640_write(dev, OV5640_REG_SYS_CTRL0, 0x02u); }
static uiox_fw_err_t ov5640_stream_off(uiox_cam_dev_t *dev)
{ dev->streaming = false; return ov5640_write(dev, OV5640_REG_SYS_CTRL0, 0x42u); }
static const uiox_cam_ops_t ov5640_ops = {
    .init       = ov5640_init,
    .deinit     = ov5640_deinit,
    .set_res    = ov5640_set_res,
    .stream_on  = ov5640_stream_on,
    .stream_off = ov5640_stream_off,
    .set_exposure = NULL,
    .set_gain   = NULL,
    .read_reg   = NULL,
    .write_reg  = NULL,
};
uiox_fw_err_t uiox_fw_cam_init(uiox_cam_dev_t *dev,const uiox_cam_ops_t *ops)
{ if(!dev||!ops||!ops->init) return UIOX_FW_ERR_INVAL;
  dev->priv=(void*)ops; return ops->init(dev); }
void uiox_fw_cam_deinit(uiox_cam_dev_t *dev)
{ if(dev&&dev->priv&&OPS_CAM(dev)->deinit) OPS_CAM(dev)->deinit(dev); }
uiox_fw_err_t uiox_fw_cam_set_res(uiox_cam_dev_t *dev, uiox_cam_res_t r)
{ if(!dev||!dev->priv||!OPS_CAM(dev)->set_res) return UIOX_FW_ERR_INVAL;
  return OPS_CAM(dev)->set_res(dev,r); }
uiox_fw_err_t uiox_fw_cam_stream_on(uiox_cam_dev_t *dev)
{ if(!dev||!dev->priv||!OPS_CAM(dev)->stream_on) return UIOX_FW_ERR_INVAL;
  return OPS_CAM(dev)->stream_on(dev); }
uiox_fw_err_t uiox_fw_cam_stream_off(uiox_cam_dev_t *dev)
{ if(!dev||!dev->priv||!OPS_CAM(dev)->stream_off) return UIOX_FW_ERR_INVAL;
  return OPS_CAM(dev)->stream_off(dev); }
uiox_fw_err_t uiox_fw_cam_init_ov5640(uiox_cam_dev_t *dev,
                                         uiox_i2c_dev_t *i2c,
                                         uint32_t gpio_reset,
                                         uint32_t gpio_pwdn)
{
    if(!dev||!i2c) return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)dev; for(size_t i=0u;i<sizeof(*dev);i++) p[i]=0u;
    dev->i2c=i2c; dev->addr=OV5640_ADDR; dev->chip=UIOX_CAM_OV5640;
    dev->gpio_reset=gpio_reset; dev->gpio_pwdn=gpio_pwdn;
    dev->mipi_lanes=2u; dev->pixel_clk_hz=96000000u;
    return uiox_fw_cam_init(dev, &ov5640_ops);
}