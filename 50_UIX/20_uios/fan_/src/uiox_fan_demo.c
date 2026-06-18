#include "uiox_fan_device.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* Simulated hardware */
static uint16_t s_rpm[2]   = {2800u, 3100u};
static int16_t  s_temp_dc[2]= {350, 420};  /* 35.0°C, 42.0°C */
static uint8_t  s_pwm[2]   = {100u, 120u};
static uint32_t s_fault     = 0u;

static int  stub_init(uiox_fan_hw_t *hw)
{ (void)hw; printf("  [hal] init  EMC2301  I2C=0x%02X  fans=%u\n",
                   hw->i2c_addr, hw->num_fans); return 0; }
static void stub_deinit(uiox_fan_hw_t *hw) { (void)hw; }
static int  stub_reg_r(uiox_fan_hw_t *hw, uint8_t r, uint8_t *v)
{ (void)hw; *v=(r==UIOX_REG_DEVICE_ID)?0x28u:
                (r==UIOX_REG_MFR_ID)?0x5Du:0; return 0; }
static int  stub_reg_w(uiox_fan_hw_t *hw, uint8_t r, uint8_t v)
{ (void)hw; printf("  [hal] reg[0x%02X]←0x%02X\n",r,v); return 0; }
static int  stub_reg_r16(uiox_fan_hw_t *hw, uint8_t r, uint16_t *v)
{ (void)hw; (void)r; *v=0; return 0; }
static int  stub_reg_w16(uiox_fan_hw_t *hw, uint8_t r, uint16_t v)
{ (void)hw; (void)r; (void)v; return 0; }
static int  stub_set_pwm(uiox_fan_hw_t *hw, uint8_t ch, uint8_t duty)
{ (void)hw; if(ch<2) s_pwm[ch]=duty;
  printf("  [hal] PWM[%u] = %u  (%u%%)\n",ch,duty,duty*100/255); return 0; }
static int  stub_get_pwm(uiox_fan_hw_t *hw, uint8_t ch, uint8_t *d)
{ (void)hw; *d=ch<2?s_pwm[ch]:0; return 0; }
static int  stub_set_rpm(uiox_fan_hw_t *hw, uint8_t ch, uint16_t rpm)
{ (void)hw; (void)ch; (void)rpm; return 0; }
static int  stub_read_rpm(uiox_fan_hw_t *hw, uint8_t ch, uint16_t *rpm)
{ (void)hw; *rpm=ch<2?s_rpm[ch]:0; return 0; }
static int  stub_read_temp(uiox_fan_hw_t *hw, uint8_t ch, int16_t *t)
{ (void)hw; *t=ch<2?s_temp_dc[ch]:250; return 0; }
static int  stub_fault_st(uiox_fan_hw_t *hw, uint32_t *f)
{ (void)hw; *f=s_fault; return 0; }
static int  stub_fault_cl(uiox_fan_hw_t *hw, uint32_t f)
{ (void)hw; s_fault&=~f; return 0; }
static int  stub_chan_en(uiox_fan_hw_t *hw, uint8_t ch, bool en)
{ (void)hw; printf("  [hal] chan[%u] %s\n",ch,en?"EN":"DIS"); return 0; }
static void stub_gpio_w(uiox_fan_hw_t *hw, uint32_t p, bool v)
{ (void)hw; printf("  [hal] GPIO pin=%u val=%d\n",p,(int)v); }
static bool stub_gpio_r(uiox_fan_hw_t *hw, uint32_t p)
{ (void)hw; (void)p; return true; }
static int  stub_wdt(uiox_fan_hw_t *hw)
{ (void)hw; return 0; }
static void stub_isr(uiox_fan_hw_t *hw) { (void)hw; }

static const uiox_fan_hw_ops_t stub_ops = {
    .init=stub_init,.deinit=stub_deinit,
    .reg_read=stub_reg_r,.reg_write=stub_reg_w,
    .reg_read16=stub_reg_r16,.reg_write16=stub_reg_w16,
    .set_pwm=stub_set_pwm,.get_pwm=stub_get_pwm,
    .set_rpm_target=stub_set_rpm,.read_rpm=stub_read_rpm,
    .read_temp=stub_read_temp,.fault_status=stub_fault_st,
    .fault_clear=stub_fault_cl,.chan_enable=stub_chan_en,
    .gpio_write=stub_gpio_w,.gpio_read=stub_gpio_r,
    .wdt_kick=stub_wdt,.isr=stub_isr,
};

static uiox_fan_hw_t s_hw = {
    .i2c_base=0x40005400uL,.i2c_addr=0x2Fu,.irq=35,
    .caps=UIOX_FAN_CAP_PWM|UIOX_FAN_CAP_TACH|UIOX_FAN_CAP_MULTI_FAN|
          UIOX_FAN_CAP_TEMP_SENSE|UIOX_FAN_CAP_STALL_DET|
          UIOX_FAN_CAP_SPIN_UP|UIOX_FAN_CAP_RAMP_CTRL|
          UIOX_FAN_CAP_FAULT_IRQ|UIOX_FAN_CAP_4WIRE,
    .bus=UIOX_FAN_BUS_I2C,.model="EMC2301",
    .num_fans=2,.num_temps=2,.pwm_freq_hz=25000u,.tach_edges=2,
    .en_pin=4,.fault_pin=5,
};

static void on_fan_event(uiox_fan_ev_t ev, uint8_t fan_id, void *ctx)
{ (void)ctx; printf("  [event] %-20s  fan=%u\n",
                    uiox_fan_ev_name(ev), fan_id); }

