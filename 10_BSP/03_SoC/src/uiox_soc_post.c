/**
 * @file    uiox_soc_post.c
 * @brief   UIOX SoC — Power-On Self Test.
 *          Zero libc dependency — no string.h, stdio.h, or stdarg.h.
 * @date    2026-07-07
 */

 #include "../include/uiox_soc_post.h"
 #include "../include/uiox_soc_hw.h"
 
 /* =========================================================================
  * Bare-metal helpers (no libc)
  * ====================================================================== */
 
 static void soc_memset_post(void *dst, int val, size_t n)
 {
     uint8_t *d = (uint8_t *)dst;
     while (n--) *d++ = (uint8_t)val;
 }
 
 static void soc_strncpy_post(char *dst, const char *src, size_t n)
 {
     size_t i = 0u;
     while (i < n - 1u && src[i]) { dst[i] = src[i]; i++; }
     dst[i] = '\0';
 }
 
 /* Minimal unsigned decimal → string (writes into buf, returns ptr) */
 static char *soc_u64_to_dec(uint64_t v, char *buf, size_t bufsz)
 {
     if (bufsz == 0u) return buf;
     char tmp[24]; int n = 0;
     if (v == 0u) { buf[0] = '0'; buf[1] = '\0'; return buf; }
     while (v && n < 23) { tmp[n++] = (char)('0' + v % 10u); v /= 10u; }
     int j = 0;
     for (int i = n - 1; i >= 0 && (size_t)j < bufsz - 1u; i--)
         buf[j++] = tmp[i];
     buf[j] = '\0';
     return buf;
 }
 
 /* ── UART output (via uiox_soc_hw_ops vtable) ─────────────────────── */
 
 static void soc_puts_post(const char *s)
 {
     const uiox_soc_hw_ops_t *ops = uiox_soc_hw_ops();
     if (!ops || !ops->uart_putc) return;
     while (*s) {
         if (*s == '\n') ops->uart_putc('\r');
         ops->uart_putc(*s++);
     }
 }
 
 static void soc_print_u64_post(uint64_t v)
 {
     char buf[24];
     soc_puts_post(soc_u64_to_dec(v, buf, sizeof(buf)));
 }
 
 /* =========================================================================
  * Individual POST tests
  * ====================================================================== */
 static uiox_soc_post_result_t post_test_cpu(uiox_soc_post_report_t *r)
{
#if defined(__aarch64__)
    /* ARM64: read MIDR_EL1 */
    uiox_uint64_t midr;
    __asm__ volatile("mrs %0, midr_el1" : "=r"(midr));
    r->cpu_midr = (uiox_uint32_t)midr;
    if ((midr & 0xFF000000u) == 0u) {
        soc_strncpy_post(r->fail_msg, "CPU: MIDR_EL1 invalid",
                         sizeof(r->fail_msg));
        return UIOX_SOC_POST_FAIL_CPU;
    }

#elif defined(__arm__)
    /* ARM32: read MIDR via CP15 */
    uiox_uint32_t midr;
    __asm__ volatile("mrc p15,0,%0,c0,c0,0" : "=r"(midr));
    r->cpu_midr = midr;
    if ((midr & 0xFF000000u) == 0u) {
        soc_strncpy_post(r->fail_msg, "CPU: MIDR invalid",
                         sizeof(r->fail_msg));
        return UIOX_SOC_POST_FAIL_CPU;
    }

#elif defined(__x86_64__) || defined(__i386__)
    /* x86: check CPUID support */
    uiox_uint32_t eax = 0u, ebx = 0u, ecx = 0u, edx = 0u;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0u));
    r->cpu_midr = eax;
    if (eax == 0u) {
        soc_strncpy_post(r->fail_msg, "CPU: CPUID returned 0",
                         sizeof(r->fail_msg));
        return UIOX_SOC_POST_FAIL_CPU;
    }

#elif defined(__riscv)
    /*
     * RISC-V: read misa CSR.
     * misa == 0 is valid on some cores, so only fail if misa
     * returns the error value 0xFFFFFFFF (illegal CSR access).
     */
    uiox_uint64_t misa = 0u;
    __asm__ volatile("csrr %0, misa" : "=r"(misa));
    r->cpu_midr = (uiox_uint32_t)(misa & 0xFFFFFFFFu);
    if (misa == (uiox_uint64_t)-1ULL) {
        soc_strncpy_post(r->fail_msg, "CPU: misa CSR read failed",
                         sizeof(r->fail_msg));
        return UIOX_SOC_POST_FAIL_CPU;
    }

#else
    /* Unknown arch — skip CPU test, report pass */
    r->cpu_midr = 0u;
