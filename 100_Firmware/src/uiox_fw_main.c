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
 
 /* =========================================================================
  * Platform forward declarations
  * ====================================================================== */
 
 extern void uiox_fw_arch_register(void);
 
 /* Kernel entry point */
 extern void __attribute__((noreturn)) uiox_kernel_main(uint64_t dtb_pa);
 
 /* =========================================================================
  * Firmware-global state
  * ====================================================================== */
 
 static uiox_fw_mem_map_t    s_mem_map;
 static uiox_fw_timer_t      s_timer;
 static uiox_fw_uart_t       s_console;
 static uiox_fw_gpio_t       s_gpio;
 static uiox_fw_power_ctx_t  s_power;
 static uiox_fw_devsw_t      s_devsw;
 
 /* =========================================================================
  * Firmware printf helpers — no 64-bit division
  * ====================================================================== */
 
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
     do { buf[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
     for (int i = n - 1; i >= 0; i--)
         uiox_fw_hw_uart_putc(buf[i]);
 }
 
 static void fw_putdec_u64(uint64_t v)
 {
     if (v == 0u) { uiox_fw_hw_uart_putc('0'); return; }
     uint32_t bot = (uint32_t)(v % 1000000000u);
     uint32_t mid = (uint32_t)((v / 1000000000u) % 1000000000u);
     uint32_t top = (uint32_t)(v / 1000000000000000000u);
     char buf[28]; int n = 0;
     uint32_t tmp;
     tmp = bot;
     do { buf[n++] = (char)('0' + tmp % 10u); tmp /= 10u; } while (tmp);
     if (top || mid) {
         while (n < 9) buf[n++] = '0';
         tmp = mid;
         do { buf[n++] = (char)('0' + tmp % 10u); tmp /= 10u; } while (tmp);
     }
     if (top) {
         while (n < 18) buf[n++] = '0';
         tmp = top;
         do { buf[n++] = (char)('0' + tmp % 10u); tmp /= 10u; } while (tmp);
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
 
 void __attribute__((noreturn)) uiox_fw_main(uint64_t dtb_pa)
 {
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
                            Q35_COM1_PORT,
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
                              Q35_PIT_PORT,
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
 
     uiox_kernel_main(dtb_pa);
 
     for (;;) uiox_fw_power_idle();
 }
 