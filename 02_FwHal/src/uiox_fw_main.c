/**
 * @file  uiox_fw_main.c
 * @brief UIOX Firmware — 8-stage init pipeline.
 *
 * Stage 1:  Platform HW init   (UART, IRQ controller, clocks)
 * Stage 2:  Memory map         (DTB probe → region table)
 * Stage 3:  IRQ manager        (register handlers for all devices)
 * Stage 4:  Timers             (SP804 / PIT / ARM-GT at 100 Hz)
 * Stage 5:  GPIO               (pin direction and IRQ config)
 * Stage 6:  Storage            (block device registration)
 * Stage 7:  Device switch table (char + block devsw population)
 * Stage 8:  Hand-off to kernel (call uiox_kernel_main())
 *
 * @version 1.0.1
 * @date    2026-06-26
 */

 #include "uiox_fw.h"
 #include <stdarg.h>
 /**
 * Integration patch for 02_FwHal/src/uiox_fw_main.c
 * Add these includes and the Stage 0 block.
 */
/* ── New includes at top of uiox_fw_main.c ──────────────────────────── */


#include "../include/uiox_fw_hw.h"       /* uiox_fw_hw_ops() accessor  */


/* =========================================================================
 * Forward declaration of linker symbol (defined in entry .S stub)
 * ====================================================================== */
extern uint8_t _fw_stack_base[];   /* bottom of firmware stack          */
/* =========================================================================
 * Bare-metal memset (no libc — replaces memset() throughout this file)
 * ====================================================================== */
static void fw_memset_fw(void *dst, int val, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    while (n--) *d++ = (uint8_t)val;
}
/* =========================================================================
 * Arch-specific HW registration macro
 * (matches the pattern already used in the original uiox_fw_main.c)
 * ====================================================================== */
#if defined(__aarch64__)
  extern void uiox_fw_hw_arm64_register(void);
  #define UIOX_FW_HW_REGISTER()  uiox_fw_hw_arm64_register()
  #define UIOX_FW_ARCH_STR       "ARM64"
#elif defined(__arm__)
  extern void uiox_fw_hw_arm32_register(void);
  #define UIOX_FW_HW_REGISTER()  uiox_fw_hw_arm32_register()
  #define UIOX_FW_ARCH_STR       "ARM32"
#else
  extern void uiox_fw_hw_x86_register(void);
  #define UIOX_FW_HW_REGISTER()  uiox_fw_hw_x86_register()
  #define UIOX_FW_ARCH_STR       "x86_64"
#endif

/* =========================================================================
 * Global state — new devices (add alongside existing s_fw_soc etc.)
 * ====================================================================== */

 static uiox_i2c_dev_t   s_i2c0;
 static uiox_spi_dev_t   s_spi0;
 static uiox_wdt_dev_t   s_wdt;
 static uiox_pcie_ctrl_t s_pcie;
 
 /* =========================================================================
  * Stage 0e: I2C master init
  * ====================================================================== */
 
 static void fw_stage_i2c(void)
 {
 #if defined(__aarch64__)
     uiox_fw_i2c_init_dw(&s_i2c0,
                           UIOX_I2C_ARM64_BASE,
                           100000000u,   /* 100 MHz APB clock             */
                           39u,          /* IRQ 39 on QEMU virt           */
                           UIOX_I2C_SPEED_FAST);
 #elif defined(__arm__)
     uiox_fw_i2c_init_dw(&s_i2c0,
                           UIOX_I2C_ARM32_BASE,
                           24000000u,
                           24u,
                           UIOX_I2C_SPEED_STANDARD);
 #else
     /* x86: SMBus — stub init */
     fw_memset_fw(&s_i2c0, 0, sizeof(s_i2c0));
 #endif
     //fw_puts_main("[FW] I2C     : OK\n");
     uiox_fw_printf("[FW] I2C     : OK\n");
 }
 
 /* =========================================================================
  * Stage 0f: SPI master init
  * ====================================================================== */
 
 static void fw_stage_spi(void)
 {
 #if defined(__aarch64__)
     uiox_fw_spi_init_pl022(&s_spi0,
                              UIOX_SPI_ARM64_BASE,
                              UIOX_SPI_ARM64_CLK,
                              40u);  /* IRQ 40 on QEMU virt */
 #elif defined(__arm__)
     uiox_fw_spi_init_pl022(&s_spi0,
                              UIOX_SPI_ARM32_BASE,
                              UIOX_SPI_ARM32_CLK,
                              11u);
 #else
     fw_memset_fw(&s_spi0, 0, sizeof(s_spi0));
 #endif
     //fw_puts_main("[FW] SPI     : OK\n");
     uiox_fw_printf("[FW] SPI     : OK\n");
 }
 
 /* =========================================================================
  * Stage 0g: Watchdog timer init (10 second timeout)
  * ====================================================================== */
 
 static void fw_stage_wdt(void)
 {
 #if defined(__aarch64__)
     uiox_fw_wdt_init_sp805(&s_wdt,
                              UIOX_WDT_ARM64_BASE,
                              UIOX_WDT_CLOCK_HZ,
                              10000u);  /* 10 s timeout */
 #elif defined(__arm__)
     uiox_fw_wdt_init_sp805(&s_wdt,
                              UIOX_WDT_ARM32_BASE,
                              UIOX_WDT_CLOCK_HZ,
                              10000u);
 #else
     fw_memset_fw(&s_wdt, 0, sizeof(s_wdt));
 #endif
     //fw_puts_main("[FW] WDT     : OK (10 s)\n");
     uiox_fw_printf("[FW] WDT     : OK (10 s)\n");
 }
 
 /* =========================================================================
  * Stage 0h: DMA controller init
  * ====================================================================== */
 
 //static void fw_stage_dma(void)
 //{
