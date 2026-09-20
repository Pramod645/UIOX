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
 * @version 2.1.0  (added PLL MMIO commit path; model-only fallback)
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

/* ── PLL MMIO binding ────────────────────────────────────────────────────────
 * SOC_CLK_BASE is the board's clock-controller register window, defined in
 * 10_BSP/03_SoC/include/uiox_soc_map.h.  It is 0x00000000UL on every QEMU
 * board (no PLL controller exists) and on the generic-* TODO boards, and it
 * aliases SOC_CLINT_BASE on RISC-V (the CLINT holds mtime, not a PLL).
 *
 * Guard contract: UIOX_SOC_CLK_MMIO_ENABLE is defined by the build system
 * ONLY for a board whose clock block is real and whose register offsets are
 * known.  When it is not defined the write path compiles to a no-op that
 * logs once — the previous behaviour, now explicit instead of silent.
 *
 * SOC_CLK_BASE is treated as "usable" only when it is a nonzero literal OR
 * the board explicitly opts in; a base of 0 would write to address 0.
 * ─────────────────────────────────────────────────────────────────────────── */
#if defined(UIOX_SOC_CLK_MMIO_ENABLE)
#  ifndef SOC_CLK_BASE
#    error "UIOX_SOC_CLK_MMIO_ENABLE set but SOC_CLK_BASE is undefined — include uiox_soc_map.h"
#  endif
#  define UIOX_SOC_CLK_MMIO_ACTIVE  1
#else
#  define UIOX_SOC_CLK_MMIO_ACTIVE  0
#endif

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
 * PLL / clock-controller MMIO
 *
 * Register layout assumed (typical CMU/CCF window).  A board that defines
 * UIOX_SOC_CLK_MMIO_ENABLE must supply these offsets — override any of them
 * before including this file if the SoC's layout differs.
 * ====================================================================== */
#ifndef SOC_CLK_PLL_SYS_CTRL
#  define SOC_CLK_PLL_SYS_CTRL   0x0000u   /* system PLL: [mul:div:en:lock] */
#endif
#ifndef SOC_CLK_PLL_CPU_CTRL
#  define SOC_CLK_PLL_CPU_CTRL   0x0004u   /* CPU PLL                        */
#endif
#ifndef SOC_CLK_PLL_DIV_DONE
#  define SOC_CLK_PLL_DIV_DONE   0x0008u   /* divider settle/status          */
#endif

/* Control-word field layout: [31:24] mul | [15:8] div | [1] enable | [0] lock */
#define UIOX_SOC_CLK_PLL_MUL_SHIFT   24u
#define UIOX_SOC_CLK_PLL_DIV_SHIFT    8u
#define UIOX_SOC_CLK_PLL_EN          (1u << 1u)
#define UIOX_SOC_CLK_PLL_LOCK        (1u << 0u)

#define UIOX_SOC_CLK_PLL_MUL_FIELD(m)  (((uiox_uint32_t)(m) & 0xFFu) << UIOX_SOC_CLK_PLL_MUL_SHIFT)
#define UIOX_SOC_CLK_PLL_DIV_FIELD(d)  (((uiox_uint32_t)(d) & 0xFFu) << UIOX_SOC_CLK_PLL_DIV_SHIFT)

#if UIOX_SOC_CLK_MMIO_ACTIVE

static volatile uiox_uint32_t *clk_reg(uiox_uint32_t off)
{
    return (volatile uiox_uint32_t *)(uiox_uintptr_t)(SOC_CLK_BASE + off);
}

/* Program one PLL and poll its lock bit.  Returns UIOX_SOC_OK on lock. */
static uiox_soc_err_t clk_pll_program(uiox_uint32_t ctrl_off,
                                      uiox_uint32_t mul,
                                      uiox_uint32_t div)
{
    uiox_uint32_t word = UIOX_SOC_CLK_PLL_EN
                       | UIOX_SOC_CLK_PLL_MUL_FIELD(mul)
                       | UIOX_SOC_CLK_PLL_DIV_FIELD(div ? div : 1u);

    *clk_reg(ctrl_off) = word;

    /* Bound the wait so a dead controller cannot hang bring-up forever. */
    for (uiox_uint32_t spin = 0u; spin < 100000u; spin++) {
        if (*clk_reg(ctrl_off) & UIOX_SOC_CLK_PLL_LOCK)
            return UIOX_SOC_OK;
    }
    return UIOX_SOC_ERR_IO;
}

/* Apply both PLLs from the context's computed values. */
static uiox_soc_err_t clk_pll_commit(uiox_clk_ctx_t *ctx,
                                     const uiox_soc_desc_t *soc)
{
    uiox_soc_err_t rc;

    /* Only touch hardware for a board that actually has a clock controller. */
    if (soc && soc->clk_base == 0u) {
        ctx->pll_sys.locked = true;
        ctx->pll_cpu.locked = true;
        SOC_LOG("CLK", "pll: no controller on this board — model only");
        return UIOX_SOC_OK;
    }

    rc = clk_pll_program(SOC_CLK_PLL_SYS_CTRL,
                         ctx->pll_sys.mul, ctx->pll_sys.div);
    if (rc != UIOX_SOC_OK) {
        ctx->pll_sys.locked = false;
        SOC_LOG("CLK", "pll_sys FAILED: no lock (ctrl=0x%x)", SOC_CLK_PLL_SYS_CTRL);
        return rc;
    }
    ctx->pll_sys.locked = true;

    rc = clk_pll_program(SOC_CLK_PLL_CPU_CTRL,
                         ctx->pll_cpu.mul, ctx->pll_cpu.div);
    if (rc != UIOX_SOC_OK) {
        ctx->pll_cpu.locked = false;
        SOC_LOG("CLK", "pll_cpu FAILED: no lock (ctrl=0x%x)", SOC_CLK_PLL_CPU_CTRL);
        return rc;
    }
    ctx->pll_cpu.locked = true;
    return UIOX_SOC_OK;
}

#else  /* !UIOX_SOC_CLK_MMIO_ACTIVE — model-only board (all QEMU targets) */

static uiox_soc_err_t clk_pll_commit(uiox_clk_ctx_t *ctx,
                                     const uiox_soc_desc_t *soc)
{
    (void)soc;
    /* Values were computed in uiox_soc_clk_init().  Nothing to write. */
    ctx->pll_sys.locked = true;
    ctx->pll_cpu.locked = true;
    SOC_LOG("CLK", "pll: MMIO not wired on this board — model only");
    return UIOX_SOC_OK;
}

#endif /* UIOX_SOC_CLK_MMIO_ACTIVE */

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

    ctx->pll_cpu.ref_hz = UIOX_SOC_CLK_REF_24MHZ;
    ctx->pll_cpu.mul    = (ctx->freq_hz[UIOX_SOC_CLK_CPU0] > 0u)
                          ? ctx->freq_hz[UIOX_SOC_CLK_CPU0] /
                            UIOX_SOC_CLK_REF_24MHZ : 1u;
    ctx->pll_cpu.div    = 1u;
    ctx->pll_cpu.out_hz = ctx->freq_hz[UIOX_SOC_CLK_CPU0];

    /* Program the PLLs.  On a model-only board this is a no-op; on a real
     * board it writes the CMU/CCF window and waits for the lock bit. */
    {
        uiox_soc_err_t prc = clk_pll_commit(ctx, soc);
        if (prc != UIOX_SOC_OK) {
            ctx->initialized = false;
            return prc;
        }
    }

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
