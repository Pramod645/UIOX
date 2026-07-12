/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_thermal.h — Thermal sensor + trip-point management
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_THERMAL_H
#define UIOX_FW_THERMAL_H
#include "uiox_fw_types.h"
#include "uiox_fw_i2c.h"
#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_THERM_MAX_ZONES  8u
#define UIOX_THERM_MAX_TRIPS  4u

typedef enum {
    UIOX_TRIP_ACTIVE   = 0,   /**< Fan speed step                    */
    UIOX_TRIP_PASSIVE  = 1,   /**< CPU throttle                      */
    UIOX_TRIP_HOT      = 2,   /**< Emergency throttle                */
    UIOX_TRIP_CRITICAL = 3,   /**< Shutdown                          */
} uiox_trip_type_t;

typedef struct { uint32_t temp_dc; uiox_trip_type_t type; } uiox_trip_t;
typedef void (*uiox_trip_cb_t)(uint8_t zone, const uiox_trip_t *trip,
                                int32_t temp_dc, void *p);

typedef struct {
    int32_t       temp_dc;    /**< Current temperature in decidegrees */
    char          name[12];
    uiox_trip_t   trips[UIOX_THERM_MAX_TRIPS];
    uint8_t       num_trips;
} uiox_therm_zone_t;

typedef struct {
    uiox_i2c_dev_t   *i2c;
    uintptr_t         mmio_base;  /**< For on-chip thermal sensor      */
    uiox_therm_zone_t zones[UIOX_THERM_MAX_ZONES];
    uint8_t           num_zones;
    uiox_trip_cb_t    trip_cb;
    void             *trip_priv;
    bool              initialized;
    void             *priv;
} uiox_therm_dev_t;

typedef struct {
    uiox_fw_err_t (*init)       (uiox_therm_dev_t *dev);
    void          (*deinit)     (uiox_therm_dev_t *dev);
    uiox_fw_err_t (*read_temp)  (uiox_therm_dev_t *dev,
                                   uint8_t zone, int32_t *temp_dc);
    uiox_fw_err_t (*set_trip)   (uiox_therm_dev_t *dev,
                                   uint8_t zone, const uiox_trip_t *trip);
    void          (*tick)       (uiox_therm_dev_t *dev);
} uiox_therm_ops_t;

uiox_fw_err_t uiox_fw_therm_init      (uiox_therm_dev_t *dev,
                                         const uiox_therm_ops_t *ops);
void          uiox_fw_therm_deinit    (uiox_therm_dev_t *dev);
uiox_fw_err_t uiox_fw_therm_read_temp (uiox_therm_dev_t *dev,
                                         uint8_t zone, int32_t *temp_dc);
void          uiox_fw_therm_tick      (uiox_therm_dev_t *dev);
void          uiox_fw_therm_set_cb    (uiox_therm_dev_t *dev,
                                         uiox_trip_cb_t cb, void *p);
uiox_fw_err_t uiox_fw_therm_add_trip  (uiox_therm_dev_t *dev, uint8_t zone,
                                         uint32_t temp_dc, uiox_trip_type_t type);
uiox_fw_err_t uiox_fw_therm_init_mmio (uiox_therm_dev_t *dev, uintptr_t base);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_THERMAL_H */
