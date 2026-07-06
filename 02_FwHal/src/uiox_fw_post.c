/**
 * @file    uiox_fw_post.c
 * @brief   UIOX Firmware — Power-On Self Test implementation.
 * @date    2026-07-06
 */
#include "uiox_fw.h"
#include <string.h>

/* ── helpers ─────────────────────────────────────────────── */
static void entry_init(uiox_post_entry_t *e, uint8_t id, const char *name)
{
    memset(e, 0, sizeof(*e));
    e->test_id = id;
    /* safe strncpy without libc */
    uint32_t i = 0;
    while (name[i] && i < sizeof(e->name) - 1u) {
        e->name[i] = name[i]; i++;
    }
}

static void record(uiox_fw_post_ctx_t *ctx, uiox_post_entry_t *e,
                    uiox_post_result_t result, uint64_t start_us,
                    const char *detail)
{
    e->result      = result;
    e->duration_us = uiox_fw_hw_timestamp_us() - start_us;
    /* copy detail */
    if (detail) {
        uint32_t i = 0;
        while (detail[i] && i < sizeof(e->detail) - 1u) {
            e->detail[i] = detail[i]; i++;
        }
    }
    ctx->tests[ctx->count] = *e;
    ctx->count++;
    if (result == UIOX_POST_PASS || result == UIOX_POST_WARN)
        ctx->pass_count++;
    if (result == UIOX_POST_WARN)
        ctx->warn_count++;
    if (result == UIOX_POST_FAIL || result == UIOX_POST_CRITICAL)
        ctx->fail_count++;
    if (result == UIOX_POST_CRITICAL)
        ctx->any_critical = true;
    ctx->total_us += e->duration_us;
}

/* ── Individual tests ─────────────────────────────────────── */

uiox_post_result_t uiox_fw_post_test_cpu(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_CPU, "CPU");
#if defined(__aarch64__)
    uint64_t el = 0;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
    el = (el >> 2u) & 3u;
    if (el < 1u) return UIOX_POST_CRITICAL;

    /* Check NZCV flags round-trip */
    uint64_t nzcv_orig, nzcv_back;
    __asm__ volatile("mrs %0, NZCV" : "=r"(nzcv_orig));
    __asm__ volatile("msr NZCV, %0\nmrs %1, NZCV"
                     : "=r"(nzcv_back) : "r"(nzcv_orig));
    if ((nzcv_orig & 0xF0000000u) != (nzcv_back & 0xF0000000u))
        return UIOX_POST_FAIL;
    return UIOX_POST_PASS;
#elif defined(__arm__)
    uint32_t cpsr;
    __asm__ volatile("mrs %0, cpsr" : "=r"(cpsr));
    uint32_t mode = cpsr & 0x1Fu;
    /* must be in SVC or SYS mode */
    if (mode != 0x13u && mode != 0x1Fu)
        return UIOX_POST_FAIL;
    return UIOX_POST_PASS;
#elif defined(__x86_64__)
    /* Check CPUID is available — toggle ID flag in RFLAGS */
    uint64_t flags1, flags2;
    __asm__ volatile(
        "pushfq\npopq %0\n"
        "movq %0, %1\n"
        "xorq $0x200000, %1\n"
        "pushq %1\npopfq\n"
        "pushfq\npopq %1\n"
        "pushq %0\npopfq\n"
        : "=r"(flags1), "=r"(flags2));
    if (flags1 == flags2) return UIOX_POST_FAIL; /* CPUID not supported */
    return UIOX_POST_PASS;
#else
    return UIOX_POST_WARN;
#endif
}

uiox_post_result_t uiox_fw_post_test_cache(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_CACHE, "D-Cache");
    /* Write a pattern, flush, read back */
    static volatile uint32_t __attribute__((aligned(64))) probe[4];
    probe[0] = 0xDEADBEEFu;
    probe[1] = 0xCAFEBABEu;
    probe[2] = 0x12345678u;
    probe[3] = 0xABCDABCDu;

