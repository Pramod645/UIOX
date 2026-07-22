/**
 * @file    uiox_fw_clock.c
 * @brief   UIOX SoC — Clock / PLL management.
 *
 * Implementation file matched to the unified uiox_soc_clk.h header.
 *
 * Provides:
 *   ── Stateless API (uiox_soc_clock_*) ─────────────────────────────
 *      Module-internal global clock table; used by driver code that
 *      does not manage an explicit context.
 *
 *   ── Stateful API (uiox_soc_clk_*) ────────────────────────────────
 *      Takes an explicit uiox_clk_ctx_t *; used by SoC backend files
 *      (uiox_soc_arm64.c, uiox_soc_x86.c, …).
 *
 *   ── Backwards-compatible forwarders (uiox_fw_clock_*) ────────────
 *      Thin wrappers so code written against the old uiox_fw_clock_*
 *      API compiles and links unchanged.
 *
 * Platform frequency defaults (QEMU / simulation):
 *   ARM64 virt  : CPU 1 GHz, BUS 500 MHz, UART 24 MHz, TIMER 62.5 MHz
 *   ARM32 versatilepb : CPU 400 MHz, BUS 200 MHz, UART 24 MHz
 *   x86_64 q35  : CPU 2 GHz, BUS 200 MHz, COM1 1.8432 MHz, PIT 1.193 MHz
 *
 * On QEMU targets set_hz() records the requested value only.
 * Real SoC ports: add CMU / CCF MMIO writes where marked.
 *
 * Matches:
 *   70_build_config/70_CPU_SoC   (SoC clock configuration)
 *   02_FwHal/include/uiox_soc_clk.h  (unified clock header)
 *
 * @version 2.0.0
 * @date    2026-07-18
 */
/**
 * @file    uiox_soc_clk.c
 * @brief   UIOX SoC — Clock / PLL management.
 * @version 2.0.1  (fixed: missing includes, removed duplicate fw wrappers)
 * @date    2026-07-18
 */

/*
 * Include order matters:
 *   1. uiox_soc_clk.h   — clock types and API declarations
 *   2. uiox_soc.h        — provides uiox_soc_memset, uiox_soc_printf,
 *                          SOC_LOG, uiox_soc_desc_t
 *
 * uiox_soc.h must come AFTER uiox_soc_clk.h so that uiox_clk_ctx_t
 * is already defined before uiox_soc.h pulls in other headers that
 * reference it.
 */
#include "uiox_soc_clk.h"
#include "uiox_soc.h"       /* uiox_soc_memset, uiox_soc_printf, SOC_LOG */

/* =========================================================================
 * Default clock frequencies per platform
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
 * Module-internal global clock table
 * ====================================================================== */
static uiox_soc_clock_t s_clocks[UIOX_SOC_CLOCK_MAX];

static void clock_set(uiox_soc_clk_id_t id,
                       const char       *name,
                       uiox_uint32_t          hz,
                       uiox_bool_t              enabled)
{
    if ((uiox_uint32_t)id >= UIOX_SOC_CLOCK_MAX) return;
    uiox_soc_clock_t *c = &s_clocks[id];
    c->id      = id;
    c->freq_hz = hz;
    c->enabled = enabled;
    for (int i = 0; i < 15 && name[i]; i++)
        c->name[i] = name[i];
    c->name[15] = '\0';
}

/* =========================================================================
 * Per-arch default table builders
 * ====================================================================== */

#if defined(__aarch64__)
static void clock_table_init(void)
{
    clock_set(UIOX_SOC_CLK_CPU,           "cpu",      CLK_ARM64_CPU_HZ,     true);
    clock_set(UIOX_SOC_CLK_BUS,           "bus",      CLK_ARM64_BUS_HZ,     true);
    clock_set(UIOX_SOC_CLK_UART0,         "uart0",    CLK_ARM64_UART_HZ,    true);
    clock_set(UIOX_SOC_CLK_UART1,         "uart1",    CLK_ARM64_UART_HZ,    false);
    clock_set(UIOX_SOC_CLK_TIMER0,        "timer0",   CLK_ARM64_TIMER_HZ,   true);
    clock_set(UIOX_SOC_CLK_GPIO,          "gpio",     CLK_ARM64_GPIO_HZ,    true);
    clock_set(UIOX_SOC_CLK_GIC,           "gic",      CLK_ARM64_BUS_HZ,     true);
    clock_set(UIOX_SOC_CLK_I2C,           "i2c",      CLK_ARM64_BUS_HZ,     false);
    clock_set(UIOX_SOC_CLK_SPI,           "spi",      CLK_ARM64_BUS_HZ,     false);
    clock_set(UIOX_SOC_CLK_ETH,           "eth",      CLK_ARM64_ETH_HZ,     false);
    clock_set(UIOX_SOC_CLK_USB,           "usb",      CLK_ARM64_BUS_HZ,     false);
    clock_set(UIOX_SOC_CLK_EMMC,          "emmc",     CLK_ARM64_STORAGE_HZ, false);
    clock_set(UIOX_SOC_CLK_PCIE,          "pcie",     CLK_ARM64_BUS_HZ,     false);
    clock_set(UIOX_SOC_CLK_GENERIC_TIMER, "gentimer", CLK_ARM64_UART_HZ,    true);
}

