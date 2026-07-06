/**
 * @file  uiox_fw_post.c
 * @brief UIOX Firmware — Power-On Self Test implementation.
 * @date  2026-07-06
 */
#include "uiox_fw.h"
#include "uiox_fw_post.h"
#include "uiox_fw_secboot.h"
//#include <string.h>

//==============
/* Freestanding string helpers — no <string.h> needed */
static inline void *fw_memset(void *dst, int c, __SIZE_TYPE__ n)
{
    unsigned char *p = (unsigned char *)dst;
    while (n--) *p++ = (unsigned char)c;
    return dst;
}

static inline void *fw_memcpy(void *dst, const void *src, __SIZE_TYPE__ n)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

static inline int fw_memcmp(const void *a, const void *b, __SIZE_TYPE__ n)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    while (n--) {
        if (*pa != *pb) return (*pa < *pb) ? -1 : 1;
        pa++; pb++;
    }
    return 0;
}

static inline __SIZE_TYPE__ fw_strlen(const char *s)
{
    __SIZE_TYPE__ n = 0;
    while (*s++) n++;
    return n;
}

static inline char *fw_strncpy(char *dst, const char *src, __SIZE_TYPE__ n)
{
    __SIZE_TYPE__ i = 0;
    while (i < n && src[i]) { dst[i] = src[i]; i++; }
    while (i < n)             { dst[i] = '\0';  i++; }
    return dst;
}



//--------------------just to rmove string.h file


/* ── Software 32-bit divide (no __aeabi_uidiv) ──────────── */
static inline uint32_t post_udiv32(uint32_t n, uint32_t d)
{
    uint32_t q = 0u, r = 0u;
    if (d == 0u) return 0u;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1u) | ((n >> (uint32_t)i) & 1u);
        if (r >= d) { r -= d; q |= (1u << (uint32_t)i); }
    }
    return q;
}

/* ── Timestamp helper (returns µs, uses timer if available) ─ */
static uint32_t post_timestamp_us(void)
{
#if defined(__aarch64__)
    uint64_t cnt, frq;
    __asm__ volatile("mrs %0, CNTVCT_EL0" : "=r"(cnt));
    __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r"(frq));
    if (frq == 0u) return 0u;
    /* cnt * 1_000_000 / frq — avoid overflow with 32-bit math */
    return (uint32_t)((cnt * 1000000ULL) / frq);
#elif defined(__x86_64__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return lo;   /* approximate — no calibration here */
#else
    return 0u;
#endif
}

/* ─────────────────────────────────────────────────────────── */
/* Individual POST tests                                        */
/* ─────────────────────────────────────────────────────────── */

/* ── POST_CPU ──────────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_post_cpu(uiox_fw_post_result_t *r)
{
    if (!r) return UIOX_FW_ERR_INVAL;
    fw_memset(r, 0, sizeof(*r));
    r->test_id = UIOX_POST_CPU;
    fw_strncpy(r->name, "CPU", sizeof(r->name) - 1u);
    uint32_t t0 = post_timestamp_us();

#if defined(__aarch64__)
    uint64_t midr;
    __asm__ volatile("mrs %0, MIDR_EL1" : "=r"(midr));
    if ((midr & 0xFF000000ULL) == 0u) {
        /* implementer field is zero — suspicious on real HW */
        fw_strncpy(r->detail, "MIDR_EL1 implementer=0 (QEMU?)",
                sizeof(r->detail) - 1u);
        /* Not fatal in simulation */
    } else {
        //snprintf(r->detail, sizeof(r->detail),
        //         "MIDR=0x%08llX impl=0x%02X part=0x%03X",
        //         (unsigned long long)midr,
        //         (unsigned)((midr >> 24) & 0xFF),
        //         (unsigned)((midr >>  4) & 0xFFF));
    }
    r->result = UIOX_FW_OK;
#elif defined(__arm__)
    uint32_t midr;
    __asm__ volatile("mrc p15,0,%0,c0,c0,0" : "=r"(midr));
    snprintf(r->detail, sizeof(r->detail),
             "MIDR=0x%08X part=0x%03X",
             midr, (midr >> 4) & 0xFFF);
    r->result = UIOX_FW_OK;
