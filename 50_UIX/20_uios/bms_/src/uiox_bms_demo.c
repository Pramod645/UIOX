/* uiox_bms_demo.c */
#include "uiox_bms_device.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* Simulated 4S Li-ion pack: 4 cells, ~3500 mAh NMC */
static uint32_t s_cell_mv[4] = { 3850,3860,3870,3900 };
static int32_t  s_current_ma = -800;  /* -800 mA = discharging */
static int16_t  s_temp_dc    = 250;   /* 25.0 °C */
static uint32_t s_fault      = 0;
static bool     s_present    = true;

static int  stub_init(uiox_bms_hw_t *hw)
{ (void)hw; printf("  [hal] init  BQ76940  I2C=0x%02X\n",hw->i2c_addr);
  hw->present=true; return 0; }
static void stub_deinit(uiox_bms_hw_t *hw) { (void)hw; }
static int  stub_reg_r(uiox_bms_hw_t *hw, uint8_t r, uint8_t *v)
{ (void)hw; *v=(r==UIOX_REG_BMS_DEVICE_ID)?0x40u:0; return 0; }
static int  stub_reg_w(uiox_bms_hw_t *hw, uint8_t r, uint8_t v)
{ (void)hw; printf("  [hal] reg[0x%02X]←0x%02X\n",r,v); return 0; }
static int  stub_reg_r16(uiox_bms_hw_t *hw, uint8_t r, uint16_t *v)
{ (void)hw; (void)r; *v=0; return 0; }
static int  stub_reg_w16(uiox_bms_hw_t *hw, uint8_t r, uint16_t v)
{ (void)hw; (void)r; (void)v; return 0; }
static int  stub_bulk(uiox_bms_hw_t *hw, uint8_t r, uint8_t *b, uint8_t l)
{ (void)hw; (void)r; memset(b,0,l); return 0; }

static int stub_measure_cells(uiox_bms_hw_t *hw)
{
    for (uint8_t i=0; i<hw->num_cells && i<4; i++)
        hw->cell_mv[i] = s_cell_mv[i];
    hw->pack_mv = 0;
    for (uint8_t i=0; i<hw->num_cells; i++) hw->pack_mv += hw->cell_mv[i];
    return 0;
}
static int stub_measure_current(uiox_bms_hw_t *hw)
{ hw->current_ma=s_current_ma; return 0; }
static int stub_measure_temp(uiox_bms_hw_t *hw)
{ hw->temp_dc[0]=s_temp_dc; return 0; }
static int stub_read_coulombs(uiox_bms_hw_t *hw, int32_t *mah)
{ (void)hw; *mah=2800; return 0; }
static int stub_set_chg(uiox_bms_hw_t *hw, bool on)
{ (void)hw; printf("  [hal] CHG FET %s\n",on?"ON":"OFF"); return 0; }
static int stub_set_dsg(uiox_bms_hw_t *hw, bool on)
{ (void)hw; printf("  [hal] DSG FET %s\n",on?"ON":"OFF"); return 0; }
static int stub_set_bal(uiox_bms_hw_t *hw, uint16_t mask)
{ (void)hw; printf("  [hal] balance mask=0x%04X\n",mask); return 0; }
static int stub_get_bal(uiox_bms_hw_t *hw, uint16_t *mask)
{ (void)hw; *mask=0; return 0; }
static int  stub_fault_st(uiox_bms_hw_t *hw, uint32_t *f)
{ (void)hw; *f=s_fault; return 0; }
static int  stub_fault_cl(uiox_bms_hw_t *hw, uint32_t f)
{ (void)hw; s_fault&=~f; return 0; }
static bool stub_present(uiox_bms_hw_t *hw) { (void)hw; return s_present; }
static int  stub_ship(uiox_bms_hw_t *hw)
{ (void)hw; printf("  [hal] ship mode\n"); return 0; }
static void stub_isr(uiox_bms_hw_t *hw) { (void)hw; }

static const uiox_bms_hw_ops_t stub_ops = {
    .init=stub_init,.deinit=stub_deinit,.reg_read=stub_reg_r,
    .reg_write=stub_reg_w,.reg_read16=stub_reg_r16,.reg_write16=stub_reg_w16,
    .bulk_read=stub_bulk,.measure_cells=stub_measure_cells,
    .measure_current=stub_measure_current,.measure_temp=stub_measure_temp,
    .read_coulombs=stub_read_coulombs,.set_chg_fet=stub_set_chg,
    .set_dsg_fet=stub_set_dsg,.set_balance=stub_set_bal,
    .get_balance=stub_get_bal,.fault_status=stub_fault_st,
    .fault_clear=stub_fault_cl,.pack_present=stub_present,
    .ship_mode=stub_ship,.isr=stub_isr,
};

