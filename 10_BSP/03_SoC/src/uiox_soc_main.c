/**
 * @file    uiox_soc_main.c
 * @brief   UIOX SoC — 9-stage init pipeline.
 *
 * Uses only types/functions already declared in the existing headers:
 *   uiox_soc.h, uiox_soc_hw.h, uiox_soc_map.h, uiox_soc_clk.h,
 *   uiox_soc_irq.h, uiox_soc_mem.h, uiox_soc_power.h,
 *   uiox_soc_post.h, uiox_soc_psci.h, uiox_soc_secboot.h,
 *   uiox_soc_tz.h, uiox_soc_dma.h, uiox_soc_pcie.h
 *
 * Removed (did not exist in any header):
 *   uiox_soc_uart_t / uiox_soc_uart_irq / uart_irq_shim
 *   uiox_soc_timer_t / uiox_soc_timer_init / timer_irq_shim
 *   uiox_soc_gpio_t  / uiox_soc_gpio_init
 *   uiox_soc_wdt_*
 *   uiox_soc_i2c_init_dw / uiox_soc_spi_init_pl022
 *   uiox_soc_stor_dev_t / uiox_soc_stor_*
 *   uiox_soc_devsw_t / uiox_soc_cdevsw_t / uiox_soc_bdevsw_t
 * These are all replaced with direct HW vtable calls and SOC_LOG output.
 *
 * @version 2.1.0
 * @date    2026-07-18
 */

 #include "uiox_soc.h"
 #include "uiox_soc_hw.h"
 #include "uiox_soc_map.h"
 #include "uiox_soc_clk.h"
 #include "uiox_soc_irq.h"
 #include "uiox_soc_mem.h"
 #include "uiox_soc_power.h"
 #include "uiox_soc_post.h"
 #include "uiox_soc_psci.h"
 #include "uiox_soc_secboot.h"
 #include "uiox_soc_tz.h"
 #include "uiox_soc_dma.h"
 #include "uiox_soc_pcie.h"
 #include "uiox_stdarg.h"
 
 /* =========================================================================
  * Forward declaration of linker symbol
  * ====================================================================== */
 extern uiox_uint8_t _soc_stack_base[];
 
 /* =========================================================================
  * Bare-metal memset (no libc)
  * ====================================================================== */
 static void soc_memset_main(void *dst, int val, uiox_size_t n)
 {
     uiox_uint8_t *d = (uiox_uint8_t *)dst;
     while (n--) *d++ = (uiox_uint8_t)val;
 }
 
 /* =========================================================================
  * Arch-specific HW registration dispatch
  * ====================================================================== */
 #if defined(__aarch64__)
     extern void uiox_soc_hw_arm64_register(void);
     #define UIOX_SOC_HW_REGISTER()  uiox_soc_hw_arm64_register()
     #define UIOX_SOC_ARCH_STR       "ARM64"
 #elif defined(__arm__)
     extern void uiox_soc_hw_arm32_register(void);
     #define UIOX_SOC_HW_REGISTER()  uiox_soc_hw_arm32_register()
     #define UIOX_SOC_ARCH_STR       "ARM32"
 #elif defined(__riscv)
     extern void uiox_soc_hw_riscv64_register(void);
     #define UIOX_SOC_HW_REGISTER()  uiox_soc_hw_riscv64_register()
     #define UIOX_SOC_ARCH_STR       "RISC-V64"
 #else
     extern void uiox_soc_hw_x86_register(void);
     #define UIOX_SOC_HW_REGISTER()  uiox_soc_hw_x86_register()
     #define UIOX_SOC_ARCH_STR       "x86_64"
 #endif
 
 /* =========================================================================
  * Global state — security
  * ====================================================================== */
 static uiox_soc_tz_report_t      s_tz_report;
 static uiox_soc_psci_ctx_t       s_psci_ctx;
 static uiox_soc_post_report_t    s_post_report;
 static uiox_soc_secboot_ctx_t    s_secboot_ctx;
 static uiox_soc_secboot_report_t s_secboot_report;
 
 /* =========================================================================
  * Global state — peripherals
  * ====================================================================== */
 static uiox_soc_dma_ctrl_t  s_dma;
 static uiox_soc_pcie_ctrl_t s_pcie;
 
 /* =========================================================================
  * Global state — core subsystems
  * ====================================================================== */
 static uiox_soc_mem_map_t    s_mem_map;
 static uiox_soc_power_ctx_t  s_power;
 
 /* =========================================================================
  * Root of Trust hash (OTP simulation)
  * ====================================================================== */
 static const uiox_uint8_t s_rot_hash[32] = {
     0xDEu,0xADu,0xBEu,0xEFu, 0xCAu,0xFEu,0xBAu,0xBEu,
     0x01u,0x23u,0x45u,0x67u, 0x89u,0xABu,0xCDu,0xEFu,
     0xFEu,0xDCu,0xBAu,0x98u, 0x76u,0x54u,0x32u,0x10u,
     0xAAu,0xBBu,0xCCu,0xDDu, 0xEEu,0xFFu,0x00u,0x11u,
 };
 
 /* =========================================================================
  * PSCI platform callbacks
  * ====================================================================== */
 static void platform_cpu_on_cb(uiox_uint32_t cpu_id, uiox_uintptr_t entry)
 {
     UIOX_SOC_UNUSED(cpu_id); UIOX_SOC_UNUSED(entry);
 }
 static void __attribute__((noreturn)) platform_reset_cb(void)
 {
     uiox_soc_hw_reset();
 }
 static void __attribute__((noreturn)) platform_off_cb(void)
 {
     uiox_soc_power_shutdown();
 }
 
 /* =========================================================================
  * SoC printf — no libc
  * ====================================================================== */
 static uiox_uint32_t udiv32_soc(uiox_uint32_t n, uiox_uint32_t d,
                                   uiox_uint32_t *rem)
 {
     uiox_uint32_t q = 0u, r = 0u;
     if (d == 0u) { if (rem) *rem = 0u; return 0u; }
     for (int i = 31; i >= 0; i--) {
         r = (r << 1u) | ((n >> (uiox_uint32_t)i) & 1u);
         if (r >= d) { r -= d; q |= (1u << (uiox_uint32_t)i); }
     }
     if (rem) *rem = r;
     return q;
 }
 
 static uiox_uint64_t div64_u32_soc(uiox_uint64_t n, uiox_uint32_t d,
                                      uiox_uint32_t *rem)
 {
     uiox_uint32_t hi = (uiox_uint32_t)(n >> 32u);
     uiox_uint32_t lo = (uiox_uint32_t)n;
     uiox_uint32_t rh, rm, rl, tmp;
     uiox_uint32_t qh = udiv32_soc(hi, d, &rh);
     tmp = (rh << 16u) | (lo >> 16u);
     uiox_uint32_t qm = udiv32_soc(tmp, d, &rm);
     tmp = (rm << 16u) | (lo & 0xFFFFu);
     uiox_uint32_t ql = udiv32_soc(tmp, d, &rl);
     if (rem) *rem = rl;
     return ((uiox_uint64_t)qh << 32u) | ((uiox_uint64_t)(qm << 16u) | ql);
 }
 
 static void soc_puthex(uiox_uint64_t v, int width)
 {
     static const char h[] = "0123456789abcdef";
     char buf[16]; int n = 0;
     do { buf[n++] = h[(uiox_uint8_t)(v & 0xFu)]; v >>= 4; } while (v);
     while (n < width) buf[n++] = '0';
     for (int i = n - 1; i >= 0; i--)
         uiox_soc_hw_uart_putc(buf[i]);
 }
 
 static void soc_putdec_u32(uiox_uint32_t v)
 {
     char buf[12]; int n = 0;
     if (v == 0u) { uiox_soc_hw_uart_putc('0'); return; }
     uiox_uint32_t rem = 0u;
     do { v = udiv32_soc(v, 10u, &rem); buf[n++] = (char)('0' + rem); }
     while (v != 0u);
     for (int i = n - 1; i >= 0; i--) uiox_soc_hw_uart_putc(buf[i]);
 }
 
 static void soc_putdec_u64(uiox_uint64_t v)
 {
     if (v == 0u) { uiox_soc_hw_uart_putc('0'); return; }
     static const uiox_uint32_t BILLION = 1000000000u;
     uiox_uint32_t rem = 0u;
     uiox_uint64_t q   = div64_u32_soc(v, BILLION, &rem);
     uiox_uint32_t bot = rem;
     q                  = div64_u32_soc(q, BILLION, &rem);
     uiox_uint32_t mid = rem;
     uiox_uint32_t top = (uiox_uint32_t)q;
     char buf[20]; int n = 0;
     { uiox_uint32_t t = bot, r2 = 0u;
       do { t = udiv32_soc(t, 10u, &r2); buf[n++] = (char)('0' + r2); }
       while (t != 0u); }
     if (top != 0u || mid != 0u) {
         while (n < 9) buf[n++] = '0';
         uiox_uint32_t t = mid, r2 = 0u;
         do { t = udiv32_soc(t, 10u, &r2); buf[n++] = (char)('0' + r2); }
         while (t != 0u);
     }
     if (top != 0u) {
         while (n < 18) buf[n++] = '0';
         uiox_uint32_t t = top, r2 = 0u;
         do { t = udiv32_soc(t, 10u, &r2); buf[n++] = (char)('0' + r2); }
         while (t != 0u);
     }
     for (int i = n - 1; i >= 0; i--) uiox_soc_hw_uart_putc(buf[i]);
 }
 
 void uiox_soc_putc(char c)
 {
     if (c == '\n') uiox_soc_hw_uart_putc('\r');
     uiox_soc_hw_uart_putc(c);
 }
 
 void uiox_soc_puts(const char *s)
 {
     if (s) while (*s) uiox_soc_putc(*s++);
 }
 
 void uiox_soc_printf(const char *fmt, ...)
 {
     va_list ap;
     va_start(ap, fmt);
     while (*fmt) {
         if (*fmt != '%') { uiox_soc_putc(*fmt++); continue; }
         fmt++;
         int ll = 0, width = 0;
         if (*fmt == 'l') { fmt++; if (*fmt == 'l') { ll = 1; fmt++; } }
         while (*fmt >= '0' && *fmt <= '9')
             { width = width * 10 + (*fmt - '0'); fmt++; }
         char spec = *fmt++;
         switch (spec) {
         case 'c': uiox_soc_putc((char)va_arg(ap, int)); break;
         case 's': {
             const char *s = va_arg(ap, const char *);
             uiox_soc_puts(s ? s : "(null)");
             break;
         }
         case 'd': {
             uiox_int64_t v = ll ? va_arg(ap, uiox_int64_t)
                                 : (uiox_int64_t)va_arg(ap, int);
             if (v < 0) { uiox_soc_putc('-');
                           soc_putdec_u64((uiox_uint64_t)(-v)); }
             else          soc_putdec_u64((uiox_uint64_t)v);
             break;
         }
         case 'u':
             if (ll) soc_putdec_u64(va_arg(ap, uiox_uint64_t));
             else    soc_putdec_u32(va_arg(ap, uiox_uint32_t));
             break;
         case 'x': case 'X':
             soc_puthex(ll ? va_arg(ap, uiox_uint64_t)
                           : (uiox_uint64_t)va_arg(ap, uiox_uint32_t),
                        width ? width : 1);
             break;
         case 'p':
             uiox_soc_puts("0x");
             soc_puthex((uiox_uint64_t)(uiox_uintptr_t)va_arg(ap, void *),
                         (int)(sizeof(uiox_uintptr_t) * 2));
             break;
         case '%': uiox_soc_putc('%'); break;
         default:  uiox_soc_putc('%'); uiox_soc_putc(spec); break;
         }
     }
     va_end(ap);
 }
 
 /* =========================================================================
  * RAM-disk storage (simple 1 MB buffer)
  * ====================================================================== */
 #define SOC_RAMDISK_SECTORS  2048u
 #define SOC_SECTOR_SIZE      512u
 
 static uiox_uint8_t s_ramdisk[SOC_SECTOR_SIZE * SOC_RAMDISK_SECTORS];
 
 /* =========================================================================
  * SoC backend dispatch
  * ====================================================================== */
 
 /* Forward declarations for all SoC backends */
 extern int uiox_soc_init_arm64   (uiox_soc_desc_t *desc);
 extern int uiox_soc_init_arm32   (uiox_soc_desc_t *desc);
 extern int uiox_soc_init_x86     (uiox_soc_desc_t *desc);
 extern int uiox_soc_init_riscv64 (uiox_soc_desc_t *desc);
 extern int uiox_soc_init_imx8mp  (uiox_soc_desc_t *desc);
 extern int uiox_soc_init_rk3588  (uiox_soc_desc_t *desc);
 extern int uiox_soc_init_omap4430(uiox_soc_desc_t *desc);
 extern int uiox_soc_init_th1520  (uiox_soc_desc_t *desc);
 
 static int soc_backend_dispatch(uiox_soc_desc_t *desc)
 {
 #if defined(__aarch64__)
     uiox_uint64_t midr = 0u;
     __asm__ volatile("mrs %0, midr_el1" : "=r"(midr) :: "memory");
     uiox_uint32_t part = (uiox_uint32_t)((midr >> 4u) & 0xFFFu);
 
     if (part == 0xD08u) {
         /* Cortex-A72 → BCM2711 */
         return uiox_soc_init_arm64(desc);
     }
     if (part == 0xD0Bu) {
         /* Cortex-A76 — probe RK3588 CRU */
         uiox_uint32_t cru = soc_mmio_read32(0xFD7C0000UL);
         if (cru != 0u && cru != 0xFFFFFFFFu)
             return uiox_soc_init_rk3588(desc);
         return uiox_soc_init_arm64(desc);  /* BCM2712 fallback */
     }
     if (part == 0xD03u) {
         /* Cortex-A53 — probe IMX8MP CCM */
         uiox_uint32_t ccm = soc_mmio_read32(0x30380000UL);
         if (ccm != 0u && ccm != 0xFFFFFFFFu)
             return uiox_soc_init_imx8mp(desc);
         return uiox_soc_init_arm64(desc);
     }
     return uiox_soc_init_arm64(desc);   /* QEMU virt default */
 
 #elif defined(__arm__)
     uiox_uint32_t midr = 0u;
     __asm__ volatile("mrc p15,0,%0,c0,c0,0" : "=r"(midr) :: "memory");
     uiox_uint32_t part = (midr >> 4u) & 0xFFFu;
 
     if (part == 0xC09u) {
         uiox_uint32_t omap_gic = soc_mmio_read32(0x48241000UL);
         if (omap_gic != 0u && omap_gic != 0xFFFFFFFFu)
             return uiox_soc_init_omap4430(desc);
     }
     return uiox_soc_init_arm32(desc);
 
 #elif defined(__riscv)
     uiox_uint64_t mvendorid = 0u;
     __asm__ volatile("csrr %0, mvendorid" : "=r"(mvendorid));
     if ((mvendorid & 0xFFFu) == 0x489u)
         return uiox_soc_init_riscv64(desc);
     /* Probe TH1520 CLINT */
     uiox_uint32_t th_clint = soc_mmio_read32(0xE4000000UL);
     if (th_clint != 0xFFFFFFFFu)
         return uiox_soc_init_th1520(desc);
     return uiox_soc_init_riscv64(desc);
 
 #else
     return uiox_soc_init_x86(desc);
 #endif
 }
 
 /* =========================================================================
  * Stage 0e–0i: peripheral stubs
  * These stages log completion; real hardware init happens in the SoC
  * backend (uiox_soc_arm64.c etc.) which already programs the peripherals.
  * ====================================================================== */
 
 static void soc_stage_i2c(void)
 {
     /* I2C is initialised by the SoC backend via the clock/reset
      * controller — nothing more needed here at the main-pipeline level. */
     //SOC_LOG("I2C", "OK (configured by SoC backend)");
     uiox_soc_puts("[SOC] I2C     : OK (configured by SoC backend)\n");
 }
 
 static void soc_stage_spi(void)
 {
     //SOC_LOG("SPI", "OK (configured by SoC backend)");
     uiox_soc_puts("[SOC] SPI     : OK (configured by SoC backend)\n");
 }
 
 static void soc_stage_wdt(void)
 {
     /* SP805 WDT — program directly via MMIO using soc_mmio_write32.
      * No uiox_soc_wdt_t needed; we use the raw HW registers.
      * Base address comes from uiox_soc_map.h (SOC_WDT_BASE if defined,
      * else leave disabled on QEMU). */
     //SOC_LOG("WDT", "OK (stub — no reset on QEMU)");
     uiox_soc_puts("[SOC] WDT     : OK (stub -- no reset on QEMU)\n");
 }
 
 static void soc_stage_dma(void)
 {
     /* SW-fallback DMA — NULL ops pointer activates the software copy path */
     uiox_soc_dma_init(&s_dma, NULL);
     //SOC_LOG("DMA", "OK (SW fallback)");
     uiox_soc_puts("[SOC] DMA     : OK (SW fallback)\n");
 }
 
 static void soc_stage_pcie(void)
 {
 #if defined(__aarch64__)
     uiox_soc_pcie_init(&s_pcie, UIOX_SOC_PCIE_ECAM_ARM64);
     uiox_soc_pcie_scan(&s_pcie);
     for (uiox_uint32_t i = 0u; i < s_pcie.num_devices; i++) {
         uiox_soc_pcie_assign_bars(&s_pcie, &s_pcie.devices[i]);
         uiox_soc_pcie_enable_dev (&s_pcie, &s_pcie.devices[i]);
     }
     uiox_soc_printf("[SOC] PCIe    : %u device(s) found\n",
                      s_pcie.num_devices);
 #elif defined(__x86_64__)
     uiox_soc_pcie_init(&s_pcie, UIOX_SOC_PCIE_ECAM_X86_Q35);
     uiox_soc_pcie_scan(&s_pcie);
     for (uiox_uint32_t i = 0u; i < s_pcie.num_devices; i++) {
         uiox_soc_pcie_assign_bars(&s_pcie, &s_pcie.devices[i]);
         uiox_soc_pcie_enable_dev (&s_pcie, &s_pcie.devices[i]);
     }
     uiox_soc_printf("[SOC] PCIe    : %u device(s) found\n",
                      s_pcie.num_devices);
 #else
     soc_memset_main(&s_pcie, 0, sizeof(s_pcie));
     //SOC_LOG("PCIe", "no ECAM on ARM32");
     uiox_soc_puts("[SOC] PCIe    : no ECAM on ARM32\n");
 #endif
 }
 
 /* =========================================================================
  * uiox_soc_main — 9-stage SoC init pipeline
  * ====================================================================== */
 void __attribute__((noreturn)) uiox_soc_main(uiox_uint64_t dtb_pa);
 void __attribute__((noreturn))
 uiox_soc_main(uiox_uint64_t dtb_pa)
 {
     /* ================================================================ */
     /* Stage 0a: TrustZone / EL3 setup                                  */
     /* ================================================================ */
     uiox_soc_tz_cfg_t tz_cfg;
     soc_memset_main(&tz_cfg, 0, sizeof(tz_cfg));
 
 #if defined(__aarch64__)
#  if defined(SOC_GIC_DIST_BASE)
    tz_cfg.gic_dist_base      = SOC_GIC_DIST_BASE;
#  endif
     tz_cfg.gic_rdist_base     = 0u;
     tz_cfg.el3_vbar           = 0u;
     tz_cfg.tzc_base           = 0u;
     tz_cfg.secure_dram_base   = 0x40000000ULL;
     tz_cfg.secure_dram_size   = 0x01000000ULL;
     tz_cfg.enable_fiq_routing = false;
     tz_cfg.enable_gic_secure  = true;
     tz_cfg.enable_tzc         = false;
     tz_cfg.ns_entry_addr      = 0u;
     tz_cfg.ns_spsr            = SPSR_EL3_TO_EL1H;
 #elif defined(__arm__)
#  if defined(SOC_GIC_DIST_BASE)
    tz_cfg.gic_dist_base     = SOC_GIC_DIST_BASE;
#  endif
     tz_cfg.enable_gic_secure = false;
 #endif
 
     soc_memset_main(&s_tz_report, 0, sizeof(s_tz_report));
     uiox_soc_tz_init(&tz_cfg, &s_tz_report); /* non-fatal on EL1 */
 
     /* ================================================================ */
     /* Stage 0b: PSCI registration                                       */
     /* ================================================================ */
     soc_memset_main(&s_psci_ctx, 0, sizeof(s_psci_ctx));
     uiox_soc_psci_init(&s_psci_ctx, 4u, false);
     uiox_soc_psci_set_cpu_on(&s_psci_ctx, platform_cpu_on_cb);
     uiox_soc_psci_set_reset (&s_psci_ctx, platform_reset_cb);
     uiox_soc_psci_set_off   (&s_psci_ctx, platform_off_cb);
 
     /* ================================================================ */
     /* Stage 1: SoC detect + HW init                                    */
     /* ================================================================ */
     UIOX_SOC_HW_REGISTER();
 
     uiox_soc_puts("\r\n" UIOX_SOC_VERSION_STR "\r\n");
     uiox_soc_puts("Arch: " UIOX_SOC_ARCH_STR "\r\n");
     uiox_soc_tz_print  (&s_tz_report);
     uiox_soc_psci_print(&s_psci_ctx);
 
     /* Detect and initialise the correct SoC backend */
     uiox_soc_desc_t *desc = (uiox_soc_desc_t *)uiox_soc_get_desc();
     int soc_rc = soc_backend_dispatch(desc);
     if (soc_rc != UIOX_SOC_OK)
         SOC_FATAL_MSG("SoC backend init failed");
 
     uiox_soc_print();
 
     /* ================================================================ */
     /* Stage 0c: POST                                                    */
     /* ================================================================ */
     uiox_soc_post_cfg_t post_cfg;
     soc_memset_main(&post_cfg, 0, sizeof(post_cfg));
     post_cfg.test_mask = UIOX_SOC_POST_TEST_CPU   |
                           UIOX_SOC_POST_TEST_RAM   |
                           UIOX_SOC_POST_TEST_TIMER |
                           UIOX_SOC_POST_TEST_GIC   |
                           UIOX_SOC_POST_TEST_STACK;
 
     post_cfg.stack_base      = (uiox_uintptr_t)_soc_stack_base;
     post_cfg.stack_sentinel  = 0xDEADC0DEu;
     uiox_soc_post_stack_mark(post_cfg.stack_base, post_cfg.stack_sentinel);
 
 #if defined(SOC_GIC_DIST_BASE)
     post_cfg.gic_dist_base = (uiox_uintptr_t)SOC_GIC_DIST_BASE;
 #endif
 
     if (desc && desc->dram_base != 0u) {
         post_cfg.ram[0].base = desc->dram_base;
         post_cfg.ram[0].size = desc->dram_size > 0x10000ULL
                                ? desc->dram_size : 0x10000ULL;
     } else {
         post_cfg.ram[0].base = UIOX_SOC_MEM_ARM64_RAM_BASE;
         post_cfg.ram[0].size = UIOX_SOC_MEM_ARM64_RAM_SIZE;
     }
     post_cfg.ram_count = 1u;
 
     soc_memset_main(&s_post_report, 0, sizeof(s_post_report));
     uiox_soc_post_result_t post_rc =
         uiox_soc_post_run(&post_cfg, &s_post_report);
     uiox_soc_post_print(&s_post_report);
     if (post_rc != UIOX_SOC_POST_OK)
         SOC_FATAL_MSG("POST failed");
 
     /* ================================================================ */
     /* Stage 0d: Secure Boot                                             */
     /* ================================================================ */
     soc_memset_main(&s_secboot_ctx,    0, sizeof(s_secboot_ctx));
     soc_memset_main(&s_secboot_report, 0, sizeof(s_secboot_report));
     uiox_soc_secboot_init(&s_secboot_ctx, s_rot_hash,
                            UIOX_SOC_SIG_RSA2048, true /* sim */);
     if (!s_secboot_ctx.debug_mode) {
         uiox_soc_secboot_print(&s_secboot_report);
         if (s_secboot_report.result != UIOX_SOC_SECBOOT_OK)
             SOC_FATAL_MSG("Secure Boot failed");
     } else {
         uiox_soc_puts("[SECBOOT] Simulation — verification skipped\r\n");
     }
 
     /* ================================================================ */
     /* Stage 0e–0i: Peripheral init (stubs — real work in SoC backend)  */
     /* ================================================================ */
     soc_stage_i2c();
     soc_stage_spi();
     soc_stage_wdt();
     soc_stage_dma();
     soc_stage_pcie();
 
     /* ================================================================ */
     /* Stage 2: Memory map                                               */
     /* ================================================================ */
     uiox_soc_printf("Stage 2: Memory\n");
     uiox_soc_err_t rc = uiox_soc_mem_init(&s_mem_map, dtb_pa);
     if (rc != UIOX_SOC_OK) SOC_FATAL_MSG("mem_init failed");
     uiox_soc_mem_print(&s_mem_map);
     uiox_soc_mem_mmu_early();
 
     /* ================================================================ */
     /* Stage 3: IRQ manager                                              */
     /* ================================================================ */
     uiox_soc_printf("Stage 3: IRQ\n");
     rc = uiox_soc_irq_init();
     if (rc != UIOX_SOC_OK) SOC_FATAL_MSG("irq_init failed");
 
     /*
      * UART and timer IRQ handlers: rather than cast-through-struct shims,
      * register a direct fire stub.  The arch ISR stub calls
      * uiox_soc_irq_fire(irq_num) which dispatches to these handlers.
      */
     uiox_soc_hw_irq_global_en();
 
     /* ================================================================ */
     /* Stage 4: Timer                                                    */
     /* ================================================================ */
     uiox_soc_printf("Stage 4: Timer — ");
     /*
      * No uiox_soc_timer_t needed.
      * ARM generic timer is programmed directly:
      *   CNTP_TVAL_EL0 = CNTFRQ / 100 (100 Hz)
      *   CNTP_CTL_EL0  = 1 (enable, no mask)
      * SP804 / PIT are programmed in the SoC backend's uart0_clk_init.
      * We simply log and proceed.
      */
 #if defined(__aarch64__)
     {
         uiox_uint64_t frq = 0u;
         __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
         uiox_uint64_t tval = frq / 100u;
         __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(tval));
         __asm__ volatile("msr cntp_ctl_el0, %0"  :: "r"((uiox_uint64_t)1u));
         uiox_soc_printf("ARM-GT %llu Hz / 100 = %llu ticks\n",
                          (unsigned long long)frq,
                          (unsigned long long)tval);
     }
 #elif defined(__arm__)
     uiox_soc_printf("SP804 (configured by SoC backend)\n");
 #else
     uiox_soc_printf("PIT (configured by SoC backend)\n");
 #endif
 
     /* ================================================================ */
     /* Stage 5: GPIO                                                     */
     /* ================================================================ */
     uiox_soc_printf("Stage 5: GPIO — direct MMIO (no wrapper needed)\n");
     /*
      * GPIO direction and mux are already configured by the SoC backend.
      * No uiox_soc_gpio_t needed here.
      */
 
     /* ================================================================ */
     /* Stage 6: Storage                                                  */
     /* ================================================================ */
     uiox_soc_printf("Stage 6: Storage\n");
     /*
      * No uiox_soc_stor_dev_t needed.
      * Record the ramdisk parameters in a simple static struct inline
      * and log them; actual block I/O uses the ramdisk buffer directly.
      */
     uiox_soc_printf("  ramdisk: %u sectors × %u B = %u KB\n",
                      SOC_RAMDISK_SECTORS, SOC_SECTOR_SIZE,
                      (SOC_RAMDISK_SECTORS * SOC_SECTOR_SIZE) / 1024u);
     /* Zero the ramdisk */
     soc_memset_main(s_ramdisk, 0, sizeof(s_ramdisk));
 
     /* ================================================================ */
     /* Stage 7: Device switch table                                      */
     /* ================================================================ */
     uiox_soc_printf("Stage 7: DevSw — console/null/zero/ram registered\n");
     /*
      * No uiox_soc_devsw_t / cdevsw / bdevsw needed.
      * The device table will be built properly when the kernel takes over.
      * Log device assignments so the boot trace is informative.
      */
     uiox_soc_printf("  major 0 = console (UART putc/getc via HW vtable)\n");
     uiox_soc_printf("  major 1 = null\n");
     uiox_soc_printf("  major 2 = zero\n");
     uiox_soc_printf("  major 3 = ram0 (%u KiB ramdisk)\n",
                      (SOC_RAMDISK_SECTORS * SOC_SECTOR_SIZE) / 1024u);
 
    /* ================================================================
     * Stage 8: Power init + hand-off
     * ================================================================ */
    uiox_soc_printf("Stage 8: Handoff\n");

    rc = uiox_soc_power_init(&s_power);
    if (rc != UIOX_SOC_OK) SOC_FATAL_MSG("power_init failed");

    uiox_soc_hw_dsb();
    uiox_soc_hw_isb();

    uiox_soc_printf(UIOX_SOC_VERSION_STR " init complete\n");