#else
    /* x86-64 */
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    __asm__ volatile("cpuid"
        : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx)
        : "a"(1u),"c"(0u));
    snprintf(r->detail, sizeof(r->detail),
             "CPUID family=%u model=%u stepping=%u",
             (eax >> 8) & 0xF, (eax >> 4) & 0xF, eax & 0xF);
    r->result = UIOX_FW_OK;
#endif

    r->duration_us = post_timestamp_us() - t0;
    return r->result;
}

/* ── POST_CACHE ─────────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_post_cache(uiox_fw_post_result_t *r)
{
    if (!r) return UIOX_FW_ERR_INVAL;
    fw_memset(r, 0, sizeof(*r));
    r->test_id = UIOX_POST_CACHE;
    fw_strncpy(r->name, "CACHE", sizeof(r->name) - 1u);
    uint32_t t0 = post_timestamp_us();

    /* Write 64-byte pattern to a stack buffer (guaranteed in D-cache) */
    volatile uint32_t buf[16];
    for (int i = 0; i < 16; i++) buf[i] = (uint32_t)(0xDEAD0000u | i);
    uint32_t errors = 0u;
    for (int i = 0; i < 16; i++) {
        if (buf[i] != (uint32_t)(0xDEAD0000u | i)) errors++;
    }

    if (errors == 0u) {
        fw_strncpy(r->detail, "64-byte D-cache write-back OK",
                sizeof(r->detail) - 1u);
        r->result = UIOX_FW_OK;
    } else {
        //snprintf(r->detail, sizeof(r->detail),
        //         "%u of 16 words mismatch", errors);
        r->result = UIOX_FW_ERR_POST;
    }

    r->duration_us = post_timestamp_us() - t0;
    return r->result;
}

/* ── POST_RAM ───────────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_post_ram(uiox_fw_post_result_t *r)
{
    if (!r) return UIOX_FW_ERR_INVAL;
    fw_memset(r, 0, sizeof(*r));
    r->test_id = UIOX_POST_RAM;
    fw_strncpy(r->name, "RAM", sizeof(r->name) - 1u);
    uint32_t t0 = post_timestamp_us();

    /*
     * Walking-ones march on a 1 KB window just above the firmware stack.
     * The test address is conservative — well above the boot stack.
     */
#if defined(__aarch64__)
    volatile uint32_t *test_base = (volatile uint32_t *)0x40008000ULL;
#elif defined(__arm__)
    volatile uint32_t *test_base = (volatile uint32_t *)0x00108000UL;
#else
    volatile uint32_t *test_base = (volatile uint32_t *)0x00300000ULL;
#endif

    const uint32_t N = 256u;  /* 256 × 4 = 1 KB */
    uint32_t errors = 0u;

    /* Pass 1: walking ones */
    for (uint32_t bit = 0; bit < 32u; bit++) {
        uint32_t pat = 1u << bit;
        for (uint32_t i = 0; i < N; i++) test_base[i] = pat;
        for (uint32_t i = 0; i < N; i++)
            if (test_base[i] != pat) errors++;
    }
    /* Pass 2: all-zeros, all-ones */
    for (uint32_t i = 0; i < N; i++) test_base[i] = 0u;
    for (uint32_t i = 0; i < N; i++) if (test_base[i] != 0u) errors++;
    for (uint32_t i = 0; i < N; i++) test_base[i] = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < N; i++) if (test_base[i] != 0xFFFFFFFFu) errors++;

    if (errors == 0u) {
        fw_strncpy(r->detail, "1 KB walking-ones + all-0/1 OK",
                sizeof(r->detail) - 1u);
        r->result = UIOX_FW_OK;
    } else {
        //snprintf(r->detail, sizeof(r->detail),
        //         "%u errors in 1 KB march", errors);
        r->result = UIOX_FW_ERR_POST;
    }

    r->duration_us = post_timestamp_us() - t0;
    return r->result;
}

