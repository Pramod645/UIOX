/**
 * @file  uiox_fw_camera.h
 * @brief UIOX FwHal — MIPI CSI-2 camera sensor HAL (OV5640 / IMX219).
 *        I2C configuration + MIPI lane enable.
 */

 #ifndef UIOX_FW_CAMERA_H
 #define UIOX_FW_CAMERA_H
 
 #include "uiox_fw_types.h"
 #include "uiox_fw_i2c.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_CAM_OV5640  = 0,
     UIOX_CAM_IMX219  = 1,
     UIOX_CAM_OV8865  = 2,
 } uiox_cam_chip_t;
 
 /* ── OV5640 key registers ────────────────────────────────────── */
 #define OV5640_ADDR             0x3Cu
 #define OV5640_REG_CHIP_ID_H    0x300Au
 #define OV5640_REG_CHIP_ID_L    0x300Bu
 #define OV5640_REG_SYS_CTRL0    0x3008u  /**< 0x42=standby, 0x02=run  */
 #define OV5640_CHIP_ID          0x5640u
 
 /* ── Resolution presets ──────────────────────────────────────── */
 typedef enum {
     UIOX_CAM_RES_VGA   = 0,   /**< 640×480                           */
     UIOX_CAM_RES_HD720 = 1,   /**< 1280×720                          */
     UIOX_CAM_RES_1080P = 2,   /**< 1920×1080                         */
     UIOX_CAM_RES_5MP   = 3,   /**< 2592×1944                         */
 } uiox_cam_res_t;
 
 typedef struct {
     uiox_i2c_dev_t *i2c;
     uint8_t         addr;
     uiox_cam_chip_t chip;
     uiox_cam_res_t  resolution;
     uint8_t         mipi_lanes;
     uint32_t        gpio_reset;
     uint32_t        gpio_pwdn;
     uint32_t        pixel_clk_hz;
     bool            streaming;
     bool            initialized;
     void           *priv;
 } uiox_cam_dev_t;
 
 typedef struct {
     uiox_fw_err_t (*init)       (uiox_cam_dev_t *dev);
     void          (*deinit)     (uiox_cam_dev_t *dev);
     uiox_fw_err_t (*set_res)    (uiox_cam_dev_t *dev, uiox_cam_res_t res);
     uiox_fw_err_t (*stream_on)  (uiox_cam_dev_t *dev);
     uiox_fw_err_t (*stream_off) (uiox_cam_dev_t *dev);
     uiox_fw_err_t (*set_exposure)(uiox_cam_dev_t *dev, uint32_t us);
     uiox_fw_err_t (*set_gain)   (uiox_cam_dev_t *dev, uint16_t gain_x100);
     uiox_fw_err_t (*read_reg)   (uiox_cam_dev_t *dev,
                                    uint16_t reg, uint8_t *val);
     uiox_fw_err_t (*write_reg)  (uiox_cam_dev_t *dev,
                                    uint16_t reg, uint8_t val);
 } uiox_cam_ops_t;
 
 uiox_fw_err_t uiox_fw_cam_init       (uiox_cam_dev_t *dev,
                                         const uiox_cam_ops_t *ops);
 void          uiox_fw_cam_deinit     (uiox_cam_dev_t *dev);
 uiox_fw_err_t uiox_fw_cam_set_res    (uiox_cam_dev_t *dev,
                                         uiox_cam_res_t res);
 uiox_fw_err_t uiox_fw_cam_stream_on  (uiox_cam_dev_t *dev);
 uiox_fw_err_t uiox_fw_cam_stream_off (uiox_cam_dev_t *dev);
 uiox_fw_err_t uiox_fw_cam_init_ov5640(uiox_cam_dev_t *dev,
                                         uiox_i2c_dev_t *i2c,
                                         uint32_t gpio_reset,
                                         uint32_t gpio_pwdn);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_CAMERA_H */
 