#elif defined(__arm__)
static void clock_table_init(void)
{
    clock_set(UIOX_SOC_CLK_CPU,           "cpu",      CLK_ARM32_CPU_HZ,     true);
    clock_set(UIOX_SOC_CLK_BUS,           "bus",      CLK_ARM32_BUS_HZ,     true);
    clock_set(UIOX_SOC_CLK_UART0,         "uart0",    CLK_ARM32_UART_HZ,    true);
    clock_set(UIOX_SOC_CLK_UART1,         "uart1",    CLK_ARM32_UART_HZ,    false);
    clock_set(UIOX_SOC_CLK_TIMER0,        "timer0",   CLK_ARM32_TIMER_HZ,   true);
    clock_set(UIOX_SOC_CLK_GPIO,          "gpio",     CLK_ARM32_GPIO_HZ,    true);
    clock_set(UIOX_SOC_CLK_GIC,           "gic",      CLK_ARM32_BUS_HZ,     true);
    clock_set(UIOX_SOC_CLK_I2C,           "i2c",      CLK_ARM32_BUS_HZ,     false);
    clock_set(UIOX_SOC_CLK_SPI,           "spi",      CLK_ARM32_BUS_HZ,     false);
    clock_set(UIOX_SOC_CLK_ETH,           "eth",      CLK_ARM32_ETH_HZ,     false);
    clock_set(UIOX_SOC_CLK_USB,           "usb",      CLK_ARM32_BUS_HZ,     false);
    clock_set(UIOX_SOC_CLK_EMMC,          "emmc",     CLK_ARM32_STORAGE_HZ, false);
    clock_set(UIOX_SOC_CLK_PCIE,          "pcie",     0u,                   false);
    clock_set(UIOX_SOC_CLK_GENERIC_TIMER, "gentimer", CLK_ARM32_UART_HZ,    true);
}

#else  /* x86_64 and others */
static void clock_table_init(void)
{
    clock_set(UIOX_SOC_CLK_CPU,           "cpu",      CLK_X86_CPU_HZ,       true);
    clock_set(UIOX_SOC_CLK_BUS,           "bus",      CLK_X86_BUS_HZ,       true);
    clock_set(UIOX_SOC_CLK_UART0,         "com1",     CLK_X86_UART_HZ,      true);
    clock_set(UIOX_SOC_CLK_UART1,         "com2",     CLK_X86_UART_HZ,      false);
    clock_set(UIOX_SOC_CLK_TIMER0,        "pit",      CLK_X86_TIMER_HZ,     true);
    clock_set(UIOX_SOC_CLK_GPIO,          "gpio",     CLK_X86_GPIO_HZ,      false);
    clock_set(UIOX_SOC_CLK_GIC,           "gic",      0u,                   false);
    clock_set(UIOX_SOC_CLK_I2C,           "i2c",      CLK_X86_BUS_HZ,       false);
    clock_set(UIOX_SOC_CLK_SPI,           "spi",      CLK_X86_BUS_HZ,       false);
    clock_set(UIOX_SOC_CLK_ETH,           "eth",      CLK_X86_ETH_HZ,       false);
    clock_set(UIOX_SOC_CLK_USB,           "usb",      CLK_X86_BUS_HZ,       false);
    clock_set(UIOX_SOC_CLK_EMMC,          "emmc",     0u,                   false);
    clock_set(UIOX_SOC_CLK_PCIE,          "pcie",     CLK_X86_BUS_HZ,       false);
    clock_set(UIOX_SOC_CLK_GENERIC_TIMER, "gentimer", 0u,                   false);
}
#endif

/* =========================================================================
 * Stateless API — uiox_soc_clock_*
 * ====================================================================== */