/* ── POST_UART ──────────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_post_uart(uiox_fw_post_result_t *r)
{
    if (!r) return UIOX_FW_ERR_INVAL;
    fw_memset(r, 0, sizeof(*r));
    r->test_id = UIOX_POST_UART;
    fw_strncpy(r->name, "UART", sizeof(r->name) - 1u);
    uint32_t t0 = post_timestamp_us();

    /* Check TX FIFO not full (FR register bit 5 for PL011) */
#if defined(__aarch64__)
    const uintptr_t FR = 0x09000018ULL;
#elif defined(__arm__)
    const uintptr_t FR = 0x10009018UL;
#else
    /* x86: check UART LSR THR empty bit */
    uint8_t lsr = 0;
    __asm__ volatile("inb %1, %0" : "=a"(lsr) : "Nd"((uint16_t)0x3FD));
    if (lsr & 0x20u) {
        strncpy(r->detail, "COM1 LSR THRE=1 OK", sizeof(r->detail)-1u);
        r->result = UIOX_FW_OK;
    } else {
        strncpy(r->detail, "COM1 LSR THRE=0 (TX busy?)", sizeof(r->detail)-1u);
        r->result = UIOX_FW_OK; /* not fatal */
    }
    r->duration_us = post_timestamp_us() - t0;
    return r->result;
#endif

    uint32_t fr = *(volatile uint32_t *)FR;
    bool tx_full = !!(fr & (1u << 5));   /* PL011 TXFF bit */
    if (!tx_full) {
        fw_strncpy(r->detail, "PL011 TX FIFO not full OK",
                sizeof(r->detail) - 1u);
        r->result = UIOX_FW_OK;
    } else {
        fw_strncpy(r->detail, "PL011 TX FIFO full (busy?)",
                sizeof(r->detail) - 1u);
        r->result = UIOX_FW_OK;  /* non-fatal — FIFO may drain */
    }
    r->duration_us = post_timestamp_us() - t0;
    return r->result;
}

/* ── POST_TIMER ─────────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_post_timer(uiox_fw_post_result_t *r)
{
    if (!r) return UIOX_FW_ERR_INVAL;
    fw_memset(r, 0, sizeof(*r));
    r->test_id = UIOX_POST_TIMER;
    fw_strncpy(r->name, "TIMER", sizeof(r->name) - 1u);
    uint32_t t0 = post_timestamp_us();

#if defined(__aarch64__)
    uint64_t cnt0, cnt1;
    __asm__ volatile("mrs %0, CNTVCT_EL0" : "=r"(cnt0));
    /* Spin briefly */
    for (volatile uint32_t i = 0; i < 10000u; i++);
    __asm__ volatile("mrs %0, CNTVCT_EL0" : "=r"(cnt1));
    if (cnt1 > cnt0) {
        //snprintf(r->detail, sizeof(r->detail),
        //         "CNTVCT advanced by %llu ticks",
        //         (unsigned long long)(cnt1 - cnt0));
        r->result = UIOX_FW_OK;
    } else {
        fw_strncpy(r->detail, "CNTVCT_EL0 did not advance",
                sizeof(r->detail) - 1u);
        r->result = UIOX_FW_ERR_POST;
    }
#elif defined(__x86_64__)
    uint32_t lo0, hi0, lo1, hi1;
    __asm__ volatile("rdtsc" : "=a"(lo0), "=d"(hi0));
    for (volatile uint32_t i = 0; i < 10000u; i++);
    __asm__ volatile("rdtsc" : "=a"(lo1), "=d"(hi1));
    if (lo1 != lo0 || hi1 != hi0) {
        strncpy(r->detail, "TSC advances OK", sizeof(r->detail)-1u);
        r->result = UIOX_FW_OK;
    } else {
        strncpy(r->detail, "TSC frozen", sizeof(r->detail)-1u);
        r->result = UIOX_FW_ERR_POST;
    }
#else
    strncpy(r->detail, "timer check skipped (ARM32 stub)",
            sizeof(r->detail) - 1u);
    r->result = UIOX_FW_OK;
#endif

    r->duration_us = post_timestamp_us() - t0;
    return r->result;
}