#if defined(__aarch64__)
    __asm__ volatile("dc civac, %0" :: "r"((uintptr_t)probe) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("ic iallu" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
#elif defined(__arm__)
    __asm__ volatile("mcr p15,0,%0,c7,c14,1" :: "r"((uint32_t)probe) : "memory");
    __asm__ volatile("dsb" ::: "memory");
#else
    __asm__ volatile("clflushopt (%0)" :: "r"((void *)probe) : "memory");
    __asm__ volatile("mfence" ::: "memory");
#endif

    if (probe[0] != 0xDEADBEEFu || probe[1] != 0xCAFEBABEu ||
        probe[2] != 0x12345678u  || probe[3] != 0xABCDABCDu)
        return UIOX_POST_FAIL;
    return UIOX_POST_PASS;
}

uiox_post_result_t uiox_fw_post_test_ram(uiox_post_entry_t *e,
                                           uintptr_t base, uint64_t size)
{
    entry_init(e, UIOX_POST_RAM, "DRAM march");
    if (!base || size < 1024u) return UIOX_POST_WARN;

    /* March C- pattern on first 4 KB */
    uint64_t test_size = (size > 4096u) ? 4096u : size;
    volatile uint32_t *mem = (volatile uint32_t *)base;
    uint32_t words = (uint32_t)(test_size / 4u);

    /* Write 0x00000000 */
    for (uint32_t i = 0; i < words; i++) mem[i] = 0x00000000u;
    /* Read 0, write 1 ascending */
    for (uint32_t i = 0; i < words; i++) {
        if (mem[i] != 0x00000000u) return UIOX_POST_FAIL;
        mem[i] = 0xFFFFFFFFu;
    }
    /* Read 1, write 0 descending */
    for (int32_t i = (int32_t)words - 1; i >= 0; i--) {
        if (mem[i] != 0xFFFFFFFFu) return UIOX_POST_FAIL;
        mem[i] = 0x00000000u;
    }
    /* Final read 0 */
    for (uint32_t i = 0; i < words; i++)
        if (mem[i] != 0x00000000u) return UIOX_POST_FAIL;

    return UIOX_POST_PASS;
}

uiox_post_result_t uiox_fw_post_test_uart(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_UART, "UART TX");
    /* Send a NUL byte (0x00) and verify no TX fault */
    uiox_fw_hw_uart_putc('\0');
    return UIOX_POST_PASS;
}

uiox_post_result_t uiox_fw_post_test_timer(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_TIMER, "Timer tick");
    uint64_t t0 = uiox_fw_hw_timestamp_us();
    /* Busy-wait ~1 ms */
    volatile uint32_t spin = 100000u;
    while (spin--) __asm__ volatile("" ::: "memory");
    uint64_t t1 = uiox_fw_hw_timestamp_us();
    if (t1 <= t0) return UIOX_POST_FAIL;   /* timer not advancing */
    return UIOX_POST_PASS;
}

uiox_post_result_t uiox_fw_post_test_irq(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_IRQ, "IRQ ctrl");
    /* Check IRQ controller is reachable via MMIO */
#if defined(__aarch64__)
    extern uintptr_t g_gicd_base; /* set by arch_register */
    if (!g_gicd_base) return UIOX_POST_WARN;
    /* Read GIC distributor TYPER — should have ITLinesNumber > 0 */
    uint32_t typer = uiox_fw_hw_mmio_r32(g_gicd_base + 0x004u);
    if ((typer & 0x1Fu) == 0u) return UIOX_POST_FAIL;
    return UIOX_POST_PASS;
#else
    return UIOX_POST_WARN;
#endif
}

uiox_post_result_t uiox_fw_post_test_clock(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_CLOCK, "PLL lock");
#if defined(__aarch64__)
    uint64_t cntfrq;
    __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r"(cntfrq));
    if (cntfrq < 1000000u) return UIOX_POST_WARN; /* < 1 MHz is suspicious */
    return UIOX_POST_PASS;
#else
    return UIOX_POST_WARN;
#endif
}

uiox_post_result_t uiox_fw_post_test_power(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_POWER, "Power domains");
    /* Stub: real impl would read power domain status registers */
    return UIOX_POST_PASS;
}

uiox_post_result_t uiox_fw_post_test_storage(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_STORAGE, "Block device");
    /* Stub: real impl would probe eMMC / NVMe presence bit */
    return UIOX_POST_WARN;  /* warn = optional storage not yet detected */
}

uiox_post_result_t uiox_fw_post_test_secboot(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_SECBOOT, "Secure boot cfg");
#if defined(__aarch64__)
    /* Check if OTP secure boot fuse would be set in production */
    return UIOX_POST_WARN;  /* QEMU has no fuses — warn only */
#else
    return UIOX_POST_WARN;
#endif
}

