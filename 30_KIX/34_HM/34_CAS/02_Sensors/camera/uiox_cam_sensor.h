/**
 * @file    uiox_cam_sensor.h
 * @brief   Image sensor abstraction (register map, modes, controls).
 *
 * One instance per physical sensor. Talks over I2C/SPI via tiny ops.
 */

 #ifndef UIOX_CAM_SENSOR_H
 #define UIOX_CAM_SENSOR_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     int  (*i2c_write8) (uint8_t dev_addr, uint16_t reg, uint8_t val);
     int  (*i2c_read8)  (uint8_t dev_addr, uint16_t reg, uint8_t *val);
     int  (*delay_ms)   (uint32_t ms);
 } uiox_cam_bus_ops_t;
 
 typedef struct {
     const char        *name;         /* "ov5640", "imx219", etc. */
     uint8_t            i2c_addr;     /* 7-bit I2C address */
     uint16_t           chip_id_reg;  /* whoami register */
     uint16_t           chip_id_val;  /* expected value */
 
     /* Current mode */
     uint16_t           width, height;
     uint32_t           link_freq_hz; /* CSI lane rate */
     uint8_t            fps;
 
     /* Controls */
     uint32_t           exposure_us;
     uint16_t           gain_x100;    /* 100 = 1.00x */
 
     const uiox_cam_bus_ops_t *bus;
     void              *priv;         /* driver private */
 } uiox_cam_sensor_t;
 
 /* Generic sensor ops for a typical RAW10 Bayer sensor */
 int  uiox_cam_sensor_detect (uiox_cam_sensor_t *s);
 int  uiox_cam_sensor_reset  (uiox_cam_sensor_t *s);
 int  uiox_cam_sensor_stream (uiox_cam_sensor_t *s, bool enable);
 
 int  uiox_cam_sensor_set_mode(uiox_cam_sensor_t *s,
                               uint16_t w, uint16_t h,
                               uint8_t fps, uint32_t link_freq_hz);
 
 int  uiox_cam_sensor_set_exposure(uiox_cam_sensor_t *s, uint32_t us);
 int  uiox_cam_sensor_set_gain    (uiox_cam_sensor_t *s, uint16_t gain_x100);
 
 #ifdef __cplusplus
 }
 #endif
 #endif
 