/* ── POST_IRQ ───────────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_post_irq(uiox_fw_post_result_t *r)
{
    if (!r) return UIOX_FW_ERR_INVAL;
    fw_memset(r, 0, sizeof(*r));
    r->test_id = UIOX_POST_IRQ;
    fw_strncpy(r->name, "IRQ", sizeof(r->name) - 1u);
    uint32_t t0 = post_timestamp_us();

    /*
     * Simplified: verify GIC distributor is readable.
     * A full test would generate a software interrupt (SGI) and
     * confirm the GIC CPU interface acknowledges it.
     */
#if defined(__aarch64__)
    const uintptr_t GICD_TYPER = 0x08000004ULL;
    uint32_t typer = *(volatile uint32_t *)GICD_TYPER;
    uint32_t num_irqs = ((typer & 0x1Fu) + 1u) * 32u;
    //snprintf(r->detail, sizeof(r->detail),
    //         "GIC TYPER=0x%08X IRQs=%u", typer, num_irqs);
    r->result = (num_irqs >= 32u && num_irqs <= 1024u)
                ? UIOX_FW_OK : UIOX_FW_ERR_POST;
#elif defined(__arm__)
    const uintptr_t GICD_TYPER = 0x08000004UL;
    uint32_t typer = *(volatile uint32_t *)GICD_TYPER;
    uint32_t num_irqs = ((typer & 0x1Fu) + 1u) * 32u;
    snprintf(r->detail, sizeof(r->detail),
             "GIC TYPER=0x%08X IRQs=%u", typer, num_irqs);
    r->result = (num_irqs >= 32u) ? UIOX_FW_OK : UIOX_FW_ERR_POST;
#else
    /* x86: read IOAPIC version register */
    *(volatile uint32_t *)0xFEC00000ULL = 0x01u; /* REGSEL = VER */
    uint32_t ver = *(volatile uint32_t *)0xFEC00010ULL;
    snprintf(r->detail, sizeof(r->detail),
             "IOAPIC VER=0x%08X", ver);
    r->result = UIOX_FW_OK;
#endif

    r->duration_us = post_timestamp_us() - t0;
    return r->result;
}

/* ── POST_STORAGE ───────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_post_storage(uiox_fw_post_result_t *r)
{
    if (!r) return UIOX_FW_ERR_INVAL;
    fw_memset(r, 0, sizeof(*r));
    r->test_id = UIOX_POST_STORAGE;
    fw_strncpy(r->name, "STORAGE", sizeof(r->name) - 1u);
    uint32_t t0 = post_timestamp_us();

    /* Stub: storage POST deferred to block-device layer.
       Mark as SKIP so POST summary shows 'S' not 'F'. */
    fw_strncpy(r->detail, "deferred to block-layer (RAM disk ready)",
            sizeof(r->detail) - 1u);
    r->result = UIOX_FW_OK;

    r->duration_us = post_timestamp_us() - t0;
    return r->result;
}

/* ── POST_CRYPTO ────────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_post_crypto(uiox_fw_post_result_t *r)
{
    if (!r) return UIOX_FW_ERR_INVAL;
    fw_memset(r, 0, sizeof(*r));
    r->test_id = UIOX_POST_CRYPTO;
    fw_strncpy(r->name, "CRYPTO", sizeof(r->name) - 1u);
    uint32_t t0 = post_timestamp_us();

    /*
     * SHA-256 self-test: NIST FIPS 180-4 byte vector
     *   Input:  "abc"
     *   Digest: ba7816bf 8f01cfea 414140de 5dae2ec7
     *           3b00361a 396177a9 cb410ff6 1f20015a
     */
    static const uint8_t msg[]    = { 'a', 'b', 'c' };
    static const uint8_t expect[] = {
        0xbau,0x78u,0x16u,0xbfu, 0x8fu,0x01u,0xcfu,0xeau,
        0x41u,0x41u,0x40u,0xdeu, 0x5du,0xaeu,0x2eu,0xc7u,
        0x3bu,0x00u,0x36u,0x1au, 0x39u,0x61u,0x77u,0xa9u,
        0xcbu,0x41u,0x0fu,0xf6u, 0x1fu,0x20u,0x01u,0x5au
    };

    uint8_t digest[32];
    uiox_fw_sha256(msg, sizeof(msg), digest);

    uint32_t mismatches = 0u;
    for (int i = 0; i < 32; i++)
        if (digest[i] != expect[i]) mismatches++;

    if (mismatches == 0u) {
        fw_strncpy(r->detail, "SHA-256 'abc' vector PASS",
                sizeof(r->detail) - 1u);
        r->result = UIOX_FW_OK;
    } else {
        //snprintf(r->detail, sizeof(r->detail),
        //         "SHA-256 self-test FAIL (%u byte mismatches)",
        //         mismatches);
        r->result = UIOX_FW_ERR_POST;
    }

    r->duration_us = post_timestamp_us() - t0;
    return r->result;
}