//     /* Pass NULL ops → uses SW fallback DMA (always works on QEMU) */
//     uiox_fw_dma_init(&s_dma, NULL);
//     //fw_puts_main("[FW] DMA     : OK (SW fallback)\n");
//     uiox_fw_printf("[FW] DMA     : OK (SW fallback)\n");
 //}
 
 /* =========================================================================
  * Stage 0i: PCIe ECAM scan
  * ====================================================================== */
 
 static void fw_stage_pcie(void)
 {
 #if defined(__aarch64__)
     uiox_fw_pcie_init(&s_pcie, UIOX_PCIE_ECAM_ARM64);
 #elif defined(__x86_64__)
     uiox_fw_pcie_init(&s_pcie, UIOX_PCIE_ECAM_X86_Q35);
 #else
     fw_memset_fw(&s_pcie, 0, sizeof(s_pcie));
     //fw_puts_main("[FW] PCIe    : no ECAM on ARM32\n");
     uiox_fw_printf("[FW] PCIe    : no ECAM on ARM32\n");
     return;
 #endif
     uiox_fw_pcie_scan(&s_pcie);
 
     /* Assign BARs and enable all found devices */
     for (uint32_t i = 0u; i < s_pcie.num_devices; i++) {
         uiox_fw_pcie_assign_bars(&s_pcie, &s_pcie.devices[i]);
         uiox_fw_pcie_enable_dev (&s_pcie, &s_pcie.devices[i]);
     }
 
     char buf[8];
     //fw_puts_main("[FW] PCIe    : ");
     uiox_fw_printf("[FW] PCIe    : ");
     /* print device count */
     uint32_t n = s_pcie.num_devices;
     char tmp[12]; int j = 0;
     if (n == 0u) { tmp[j++] = '0'; }
     else { while (n) { tmp[j++] = (char)('0' + n % 10u); n /= 10u; } }
     for (int k = j-1; k >= 0; k--) buf[j-1-k] = tmp[k];
     buf[j] = '\0';
     //fw_puts_main(buf);
     uiox_fw_printf(buf);
     //fw_puts_main(" device(s) found\n");
     uiox_fw_printf(" device(s) found\n");
 }
 
/* Simulated Root of Trust public-key hash (OTP / ROM constant on real HW) */
static const uint8_t s_rot_hash[32] = {
    0xDEu,0xADu,0xBEu,0xEFu, 0xCAu,0xFEu,0xBAu,0xBEu,
    0x01u,0x23u,0x45u,0x67u, 0x89u,0xABu,0xCDu,0xEFu,
    0xFEu,0xDCu,0xBAu,0x98u, 0x76u,0x54u,0x32u,0x10u,
    0xAAu,0xBBu,0xCCu,0xDDu, 0xEEu,0xFFu,0x00u,0x11u,
};
/* =========================================================================
 * Platform PSCI callbacks
 * ====================================================================== */
static void platform_cpu_on_cb(uint32_t cpu_id, uintptr_t entry)
{
    /* Real: write entry to per-CPU mailbox, assert power-on sequence */
    (void)cpu_id; (void)entry;
}
static void __attribute__((noreturn)) platform_reset_cb(void)
{
    uiox_fw_hw_reset();
}
//static void __attribute__((noreturn)) platform_off_cb(void)
//{
//    uiox_fw_power_shutdown();
//}
/* =========================================================================
 * Firmware printf shim — uses registered HAL UART (no libc)
 * ====================================================================== */
