/**
 * @file  uiox_fw_post.c
 * @brief UIOX Firmware — Power-On Self Test.
 *        Zero libc dependency — no string.h, stdio.h, or stdarg.h.
 * @date  2026-07-07
 */

 #include "../include/uiox_fw_post.h"
 #include "../include/uiox_fw_hw.h"
 
 /* =========================================================================
  * Bare-metal helpers (no libc)
  * ====================================================================== */
 
 static void fw_memset(void *dst, int val, size_t n)
 {
     uint8_t *d = (uint8_t *)dst;
     while (n--) *d++ = (uint8_t)val;
 }
 
 static void fw_strncpy(char *dst, const char *src, size_t n)
 {
     size_t i = 0u;
     while (i < n - 1u && src[i]) { dst[i] = src[i]; i++; }
     dst[i] = '\0';
 }
 
 /* Minimal unsigned decimal → string (writes into buf, returns ptr) */
 static char *fw_u64_to_dec(uint64_t v, char *buf, size_t bufsz)
 {
     if (bufsz == 0u) return buf;
     char tmp[24]; int n = 0;
     if (v == 0u) { buf[0]='0'; buf[1]='\0'; return buf; }
     while (v && n < 23) { tmp[n++] = (char)('0' + v % 10u); v /= 10u; }
     int j = 0;
     for (int i = n-1; i >= 0 && (size_t)j < bufsz-1u; i--) buf[j++] = tmp[i];
     buf[j] = '\0';
     return buf;
 }
 
 /* Minimal hex → string (8 hex digits) */
 static char *fw_u32_to_hex(uint32_t v, char *buf)
 {
     static const char h[] = "0123456789abcdef";
     for (int i = 7; i >= 0; i--) { buf[i] = h[v & 0xFu]; v >>= 4; }
     buf[8] = '\0';
     return buf;
 }
 
 /* UART output using registered HAL ops */
 static void fw_puts(const char *s)
 {
     /* uiox_fw_hw_ops() is declared in uiox_fw_hw.h (already included) */
     const uiox_fw_hw_ops_t *ops = uiox_fw_hw_ops();
     if (!ops || !ops->uart_putc) return;
     while (*s) {
         if (*s == '\n') ops->uart_putc('\r');
         ops->uart_putc(*s++);
     }
 }
 
 /* Print label + decimal number */
 static void fw_print_u64(const char *label, uint64_t v)
 {
     char buf[24];
     fw_puts(label);
     fw_u64_to_dec(v, buf, sizeof(buf));
     fw_puts(buf);
     fw_puts("\n");
 }
 
 /* Print label + 0x + hex32 */
 static void fw_print_hex32(const char *label, uint32_t v)
 {
     char buf[12];
     fw_puts(label);
     fw_puts("0x");
     fw_u32_to_hex(v, buf);
     fw_puts(buf);
     fw_puts("\n");
 }
 
 /* =========================================================================
  * CRC32 (IEEE 802.3 / 0xEDB88320)
  * ====================================================================== */
 
 static uint32_t s_crc_tbl[256];
 static bool     s_crc_ready = false;
 
 static void crc32_build_table(void)
 {
     if (s_crc_ready) return;
     for (uint32_t i = 0u; i < 256u; i++) {
         uint32_t c = i;
         for (int j = 0; j < 8; j++)
             c = (c & 1u) ? (0xEDB88320u ^ (c >> 1u)) : (c >> 1u);
         s_crc_tbl[i] = c;
     }
     s_crc_ready = true;
 }
 
 uint32_t uiox_fw_crc32(const uint8_t *data, size_t len)
 {
     crc32_build_table();
     uint32_t crc = 0xFFFFFFFFu;
     for (size_t i = 0u; i < len; i++)
         crc = s_crc_tbl[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8u);
     return crc ^ 0xFFFFFFFFu;
 }
 
 /* =========================================================================
  * Stack sentinel
  * ====================================================================== */
 
 void uiox_fw_post_stack_mark(uintptr_t stack_base, uint32_t sentinel)
 {
     *((volatile uint32_t *)stack_base) = sentinel;
 }
 
 /* =========================================================================
  * POST tests
  * ====================================================================== */
 
 static uiox_post_result_t test_cpu(uiox_post_report_t *r)
 {
 #if defined(__aarch64__)
     uint64_t midr;
     __asm__ volatile("mrs %0, midr_el1" : "=r"(midr));
     r->cpu_midr = (uint32_t)midr;
     if (midr == 0u || midr == 0xFFFFFFFFFFFFFFFFULL) {
         fw_strncpy(r->fail_msg, "MIDR_EL1 invalid", sizeof(r->fail_msg));
         return UIOX_POST_FAIL_CPU;
     }
     uint64_t mpidr;
     __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
     if (!(mpidr & (1ULL << 31u))) {
         fw_strncpy(r->fail_msg, "MPIDR_EL1 RES1 not set", sizeof(r->fail_msg));
         return UIOX_POST_FAIL_CPU;
     }
     uint64_t freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     r->timer_freq_hz = freq;
     if (freq < 1000000ULL || freq > 200000000ULL) {
         fw_strncpy(r->fail_msg, "CNTFRQ_EL0 out of range", sizeof(r->fail_msg));
         return UIOX_POST_FAIL_CPU;
     }
 #elif defined(__arm__)
     uint32_t midr;
     __asm__ volatile("mrc p15,0,%0,c0,c0,0" : "=r"(midr));
     r->cpu_midr = midr;
     if (midr == 0u || midr == 0xFFFFFFFFu) {
         fw_strncpy(r->fail_msg, "MIDR invalid (ARM32)", sizeof(r->fail_msg));
         return UIOX_POST_FAIL_CPU;
     }
 #elif defined(__x86_64__)
     uint32_t eax, ebx, ecx, edx;
     __asm__ volatile("cpuid"
                      : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx)
                      : "0"(0u));
     r->cpu_midr = eax;
     if (eax == 0u) {
         fw_strncpy(r->fail_msg, "CPUID leaf 0 returned 0", sizeof(r->fail_msg));
         return UIOX_POST_FAIL_CPU;
     }
 #endif
     return UIOX_POST_OK;
 }
 
 static uiox_post_result_t test_ram(const uiox_post_cfg_t *cfg,
                                      uiox_post_report_t *r)
 {
     static const uint32_t patterns[] = {
         0xAAAAAAAAu, 0x55555555u, 0xFFFFFFFFu, 0x00000000u,
         0xDEADBEEFu, 0xCAFEBABEu
     };
 
     for (uint32_t ri = 0u; ri < cfg->ram_count; ri++) {
         uint64_t base      = cfg->ram[ri].base;
         uint64_t size      = cfg->ram[ri].size;
         uint64_t test_size = (size > 65536u) ? 65536u : size;
         volatile uint32_t *mem = (volatile uint32_t *)(uintptr_t)base;
         uint32_t words = (uint32_t)(test_size / sizeof(uint32_t));
 
         for (uint32_t pi = 0u;
              pi < sizeof(patterns)/sizeof(patterns[0]); pi++) {
             uint32_t pat = patterns[pi];
             for (uint32_t i = 0u; i < words; i++) mem[i] = pat;
             __asm__ volatile("" ::: "memory");
             for (uint32_t i = 0u; i < words; i++) {
                 if (mem[i] != pat) {
                     fw_strncpy(r->fail_msg,
                                "RAM walk pattern mismatch",
                                sizeof(r->fail_msg));
                     return UIOX_POST_FAIL_RAM;
                 }
             }
             r->ram_tested_bytes += test_size;
         }
     }
     return UIOX_POST_OK;
 }
 
 static uiox_post_result_t test_rom_crc(const uiox_post_cfg_t *cfg,
                                          uiox_post_report_t *r)
 {
     if (cfg->rom_size == 0u) return UIOX_POST_OK;
     uint32_t actual = uiox_fw_crc32(
         (const uint8_t *)(uintptr_t)cfg->rom_base,
         cfg->rom_size);
     r->rom_crc32_actual = actual;
     if (actual != cfg->rom_crc32_expected) {
         fw_strncpy(r->fail_msg, "ROM CRC32 mismatch", sizeof(r->fail_msg));
         return UIOX_POST_FAIL_ROM_CRC;
     }
     return UIOX_POST_OK;
 }
 
 static uiox_post_result_t test_timer(uiox_post_report_t *r)
 {
     uint64_t t0, t1;
 #if defined(__aarch64__)
     __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(t0) :: "memory");
     volatile uint32_t n = 10000u; while (n--) ;
     __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(t1) :: "memory");
 #elif defined(__arm__)
     t0 = (uint64_t)(*((volatile uint32_t *)0x101E2004u));
     volatile uint32_t n = 10000u; while (n--) ;
     t1 = (uint64_t)(*((volatile uint32_t *)0x101E2004u));
 #else
     uint32_t lo, hi;
     __asm__ volatile("rdtsc" : "=a"(lo),"=d"(hi));
     t0 = ((uint64_t)hi << 32u) | lo;
     volatile uint32_t n = 10000u; while (n--) ;
     __asm__ volatile("rdtsc" : "=a"(lo),"=d"(hi));
     t1 = ((uint64_t)hi << 32u) | lo;
 #endif
     if (t1 == t0) {
         fw_strncpy(r->fail_msg, "Timer not advancing", sizeof(r->fail_msg));
         return UIOX_POST_FAIL_TIMER;
     }
     if (r->timer_freq_hz == 0u)
         r->timer_freq_hz = t1 - t0;
     return UIOX_POST_OK;
 }
 
 static uiox_post_result_t test_gic(const uiox_post_cfg_t *cfg,
                                      uiox_post_report_t *r)
 {
     if (cfg->gic_dist_base == 0u) return UIOX_POST_OK;
     volatile uint32_t *gicd_typer =
         (volatile uint32_t *)(cfg->gic_dist_base + 0x004u);
     uint32_t typer = *gicd_typer;
     r->gic_typer = typer;
     if ((typer & 0x1Fu) == 0u) {
         fw_strncpy(r->fail_msg, "GICD_TYPER ITLines==0", sizeof(r->fail_msg));
         return UIOX_POST_FAIL_GIC;
     }
     return UIOX_POST_OK;
 }
 
 static uiox_post_result_t test_stack(const uiox_post_cfg_t *cfg)
 {
     if (cfg->stack_base == 0u) return UIOX_POST_OK;
     uint32_t val = *((volatile uint32_t *)cfg->stack_base);
     if (val != cfg->stack_sentinel) {
         return UIOX_POST_FAIL_STACK;
     }
     return UIOX_POST_OK;
 }
 
 /* =========================================================================
  * uiox_fw_post_run
  * ====================================================================== */
 
 uiox_post_result_t uiox_fw_post_run(const uiox_post_cfg_t *cfg,
                                       uiox_post_report_t *report)
 {
     if (!cfg) return UIOX_POST_FAIL_GENERIC;
     uiox_post_report_t local;
     uiox_post_report_t *r = report ? report : &local;
     fw_memset(r, 0, sizeof(*r));
     r->overall = UIOX_POST_OK;
 
 #define RUN_TEST(flag, fn, ...) \
     if (cfg->test_mask & (flag)) { \
         uiox_post_result_t _rc = fn(__VA_ARGS__); \
         if (_rc == UIOX_POST_OK) r->passed_tests |= (flag); \
         else { r->failed_tests |= (flag); \
                if (r->overall == UIOX_POST_OK) r->overall = _rc; } \
     }
 
     RUN_TEST(UIOX_POST_TEST_CPU,     test_cpu,     r)
     RUN_TEST(UIOX_POST_TEST_RAM,     test_ram,     cfg, r)
     RUN_TEST(UIOX_POST_TEST_ROM_CRC, test_rom_crc, cfg, r)
     RUN_TEST(UIOX_POST_TEST_TIMER,   test_timer,   r)
     RUN_TEST(UIOX_POST_TEST_GIC,     test_gic,     cfg, r)
     RUN_TEST(UIOX_POST_TEST_STACK,   test_stack,   cfg)
 
 #undef RUN_TEST
     return r->overall;
 }
 
 /* =========================================================================
  * uiox_fw_post_print — no printf/puts, HAL UART only
  * ====================================================================== */
 
 void uiox_fw_post_print(const uiox_post_report_t *r)
 {
     if (!r) return;
     char buf[24];
 
     fw_puts("\r\n[POST] ===== Power-On Self Test =====\r\n");
     fw_puts("[POST] Overall    : ");
     fw_puts(r->overall == UIOX_POST_OK ? "PASS\n" : "FAIL\n");
 
     fw_puts("[POST] Passed     : 0x");
     fw_u32_to_hex(r->passed_tests, buf); buf[2] = '\0';  /* 2 nibbles */
     /* Print only 2 hex chars for 8-bit masks */
     const char *hx = "0123456789abcdef";
     char mask2[3];
     mask2[0] = hx[(r->passed_tests >> 4u) & 0xFu];
     mask2[1] = hx[ r->passed_tests        & 0xFu];
     mask2[2] = '\0';
     fw_puts(mask2); fw_puts("  Failed: 0x");
     mask2[0] = hx[(r->failed_tests >> 4u) & 0xFu];
     mask2[1] = hx[ r->failed_tests        & 0xFu];
     fw_puts(mask2); fw_puts("\n");
 
     if (r->cpu_midr)
         fw_print_hex32("[POST] CPU MIDR   : ", r->cpu_midr);
     if (r->timer_freq_hz)
         fw_print_u64("[POST] Timer Hz   : ", r->timer_freq_hz);
     if (r->rom_crc32_actual)
         fw_print_hex32("[POST] ROM CRC32  : ", r->rom_crc32_actual);
     if (r->ram_tested_bytes)
         fw_print_u64("[POST] RAM tested : ", r->ram_tested_bytes);
     if (r->gic_typer)
         fw_print_hex32("[POST] GIC TYPER  : ", r->gic_typer);
     if (r->overall != UIOX_POST_OK) {
         fw_puts("[POST] FAIL reason: ");
         fw_puts(r->fail_msg);
         fw_puts("\n");
     }
     fw_puts("[POST] =====================================\r\n");
 
     (void)buf;
 }
 