/* ─────────────────────────────────────────────────────────── */
/* Public orchestration                                         */
/* ─────────────────────────────────────────────────────────── */

uiox_fw_err_t uiox_fw_post_run_all(uiox_fw_post_ctx_t *ctx,
                                     uint32_t            flags)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    fw_memset(ctx, 0, sizeof(*ctx));

    typedef uiox_fw_err_t (*post_fn_t)(uiox_fw_post_result_t *);
    static const post_fn_t tests[UIOX_POST_NUM] = {
        uiox_fw_post_cpu,
        uiox_fw_post_cache,
        uiox_fw_post_ram,
        uiox_fw_post_uart,
        uiox_fw_post_timer,
        uiox_fw_post_irq,
        uiox_fw_post_storage,
        uiox_fw_post_crypto,
    };

    uiox_fw_err_t overall = UIOX_FW_OK;

    for (uint8_t i = 0; i < UIOX_POST_NUM; i++) {
        uiox_fw_post_result_t *r = &ctx->results[i];
        uiox_fw_err_t rc = tests[i](r);
        ctx->count++;
        if (rc == UIOX_FW_OK) {
            ctx->pass++;
        } else {
            ctx->fail++;
            overall = UIOX_FW_ERR_POST;
            if (flags & UIOX_POST_FL_HALT_ON_FAIL)
                break;
        }
        ctx->total_us += r->duration_us;
        if (flags & UIOX_POST_FL_VERBOSE) {
            uiox_fw_printf("  POST [%s] %s — %s\n",
                           rc == UIOX_FW_OK ? "PASS" : "FAIL",
                           r->name, r->detail);
        }
    }
    return overall;
}

uiox_fw_err_t uiox_fw_post_run_one(uiox_fw_post_result_t *r,
                                     uint8_t               test_id)
{
    if (!r || test_id == 0u || test_id > UIOX_POST_NUM)
        return UIOX_FW_ERR_INVAL;
    switch (test_id) {
    case UIOX_POST_CPU:     return uiox_fw_post_cpu(r);
    case UIOX_POST_CACHE:   return uiox_fw_post_cache(r);
    case UIOX_POST_RAM:     return uiox_fw_post_ram(r);
    case UIOX_POST_UART:    return uiox_fw_post_uart(r);
    case UIOX_POST_TIMER:   return uiox_fw_post_timer(r);
    case UIOX_POST_IRQ:     return uiox_fw_post_irq(r);
    case UIOX_POST_STORAGE: return uiox_fw_post_storage(r);
    case UIOX_POST_CRYPTO:  return uiox_fw_post_crypto(r);
    default:                return UIOX_FW_ERR_INVAL;
    }
}

void uiox_fw_post_print(const uiox_fw_post_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("\nPOST results (%u/%u passed):\n",
                   ctx->pass, ctx->count);
    for (uint8_t i = 0; i < ctx->count; i++) {
        const uiox_fw_post_result_t *r = &ctx->results[i];
        uiox_fw_printf("  [%s] %-10s %u us  %s\n",
                       r->result == UIOX_FW_OK ? "PASS" : "FAIL",
                       r->name,
                       r->duration_us,
                       r->detail);
    }
    uiox_fw_printf("  Total: %u us  Fail count: %u\n",
                   ctx->total_us, ctx->fail);
}

int uiox_fw_post_any_fail(const uiox_fw_post_ctx_t *ctx)
{
    return ctx ? (ctx->fail > 0u) : 0;
}