static uiox_bms_hw_t s_hw = {
    .i2c_base=0x40005400uL,.i2c_addr=0x08u,.irq=45,
    .caps=UIOX_BMS_CAP_CELL_VOLTAGE|UIOX_BMS_CAP_PACK_VOLTAGE|
          UIOX_BMS_CAP_CURRENT|UIOX_BMS_CAP_TEMPERATURE|
          UIOX_BMS_CAP_OVP|UIOX_BMS_CAP_UVP|UIOX_BMS_CAP_OCP_DSG|
          UIOX_BMS_CAP_SCP|UIOX_BMS_CAP_OTP|UIOX_BMS_CAP_BALANCING|
          UIOX_BMS_CAP_FET_CHG|UIOX_BMS_CAP_FET_DSG,
    .bus=UIOX_BMS_BUS_I2C,.model="BQ76940",
    .num_cells=4,.num_temps=1,.shunt_uohm=5000u,
    .chg_fet_pin=6,.dsg_fet_pin=7,.alert_pin=8,.pres_pin=9,
};

static void on_bms_event(uiox_bms_ev_t ev, void *ctx)
{ (void)ctx; printf("  [event] %s\n", uiox_bms_ev_name(ev)); }

int main(void)
{
    printf("=== UIOX BMS Stack Demo ===\n\n");

    static const uiox_bms_batt_t batt = {
        .nominal_mah=3500u,.full_mah=3450u,.design_mah=3500u,
        .vfull_mv=16800u,.vempty_mv=11200u,
        .vcell_ovp_mv=4250u,.vcell_uvp_mv=2800u,
        .ocp_chg_ma=4000u,.ocp_dsg_ma=8000u,
        .max_temp_dc=600,.min_temp_dc=-100,.cycle_count=42,
    };

    printf("--- Open ---\n");
    uiox_bms_device_t dev;
    uiox_bms_open_params_t p={.hw=&s_hw,.hw_ops=&stub_ops,
                               .batt=batt,.evt_cb=on_bms_event};
    int rc = uiox_bms_open(&dev, &p);
    if (rc<0){printf("[error] open: %d\n",rc); return 1;}

    printf("\n--- Start ---\n");
    rc = uiox_bms_start(&dev);
    printf("  State: %s  rc=%d\n",
           uiox_bms_state_name(dev.subsys.state),rc);

    printf("\n--- BMS info ---\n");
    uiox_bms_print_info(&dev);

    printf("\n--- OCV→SoC lookup ---\n");
    uint32_t test_ocv[]={2800u,3300u,3700u,3820u,4100u,4200u};
    for(int i=0;i<6;i++)
        printf("  OCV %u mV → SoC %u %%\n",
               test_ocv[i], uiox_bms_algo_ocv_to_soc(test_ocv[i]));

    printf("\n--- Discharge simulation (8 ticks × 1s) ---\n");
    s_current_ma = -1200;
    for(uint32_t t=1000;t<=8000;t+=1000){
        /* Simulate voltage droop */
        for(uint8_t c=0;c<4;c++)
            if(s_cell_mv[c]>3600u) s_cell_mv[c]-=5u;
        uiox_bms_tick(&dev, t);
        printf("  t=%us  SoC=%u%%  pack=%umV  I=%+dmA  TTE=%dmin\n",
               t/1000u, uiox_bms_soc(&dev),
               uiox_bms_pack_mv(&dev), uiox_bms_current(&dev),
               uiox_bms_tte_min(&dev));
    }

    printf("\n--- Switch to charging ---\n");
    s_current_ma = +1750;
    for(uint32_t t=9000;t<=12000;t+=1000){
        for(uint8_t c=0;c<4;c++) s_cell_mv[c]+=8u;
        uiox_bms_tick(&dev, t);
        printf("  t=%us  SoC=%u%%  I=%+dmA  state=%s\n",
               t/1000u, uiox_bms_soc(&dev),
               uiox_bms_current(&dev),
               uiox_bms_state_name(dev.subsys.state));
    }

    printf("\n--- Simulate OVP fault ---\n");
    s_fault = UIOX_BMS_FAULT_OVP;
    uiox_bms_tick(&dev, 13000u);

    printf("\n--- Telemetry snapshot ---\n");
    uiox_bms_telem_t snap;
    uiox_bms_get_telemetry(&dev, &snap, 13500u);
    printf("  pack=%umV  I=%+dmA  SoC=%u%%  SoH=%u%%  remain=%dmAh\n",
           snap.pack_mv,snap.current_ma,snap.soc_pct,
           snap.soh_pct,snap.remain_mah);

    printf("\n--- Statistics ---\n");
    uiox_bms_print_stats(&dev);

    printf("\n--- Event log ---\n");
    uiox_bms_print_events();

    printf("\n--- Stop ---\n");
    uiox_bms_stop(&dev);
    uiox_bms_close(&dev);
    printf("  Device: CLOSED\n");
    printf("\n=== UIOX BMS Demo complete ===\n");
    return 0;
}
