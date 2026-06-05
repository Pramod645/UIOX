#include "uiox_therm_device.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* Simulated sensor readings (°C × 10) */
static int16_t s_temps[3] = { 350, 450, 280 };
static bool    s_alert    = false;

static int  stub_init(uiox_therm_hw_t *hw)
{ (void)hw; printf("  [hal] init  %s  I2C=0x%02X  ch=%u\n",
                   hw->model, hw->i2c_addr, hw->num_channels); return 0; }
static void stub_deinit(uiox_therm_hw_t *hw) { (void)hw; }
static int  stub_reg_r(uiox_therm_hw_t *hw, uint8_t r, uint8_t *v)
{ (void)hw; (void)r; *v = 0; return 0; }
static int  stub_reg_w(uiox_therm_hw_t *hw, uint8_t r, uint8_t v)
{ (void)hw; printf("  [hal] reg[0x%02X]←0x%02X\n",r,v); return 0; }
static int  stub_reg_r16(uiox_therm_hw_t *hw, uint8_t r, uint16_t *v)
{ (void)hw; (void)r; *v=0; return 0; }
static int  stub_reg_w16(uiox_therm_hw_t *hw, uint8_t r, uint16_t v)
{ (void)hw; printf("  [hal] reg16[0x%02X]←0x%04X\n",r,v); return 0; }
static int  stub_read_temp(uiox_therm_hw_t *hw, uint8_t ch, int16_t *t)
{ (void)hw; *t=ch<3?s_temps[ch]:250; return 0; }
static int  stub_set_t_high(uiox_therm_hw_t *hw, int16_t t)
{ (void)hw; printf("  [hal] T_high=%.1f°C\n",(float)t/10.0f); return 0; }
static int  stub_set_t_hyst(uiox_therm_hw_t *hw, int16_t t)
{ (void)hw; printf("  [hal] T_hyst=%.1f°C\n",(float)t/10.0f); return 0; }
static int  stub_set_t_crit(uiox_therm_hw_t *hw, int16_t t)
{ (void)hw; printf("  [hal] T_crit=%.1f°C\n",(float)t/10.0f); return 0; }
static int  stub_set_res(uiox_therm_hw_t *hw, uiox_therm_res_t r)
{ (void)hw; (void)r; return 0; }
static int  stub_set_mode(uiox_therm_hw_t *hw, bool sd)
{ (void)hw; printf("  [hal] mode %s\n",sd?"SHUTDOWN":"NORMAL"); return 0; }
static int  stub_oneshot(uiox_therm_hw_t *hw) { (void)hw; return 0; }
static int  stub_alert_st(uiox_therm_hw_t *hw, bool *al)
{ (void)hw; *al=s_alert; return 0; }
static int  stub_alert_cl(uiox_therm_hw_t *hw)
{ (void)hw; s_alert=false; return 0; }
static int  stub_adc_r(uiox_therm_hw_t *hw, uint8_t ch, uint16_t *raw)
{ (void)hw; (void)ch; *raw=2048; return 0; }
static bool stub_gpio_r(uiox_therm_hw_t *hw, uint32_t p)
{ (void)hw; (void)p; return false; }
static void stub_isr(uiox_therm_hw_t *hw) { (void)hw; }

static const uiox_therm_hw_ops_t stub_ops = {
    .init=stub_init,.deinit=stub_deinit,
    .reg_read=stub_reg_r,.reg_write=stub_reg_w,
    .reg_read16=stub_reg_r16,.reg_write16=stub_reg_w16,
    .read_temp=stub_read_temp,.set_t_high=stub_set_t_high,
    .set_t_hyst=stub_set_t_hyst,.set_t_crit=stub_set_t_crit,
    .set_resolution=stub_set_res,.set_mode=stub_set_mode,
    .oneshot=stub_oneshot,.alert_status=stub_alert_st,
    .alert_clear=stub_alert_cl,.adc_read=stub_adc_r,
    .gpio_read=stub_gpio_r,.isr=stub_isr,
};

static uiox_therm_hw_t s_hw = {
    .i2c_base=0x40005400uL,.i2c_addr=0x48u,.irq=38,
    .caps=UIOX_THERM_CAP_I2C|UIOX_THERM_CAP_ALERT|UIOX_THERM_CAP_THERM|
          UIOX_THERM_CAP_ONESHOT|UIOX_THERM_CAP_SHUTDOWN|
          UIOX_THERM_CAP_RESOLUTION|UIOX_THERM_CAP_MULTI_CH,
    .type=UIOX_THERM_TYPE_PCT2075,.bus=UIOX_THERM_BUS_I2C,
    .resolution=UIOX_THERM_RES_9BIT,.alert_mode=UIOX_THERM_ALERT_INTERRUPT,
    .model="PCT2075",.num_channels=3,
    .t_high_dc=800,.t_hyst_dc=750,.t_crit_dc=1000,
    .alert_pin=6,.therm_pin=7,
};

static void on_thermal_event(uiox_therm_ev_t ev, uint8_t sid, void *ctx)
{ (void)ctx; printf("  [event] %-22s  sensor=%u\n",
                    uiox_therm_ev_name(ev), sid); }

