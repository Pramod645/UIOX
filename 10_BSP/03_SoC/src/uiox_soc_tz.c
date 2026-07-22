/**
 * @file    uiox_soc_tz.c
 * @brief   UIOX SoC — TrustZone / EL3 secure world setup.
 *          Zero libc dependency — no string.h or stdio.h.
 * @date    2026-07-07
 */

 #include "../include/uiox_soc_tz.h"
 #include "../include/uiox_soc_hw.h"
 
 /* =========================================================================
  * Bare-metal helpers
  * ====================================================================== */
 
 static void soc_memset_tz(void *dst, int val, uiox_size_t n)
 {
     uiox_uint8_t *d = (uiox_uint8_t *)dst;
     while (n--) *d++ = (uiox_uint8_t)val;
 }
 
 static void soc_strncpy_tz(char *dst, const char *src, uiox_size_t n)
 {
    uiox_size_t i = 0u;
     while (i < n - 1u && src[i]) { dst[i] = src[i]; i++; }
     dst[i] = '\0';
 }
 
 static void soc_puts_tz(const char *s)
 {
     const uiox_soc_hw_ops_t *ops = uiox_soc_hw_ops();
     if (!ops || !ops->uart_putc) return;
     while (*s) {
         if (*s == '\n') ops->uart_putc('\r');
         ops->uart_putc(*s++);
     }
 }
 
 static char *soc_u64_hex_tz(uiox_uint64_t v, char *buf)
 {
     static const char h[] = "0123456789abcdef";
     for (int i = 15; i >= 0; i--) { buf[i] = h[v & 0xFu]; v >>= 4; }
     buf[16] = '\0';
     return buf;
 }
 
 static char *soc_u32_dec_tz(uiox_uint32_t v, char *buf)
 {
     char tmp[12]; int n = 0;
     if (v == 0u) { buf[0] = '0'; buf[1] = '\0'; return buf; }
     while (v && n < 11) { tmp[n++] = (char)('0' + v % 10u); v /= 10u; }
     int j = 0;
     for (int i = n - 1; i >= 0; i--) buf[j++] = tmp[i];
     buf[j] = '\0';
     return buf;
 }
 
 /* =========================================================================
  * Current Exception Level (ARM only)
  * ====================================================================== */
 
  uiox_uint32_t uiox_soc_tz_current_el(void)
 {
 #if defined(__aarch64__)
     uiox_uint64_t el;
     __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
     return (uiox_uint32_t)((el >> 2u) & 0x3u);
 #elif defined(__arm__)
     /* Read CPSR and extract mode bits */
     uiox_uint32_t cpsr;
     __asm__ volatile("mrs %0, cpsr" : "=r"(cpsr));
     /* Monitor mode (0x16) = EL3, Hyp (0x1A) = EL2, else EL1 */
     uiox_uint32_t mode = cpsr & 0x1Fu;
     if (mode == 0x16u) return 3u;
     if (mode == 0x1Au) return 2u;
     return 1u;
 #else
     /* x86_64: no EL concept — return 0 to indicate not applicable */
     return 0u;
 #endif
 }
 
 /* =========================================================================
  * VBAR install
  * ====================================================================== */
 
 uiox_soc_tz_result_t uiox_soc_tz_set_vbar(uiox_uintptr_t vbar_pa)
 {
 #if defined(__aarch64__)
     __asm__ volatile("msr vbar_el3, %0; isb" :: "r"(vbar_pa) : "memory");
     return UIOX_SOC_TZ_OK;
 #elif defined(__arm__)
     /* Monitor VBAR (MVBAR) */
     __asm__ volatile("mcr p15,0,%0,c12,c0,1; isb"
                      :: "r"(vbar_pa) : "memory");
     return UIOX_SOC_TZ_OK;
 #else
     (void)vbar_pa;
     return UIOX_SOC_TZ_OK;   /* x86: IDT is set up separately in idt.c */
 #endif
 }
 
 /* =========================================================================
  * GIC secure group configuration
  * ====================================================================== */
 
 uiox_soc_tz_result_t uiox_soc_tz_gic_secure(uiox_uintptr_t gicd_base,
                                                uiox_uint32_t  num_irqs,
                                                uiox_uint32_t  secure_irq_mask)
 {
     if (gicd_base == 0u) return UIOX_SOC_TZ_ERR_GIC;
 
     /* GICD_IGROUPR: 1 = NS Group 1, 0 = Secure Group 0
      * Write ~secure_irq_mask to first register to set secure IRQs */
     uiox_uint32_t regs = (num_irqs + 31u) / 32u;
     for (uiox_uint32_t i = 0u; i < regs; i++) {
         uiox_uintptr_t addr = gicd_base + 0x80u + (uiox_uintptr_t)(i * 4u);
         /* First register: apply secure_irq_mask; rest: all NS */
         uiox_uint32_t val = (i == 0u) ? ~secure_irq_mask : 0xFFFFFFFFu;
         soc_mmio_write32(addr, val);
     }
     return UIOX_SOC_TZ_OK;
 }
 
 /* =========================================================================
  * TZC-400 region configuration
  * ====================================================================== */
 
 uiox_soc_tz_result_t uiox_soc_tzc_set_region(uiox_uintptr_t                   tzc_base,
                                                uiox_uint32_t                    region_id,
                                                const uiox_soc_tzc_region_t *r)
 {
     if (tzc_base == 0u || !r) return UIOX_SOC_TZ_ERR_TZC;
     if (region_id >= UIOX_SOC_TZ_MAX_REGIONS) return UIOX_SOC_TZ_ERR_INVAL;
 
     /*
      * TZC-400 register layout (per ARM IHI0049B):
      *   REGION_BASE_LOW   = base + 0x100 + n*0x20
      *   REGION_BASE_HIGH  = base + 0x104 + n*0x20
      *   REGION_TOP_LOW    = base + 0x108 + n*0x20
      *   REGION_TOP_HIGH   = base + 0x10C + n*0x20
      *   REGION_ATTRIBUTES = base + 0x110 + n*0x20
      *   REGION_ID_ACCESS  = base + 0x114 + n*0x20
      */
     uiox_uintptr_t rbase = tzc_base + 0x100u + (uiox_uintptr_t)(region_id * 0x20u);
 
     soc_mmio_write32(rbase + 0x00u, (uiox_uint32_t)(r->base & 0xFFFFFFFFu));
     soc_mmio_write32(rbase + 0x04u, (uiox_uint32_t)(r->base >> 32u));
     soc_mmio_write32(rbase + 0x08u, (uiox_uint32_t)(r->top  & 0xFFFFFFFFu));
     soc_mmio_write32(rbase + 0x0Cu, (uiox_uint32_t)(r->top  >> 32u));
 
     /* Attributes: [1:0] = filter enable, [31] = region enable */
     uiox_uint32_t attr = 0x3u | (1u << 31u);
     if (r->secure_only) attr |= (1u << 2u);
     soc_mmio_write32(rbase + 0x10u, attr);
 
     /* ID access: NSAID read/write enable bitmasks */
     uiox_uint32_t id_access = ((uiox_uint32_t)r->nsaid_rd_en << 16u) |
                           (uiox_uint32_t)r->nsaid_wr_en;
     soc_mmio_write32(rbase + 0x14u, id_access);
 
     return UIOX_SOC_TZ_OK;
 }
 
 /* =========================================================================
  * ERET to non-secure world
  * ====================================================================== */
 
 void __attribute__((noreturn))
 uiox_soc_tz_eret_to_ns(uiox_uintptr_t entry, uiox_uint64_t spsr, uiox_uint64_t x0_arg)
 {
 #if defined(__aarch64__)
     __asm__ volatile(
         "msr elr_el3,  %0   \n"
         "msr spsr_el3, %1   \n"
         "mov x0,       %2   \n"
         "eret               \n"
         :: "r"(entry), "r"(spsr), "r"(x0_arg)
         : "x0", "memory"
     );
     #elif defined(__arm__)
     /*
      * ARM32 Monitor-mode ERET via banked registers (lr_mon, spsr_mon)
      * requires Security Extensions (-march=armv7-a+sec).
      * On QEMU versatilepb / virt the firmware starts at SVC mode,
      * so Monitor mode is not available and this path is never reached.
      *
      * Use an indirect branch via a function pointer instead, which
      * compiles with any ARMv7-A variant and achieves the same result
      * when called from SVC/HYP mode.
      */
     (void)spsr; (void)x0_arg;
     if (entry) {
         /* Cast entry to a void function and jump to it */
         void (*fn)(void) = (void (*)(void))(uiox_uintptr_t)entry;
         fn();
     }
 #else
     (void)entry; (void)spsr; (void)x0_arg;
     /* x86: no ERET — fall through to halt */
 #endif
     for (;;)
#if defined(__aarch64__) || defined(__arm__)
        __asm__ volatile("wfi" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
        __asm__ volatile("hlt" ::: "memory");
#elif defined(__riscv)
        __asm__ volatile("wfi" ::: "memory");
#else
        __asm__ volatile("" ::: "memory");
#endif
 }
 
 /* =========================================================================
  * uiox_soc_tz_init — master EL3 setup sequencer
  * ====================================================================== */
 
 uiox_soc_tz_result_t uiox_soc_tz_init(const uiox_soc_tz_cfg_t *cfg,
                                         uiox_soc_tz_report_t    *report)
 {
     uiox_soc_tz_report_t local;
     uiox_soc_tz_report_t *r = report ? report : &local;
     soc_memset_tz(r, 0, sizeof(*r));
     r->result = UIOX_SOC_TZ_OK;
 
     if (!cfg) {
         r->result = UIOX_SOC_TZ_ERR_INVAL;
         soc_strncpy_tz(r->fail_msg, "cfg is NULL", sizeof(r->fail_msg));
         return r->result;
     }
 
     r->current_el = uiox_soc_tz_current_el();
 
 #if defined(__aarch64__) || defined(__arm__)
     /* Step 1: verify we are at EL3 / Monitor mode */
     if (r->current_el < 3u) {
         r->result = UIOX_SOC_TZ_ERR_EL;
         soc_strncpy_tz(r->fail_msg,
                         "Not running at EL3 / Monitor mode",
                         sizeof(r->fail_msg));
         return r->result;
     }
 
     /* Step 2: configure SCR_EL3 */
 #if defined(__aarch64__)
     uiox_uint64_t scr = SCR_EL3_SOC_VALUE;
     if (cfg->enable_fiq_routing) scr |= SCR_EL3_FIQ;
     __asm__ volatile("msr scr_el3, %0; isb" :: "r"(scr) : "memory");
     r->scr_el3_value = scr;
 
     /* Step 3: configure CPTR_EL3 — allow FP/SVE from EL1/EL2 */
     uiox_uint64_t cptr = CPTR_EL3_SOC_VALUE;
     __asm__ volatile("msr cptr_el3, %0; isb" :: "r"(cptr) : "memory");
     r->cptr_el3_value = cptr;
     r->fpu_enabled    = true;
 #endif
 
     /* Step 4: install EL3 exception vector table */
     if (cfg->el3_vbar) {
         uiox_soc_tz_result_t vrc = uiox_soc_tz_set_vbar(cfg->el3_vbar);
         if (vrc != UIOX_SOC_TZ_OK) {
             r->result = vrc;
             soc_strncpy_tz(r->fail_msg, "VBAR install failed",
                             sizeof(r->fail_msg));
             return r->result;
         }
     }
 
     /* Step 5: configure GIC secure groups */
     if (cfg->enable_gic_secure && cfg->gic_dist_base) {
         uiox_soc_tz_result_t grc = uiox_soc_tz_gic_secure(
                                         cfg->gic_dist_base,
                                         256u,     /* assume 256 IRQs */
                                         0xFFu);   /* first 8 = secure */
         if (grc != UIOX_SOC_TZ_OK) {
             r->result = grc;
             soc_strncpy_tz(r->fail_msg, "GIC secure config failed",
                             sizeof(r->fail_msg));
             return r->result;
         }
         r->gic_groups_set = 1u;
     }
 
     /* Step 6: configure TZC-400 regions */
     if (cfg->enable_tzc && cfg->tzc_base) {
         for (uiox_uint32_t i = 0u; i < cfg->tzc_region_count; i++) {
             uiox_soc_tz_result_t trc =
                 uiox_soc_tzc_set_region(cfg->tzc_base, i,
                                          &cfg->tzc_regions[i]);
             if (trc != UIOX_SOC_TZ_OK) {
                 r->result = trc;
                 soc_strncpy_tz(r->fail_msg, "TZC region config failed",
                                 sizeof(r->fail_msg));
                 return r->result;
             }
         }
         r->tzc_regions_set = cfg->tzc_region_count;
     }
 
     /* Step 7: secure DRAM region — mark as TZC secure-only if TZC present */
     if (cfg->enable_tzc && cfg->tzc_base &&
         cfg->secure_dram_size > 0u) {
         uiox_soc_tzc_region_t sec_region = {
             .base        = cfg->secure_dram_base,
             .top         = cfg->secure_dram_base +
                            cfg->secure_dram_size - 1u,
             .nsaid_rd_en = 0x00u,  /* no NS read access */
             .nsaid_wr_en = 0x00u,  /* no NS write access */
             .secure_only = true,
         };
         uiox_soc_tzc_set_region(cfg->tzc_base,
                                   cfg->tzc_region_count,
                                   &sec_region);
     }
 
     r->smc_enabled = true;
     soc_puts_tz("[SOC] TZ init OK — ERET to NS entry 0x");
     char buf[20];
     soc_puts_tz(soc_u64_hex_tz((uiox_uint64_t)cfg->ns_entry_addr, buf));
     soc_puts_tz("\n");
 
 #else   /* x86_64 */
     /*
      * x86_64: TrustZone does not apply.
      * SMM protection would be configured here in a real port.
      * Report success immediately.
      */
     r->current_el  = 0u;
     r->smc_enabled = false;
     soc_puts_tz("[SOC] TZ init: x86_64 stub — SMM not configured\n");
 #endif  /* arch */
 
     return r->result;
 }
 
 /* =========================================================================
  * uiox_soc_tz_print
  * ====================================================================== */
 
 void uiox_soc_tz_print(const uiox_soc_tz_report_t *r)
 {
     if (!r) return;
     char buf[20];
     soc_puts_tz("[SOC] TZ report:\n");
     soc_puts_tz("  result         : ");
     soc_puts_tz(r->result == UIOX_SOC_TZ_OK ? "OK" : "FAIL");
     soc_puts_tz("\n  current_el     : EL");
     soc_puts_tz(soc_u32_dec_tz(r->current_el, buf));
     soc_puts_tz("\n  scr_el3        : 0x");
     soc_puts_tz(soc_u64_hex_tz(r->scr_el3_value, buf));
     soc_puts_tz("\n  cptr_el3       : 0x");
     soc_puts_tz(soc_u64_hex_tz(r->cptr_el3_value, buf));
     soc_puts_tz("\n  fpu_enabled    : ");
     soc_puts_tz(r->fpu_enabled   ? "yes" : "no");
     soc_puts_tz("\n  smc_enabled    : ");
     soc_puts_tz(r->smc_enabled   ? "yes" : "no");
     soc_puts_tz("\n  gic_groups_set : ");
     soc_puts_tz(soc_u32_dec_tz(r->gic_groups_set, buf));
     soc_puts_tz("\n  tzc_regions    : ");
     soc_puts_tz(soc_u32_dec_tz(r->tzc_regions_set, buf));
     soc_puts_tz("\n");
     if (r->result != UIOX_SOC_TZ_OK) {
         soc_puts_tz("  fail_msg       : ");
         soc_puts_tz(r->fail_msg);
         soc_puts_tz("\n");
     }
 }
 