/*static void fw_puts_main(const char *s)
{
    const uiox_fw_hw_ops_t *hal = uiox_fw_hal_get();
    if (!hal || !hal->uart_putc) return;
    while (*s) {
        if (*s == '\n') hal->uart_putc('\r');
        hal->uart_putc(*s++);
    }
}*/
static void fw_puts_main(const char *s)
{
    /* uiox_fw_hw_ops() is declared in uiox_fw_hw.h — already included */
    const uiox_fw_hw_ops_t *ops = uiox_fw_hw_ops();
    if (!ops || !ops->uart_putc) return;
    while (*s) {
        if (*s == '\n') ops->uart_putc('\r');
        ops->uart_putc(*s++);
    }
}

 /* =========================================================================
  * Platform forward declarations
  * ====================================================================== */
 
 extern void uiox_fw_arch_register(void);
 
 /* Kernel entry point */
 extern void __attribute__((noreturn)) uiox_kernel_main(uint64_t dtb_pa);
 
 /* =========================================================================
  * Firmware-global state
  * ====================================================================== */
 
 //static uiox_fw_mem_map_t    s_mem_map;
 static uiox_fw_timer_t      s_timer;
 static uiox_fw_uart_t       s_console;
 static uiox_fw_gpio_t       s_gpio;
 //static uiox_fw_power_ctx_t  s_power;
 static uiox_fw_devsw_t      s_devsw;
 
 /* =========================================================================
  * Firmware printf helpers — no 64-bit division
  * ====================================================================== */
 
/*
 * Software 32-bit unsigned divide — NO / or % operator.
 * Eliminates __aeabi_uidiv, __aeabi_uidivmod, __aeabi_uldivmod.
 */
static uint32_t udiv32_softp(uint32_t n, uint32_t d, uint32_t *rem_out)
{
    uint32_t q = 0u, r = 0u;
    if (d == 0u) { if (rem_out) *rem_out = 0u; return 0u; }
    for (int i = 31; i >= 0; i--) {
        r = (r << 1u) | ((n >> (uint32_t)i) & 1u);
        if (r >= d) { r -= d; q |= (1u << (uint32_t)i); }
    }
    if (rem_out) *rem_out = r;
    return q;
}
/*
 * 64-bit divide by 32-bit using only 32-bit operations.
 * Avoids __aeabi_uldivmod entirely.
 */