#endif

    return UIOX_SOC_POST_OK;
}
 
 static uiox_soc_post_result_t post_test_ram(
         const uiox_soc_post_cfg_t *cfg,
         uiox_soc_post_report_t    *r)
 {
     static const uint32_t patterns[] = {
         0xAAAAAAAAu, 0x55555555u, 0xFFFFFFFFu, 0x00000000u,
         0xDEADBEEFu
     };
 
     r->ram_tested_bytes = 0u;
 
     for (uint32_t ri = 0u; ri < cfg->ram_count; ri++) {
         uint64_t base = cfg->ram[ri].base;
         uint64_t test_sz = cfg->ram[ri].size < 0x10000ULL
                            ? cfg->ram[ri].size : 0x10000ULL;
 
         for (uint32_t pi = 0u;
              pi < sizeof(patterns) / sizeof(patterns[0]); pi++) {
             volatile uint32_t *p = (volatile uint32_t *)(uintptr_t)base;
             for (uint64_t w = 0u; w < test_sz / 4u; w++)
                 p[w] = patterns[pi];
             for (uint64_t w = 0u; w < test_sz / 4u; w++) {
                 if (p[w] != patterns[pi]) {
                     soc_strncpy_post(r->fail_msg,
                                      "RAM: pattern mismatch",
                                      sizeof(r->fail_msg));
                     return UIOX_SOC_POST_FAIL_RAM;
                 }
             }
         }
         r->ram_tested_bytes += test_sz;
     }
     return UIOX_SOC_POST_OK;
 }
 
 static uiox_soc_post_result_t post_test_rom_crc(
         const uiox_soc_post_cfg_t *cfg,
         uiox_soc_post_report_t    *r)
 {
     uint32_t crc = uiox_soc_crc32(
                         (const uint8_t *)(uintptr_t)cfg->rom_base,
                         cfg->rom_size);
     r->rom_crc32_actual = crc;
     if (crc != cfg->rom_crc32_expected) {
         soc_strncpy_post(r->fail_msg, "ROM: CRC32 mismatch",
                          sizeof(r->fail_msg));
         return UIOX_SOC_POST_FAIL_ROM_CRC;
     }
     return UIOX_SOC_POST_OK;
 }
 
 static uiox_soc_post_result_t post_test_timer(uiox_soc_post_report_t *r)
 {
 #if defined(__aarch64__)
     uiox_uint64_t freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     r->timer_freq_hz = freq;
     if (freq < 1000000u || freq > 1000000000u) {
         soc_strncpy_post(r->fail_msg, "TIMER: CNTFRQ out of range",
                          sizeof(r->fail_msg));
         return UIOX_SOC_POST_FAIL_TIMER;
     }
     uiox_uint64_t t0, t1;
     __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
     for (volatile int i = 0; i < 1000; i++) __asm__ volatile("nop");
     __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t1));
     if (t1 == t0) {
         soc_strncpy_post(r->fail_msg, "TIMER: counter not counting",
                          sizeof(r->fail_msg));
         return UIOX_SOC_POST_FAIL_TIMER;
     }
 
 #elif defined(__riscv)
     /*
      * RISC-V: read mtime from CLINT.
      * If the address is valid the counter should advance.
      */
     volatile uiox_uint64_t *mtime =
         (volatile uiox_uint64_t *)(uiox_uintptr_t)0x200BFF8u; /* CLINT */
     uiox_uint64_t t0 = *mtime;
     for (volatile int i = 0; i < 10000; i++) __asm__ volatile("nop");
     uiox_uint64_t t1 = *mtime;
     r->timer_freq_hz = 10000000u;   /* nominal 10 MHz */
     if (t1 == t0) {
         soc_strncpy_post(r->fail_msg, "TIMER: mtime not advancing",
                          sizeof(r->fail_msg));
         return UIOX_SOC_POST_FAIL_TIMER;
     }
 
 #elif defined(__arm__)
     /* ARM32: use CP15 generic timer frequency if available */
     uiox_uint32_t freq = 0u;
     __asm__ volatile("mrc p15,0,%0,c14,c0,0" : "=r"(freq));
     r->timer_freq_hz = (uiox_uint64_t)freq;
     /* Cannot easily check counter advance from SVC mode; report pass */
 
 #else
     /* x86 / unknown: skip timer test */
     r->timer_freq_hz = 0u;
 #endif
 
     return UIOX_SOC_POST_OK;
 }
 
 
 static uiox_soc_post_result_t post_test_gic(
         const uiox_soc_post_cfg_t *cfg,
         uiox_soc_post_report_t    *r)
 {
     if (cfg->gic_dist_base == 0u) return UIOX_SOC_POST_OK;
     uint32_t typer = soc_mmio_read32(
                          (uintptr_t)(cfg->gic_dist_base + 0x04u));
     r->gic_typer = typer;
     if ((typer & 0x1Fu) == 0u) {
         soc_strncpy_post(r->fail_msg, "GIC: GICD_TYPER.ITLines = 0",
                          sizeof(r->fail_msg));
         return UIOX_SOC_POST_FAIL_GIC;
     }
     return UIOX_SOC_POST_OK;
 }
 
 static uiox_soc_post_result_t post_test_stack(
         const uiox_soc_post_cfg_t *cfg)
 {
     if (cfg->stack_base == 0u) return UIOX_SOC_POST_OK;
     volatile uint32_t *sentinel =
         (volatile uint32_t *)(uintptr_t)cfg->stack_base;
     if (*sentinel != cfg->stack_sentinel) {
         return UIOX_SOC_POST_FAIL_STACK;
     }
     return UIOX_SOC_POST_OK;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 void uiox_soc_post_stack_mark(uintptr_t stack_base, uint32_t sentinel)
 {
     if (stack_base)
         *((volatile uint32_t *)stack_base) = sentinel;
 }
 
 uiox_soc_post_result_t uiox_soc_post_run(const uiox_soc_post_cfg_t *cfg,
                                            uiox_soc_post_report_t   *rpt)
 {
     uiox_soc_post_report_t local;
     uiox_soc_post_report_t *r = rpt ? rpt : &local;
     soc_memset_post(r, 0, sizeof(*r));
 
 #define RUN_TEST(id, expr) \
     do { \
         if (cfg->test_mask & (id)) { \
             uiox_soc_post_result_t _rc = (expr); \
             if (_rc != UIOX_SOC_POST_OK) { \
                 r->failed_tests |= (id); \
                 if (r->overall == UIOX_SOC_POST_OK) r->overall = _rc; \
             } else { \
                 r->passed_tests |= (id); \
             } \
         } \
     } while (0)
 
     RUN_TEST(UIOX_SOC_POST_TEST_CPU,     post_test_cpu(r));
     RUN_TEST(UIOX_SOC_POST_TEST_RAM,     post_test_ram(cfg, r));
     RUN_TEST(UIOX_SOC_POST_TEST_ROM_CRC, post_test_rom_crc(cfg, r));
     RUN_TEST(UIOX_SOC_POST_TEST_TIMER,   post_test_timer(r));
     RUN_TEST(UIOX_SOC_POST_TEST_GIC,     post_test_gic(cfg, r));
     RUN_TEST(UIOX_SOC_POST_TEST_STACK,   post_test_stack(cfg));
 
 #undef RUN_TEST
 
     return r->overall;
 }
 
 void uiox_soc_post_print(const uiox_soc_post_report_t *r)
 {
     soc_puts_post("[SOC] POST report:\n");
     soc_puts_post("  overall      : ");
     soc_puts_post(r->overall == UIOX_SOC_POST_OK ? "PASS" : "FAIL");
     soc_puts_post("\n  passed_tests : 0x");
     /* hex print for bitmask */
     char buf[12];
     uint32_t v = r->passed_tests;
     int i = 7;
     buf[8] = '\n'; buf[9] = '\0';
     static const char h[] = "0123456789abcdef";
     while (i >= 0) { buf[i--] = h[v & 0xFu]; v >>= 4; }
     soc_puts_post(buf);
     soc_puts_post("  failed_tests : 0x");
     v = r->failed_tests; i = 7;
     while (i >= 0) { buf[i--] = h[v & 0xFu]; v >>= 4; }
     soc_puts_post(buf);
     if (r->overall != UIOX_SOC_POST_OK) {
         soc_puts_post("  fail_msg     : ");
         soc_puts_post(r->fail_msg);
         soc_puts_post("\n");
     }
     soc_puts_post("  cpu_midr     : 0x");
     v = r->cpu_midr; i = 7;
     while (i >= 0) { buf[i--] = h[v & 0xFu]; v >>= 4; }
     soc_puts_post(buf);
     soc_puts_post("  timer_freq   : ");
     soc_print_u64_post(r->timer_freq_hz);
     soc_puts_post(" Hz\n");
     soc_puts_post("  ram_tested   : ");
     soc_print_u64_post(r->ram_tested_bytes);
     soc_puts_post(" B\n");
 }
 
 /* =========================================================================
  * CRC32 (IEEE 802.3 polynomial)
  * ====================================================================== */
 
 uint32_t uiox_soc_crc32(const uint8_t *data, size_t len)
 {
     uint32_t crc = 0xFFFFFFFFu;
     while (len--) {
         crc ^= *data++;
         for (int b = 0; b < 8; b++) {
             if (crc & 1u) crc = (crc >> 1u) ^ 0xEDB88320u;
             else          crc >>= 1u;
         }
     }
     return crc ^ 0xFFFFFFFFu;
 }
 