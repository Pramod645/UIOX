/**
 * @file  uiox_fw_tz.c
 * @brief UIOX Firmware — TrustZone / EL3 secure world setup.
 *        Zero libc dependency — no string.h or stdio.h.
 * @date  2026-07-07
 */

 #include "../include/uiox_fw_tz.h"
 #include "../include/uiox_fw_hw.h"
 
 /* =========================================================================
  * Bare-metal helpers
  * ====================================================================== */
 
 static void fw_memset_tz(void *dst, int val, size_t n)
 { uint8_t *d = (uint8_t *)dst; while (n--) *d++ = (uint8_t)val; }
 
 static void fw_strncpy_tz(char *dst, const char *src, size_t n)
 { size_t i=0u; while(i<n-1u && src[i]){dst[i]=src[i];i++;} dst[i]='\0'; }
 
 static void fw_puts_tz(const char *s)
{
    /* uiox_fw_hw_ops() is declared in uiox_fw_hw.h (already included) */
    const uiox_fw_hw_ops_t *ops = uiox_fw_hw_ops();
    if (!ops || !ops->uart_putc) return;
    while (*s) {
        if (*s == '\n') ops->uart_putc('\r');
        ops->uart_putc(*s++);
    }
}


 static char *fw_u64_hex_tz(uint64_t v, char *buf)
 {
     static const char h[] = "0123456789abcdef";
     for (int i = 15; i >= 0; i--) { buf[i] = h[v & 0xFu]; v >>= 4; }
     buf[16] = '\0'; return buf;
 }
 
 static char *fw_u32_dec_tz(uint32_t v, char *buf)
 {
     char tmp[12]; int n=0;
     if (v==0u){buf[0]='0';buf[1]='\0';return buf;}
     while(v&&n<11){tmp[n++]=(char)('0'+v%10u);v/=10u;}
     int j=0; for(int i=n-1;i>=0;i--) buf[j++]=tmp[i]; buf[j]='\0';
     return buf;
 }
 
 /* =========================================================================
  * EL query
  * ====================================================================== */
 
 uint32_t uiox_fw_tz_current_el(void)
 {
 #if defined(__aarch64__)
     uint64_t el;
     __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
     return (uint32_t)((el >> 2u) & 0x3u);
 #elif defined(__arm__)
     uint32_t cpsr;
     __asm__ volatile("mrs %0, cpsr" : "=r"(cpsr));
     return ((cpsr & 0x1Fu) == 0x16u) ? 3u : 1u;
 #else
     return 0u;
 #endif
 }
 
 /* =========================================================================
  * GIC secure group config
  * ====================================================================== */
 
 uiox_tz_result_t uiox_fw_tz_gic_secure(uintptr_t gicd_base,
                                           uint32_t num_irqs,
                                           uint32_t secure_irq_mask)
 {
     if (gicd_base == 0u) return UIOX_TZ_OK;
     volatile uint32_t *gicd = (volatile uint32_t *)gicd_base;
     gicd[0x000u / 4u] = 0u;  /* disable distributor */
     uint32_t num_regs = (num_irqs + 31u) / 32u;
     for (uint32_t i = 0u; i < num_regs; i++) {
         uint32_t ns_bits = 0xFFFFFFFFu;
         if (i == 0u) ns_bits &= ~secure_irq_mask;
         gicd[(0x080u + i * 4u) / 4u] = ns_bits;
     }
     gicd[0x000u / 4u] = 0x3u;  /* re-enable */
     return UIOX_TZ_OK;
 }
 
 /* =========================================================================
  * TZC-400 region
  * ====================================================================== */
 
 #define TZC_REGION_BASE_LO(n) (0x100u + (n)*0x20u + 0x00u)
 #define TZC_REGION_BASE_HI(n) (0x100u + (n)*0x20u + 0x04u)
 #define TZC_REGION_TOP_LO(n)  (0x100u + (n)*0x20u + 0x08u)
 #define TZC_REGION_TOP_HI(n)  (0x100u + (n)*0x20u + 0x0Cu)
 #define TZC_REGION_ATTR(n)    (0x100u + (n)*0x20u + 0x10u)
 #define TZC_ATTR_S_RD         (1u << 30)
 #define TZC_ATTR_S_WR         (1u << 31)
 #define TZC_REGION_EN         (1u << 0)
 
 uiox_tz_result_t uiox_fw_tzc_set_region(uintptr_t tzc_base,
                                           uint32_t region_id,
                                           const uiox_tzc_region_t *r)
 {
     if (tzc_base == 0u || !r || region_id > 8u) return UIOX_TZ_OK;
     volatile uint32_t *tzc = (volatile uint32_t *)tzc_base;
     tzc[TZC_REGION_BASE_LO(region_id)/4u] = (uint32_t)( r->base & 0xFFFFFFFFu);
     tzc[TZC_REGION_BASE_HI(region_id)/4u] = (uint32_t)((r->base>>32u) & 0xFFFFFFFFu);
     tzc[TZC_REGION_TOP_LO(region_id) /4u] = (uint32_t)( r->top  & 0xFFFFFFFFu);
     tzc[TZC_REGION_TOP_HI(region_id) /4u] = (uint32_t)((r->top >>32u) & 0xFFFFFFFFu);
     uint32_t attr = TZC_REGION_EN | TZC_ATTR_S_RD | TZC_ATTR_S_WR;
     if (!r->secure_only)
         attr |= ((uint32_t)r->nsaid_rd_en<<8u) | ((uint32_t)r->nsaid_wr_en<<24u);
     tzc[TZC_REGION_ATTR(region_id)/4u] = attr;
     return UIOX_TZ_OK;
 }
 
 /* =========================================================================
  * EL3 vector table
  * ====================================================================== */
 
 uiox_tz_result_t uiox_fw_tz_set_vbar(uintptr_t vbar_pa)
 {
     if (vbar_pa == 0u || (vbar_pa & 0x7FFu)) return UIOX_TZ_ERR_VECTORS;
 #if defined(__aarch64__)
     __asm__ volatile("msr vbar_el3, %0; isb" :: "r"(vbar_pa) : "memory");
 #elif defined(__arm__)
     __asm__ volatile("mcr p15,0,%0,c12,c0,1; isb"
                      :: "r"((uint32_t)vbar_pa));
 #endif
     return UIOX_TZ_OK;
 }
 
 /* =========================================================================
  * ERET to non-secure world
  * ====================================================================== */
 
 void __attribute__((noreturn))
 uiox_fw_tz_eret_to_ns(uintptr_t entry, uint64_t spsr, uint64_t x0_arg)
 {
 #if defined(__aarch64__)
     __asm__ volatile(
         "msr elr_el3,  %0\n"
         "msr spsr_el3, %1\n"
         "mov x0, %2\n"
         "mov x1, xzr\n"
         "mov x2, xzr\n"
         "mov x3, xzr\n"
         "eret\n"
         :: "r"((uint64_t)entry), "r"(spsr), "r"(x0_arg)
         : "x0","x1","x2","x3","memory");
 #elif defined(__arm__)
     __asm__ volatile(
         "msr lr_abt, %0\n"
         "msr spsr_mon, %1\n"
         "movs pc, lr\n"
         :: "r"((uint32_t)entry), "r"((uint32_t)spsr) : "memory");
 #endif
     for (;;) ;
 }
 
 /* =========================================================================
  * uiox_fw_tz_init
  * ====================================================================== */
 
 uiox_tz_result_t uiox_fw_tz_init(const uiox_tz_cfg_t *cfg,
                                     uiox_tz_report_t *report)
 {
     uiox_tz_report_t local;
     uiox_tz_report_t *r = report ? report : &local;
     fw_memset_tz(r, 0, sizeof(*r));
     r->current_el = uiox_fw_tz_current_el();
 
 #if defined(__aarch64__)
     if (r->current_el != 3u) {
         /* Not at EL3 — non-fatal on QEMU where firmware runs at EL1 */
         r->result = UIOX_TZ_ERR_EL;
         fw_strncpy_tz(r->fail_msg, "Not at EL3 (QEMU simulation)",
                       sizeof(r->fail_msg));
         return r->result;
     }
 
     /* SCR_EL3 */
     uint64_t scr = SCR_EL3_NS | SCR_EL3_RW | SCR_EL3_HCE;
     if (cfg && cfg->enable_fiq_routing) scr |= SCR_EL3_FIQ;
     __asm__ volatile("msr scr_el3, %0; isb" :: "r"(scr) : "memory");
     r->scr_el3_value = scr;
     r->smc_enabled   = !(scr & SCR_EL3_SMD);
 
     /* CPTR_EL3 */
     uint64_t cptr = CPTR_EL3_FW_VALUE;
     __asm__ volatile("msr cptr_el3, %0; isb" :: "r"(cptr) : "memory");
     r->cptr_el3_value = cptr;
     r->fpu_enabled    = !(cptr & CPTR_EL3_TFP);
 
     /* EL3 vector table */
     if (cfg && cfg->el3_vbar) {
         uiox_tz_result_t vr = uiox_fw_tz_set_vbar(cfg->el3_vbar);
         if (vr != UIOX_TZ_OK) {
             r->result = vr;
             fw_strncpy_tz(r->fail_msg, "VBAR_EL3 install failed",
                           sizeof(r->fail_msg));
             return r->result;
         }
     }
 
     /* GIC secure groups */
     if (cfg && cfg->enable_gic_secure && cfg->gic_dist_base) {
         uiox_fw_tz_gic_secure(cfg->gic_dist_base, 256u, 0x0000FFFFu);
         r->gic_groups_set = 1u;
     }
 
     /* TZC-400 */
     if (cfg && cfg->enable_tzc && cfg->tzc_base) {
         for (uint32_t i = 0u; i < cfg->tzc_region_count; i++) {
             uiox_fw_tzc_set_region(cfg->tzc_base, i + 1u,
                                     &cfg->tzc_regions[i]);
             r->tzc_regions_set++;
         }
     }
 
     /* Secure DRAM lock */
     if (cfg && cfg->secure_dram_size > 0u && cfg->tzc_base) {
         uiox_tzc_region_t sec = {
             .base        = cfg->secure_dram_base,
             .top         = cfg->secure_dram_base + cfg->secure_dram_size - 1u,
             .nsaid_rd_en = 0u,
             .nsaid_wr_en = 0u,
             .secure_only = true,
         };
         uiox_fw_tzc_set_region(cfg->tzc_base, 0u, &sec);
         r->tzc_regions_set++;
     }
 
 #elif defined(__arm__)
     uint32_t scr;
     __asm__ volatile("mrc p15,0,%0,c1,c1,0" : "=r"(scr));
     scr |= (1u<<0) | (1u<<8);   /* NS=1, HCE=1 */
     __asm__ volatile("mcr p15,0,%0,c1,c1,0; isb" :: "r"(scr));
     r->scr_el3_value = scr;
     /* Allow VFP/NEON in NS world */
     uint32_t nsacr;
     __asm__ volatile("mrc p15,0,%0,c1,c1,2" : "=r"(nsacr));
     nsacr |= (3u << 10u);
     __asm__ volatile("mcr p15,0,%0,c1,c1,2" :: "r"(nsacr));
     r->fpu_enabled = true;
     r->current_el  = 3u;
 #else
     /* x86: stub */
     r->fpu_enabled = true;
     r->smc_enabled = false;
 #endif
 
     r->result = UIOX_TZ_OK;
     return UIOX_TZ_OK;
 }
 
 /* =========================================================================
  * uiox_fw_tz_print — no printf
  * ====================================================================== */
 
 void uiox_fw_tz_print(const uiox_tz_report_t *r)
 {
     if (!r) return;
     char buf[20];
 
     fw_puts_tz("[TZ] Result       : ");
     fw_puts_tz(r->result == UIOX_TZ_OK ? "OK\n" : "FAIL\n");
 
     fw_puts_tz("[TZ] Current EL   : ");
     fw_u32_dec_tz(r->current_el, buf); fw_puts_tz(buf); fw_puts_tz("\n");
 
     fw_puts_tz("[TZ] SCR_EL3      : 0x");
     fw_u64_hex_tz(r->scr_el3_value, buf); fw_puts_tz(buf); fw_puts_tz("\n");
 
     fw_puts_tz("[TZ] CPTR_EL3     : 0x");
     fw_u64_hex_tz(r->cptr_el3_value, buf); fw_puts_tz(buf); fw_puts_tz("\n");
 
     fw_puts_tz("[TZ] FPU enabled  : "); fw_puts_tz(r->fpu_enabled ? "YES\n":"NO\n");
     fw_puts_tz("[TZ] SMC enabled  : "); fw_puts_tz(r->smc_enabled ? "YES\n":"NO\n");
 
     fw_puts_tz("[TZ] GIC groups   : ");
     fw_u32_dec_tz(r->gic_groups_set, buf); fw_puts_tz(buf); fw_puts_tz("\n");
 
     fw_puts_tz("[TZ] TZC regions  : ");
     fw_u32_dec_tz(r->tzc_regions_set, buf); fw_puts_tz(buf); fw_puts_tz("\n");
 
     if (r->result != UIOX_TZ_OK) {
         fw_puts_tz("[TZ] Fail reason  : ");
         fw_puts_tz(r->fail_msg); fw_puts_tz("\n");
     }
 }
 