uiox_soc_err_t uiox_soc_clock_init(void)
{
    uiox_soc_memset(s_clocks, 0, sizeof(s_clocks));
    clock_table_init();
    SOC_LOG("CLK", "clock registry init OK (%u entries)",
            (uiox_uint32_t)UIOX_SOC_CLOCK_MAX);
    return UIOX_SOC_OK;
}

uiox_soc_err_t uiox_soc_clock_enable(uiox_soc_clk_id_t id)
{
    if ((uiox_uint32_t)id >= UIOX_SOC_CLOCK_MAX) return UIOX_SOC_ERR_INVAL;
    s_clocks[id].enabled = true;
    SOC_LOG("CLK", "enable  %-10s  %u Hz",
            s_clocks[id].name, s_clocks[id].freq_hz);
    return UIOX_SOC_OK;
}

uiox_soc_err_t uiox_soc_clock_disable(uiox_soc_clk_id_t id)
{
    if ((uiox_uint32_t)id >= UIOX_SOC_CLOCK_MAX) return UIOX_SOC_ERR_INVAL;
    if (id == UIOX_SOC_CLK_CPU || id == UIOX_SOC_CLK_BUS)
        return UIOX_SOC_ERR_PERM;
    s_clocks[id].enabled = false;
    SOC_LOG("CLK", "disable %-10s", s_clocks[id].name);
    return UIOX_SOC_OK;
}

uiox_uint32_t uiox_soc_clock_get_hz(uiox_soc_clk_id_t id)
{
    if ((uiox_uint32_t)id >= UIOX_SOC_CLOCK_MAX) return 0u;
    return s_clocks[id].enabled ? s_clocks[id].freq_hz : 0u;
}

uiox_soc_err_t uiox_soc_clock_set_hz(uiox_soc_clk_id_t id, uiox_uint32_t hz)
{
    if ((uiox_uint32_t)id >= UIOX_SOC_CLOCK_MAX) return UIOX_SOC_ERR_INVAL;
    if (hz == 0u)                            return UIOX_SOC_ERR_INVAL;
    s_clocks[id].freq_hz = hz;
    SOC_LOG("CLK", "set_hz  %-10s  %u Hz", s_clocks[id].name, hz);
    return UIOX_SOC_OK;
}

void uiox_soc_clock_print(void)
{
    uiox_soc_printf("[SOC] Clock table (%u entries):\n",
                     (uiox_uint32_t)UIOX_SOC_CLOCK_MAX);
    for (uiox_uint32_t i = 0u; i < (uiox_uint32_t)UIOX_SOC_CLOCK_MAX; i++) {
        const uiox_soc_clock_t *c = &s_clocks[i];
        uiox_soc_printf("  [%2u] %-10s  %10u Hz  %s\n",
                         i, c->name, c->freq_hz,
                         c->enabled ? "ON" : "OFF");
    }
}

/* =========================================================================
 * Stateful API — uiox_soc_clk_*
 * ====================================================================== */

uiox_soc_err_t uiox_soc_clk_init(uiox_clk_ctx_t        *ctx,
                                   const uiox_soc_desc_t *soc)
{
    if (!ctx || !soc) return UIOX_SOC_ERR_INVAL;

    uiox_soc_err_t rc = uiox_soc_clock_init();
    if (rc != UIOX_SOC_OK) return rc;

    for (uiox_uint32_t i = 0u; i < UIOX_SOC_CLK__COUNT; i++) {
        ctx->freq_hz[i] = s_clocks[i].freq_hz;
        ctx->enabled[i] = s_clocks[i].enabled;
    }

    if (soc->cpu_freq_khz > 0u)
        ctx->freq_hz[UIOX_SOC_CLK_CPU0] = soc->cpu_freq_khz * 1000u;

    ctx->pll_sys.ref_hz = UIOX_SOC_CLK_REF_24MHZ;
    ctx->pll_sys.mul    = (ctx->freq_hz[UIOX_SOC_CLK_BUS] > 0u)
                          ? ctx->freq_hz[UIOX_SOC_CLK_BUS] /
                            UIOX_SOC_CLK_REF_24MHZ : 1u;
    ctx->pll_sys.div    = 1u;
    ctx->pll_sys.out_hz = ctx->freq_hz[UIOX_SOC_CLK_BUS];
    ctx->pll_sys.locked = true;

    ctx->pll_cpu.ref_hz = UIOX_SOC_CLK_REF_24MHZ;
    ctx->pll_cpu.mul    = (ctx->freq_hz[UIOX_SOC_CLK_CPU0] > 0u)
                          ? ctx->freq_hz[UIOX_SOC_CLK_CPU0] /
                            UIOX_SOC_CLK_REF_24MHZ : 1u;
    ctx->pll_cpu.div    = 1u;
    ctx->pll_cpu.out_hz = ctx->freq_hz[UIOX_SOC_CLK_CPU0];
    ctx->pll_cpu.locked = true;

    ctx->initialized = true;

    SOC_LOG("CLK", "ctx init  cpu=%u MHz  bus=%u MHz",
            ctx->freq_hz[UIOX_SOC_CLK_CPU0] / 1000000u,
            ctx->freq_hz[UIOX_SOC_CLK_BUS]  / 1000000u);
    return UIOX_SOC_OK;
}