int main(void)
{
    printf("=== UIOX Fan Controller Stack Demo ===\n\n");

    printf("--- Open ---\n");
    uiox_fan_device_t dev;
    uiox_fan_open_params_t p = {
        .hw=&s_hw,.hw_ops=&stub_ops,
        .critical_temp_dc=950,  /* 95.0°C */
        .evt_cb=on_fan_event,
    };
    int rc = uiox_fan_open(&dev, &p);
    if(rc<0){printf("[error] open: %d\n",rc); return 1;}

    printf("\n--- Register fans ---\n");
    static const uiox_fan_ch_t fans[] = {
        {"CPU_FAN",0,500,5000,30,255,100,0,false,true,false,0,0,false},
        {"SYS_FAN",1,300,4000,25,200, 80,0,false,true,false,0,0,false},
    };
    for(int i=0;i<2;i++) uiox_fan_add_fan(&dev, &fans[i]);
    printf("  CPU_FAN (chan 0)  SYS_FAN (chan 1)\n");

    printf("\n--- Add thermal zones (step curve) ---\n");
    static const uiox_fan_zone_t cpu_zone = {
        .zone_id=0,.temp_sensor_id=0,.fan_id=0,
        .ctrl_type=UIOX_FAN_CTRL_STEP,
        .trips={
            {.temp_dc=400,.duty= 51},  /* 40°C → 20% */
            {.temp_dc=500,.duty=102},  /* 50°C → 40% */
            {.temp_dc=600,.duty=153},  /* 60°C → 60% */
            {.temp_dc=700,.duty=204},  /* 70°C → 80% */
            {.temp_dc=800,.duty=255},  /* 80°C → 100% */
        },
        .num_trips=5,.active=true,
    };
    static const uiox_fan_zone_t sys_zone = {
        .zone_id=1,.temp_sensor_id=1,.fan_id=1,
        .ctrl_type=UIOX_FAN_CTRL_HYSTERESIS,
        .hyst_on_dc=500,.hyst_off_dc=400,.active=true,
    };
    uiox_fan_add_zone(&dev, &cpu_zone);
    uiox_fan_add_zone(&dev, &sys_zone);
    printf("  CPU zone: step curve  SYS zone: hysteresis\n");

    printf("\n--- Start ---\n");
    rc = uiox_fan_start(&dev);
    printf("  State: %s  rc=%d\n",
           uiox_fan_state_name(dev.subsys.state), rc);
    uiox_fan_print_info(&dev);

    printf("\n--- Thermal simulation (8 ticks) ---\n");
    int16_t sim_temps[][2] = {
        {350,380},{420,450},{500,480},{620,510},
        {700,550},{750,600},{650,520},{500,450},
    };
    for(int t=0; t<8; t++){
        s_temp_dc[0] = sim_temps[t][0];
        s_temp_dc[1] = sim_temps[t][1];
        /* Simulate RPM proportional to PWM */
        for(uint8_t c=0;c<2;c++)
            s_rpm[c]=(uint16_t)((uint32_t)s_pwm[c]*5000u/255u);
        uint32_t ms = (uint32_t)((t+1)*500u);
        uiox_fan_tick(&dev, ms);
        printf("  t=%us  CPU=%.1f°C→%u%%/%uRPM  "
               "SYS=%.1f°C→%u%%/%uRPM\n",
               (t+1)/2,
               (float)s_temp_dc[0]/10.0f,
               uiox_fan_get_pct(&dev,0),
               uiox_fan_get_rpm(&dev,0),
               (float)s_temp_dc[1]/10.0f,
               uiox_fan_get_pct(&dev,1),
               uiox_fan_get_rpm(&dev,1));
    }

    printf("\n--- Manual override: SYS fan 50%% ---\n");
    uiox_fan_set_manual(&dev, 1u, true, 128u, 5000u);
    uiox_fan_tick(&dev, 5100u);
    printf("  SYS fan manual: %u%%  RPM=%u\n",
           uiox_fan_get_pct(&dev,1), uiox_fan_get_rpm(&dev,1));

    printf("\n--- Restore auto ---\n");
    uiox_fan_set_manual(&dev, 1u, false, 0u, 5200u);

    printf("\n--- Simulate stall fault ---\n");
    s_rpm[0] = 0u;
    s_fault  = UIOX_FAN_FAULT_STALL;
    uiox_fan_tick(&dev, 5500u);

    printf("\n--- Telemetry snapshot ---\n");
    uiox_fan_telem_t snap;
    uiox_fan_get_telemetry(&dev, &snap, 6000u);
    printf("  Fan0: %u RPM  %u%%PWM\n", snap.rpm[0], snap.pwm[0]*100u/255u);
    printf("  Fan1: %u RPM  %u%%PWM\n", snap.rpm[1], snap.pwm[1]*100u/255u);
    printf("  Temp0: %.1f°C  Temp1: %.1f°C\n",
           (float)snap.temp_dc[0]/10.0f,(float)snap.temp_dc[1]/10.0f);

    printf("\n--- Statistics ---\n");
    uiox_fan_print_stats(&dev);

    printf("\n--- Event log ---\n");
    uiox_fan_print_events();

    printf("\n--- Stop ---\n");
    uiox_fan_stop(&dev);
    uiox_fan_close(&dev);
    printf("  Device: CLOSED\n");
    printf("\n=== UIOX Fan Controller Demo complete ===\n");
    return 0;
}
