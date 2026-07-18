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
 * uiox_fw_main — complete function with Stage 0 prepended
 * ====================================================================== */
void __attribute__((noreturn))
uiox_fw_main(uint64_t dtb_pa)
{
  
}
 