uiox_uint32_t uiox_soc_clk_get_hz(const uiox_clk_ctx_t *ctx,
                               uiox_soc_clk_id_t     id)
{
    if (!ctx || (uiox_uint32_t)id >= UIOX_SOC_CLK__COUNT) return 0u;
    return ctx->enabled[id] ? ctx->freq_hz[id] : 0u;
}

uiox_soc_err_t uiox_soc_clk_enable(uiox_clk_ctx_t    *ctx,
                                     uiox_soc_clk_id_t  id)
{
    if (!ctx || (uiox_uint32_t)id >= UIOX_SOC_CLK__COUNT)
        return UIOX_SOC_ERR_INVAL;
    ctx->enabled[id] = true;
    if ((uiox_uint32_t)id < UIOX_SOC_CLOCK_MAX)
        s_clocks[id].enabled = true;
    return UIOX_SOC_OK;
}

uiox_soc_err_t uiox_soc_clk_disable(uiox_clk_ctx_t    *ctx,
                                      uiox_soc_clk_id_t  id)
{
    if (!ctx || (uiox_uint32_t)id >= UIOX_SOC_CLK__COUNT)
        return UIOX_SOC_ERR_INVAL;
    if (id == UIOX_SOC_CLK_CPU0 || id == UIOX_SOC_CLK_BUS)
        return UIOX_SOC_ERR_PERM;
    ctx->enabled[id] = false;
    if ((uiox_uint32_t)id < UIOX_SOC_CLOCK_MAX)
        s_clocks[id].enabled = false;
    return UIOX_SOC_OK;
}

void uiox_soc_clk_print(const uiox_clk_ctx_t *ctx)
{
    if (!ctx) { uiox_soc_clock_print(); return; }

    uiox_soc_printf("[SOC] Clock context (%u entries):\n",
                     (uiox_uint32_t)UIOX_SOC_CLK__COUNT);
    for (uiox_uint32_t i = 0u; i < (uiox_uint32_t)UIOX_SOC_CLK__COUNT; i++) {
        uiox_soc_printf("  [%2u] %-10s  %10u Hz  %s\n",
                         i,
                         ((uiox_uint32_t)i < UIOX_SOC_CLOCK_MAX &&
                          s_clocks[i].name[0])
                             ? s_clocks[i].name : "?",
                         ctx->freq_hz[i],
                         ctx->enabled[i] ? "ON" : "OFF");
    }
    uiox_soc_printf("  pll_sys : %u MHz  locked=%s\n",
                     ctx->pll_sys.out_hz / 1000000u,
                     ctx->pll_sys.locked ? "yes" : "no");
    uiox_soc_printf("  pll_cpu : %u MHz  locked=%s\n",
                     ctx->pll_cpu.out_hz / 1000000u,
                     ctx->pll_cpu.locked ? "yes" : "no");
}

/* =========================================================================
 * Backwards-compatible forwarders — uiox_fw_clock_*
 *
 * Declared as EXTERN in uiox_soc_clk.h (not static inline).
 * Defined here ONCE. No redefinition conflict possible.
 * ====================================================================== */

uiox_soc_err_t uiox_fw_clock_init(void)
{
    return uiox_soc_clock_init();
}

uiox_soc_err_t uiox_fw_clock_enable(uiox_soc_clk_id_t id)
{
    return uiox_soc_clock_enable(id);
}

uiox_soc_err_t uiox_fw_clock_disable(uiox_soc_clk_id_t id)
{
    return uiox_soc_clock_disable(id);
}

uiox_uint32_t uiox_fw_clock_get_hz(uiox_soc_clk_id_t id)
{
    return uiox_soc_clock_get_hz(id);
}

uiox_soc_err_t uiox_fw_clock_set_hz(uiox_soc_clk_id_t id, uiox_uint32_t hz)
{
    return uiox_soc_clock_set_hz(id, hz);
}

void uiox_fw_clock_print(void)
{
    uiox_soc_clock_print();
}