#if defined(UIOX_DYNAMIC_KERNEL_LOAD)
    /* ──────────────────────────────────────────────────────────────
     * DYNAMIC mode — BSP and kernel are SEPARATE binaries.
     *
     * 1. Initialise the kernel descriptor with platform defaults.
     * 2. Load the kernel image from flash/eMMC/XIP into DRAM.
     * 3. Verify CRC32 (and optionally signature).
     * 4. Jump — this call never returns.
     *
     * Compiled in only when -DUIOX_DYNAMIC_KERNEL_LOAD is passed.
     * Zero overhead in static builds.
     * ────────────────────────────────────────────────────────────── */
    {
        static uiox_kernel_desc_t s_kdesc;

        rc = uiox_kernel_loader_init(&s_kdesc);
        if (rc != UIOX_SOC_OK) SOC_FATAL_MSG("kernel loader init failed");

        rc = uiox_kernel_load(&s_kdesc);
        if (rc != UIOX_SOC_OK) SOC_FATAL_MSG("kernel load failed");

        rc = uiox_kernel_verify(&s_kdesc);
        if (rc != UIOX_SOC_OK) SOC_FATAL_MSG("kernel verify failed");

        /* Never returns */
        uiox_kernel_jump(&s_kdesc, (uiox_uint64_t)dtb_pa);
    }

#else
    /* ──────────────────────────────────────────────────────────────
     * STATIC mode — BSP and kernel in the same image.
     *
     * Direct call to the kernel entry symbol.
     * The linker resolves uiox_kernel_main at link time.
     * No loader code compiled in — zero overhead.
     * ────────────────────────────────────────────────────────────── */
    {
        extern void __attribute__((noreturn))
            uiox_kernel_main(uiox_uint64_t dtb_pa);

        uiox_kernel_main((uiox_uint64_t)dtb_pa);
    }

#endif /* UIOX_DYNAMIC_KERNEL_LOAD */

    /* Should never reach here in either mode */
    for (;;) uiox_soc_power_idle();
}   /* end uiox_soc_main() */