/* Trip callbacks */
static void on_throttle(uint8_t zone, uiox_therm_trip_type_t type,
                         bool up, void *ctx)
{ (void)type;(void)ctx;
  printf("  [trip] Zone %u throttle %s\n",zone,up?"ON":"OFF"); }

int main(void)
{
    printf("=== UIOX Thermal Sensor Stack Demo ===\n\n");

    printf("--- Open ---\n");
    uiox_therm_device_t dev;
    uiox_therm_open_params_t p = {
        .hw=&s_hw,.hw_ops=&stub_ops,
        .t_high_dc=800,.t_hyst_dc=750,.t_crit_dc=1000,
        .meas_interval_ms=500,.evt_cb=on_thermal_event,
    };
    int rc = uiox_therm_open(&dev, &p);
    if (rc<0){printf("[error] open: %d\n",rc);return 1;}

    printf("\n--- Register sensors ---\n");
    static const uiox_therm_sensor_t sensors[] = {
        {"CPU_TEMP", 0,0,UIOX_THERM_TYPE_PCT2075,0,-400,1200,0,0,4,0,0,true,false},
        {"GPU_TEMP", 1,1,UIOX_THERM_TYPE_PCT2075,0,-400,1200,0,0,4,0,0,true,false},
        {"AMB_TEMP", 2,2,UIOX_THERM_TYPE_PCT2075,0,-400, 800,0,0,2,0,0,true,false},
    };
    for (int i=0;i<3;i++) uiox_therm_add_sensor(&dev, &sensors[i]);

    printf("\n--- Register thermal zones ---\n");
    static uiox_therm_zone_t cpu_zone = {
        .zone_id=0,.sensor_name="CPU_TEMP",.active=true,
        .trips={
            {UIOX_THERM_TRIP_PASSIVE,700,50,false,on_throttle,NULL},
            {UIOX_THERM_TRIP_HOT,    850,50,false,NULL,NULL},
            {UIOX_THERM_TRIP_CRITICAL,1000,50,false,NULL,NULL},
        },
        .num_trips=3,
    };
    static uiox_therm_zone_t gpu_zone = {
        .zone_id=1,.sensor_name="GPU_TEMP",.active=true,
        .trips={
            {UIOX_THERM_TRIP_ACTIVE,650,50,false,NULL,NULL},
            {UIOX_THERM_TRIP_HOT,   900,50,false,NULL,NULL},
        },
        .num_trips=2,
    };
    uiox_therm_add_zone(&dev, &cpu_zone);
    uiox_therm_add_zone(&dev, &gpu_zone);

    printf("\n--- Start ---\n");
    rc = uiox_therm_start(&dev);
    printf("  State: %s  rc=%d\n",
           uiox_therm_state_name(dev.subsys.state), rc);
    uiox_therm_print_info(&dev);

    printf("\n--- Temperature simulation (8 ticks) ---\n");
    int16_t sim[][3] = {
        {350,450,280},{420,500,290},{600,610,300},{720,680,310},
        {800,750,320},{850,820,320},{750,760,315},{650,700,305},
    };
    for (int t=0;t<8;t++) {
        s_temps[0]=sim[t][0]; s_temps[1]=sim[t][1]; s_temps[2]=sim[t][2];
        uint32_t ms=(uint32_t)((t+1)*500u);
        uiox_therm_tick(&dev, ms);
        printf("  t=%us  CPU=%.1f°C  GPU=%.1f°C  AMB=%.1f°C"
               "  state=%s\n",
               (t+1)/2,
               (float)uiox_therm_read(&dev,"CPU_TEMP")/10.0f,
               (float)uiox_therm_read(&dev,"GPU_TEMP")/10.0f,
               (float)uiox_therm_read(&dev,"AMB_TEMP")/10.0f,
               uiox_therm_state_name(dev.subsys.state));
    }

    printf("\n--- Simulate hardware alert ---\n");
    s_alert=true;
    dev.subsys.tif.hw->alert_pending=true;
    uiox_therm_tick(&dev, 5000u);

    printf("\n--- Set new alert threshold ---\n");
    uiox_therm_set_alert(&dev, 900, 850);

    printf("\n--- Telemetry snapshot ---\n");
    uiox_therm_telem_t snap;
    uiox_therm_get_telemetry(&dev, &snap, 5500u);
    for(uint8_t i=0;i<snap.num_channels;i++)
        printf("  ch[%u] = %.1f°C  alert=%s\n",
               i,(float)snap.temp_dc[i]/10.0f,
               snap.alert[i]?"YES":"no");

    printf("\n--- Statistics ---\n");
    uiox_therm_print_stats(&dev);

    printf("\n--- Event log ---\n");
    uiox_therm_print_events();

    printf("\n--- Stop ---\n");
    uiox_therm_stop(&dev);
    uiox_therm_close(&dev);
    printf("  Device: CLOSED\n");
    printf("\n=== UIOX Thermal Sensor Demo complete ===\n");
    return 0;
}