static uint64_t div64_by_u32(uint64_t n, uint32_t d, uint32_t *rem_out)
{
    uint32_t hi  = (uint32_t)(n >> 32u);
    uint32_t lo  = (uint32_t)(n);
    uint32_t rh  = 0u;
    uint32_t qh  = udiv32_softp(hi, d, &rh);
    uint32_t tmp, qm, rm, ql, rl;
    /* upper 16 bits of lo */
    tmp = (rh << 16u) | (lo >> 16u);
    qm  = udiv32_softp(tmp, d, &rm);
    /* lower 16 bits of lo */
    tmp = (rm << 16u) | (lo & 0xFFFFu);
    ql  = udiv32_softp(tmp, d, &rl);
    if (rem_out) *rem_out = rl;
    return ((uint64_t)qh << 32u) | ((uint64_t)(qm << 16u) | ql);
}


  
 static void fw_puthex(uint64_t v, int width)
 {
     static const char h[] = "0123456789abcdef";
     char buf[16]; int n = 0;
     do { buf[n++] = h[(uint8_t)(v & 0xFu)]; v >>= 4; } while (v);
     while (n < width) buf[n++] = '0';
     for (int i = n - 1; i >= 0; i--)
         uiox_fw_hw_uart_putc(buf[i]);
 }
 
 static void fw_putdec_u32(uint32_t v)
 {
     char buf[12]; int n = 0;
     if (v == 0u) { uiox_fw_hw_uart_putc('0'); return; }
     uint32_t rem = 0u;
     do {
         v = udiv32_softp(v, 10u, &rem);
         buf[n++] = (char)('0' + rem);
     } while (v != 0u);
     for (int i = n - 1; i >= 0; i--)
         uiox_fw_hw_uart_putc(buf[i]);
 }
 
 static void fw_putdec_u64(uint64_t v)
{
    if (v == 0u) { uiox_fw_hw_uart_putc('0'); return; }
    /* Split into three groups of 9 decimal digits using
     * div64_by_u32 — zero 64-bit division operators. */
    static const uint32_t BILLION = 1000000000u;
    uint32_t rem = 0u;
    uint64_t q;
    q = div64_by_u32(v, BILLION, &rem);
    uint32_t bot = rem;
    q = div64_by_u32(q, BILLION, &rem);
    uint32_t mid = rem;
    uint32_t top = (uint32_t)q;   /* <= 18 for max uint64 */
    char buf[20]; int n = 0;
    /* bottom group */
    {
        uint32_t tmp = bot, r2 = 0u;
        do {
            tmp = udiv32_softp(tmp, 10u, &r2);
            buf[n++] = (char)('0' + r2);
        } while (tmp != 0u);
    }
    if (top != 0u || mid != 0u) {
        while (n < 9) buf[n++] = '0';
        /* middle group */
        uint32_t tmp = mid, r2 = 0u;
        do {
            tmp = udiv32_softp(tmp, 10u, &r2);
            buf[n++] = (char)('0' + r2);
        } while (tmp != 0u);
    }
    if (top != 0u) {
        while (n < 18) buf[n++] = '0';
        /* top group */
        uint32_t tmp = top, r2 = 0u;
        do {
            tmp = udiv32_softp(tmp, 10u, &r2);
            buf[n++] = (char)('0' + r2);
        } while (tmp != 0u);
    }
    for (int i = n - 1; i >= 0; i--)
        uiox_fw_hw_uart_putc(buf[i]);
}

 void uiox_fw_putc(char c)
 {
     if (c == '\n') uiox_fw_hw_uart_putc('\r');
     uiox_fw_hw_uart_putc(c);
 }
 
 void uiox_fw_puts(const char *s)
 { if (s) while (*s) uiox_fw_putc(*s++); }
 #if 0
 void uiox_fw_printf(const char *fmt, ...)
 {
     va_list ap;
     va_start(ap, fmt);
     while (*fmt) {
         if (*fmt != '%') { uiox_fw_putc(*fmt++); continue; }
         fmt++;
         int  ll    = 0;
         int  width = 0;
         if (*fmt == 'l') { fmt++; if (*fmt == 'l') { ll = 1; fmt++; } }
         while (*fmt >= '0' && *fmt <= '9')
             { width = width * 10 + (*fmt - '0'); fmt++; }
         char spec = *fmt++;
         switch (spec) {
         case 'c':
             uiox_fw_putc((char)va_arg(ap, int));
             break;
         case 's': {
             const char *s = va_arg(ap, const char *);
             uiox_fw_puts(s ? s : "(null)");
             break;
         }
         case 'd': {
             int64_t v = ll ? va_arg(ap, int64_t)
                            : (int64_t)va_arg(ap, int);
             if (v < 0) { uiox_fw_putc('-');
                           fw_putdec_u64((uint64_t)(-v)); }
             else          fw_putdec_u64((uint64_t)v);
             break;
         }
         case 'u':
             if (ll) fw_putdec_u64(va_arg(ap, uint64_t));
             else    fw_putdec_u32(va_arg(ap, uint32_t));
             break;
         case 'x': case 'X':
             fw_puthex(ll ? va_arg(ap, uint64_t)
                          : (uint64_t)va_arg(ap, uint32_t),
                       width ? width : 1);
             break;
         case 'p':
             uiox_fw_puts("0x");
             fw_puthex((uint64_t)(uintptr_t)va_arg(ap, void *),
                        (int)(sizeof(uintptr_t) * 2));
             break;
         case '%': uiox_fw_putc('%'); break;
         default:  uiox_fw_putc('%'); uiox_fw_putc(spec); break;
         }
     }
     va_end(ap);
 }
 #endif
 /* =========================================================================
  * Stub char-device ops
  * ====================================================================== */
 
 static uiox_fw_err_t console_open (uint32_t minor, int flags)
 { UIOX_FW_UNUSED(minor); UIOX_FW_UNUSED(flags); return UIOX_FW_OK; }
 static void          console_close(uint32_t minor) { UIOX_FW_UNUSED(minor); }
 static int           console_read (uint32_t minor)
 { UIOX_FW_UNUSED(minor); return uiox_fw_uart_getc(&s_console); }
 static uiox_fw_err_t console_write(uint32_t minor, char c)
 { UIOX_FW_UNUSED(minor); uiox_fw_uart_putc(&s_console, c); return UIOX_FW_OK; }
 static uiox_fw_err_t console_ioctl(uint32_t minor, uint32_t cmd, uintptr_t arg)
 { UIOX_FW_UNUSED(minor); UIOX_FW_UNUSED(cmd); UIOX_FW_UNUSED(arg);
   return UIOX_FW_ERR_UNSUP; }
 
 static uiox_fw_err_t null_open  (uint32_t m, int f)
 { UIOX_FW_UNUSED(m); UIOX_FW_UNUSED(f); return UIOX_FW_OK; }
 static void          null_close (uint32_t m) { UIOX_FW_UNUSED(m); }
 static int           null_read  (uint32_t m) { UIOX_FW_UNUSED(m); return -1; }
 static uiox_fw_err_t null_write (uint32_t m, char c)
 { UIOX_FW_UNUSED(m); UIOX_FW_UNUSED(c); return UIOX_FW_OK; }
 static uiox_fw_err_t null_ioctl (uint32_t m, uint32_t cmd, uintptr_t a)
 { UIOX_FW_UNUSED(m); UIOX_FW_UNUSED(cmd); UIOX_FW_UNUSED(a);
   return UIOX_FW_ERR_UNSUP; }
 
 static int zero_read(uint32_t m) { UIOX_FW_UNUSED(m); return 0; }
 
 /* =========================================================================
  * Timer tick callback
  * ====================================================================== */
 
 static void fw_timer_tick_cb(void *priv)
 {
     UIOX_FW_UNUSED(priv);
 }
 
 /* =========================================================================
  * RAM-disk storage (1 MB)
  * ====================================================================== */
 
 static uint8_t s_ramdisk_storage[512u * 2048u];
 
 static uiox_fw_err_t ramdisk_read(void *priv, uint64_t lba,
                                     uint8_t *buf, uint32_t count)
 {
     uint8_t  *disk = (uint8_t *)priv;
     uint64_t  max  = sizeof(s_ramdisk_storage) / UIOX_FW_STOR_SECTOR_SIZE;
     if (!buf || lba + count > max) return UIOX_FW_ERR_OVERFLOW;
     uiox_fw_memcpy(buf,
                     disk + (size_t)(lba * UIOX_FW_STOR_SECTOR_SIZE),
                     (size_t)count * UIOX_FW_STOR_SECTOR_SIZE);
     return UIOX_FW_OK;
 }
 
 static uiox_fw_err_t ramdisk_write(void *priv, uint64_t lba,
                                      const uint8_t *buf, uint32_t count)
 {
     uint8_t  *disk = (uint8_t *)priv;
     uint64_t  max  = sizeof(s_ramdisk_storage) / UIOX_FW_STOR_SECTOR_SIZE;
     if (!buf || lba + count > max) return UIOX_FW_ERR_OVERFLOW;
     uiox_fw_memcpy(disk + (size_t)(lba * UIOX_FW_STOR_SECTOR_SIZE),
                     buf,
                     (size_t)count * UIOX_FW_STOR_SECTOR_SIZE);
     return UIOX_FW_OK;
 }
 
 /* =========================================================================
  * IRQ handler shims — correct signature, no cast-function-type warning
  *
  * uiox_fw_irq_handler_t is:  void (*)(uint32_t irq, void *priv)
  * uiox_fw_uart_irq()  takes:  (uiox_fw_uart_t *)
  * uiox_fw_timer_irq() takes:  (uiox_fw_timer_t *)
  *
  * These shims have the correct IRQ-manager signature and internally
  * call the real handlers — no function-pointer casts needed anywhere.
  * ====================================================================== */
 
 static void uart_irq_shim(uint32_t irq, void *priv)
 {
     UIOX_FW_UNUSED(irq);
     uiox_fw_uart_irq((uiox_fw_uart_t *)priv);
 }
 
 static void timer_irq_shim(uint32_t irq, void *priv)
 {
     UIOX_FW_UNUSED(irq);
     uiox_fw_timer_irq((uiox_fw_timer_t *)priv);
 }
 
 /* =========================================================================
  * uiox_fw_main — 8-stage firmware init pipeline
  * ====================================================================== */
 
