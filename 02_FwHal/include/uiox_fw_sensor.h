/**
 * @file  uiox_fw_sensor.h
 * @brief UIOX Firmware — Sensor driver interface.
 *        Matches 30_DeviceDrivers/03_Sensors / 20_DriverInterfaces/02_Sensors.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_SENSOR_H
 #define UIOX_FW_SENSOR_H
 
 #include "uiox_fw_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FW_SENSOR_MAX      16u
 #define UIOX_FW_SENSOR_NAME_LEN 24u
 
 typedef enum {
     UIOX_FW_SENS_TEMPERATURE = 0,
     UIOX_FW_SENS_HUMIDITY,
     UIOX_FW_SENS_PRESSURE,
     UIOX_FW_SENS_ACCEL,
     UIOX_FW_SENS_GYRO,
     UIOX_FW_SENS_LIGHT,
     UIOX_FW_SENS_PROXIMITY,
     UIOX_FW_SENS_VOLTAGE,
     UIOX_FW_SENS_CURRENT,
     UIOX_FW_SENS_GENERIC,
 } uiox_fw_sens_type_t;
 
 typedef enum {
     UIOX_FW_SENS_BUS_I2C = 0,
     UIOX_FW_SENS_BUS_SPI,
     UIOX_FW_SENS_BUS_MMIO,
     UIOX_FW_SENS_BUS_VIRTIO,
 } uiox_fw_sens_bus_t;
 
 typedef struct {
     int32_t  val[3];    /**< Up to 3-axis readings (× 1000 fixed-point) */
     uint32_t timestamp; /**< ms since firmware start                    */
     bool     valid;
 } uiox_fw_sens_sample_t;
 
 typedef struct {
     uiox_fw_sens_type_t type;
     uiox_fw_sens_bus_t  bus;
     char                name[UIOX_FW_SENSOR_NAME_LEN];
     uint32_t            addr;      /**< I2C address or SPI CS             */
     uintptr_t           base;      /**< MMIO base (if MMIO bus)           */
     uint32_t            irq;
     bool                present;
     void               *priv;
     /* Ops */
     uiox_fw_err_t (*init)    (void *priv);
     uiox_fw_err_t (*read)    (void *priv, uiox_fw_sens_sample_t *s);
     uiox_fw_err_t (*config)  (void *priv, uint32_t odr_hz);
     /* Stats */
     uint64_t            samples;
     uint32_t            errors;
 } uiox_fw_sensor_t;
 
 /* API */
 uiox_fw_err_t  uiox_fw_sensor_init    (void);
 uiox_fw_err_t  uiox_fw_sensor_register(uiox_fw_sensor_t *s);
 uiox_fw_sensor_t *uiox_fw_sensor_get  (uint32_t idx);
 uint32_t       uiox_fw_sensor_count   (void);
 uiox_fw_err_t  uiox_fw_sensor_read    (uiox_fw_sensor_t *s,
                                          uiox_fw_sens_sample_t *out);
 void           uiox_fw_sensor_print   (const uiox_fw_sensor_t *s);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_SENSOR_H */
 