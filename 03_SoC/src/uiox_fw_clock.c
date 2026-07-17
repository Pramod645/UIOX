/**
 * @file  uiox_fw_clock.c
 * @brief UIOX Firmware — Clock / PLL management.
 *
 * Provides a platform-agnostic clock registry. Each peripheral clock
 * entry has a frequency, enable state, and name. On QEMU targets the
 * clocks are fixed (no PLL reconfiguration required), so set_hz()
 * records the requested value but does not write hardware.
 *
 * Real SoC ports would add MMIO writes to the appropriate CMU / CCF
 * (Clock Control Framework) registers here.
 *
 * Matches:
 *   70_build_config/70_CPU_SoC  (SoC clock configuration)
 *   uiox_fw_clock.h public API
 *
 * @version 1.0.0
 * @date    2026-06-25
 */
//uioxfwclock.c | Clock registry with per-platform default frequency tables (ARM64/ARM32/x86), enable/disable with CPU+BUS protection, get/sethz (records value; real SoC would write CMU registers), print table |

 #include "uiox_fw.h"

 /* =========================================================================
  * Default clock frequencies for each supported platform
  * ====================================================================== */
 
 /* ARM64 QEMU virt */
 #define CLK_ARM64_CPU_HZ        1000000000u
 #define CLK_ARM64_BUS_HZ         500000000u
 #define CLK_ARM64_UART_HZ         24000000u
 #define CLK_ARM64_TIMER_HZ        62500000u
 #define CLK_ARM64_GPIO_HZ         24000000u
 #define CLK_ARM64_ETH_HZ         125000000u
 #define CLK_ARM64_STORAGE_HZ      50000000u
 
 /* ARM32 QEMU versatilepb */
 #define CLK_ARM32_CPU_HZ         400000000u
 #define CLK_ARM32_BUS_HZ         200000000u
 #define CLK_ARM32_UART_HZ         24000000u
 #define CLK_ARM32_TIMER_HZ         1000000u
 #define CLK_ARM32_GPIO_HZ         24000000u
 #define CLK_ARM32_ETH_HZ          25000000u
 #define CLK_ARM32_STORAGE_HZ      25000000u
 
 /* x86_64 QEMU q35 */
 #define CLK_X86_CPU_HZ          2000000000u
 #define CLK_X86_BUS_HZ           200000000u
 #define CLK_X86_UART_HZ            1843200u
 #define CLK_X86_TIMER_HZ           1193182u
 #define CLK_X86_GPIO_HZ                  0u
 #define CLK_X86_ETH_HZ           125000000u
 #define CLK_X86_STORAGE_HZ       100000000u
 
 /* =========================================================================
  * Clock table
  * ====================================================================== */
 
 static uiox_fw_clock_t s_clocks[UIOX_FW_CLK_MAX];
 
 static void clock_set(uiox_fw_clk_id_t id, const char *name,
                        uint32_t hz, bool enabled)
 {
     if (id >= UIOX_FW_CLK_MAX) return;
     uiox_fw_clock_t *c = &s_clocks[id];
     c->id      = id;
     c->freq_hz = hz;
     c->enabled = enabled;
     for (int i = 0; i < 15 && name[i]; i++)
         c->name[i] = name[i];
     c->name[15] = '\0';
 }
 
 /* =========================================================================
  * Per-arch default table builders
  * Each is compiled ONLY for its own architecture — no unused-function
  * warnings on the other two.
  * ====================================================================== */
 
 #if defined(__aarch64__)
 static void clock_table_init(void)
 {
     clock_set(UIOX_FW_CLK_CPU,     "cpu",     CLK_ARM64_CPU_HZ,     true);
     clock_set(UIOX_FW_CLK_BUS,     "bus",     CLK_ARM64_BUS_HZ,     true);
     clock_set(UIOX_FW_CLK_UART0,   "uart0",   CLK_ARM64_UART_HZ,    true);
     clock_set(UIOX_FW_CLK_UART1,   "uart1",   CLK_ARM64_UART_HZ,    false);
     clock_set(UIOX_FW_CLK_TIMER0,  "timer0",  CLK_ARM64_TIMER_HZ,   true);
     clock_set(UIOX_FW_CLK_GPIO,    "gpio",    CLK_ARM64_GPIO_HZ,    true);
     clock_set(UIOX_FW_CLK_I2C,     "i2c",     CLK_ARM64_BUS_HZ,     false);
     clock_set(UIOX_FW_CLK_SPI,     "spi",     CLK_ARM64_BUS_HZ,     false);
     clock_set(UIOX_FW_CLK_ETH,     "eth",     CLK_ARM64_ETH_HZ,     false);
     clock_set(UIOX_FW_CLK_STORAGE, "storage", CLK_ARM64_STORAGE_HZ, false);
 }
 
 #elif defined(__arm__)
 static void clock_table_init(void)
 {
     clock_set(UIOX_FW_CLK_CPU,     "cpu",     CLK_ARM32_CPU_HZ,     true);
     clock_set(UIOX_FW_CLK_BUS,     "bus",     CLK_ARM32_BUS_HZ,     true);
     clock_set(UIOX_FW_CLK_UART0,   "uart0",   CLK_ARM32_UART_HZ,    true);
     clock_set(UIOX_FW_CLK_UART1,   "uart1",   CLK_ARM32_UART_HZ,    false);
     clock_set(UIOX_FW_CLK_TIMER0,  "timer0",  CLK_ARM32_TIMER_HZ,   true);
     clock_set(UIOX_FW_CLK_GPIO,    "gpio",    CLK_ARM32_GPIO_HZ,    true);
     clock_set(UIOX_FW_CLK_I2C,     "i2c",     CLK_ARM32_BUS_HZ,     false);
     clock_set(UIOX_FW_CLK_SPI,     "spi",     CLK_ARM32_BUS_HZ,     false);
     clock_set(UIOX_FW_CLK_ETH,     "eth",     CLK_ARM32_ETH_HZ,     false);
     clock_set(UIOX_FW_CLK_STORAGE, "storage", CLK_ARM32_STORAGE_HZ, false);
 }
 
 #else  /* x86_64 */
 static void clock_table_init(void)
 {
     clock_set(UIOX_FW_CLK_CPU,     "cpu",     CLK_X86_CPU_HZ,       true);
     clock_set(UIOX_FW_CLK_BUS,     "bus",     CLK_X86_BUS_HZ,       true);
     clock_set(UIOX_FW_CLK_UART0,   "com1",    CLK_X86_UART_HZ,      true);
     clock_set(UIOX_FW_CLK_UART1,   "com2",    CLK_X86_UART_HZ,      false);
     clock_set(UIOX_FW_CLK_TIMER0,  "pit",     CLK_X86_TIMER_HZ,     true);
     clock_set(UIOX_FW_CLK_GPIO,    "gpio",    CLK_X86_GPIO_HZ,      false);
     clock_set(UIOX_FW_CLK_I2C,     "i2c",     CLK_X86_BUS_HZ,       false);
     clock_set(UIOX_FW_CLK_SPI,     "spi",     CLK_X86_BUS_HZ,       false);
     clock_set(UIOX_FW_CLK_ETH,     "eth",     CLK_X86_ETH_HZ,       false);
     clock_set(UIOX_FW_CLK_STORAGE, "storage", CLK_X86_STORAGE_HZ,   false);
 }
 #endif
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_clock_init(void)
 {
     uiox_fw_memset(s_clocks, 0, sizeof(s_clocks));
     clock_table_init();
     FW_LOG("CLK", "clock registry init OK (%u entries)",
            (uint32_t)UIOX_FW_CLK_MAX);
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_clock_enable(uiox_fw_clk_id_t id)
 {
     if (id >= UIOX_FW_CLK_MAX) return UIOX_FW_ERR_INVAL;
     s_clocks[id].enabled = true;
     FW_LOG("CLK", "enable  %-10s  %u Hz",
            s_clocks[id].name, s_clocks[id].freq_hz);
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_clock_disable(uiox_fw_clk_id_t id)
 {
     if (id >= UIOX_FW_CLK_MAX) return UIOX_FW_ERR_INVAL;
     if (id == UIOX_FW_CLK_CPU || id == UIOX_FW_CLK_BUS)
         return UIOX_FW_ERR_PERM;
     s_clocks[id].enabled = false;
     FW_LOG("CLK", "disable %-10s", s_clocks[id].name);
     return UIOX_FW_OK;
 }
 
 uint32_t uiox_fw_clock_get_hz(uiox_fw_clk_id_t id)
 {
     if (id >= UIOX_FW_CLK_MAX) return 0u;
     return s_clocks[id].enabled ? s_clocks[id].freq_hz : 0u;
 }
 
 uiox_fw_err_t uiox_fw_clock_set_hz(uiox_fw_clk_id_t id, uint32_t hz)
 {
     if (id >= UIOX_FW_CLK_MAX) return UIOX_FW_ERR_INVAL;
     if (hz == 0u) return UIOX_FW_ERR_INVAL;
     s_clocks[id].freq_hz = hz;
     FW_LOG("CLK", "set_hz  %-10s  %u Hz", s_clocks[id].name, hz);
     return UIOX_FW_OK;
 }
 
 void uiox_fw_clock_print(void)
 {
     uiox_fw_printf("[FW] Clock table (%u entries):\n",
                     (uint32_t)UIOX_FW_CLK_MAX);
     for (uint32_t i = 0u; i < (uint32_t)UIOX_FW_CLK_MAX; i++) {
         const uiox_fw_clock_t *c = &s_clocks[i];
         uiox_fw_printf("  [%2u] %-10s  %10u Hz  %s\n",
                         i, c->name, c->freq_hz,
                         c->enabled ? "ON" : "OFF");
     }
 }
 