/* ====================================================================
 * Stage 0 block — insert at the very top of uiox_fw_main(), before
 * the existing "Stage 1: HW init" comment.
 * ==================================================================== */
/* =========================================================================
 * uiox_fw_main — complete function with Stage 0 prepended
 * ====================================================================== */
void __attribute__((noreturn))
uiox_fw_main(uint64_t dtb_pa)
{
    #if 0 // as moded to soc instead of firmware
    /* ================================================================ */
    /* Stage 0a: TrustZone / EL3 setup                                  */
    /* ================================================================ */
    uiox_tz_cfg_t tz_cfg;
    fw_memset_fw(&tz_cfg, 0, sizeof(tz_cfg));  /* ← fw_memset, not memset */
#if defined(__aarch64__)
    tz_cfg.gic_dist_base      = 0x08000000u;
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
    tz_cfg.gic_dist_base      = 0x10140000u;
    tz_cfg.enable_gic_secure  = false;
#endif
    /* Non-fatal: QEMU typically starts at EL1, not EL3 */
    fw_memset_fw(&s_tz_report, 0, sizeof(s_tz_report));
    uiox_tz_result_t tz_rc = uiox_fw_tz_init(&tz_cfg, &s_tz_report);
    (void)tz_rc;
    /* ================================================================ */
    /* Stage 0b: PSCI registration                                       */
    /* ================================================================ */
    fw_memset_fw(&s_psci_ctx, 0, sizeof(s_psci_ctx));
    uiox_fw_psci_init(&s_psci_ctx, 4u, /*use_smc=*/false);
    uiox_fw_psci_set_cpu_on(&s_psci_ctx, platform_cpu_on_cb);
    uiox_fw_psci_set_reset (&s_psci_ctx, platform_reset_cb);
    uiox_fw_psci_set_off   (&s_psci_ctx, platform_off_cb);
    /* ================================================================ */
    /* Stage 1: HW init                                                  */
    /* Register arch vtable → UART becomes live after this call.        */
    /* ================================================================ */
    UIOX_FW_HW_REGISTER();   /* ← corrected macro name, no uiox_boot_ */
    /* Console is now live — print banner */
    fw_puts_main("\r\n" UIOX_FW_VERSION_STR " [" UIOX_FW_URL "]\r\n");
    fw_puts_main("Arch: " UIOX_FW_ARCH_STR "\r\n");
    /* Print TZ + PSCI results */
    uiox_fw_tz_print  (&s_tz_report);
    uiox_fw_psci_print(&s_psci_ctx);
    /* ================================================================ */
    /* Stage 0c: POST                                                    */
    /* (after Stage 1 so UART is available for the report)              */
    /* ================================================================ */
    uiox_post_cfg_t post_cfg;
    fw_memset_fw(&post_cfg, 0, sizeof(post_cfg));  /* ← fw_memset */
    post_cfg.test_mask = UIOX_POST_TEST_CPU   |
                          UIOX_POST_TEST_RAM   |
                          UIOX_POST_TEST_TIMER |
                          UIOX_POST_TEST_GIC   |
                          UIOX_POST_TEST_STACK;
    /* Stack sentinel — written at firmware startup */
    post_cfg.stack_base     = (uintptr_t)_fw_stack_base;  /* extern sym */
    post_cfg.stack_sentinel = 0xDEADC0DEu;
    uiox_fw_post_stack_mark(post_cfg.stack_base, post_cfg.stack_sentinel);
#if defined(__aarch64__)
    post_cfg.gic_dist_base  = 0x08000000u;
    post_cfg.ram[0].base    = 0x40000000ULL;
    post_cfg.ram[0].size    = 0x04000000ULL;
    post_cfg.ram_count      = 1u;
#elif defined(__arm__)
    post_cfg.gic_dist_base  = 0x10140000u;
    post_cfg.ram[0].base    = 0x00100000ULL;
    post_cfg.ram[0].size    = 0x00F00000ULL;
    post_cfg.ram_count      = 1u;
#else
    post_cfg.gic_dist_base  = 0u;
    post_cfg.ram[0].base    = 0x00100000ULL;
    post_cfg.ram[0].size    = 0x03F00000ULL;
    post_cfg.ram_count      = 1u;
#endif
    fw_memset_fw(&s_post_report, 0, sizeof(s_post_report));
    uiox_post_result_t post_rc =
        uiox_fw_post_run(&post_cfg, &s_post_report);
    uiox_fw_post_print(&s_post_report);
    if (post_rc != UIOX_POST_OK) {
        fw_puts_main("[FW] FATAL: POST failed\r\n");
        for (;;) ;
    }
    /* ================================================================ */
    /* Stage 0d: Secure Boot                                             */
    /* ================================================================ */
    fw_memset_fw(&s_secboot_ctx,    0, sizeof(s_secboot_ctx));
    fw_memset_fw(&s_secboot_report, 0, sizeof(s_secboot_report));
    uiox_fw_secboot_init(&s_secboot_ctx,
                          s_rot_hash,
                          UIOX_SIG_RSA2048,
                          /*sim_mode=*/true);
    if (!s_secboot_ctx.debug_mode) {
        /* Production: uiox_fw_secboot_verify_cert + verify_image here */
        uiox_fw_secboot_print(&s_secboot_report);
        if (s_secboot_report.result != UIOX_SECBOOT_OK) {
            fw_puts_main("[FW] FATAL: Secure Boot failed\r\n");
            for (;;) ;
        }
    } else {
        fw_puts_main("[SECBOOT] Simulation — verification skipped\r\n");
    }
        #endif  /* 0: as moded to soc instead of firmware */
    /* ================================================================ */
    /* Stage 0e–0i: New peripheral HAL init                             */
    /* (run after UART is live, after POST + secure boot)               */
    /* ================================================================ */
#if 0
    fw_stage_i2c();
    fw_stage_spi();
    fw_stage_wdt();
    fw_stage_dma();
    fw_stage_pcie();

    /* WDT kick — firmware init takes a few seconds, prevent false reset */
    uiox_fw_wdt_kick(&s_wdt);

    /* ================================================================ */
    /* Stages 2–8: existing firmware pipeline (unchanged below here)    */
    /* ================================================================ */
    /* ... rest of original uiox_fw_main() stages 2–8 ... */
    /* ================================================================ */
    /* Stages 1–8: existing firmware pipeline (unchanged)               */
    /* ================================================================ */
    /* ... (existing Stage 1 through Stage 8 code follows) ... */
     /* ================================================================== */
     /* Stage 1: Platform HW init                                           */
     /* ================================================================== */
 
     uiox_fw_arch_register();
 
     {
         uiox_fw_uart_cfg_t cfg = UIOX_FW_UART_CFG_DEFAULT;
 #if defined(__aarch64__)
         uiox_fw_uart_init(&s_console,
                            UIOX_MEM_ARM64_UART0,
                            /*is_pl011=*/true,
                            UIOX_IRQ_ARM64_UART0, &cfg);
 #elif defined(__arm__)
         uiox_fw_uart_init(&s_console,
                            UIOX_MEM_ARM32_UART0,
                            /*is_pl011=*/true,
                            UIOX_IRQ_ARM32_UART0, &cfg);
 #else
         uiox_fw_uart_init(&s_console,
                            X86_COM1_PORT,
                            /*is_pl011=*/false,
                            UIOX_IRQ_X86_COM1, &cfg);
 #endif
     }
 
     uiox_fw_printf("\n" UIOX_FW_VERSION_STR " [" UIOX_FW_URL "]\n");
     uiox_fw_printf("Stage 1: HW init  arch=%s\n",
 #if   defined(__aarch64__)
                     "ARM64"
 #elif defined(__arm__)
                     "ARM32"
 #else
                     "x86_64"
 #endif
                    ); 
     /* ================================================================== */
     /* Stage 2: Memory map                                                 */
     /* ================================================================== */
 
     uiox_fw_printf("Stage 2: Memory\n");
     uiox_fw_err_t rc = uiox_fw_mem_init(&s_mem_map, dtb_pa);
     if (rc != UIOX_FW_OK)
         FW_FATAL("mem_init failed (%d)", (int)rc);
     uiox_fw_mem_print(&s_mem_map);
     uiox_fw_mem_mmu_early();
 
     /* ================================================================== */
     /* Stage 3: IRQ manager                                                */
     /* ================================================================== */
 
     uiox_fw_printf("Stage 3: IRQ\n");
     rc = uiox_fw_irq_init();
     if (rc != UIOX_FW_OK)
         FW_FATAL("irq_init failed (%d)", (int)rc);
 
     /* Register UART RX handler via shim — no cast, no warning */
 #if defined(__aarch64__)
     uiox_fw_irq_register(UIOX_IRQ_ARM64_UART0, uart_irq_shim, &s_console);
 #elif defined(__arm__)
     uiox_fw_irq_register(UIOX_IRQ_ARM32_UART0, uart_irq_shim, &s_console);
 #else
     uiox_fw_irq_register(UIOX_IRQ_X86_COM1 + UIOX_IRQ_X86_REMAP_BASE,
                           uart_irq_shim, &s_console);
 #endif
 
     uiox_fw_hw_irq_global_en();
 
     /* ================================================================== */
     /* Stage 4: Timers                                                     */
     /* ================================================================== */
 
     uiox_fw_printf("Stage 4: Timers\n");
 
 #if defined(__aarch64__)
     rc = uiox_fw_timer_init(&s_timer,
                              UIOX_FW_TIMER_ARM_GT,
                              0u,
                              UIOX_IRQ_ARM64_TIMER0,
                              100u);
 #elif defined(__arm__)
     rc = uiox_fw_timer_init(&s_timer,
                              UIOX_FW_TIMER_SP804,
                              UIOX_MEM_ARM32_TIMER0,
                              UIOX_IRQ_ARM32_TIMER0,
                              100u);
 #else
     rc = uiox_fw_timer_init(&s_timer,
                              UIOX_FW_TIMER_PIT,
                              X86_COM1_PORT,
                              UIOX_IRQ_X86_TIMER + UIOX_IRQ_X86_REMAP_BASE,
                              100u);
 #endif
     if (rc != UIOX_FW_OK)
         FW_FATAL("timer_init failed (%d)", (int)rc);
 
     uiox_fw_timer_set_cb(&s_timer, fw_timer_tick_cb, NULL);
 
     /* Register timer IRQ handler via shim — one registration only */
 #if defined(__aarch64__)
     uiox_fw_irq_register(UIOX_IRQ_ARM64_TIMER0,  timer_irq_shim, &s_timer);
 #elif defined(__arm__)
     uiox_fw_irq_register(UIOX_IRQ_ARM32_TIMER0,  timer_irq_shim, &s_timer);
 #else
     uiox_fw_irq_register(UIOX_IRQ_X86_TIMER + UIOX_IRQ_X86_REMAP_BASE,
                           timer_irq_shim, &s_timer);
 #endif
 
     uiox_fw_printf("  Timer: %u Hz tick\n", s_timer.hz);
 
     /* ================================================================== */
     /* Stage 5: GPIO                                                       */
     /* ================================================================== */
 
     uiox_fw_printf("Stage 5: GPIO\n");
 #if defined(__aarch64__)
     rc = uiox_fw_gpio_init(&s_gpio,
                             UIOX_MEM_ARM64_GPIO,
                             UIOX_IRQ_ARM64_GPIO,
                             8u);
 #elif defined(__arm__)
     rc = uiox_fw_gpio_init(&s_gpio,
                             UIOX_MEM_ARM32_GPIO,
                             UIOX_IRQ_ARM32_GPIO,
                             8u);
 #else
     rc = UIOX_FW_OK;
     uiox_fw_memset(&s_gpio, 0, sizeof(s_gpio));
     uiox_fw_printf("  GPIO: no dedicated GPIO on x86 q35 (stub)\n");
 #endif
     if (rc != UIOX_FW_OK)
         FW_FATAL("gpio_init failed (%d)", (int)rc);
 
     /* ================================================================== */
     /* Stage 6: Storage                                                    */
     /* ================================================================== */
 
     uiox_fw_printf("Stage 6: Storage\n");
     uiox_fw_stor_init();
 
     static uiox_fw_stor_dev_t s_ram_dev;
     uiox_fw_memset(&s_ram_dev, 0, sizeof(s_ram_dev));
     s_ram_dev.type        = UIOX_FW_STOR_RAMDISK;
     s_ram_dev.num_sectors =
         sizeof(s_ramdisk_storage) / UIOX_FW_STOR_SECTOR_SIZE;
     s_ram_dev.sector_size = UIOX_FW_STOR_SECTOR_SIZE;
     s_ram_dev.present     = true;
     s_ram_dev.read_only   = false;
     s_ram_dev.priv        = s_ramdisk_storage;
     s_ram_dev.read        = ramdisk_read;
     s_ram_dev.write       = ramdisk_write;
     s_ram_dev.flush       = NULL;
 
     /* Copy device name without string.h */
     static const char ramdisk_name[] = "ramdisk";
     for (int i = 0; i < (int)sizeof(ramdisk_name); i++)
         s_ram_dev.name[i] = ramdisk_name[i];
 
     uiox_fw_stor_register(&s_ram_dev);
     uiox_fw_stor_print();
 
     /* ================================================================== */
     /* Stage 7: Device switch table                                        */
     /* ================================================================== */
 
     uiox_fw_printf("Stage 7: DevSw\n");
     uiox_fw_devsw_init(&s_devsw);
 
     static const uiox_fw_cdevsw_t c_console = {
         .name   = "console",
         .major  = UIOX_DEV_MAJOR_CONSOLE,
         .copen  = console_open,
         .cclose = console_close,
         .cread  = console_read,
         .cwrite = console_write,
         .cioctl = console_ioctl,
         .ready  = true,
     };
     static const uiox_fw_cdevsw_t c_null = {
         .name   = "null",
         .major  = UIOX_DEV_MAJOR_NULL,
         .copen  = null_open,
         .cclose = null_close,
         .cread  = null_read,
         .cwrite = null_write,
         .cioctl = null_ioctl,
         .ready  = true,
     };
     static const uiox_fw_cdevsw_t c_zero = {
         .name   = "zero",
         .major  = UIOX_DEV_MAJOR_ZERO,
         .copen  = null_open,
         .cclose = null_close,
         .cread  = zero_read,
         .cwrite = null_write,
         .cioctl = null_ioctl,
         .ready  = true,
     };
 
     uiox_fw_cdev_register(&s_devsw, &c_console);
     uiox_fw_cdev_register(&s_devsw, &c_null);
     uiox_fw_cdev_register(&s_devsw, &c_zero);
 
     static uiox_fw_bdevsw_t b_ram = {
         .name       = "ram0",
         .major      = UIOX_DEV_MAJOR_RAM,
         .bopen      = NULL,
         .bclose     = NULL,
         .strategy   = NULL,
         .bioctl     = NULL,
         .num_blocks = (512u * 2048u) / UIOX_FW_STOR_SECTOR_SIZE,
         .block_size = UIOX_FW_STOR_SECTOR_SIZE,
         .ready      = true,
     };
     uiox_fw_bdev_register(&s_devsw, &b_ram);
     uiox_fw_devsw_print(&s_devsw);
 
     /* ================================================================== */
     /* Stage 8: Power init and hand-off to kernel                          */
     /* ================================================================== */
 
     uiox_fw_printf("Stage 8: Handoff → kernel\n");
 
     rc = uiox_fw_power_init(&s_power);
     if (rc != UIOX_FW_OK)
         FW_FATAL("power_init failed (%d)", (int)rc);
 
     uiox_fw_printf(UIOX_FW_VERSION_STR
                     " init complete — jumping to kernel\n");
 
     uiox_fw_hw_dsb();
     uiox_fw_hw_isb();
 
     //uiox_kernel_main(dtb_pa);
 
     for (;;) uiox_fw_power_idle();
     #endif
 }
 