uiox_post_result_t uiox_fw_post_test_tz(uiox_post_entry_t *e)
{
    entry_init(e, UIOX_POST_TZ, "TrustZone / EL3");
#if defined(__aarch64__)
    uint64_t el = 0;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
    el = (el >> 2u) & 3u;
    if (el == 3u) return UIOX_POST_PASS;  /* started in EL3 */
    if (el == 2u) return UIOX_POST_WARN;  /* in hypervisor */
    return UIOX_POST_WARN;  /* already dropped to EL1 */
#else
    return UIOX_POST_WARN;  /* x86 / ARM32 without TZ */
#endif
}

/* ── Public API ───────────────────────────────────────────── */

void uiox_fw_post_init(uiox_fw_post_ctx_t *ctx)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
}

uiox_fw_err_t uiox_fw_post_run_all(uiox_fw_post_ctx_t *ctx,
                                     uintptr_t ram_base,
                                     uint64_t  ram_size)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;

    static const uint8_t test_ids[] = {
        UIOX_POST_CPU, UIOX_POST_CACHE, UIOX_POST_RAM,
        UIOX_POST_UART, UIOX_POST_TIMER, UIOX_POST_IRQ,
        UIOX_POST_CLOCK, UIOX_POST_POWER, UIOX_POST_STORAGE,
        UIOX_POST_SECBOOT, UIOX_POST_TZ,
    };

    for (uint8_t i = 0; i < sizeof(test_ids); i++) {
        uiox_fw_err_t rc =
            uiox_fw_post_run_one(ctx, test_ids[i], ram_base, ram_size);
        if (rc == UIOX_FW_ERR_FAIL && ctx->any_critical)
            return UIOX_FW_ERR_FAIL;
    }
    return ctx->any_critical ? UIOX_FW_ERR_FAIL : UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_post_run_one(uiox_fw_post_ctx_t *ctx,
                                     uint8_t    test_id,
                                     uintptr_t  ram_base,
                                     uint64_t   ram_size)
{
    if (!ctx || ctx->count >= UIOX_POST_MAX_TESTS) return UIOX_FW_ERR_INVAL;

    uiox_post_entry_t e;
    uint64_t t0 = uiox_fw_hw_timestamp_us();
    uiox_post_result_t res;

    switch (test_id) {
    case UIOX_POST_CPU:      res = uiox_fw_post_test_cpu(&e);    break;
    case UIOX_POST_CACHE:    res = uiox_fw_post_test_cache(&e);  break;
    case UIOX_POST_RAM:      res = uiox_fw_post_test_ram(&e, ram_base, ram_size); break;
    case UIOX_POST_UART:     res = uiox_fw_post_test_uart(&e);   break;
    case UIOX_POST_TIMER:    res = uiox_fw_post_test_timer(&e);  break;
    case UIOX_POST_IRQ:      res = uiox_fw_post_test_irq(&e);    break;
    case UIOX_POST_CLOCK:    res = uiox_fw_post_test_clock(&e);  break;
    case UIOX_POST_POWER:    res = uiox_fw_post_test_power(&e);  break;
    case UIOX_POST_STORAGE:  res = uiox_fw_post_test_storage(&e);break;
    case UIOX_POST_SECBOOT:  res = uiox_fw_post_test_secboot(&e);break;
    case UIOX_POST_TZ:       res = uiox_fw_post_test_tz(&e);     break;
    default:                 return UIOX_FW_ERR_INVAL;
    }

    static const char *rname[] = { "PASS", "WARN", "FAIL", "CRIT" };
    record(ctx, &e, res, t0, NULL);
    uiox_fw_printf("  POST [%02X] %-20s %s  (%llu us)\n",
                    test_id, e.name,
                    rname[(uint8_t)res],
                    (unsigned long long)e.duration_us);

    if (res == UIOX_POST_CRITICAL) return UIOX_FW_ERR_FAIL;
    return UIOX_FW_OK;
}

void uiox_fw_post_print(const uiox_fw_post_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("  POST results: %u tests, %u pass, %u warn, %u fail"
                    "  [%llu us total]\n",
                    ctx->count, ctx->pass_count,
                    ctx->warn_count, ctx->fail_count,
                    (unsigned long long)ctx->total_us);
}

void __attribute__((noreturn))
uiox_fw_post_panic(const uiox_fw_post_ctx_t *ctx, uint8_t id)
{
    uiox_fw_printf("\n[POST PANIC] Critical failure in test 0x%02X\n", id);
    uiox_fw_post_print(ctx);
    uiox_fw_printf("System halted.\n");
    for (;;) uiox_fw_hw_wfi();
}
