/**
 * @file  uiox_fw_sensor.c
 * @brief UIOX Firmware — Sensor driver registry.
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 static uiox_fw_sensor_t *s_sensors[UIOX_FW_SENSOR_MAX];
 static uint32_t          s_count = 0u;
 
 uiox_fw_err_t uiox_fw_sensor_init(void)
 {
     for (uint32_t i = 0u; i < UIOX_FW_SENSOR_MAX; i++) s_sensors[i] = NULL;
     s_count = 0u;
     FW_LOG("SENSOR", "registry init OK");
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_sensor_register(uiox_fw_sensor_t *s)
 {
     if (!s || s_count >= UIOX_FW_SENSOR_MAX) return UIOX_FW_ERR_OVERFLOW;
     if (s->init && s->init(s->priv) != UIOX_FW_OK) {
         FW_ERR("sensor %s init failed", s->name);
         return UIOX_FW_ERR_IO;
     }
     s->present      = true;
     s->samples      = 0u;
     s->errors       = 0u;
     s_sensors[s_count++] = s;
     FW_LOG("SENSOR", "registered %s (type=%u)", s->name, (uint32_t)s->type);
     return UIOX_FW_OK;
 }
 
 uiox_fw_sensor_t *uiox_fw_sensor_get(uint32_t idx)
 { return idx < s_count ? s_sensors[idx] : NULL; }
 
 uint32_t uiox_fw_sensor_count(void) { return s_count; }
 
 uiox_fw_err_t uiox_fw_sensor_read(uiox_fw_sensor_t *s,
                                     uiox_fw_sens_sample_t *out)
 {
     if (!s || !out || !s->read) return UIOX_FW_ERR_INVAL;
     uiox_fw_err_t rc = s->read(s->priv, out);
     if (rc == UIOX_FW_OK) s->samples++;
     else                   s->errors++;
     return rc;
 }
 
 void uiox_fw_sensor_print(const uiox_fw_sensor_t *s)
 {
     static const char *type_names[] = {
         "TEMP","HUMID","PRESS","ACCEL","GYRO",
         "LIGHT","PROX","VOLT","CURR","GENERIC"
     };
     if (!s) return;
     uint8_t t = (uint8_t)s->type;
     uiox_fw_printf("  Sensor %-16s  type=%-7s  present=%d"
                     "  samples=%llu  errors=%u\n",
                     s->name,
                     t < 10u ? type_names[t] : "?",
                     (int)s->present,
                     (unsigned long long)s->samples,
                     s->errors);
 }
 