#include "uiox_cam_sensor.h"
#include <errno.h>

/* Helper: write a register list */
typedef struct { uint16_t reg; uint8_t val; } reg8_t;
static int wr_list(const uiox_cam_bus_ops_t *bus, uint8_t addr, const reg8_t *lst)
{
    for (int i = 0; lst[i].reg != 0xFFFF; i++) {
        int rc = bus->i2c_write8(addr, lst[i].reg, lst[i].val);
        if (rc < 0) return rc;
    }
    return 0;
}

int uiox_cam_sensor_detect(uiox_cam_sensor_t *s)
{
    if (!s || !s->bus || !s->bus->i2c_read8) return -EINVAL;
    uint8_t v = 0;
    int rc = s->bus->i2c_read8(s->i2c_addr, s->chip_id_reg, &v);
    if (rc < 0) return rc;
    return (v == (s->chip_id_val & 0xFF)) ? 0 : -ENODEV;
}

int uiox_cam_sensor_reset(uiox_cam_sensor_t *s)
{
    if (!s || !s->bus) return -EINVAL;
    /* Typical soft reset sequence; replace with sensor-specific regs */
    s->bus->i2c_write8(s->i2c_addr, 0x0103, 0x01); /* software reset */
    if (s->bus->delay_ms) s->bus->delay_ms(5);
    return 0;
}

int uiox_cam_sensor_stream(uiox_cam_sensor_t *s, bool enable)
{
    if (!s || !s->bus) return -EINVAL;
    return s->bus->i2c_write8(s->i2c_addr, 0x0100, enable ? 0x01 : 0x00);
}

int uiox_cam_sensor_set_mode(uiox_cam_sensor_t *s,
                             uint16_t w, uint16_t h,
                             uint8_t fps, uint32_t link_freq_hz)
{
    if (!s || !s->bus) return -EINVAL;
    s->width = w; s->height = h; s->fps = fps; s->link_freq_hz = link_freq_hz;

    /* Sensor-specific mode table would go here. We stub minimal timing. */
    (void)link_freq_hz; (void)fps;
    /* Program crop / output size regs (example addresses) */
    s->bus->i2c_write8(s->i2c_addr, 0x3808, (uint8_t)(w >> 8));
    s->bus->i2c_write8(s->i2c_addr, 0x3809, (uint8_t)(w & 0xFF));
    s->bus->i2c_write8(s->i2c_addr, 0x380A, (uint8_t)(h >> 8));
    s->bus->i2c_write8(s->i2c_addr, 0x380B, (uint8_t)(h & 0xFF));
    return 0;
}

int uiox_cam_sensor_set_exposure(uiox_cam_sensor_t *s, uint32_t us)
{
    if (!s || !s->bus) return -EINVAL;
    s->exposure_us = us;
    /* Convert microseconds to line counts and program (example regs) */
    uint16_t lines = (uint16_t)(us / 30); /* placeholder */
    s->bus->i2c_write8(s->i2c_addr, 0x3500, (uint8_t)((lines >> 12) & 0x0F));
    s->bus->i2c_write8(s->i2c_addr, 0x3501, (uint8_t)((lines >> 4) & 0xFF));
    s->bus->i2c_write8(s->i2c_addr, 0x3502, (uint8_t)((lines & 0x0F) << 4));
    return 0;
}

int uiox_cam_sensor_set_gain(uiox_cam_sensor_t *s, uint16_t gain_x100)
{
    if (!s || !s->bus) return -EINVAL;
    s->gain_x100 = gain_x100;
    /* Example linear gain mapping */
    uint16_t reg = (uint16_t)(gain_x100 / 100u);
    s->bus->i2c_write8(s->i2c_addr, 0x3508, (uint8_t)(reg >> 8));
    s->bus->i2c_write8(s->i2c_addr, 0x3509, (uint8_t)(reg & 0xFF));
    return 0;
}
