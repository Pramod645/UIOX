/**
 * @file    uiox_soc_psci.c
 * @brief   UIOX SoC — PSCI 1.1 registration and dispatch.
 *          Zero libc dependency — no string.h or stdio.h.
 * @date    2026-07-07
 */

 #include "../include/uiox_soc_psci.h"
 #include "../include/uiox_soc_hw.h"
 
 /* =========================================================================
  * Bare-metal helpers
  * ====================================================================== */
 
 static void soc_memset_ps(void *dst, int val, uiox_size_t n)
 {
     uiox_uint8_t *d = (uiox_uint8_t *)dst;
     while (n--) *d++ = (uiox_uint8_t)val;
 }
 
 static void soc_puts_ps(const char *s)
 {
     const uiox_soc_hw_ops_t *ops = uiox_soc_hw_ops();
     if (!ops || !ops->uart_putc) return;
     while (*s) {
         if (*s == '\n') ops->uart_putc('\r');
         ops->uart_putc(*s++);
     }
 }
 
 static char *soc_u32_dec_ps(uiox_uint32_t v, char *buf)
 {
     char tmp[12]; int n = 0;
     if (v == 0u) { buf[0] = '0'; buf[1] = '\0'; return buf; }
     while (v && n < 11) { tmp[n++] = (char)('0' + v % 10u); v /= 10u; }
     int j = 0;
     for (int i = n - 1; i >= 0; i--) buf[j++] = tmp[i];
     buf[j] = '\0';
     return buf;
 }
 
 static char *soc_u64_hex_ps(uiox_uint64_t v, char *buf)
 {
     static const char h[] = "0123456789abcdef";
     for (int i = 15; i >= 0; i--) { buf[i] = h[v & 0xFu]; v >>= 4; }
     buf[16] = '\0';
     return buf;
 }
 
 /* =========================================================================
  * Module-level PSCI context pointer
  * ====================================================================== */
 
 static uiox_soc_psci_ctx_t *s_psci_ctx = NULL;
 
 /* =========================================================================
  * Individual PSCI handler implementations
  * ====================================================================== */
 
 uiox_int64_t uiox_soc_psci_version(uiox_uint64_t a1, uiox_uint64_t a2,
                                 uiox_uint64_t a3, uiox_uint64_t a4)
 {
     (void)a1; (void)a2; (void)a3; (void)a4;
     return (uiox_int64_t)PSCI_VERSION_VALUE;
 }
 
 uiox_int64_t uiox_soc_psci_cpu_on(uiox_uint64_t a1, uiox_uint64_t a2,
                                uiox_uint64_t a3, uiox_uint64_t a4)
 {
     (void)a4;
     if (!s_psci_ctx) return PSCI_RET_INTERNAL_FAILURE;
 
     uiox_uint32_t cpu_id   = (uiox_uint32_t)(a1 & 0xFFu);
     uiox_uint64_t entry_pa = a2;
     uiox_uint64_t ctx_id   = a3;
 
     if (cpu_id >= s_psci_ctx->num_cpus) return PSCI_RET_INVALID_PARAMS;
     if (s_psci_ctx->cpus[cpu_id].state == UIOX_SOC_CPU_STATE_ON)
         return PSCI_RET_ALREADY_ON;
 
     s_psci_ctx->cpus[cpu_id].warm_entry  = (uiox_uintptr_t)entry_pa;
     s_psci_ctx->cpus[cpu_id].context_id  = ctx_id;
     s_psci_ctx->cpus[cpu_id].state       = UIOX_SOC_CPU_STATE_ON_PEND;
     s_psci_ctx->cpu_on_count++;
 
     if (s_psci_ctx->platform_cpu_on)
         s_psci_ctx->platform_cpu_on(cpu_id, (uiox_uintptr_t)entry_pa);
 
     s_psci_ctx->cpus[cpu_id].state = UIOX_SOC_CPU_STATE_ON;
     return PSCI_RET_SUCCESS;
 }
 
 uiox_int64_t uiox_soc_psci_cpu_off(uiox_uint64_t a1, uiox_uint64_t a2,
                                 uiox_uint64_t a3, uiox_uint64_t a4)
 {
     (void)a1; (void)a2; (void)a3; (void)a4;
     if (!s_psci_ctx) return PSCI_RET_INTERNAL_FAILURE;
 
     /* Identify current CPU by scanning affinity fields */
 #if defined(__aarch64__)
 uiox_uint64_t mpidr;
     __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
     mpidr &= 0xFF00FFFFFFu;
     for (uiox_uint32_t i = 0u; i < s_psci_ctx->num_cpus; i++) {
         if (s_psci_ctx->cpus[i].affinity == mpidr) {
             s_psci_ctx->cpus[i].state = UIOX_SOC_CPU_STATE_OFF;
             s_psci_ctx->cpu_off_count++;
             if (s_psci_ctx->platform_cpu_off)
                 s_psci_ctx->platform_cpu_off(i);
             break;
         }
     }
 #endif
     return PSCI_RET_SUCCESS;
 }
 
 uiox_int64_t uiox_soc_psci_cpu_suspend(uiox_uint64_t a1, uiox_uint64_t a2,
                                     uiox_uint64_t a3, uiox_uint64_t a4)
 {
     (void)a1; (void)a2; (void)a3; (void)a4;
     if (!s_psci_ctx) return PSCI_RET_INTERNAL_FAILURE;
     s_psci_ctx->suspend_count++;
     /* Minimal: WFI until woken */
#if defined(__aarch64__) || defined(__arm__)
    __asm__ volatile("wfi" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("hlt" ::: "memory");
#elif defined(__riscv)
    __asm__ volatile("wfi" ::: "memory");
#else
    /* Generic fallback — spin without hardware hint */
    __asm__ volatile("" ::: "memory");
#endif
     return PSCI_RET_SUCCESS;
 }
 
 uiox_int64_t uiox_soc_psci_affinity_info(uiox_uint64_t a1, uiox_uint64_t a2,
                                       uiox_uint64_t a3, uiox_uint64_t a4)
 {
     (void)a3; (void)a4;
     if (!s_psci_ctx) return PSCI_RET_INTERNAL_FAILURE;
     uiox_uint64_t target_affinity = a1;
     /* a2 = lowest affinity level (0 = core) */
     (void)a2;
     for (uiox_uint32_t i = 0u; i < s_psci_ctx->num_cpus; i++) {
         if (s_psci_ctx->cpus[i].affinity == target_affinity) {
             switch (s_psci_ctx->cpus[i].state) {
                 case UIOX_SOC_CPU_STATE_ON:      return PSCI_AFFINITY_LEVEL_ON;
                 case UIOX_SOC_CPU_STATE_ON_PEND: return PSCI_AFFINITY_LEVEL_ON_PEND;
                 default:                          return PSCI_AFFINITY_LEVEL_OFF;
             }
         }
     }
     return PSCI_RET_INVALID_PARAMS;
 }
 
 uiox_int64_t uiox_soc_psci_system_off(uiox_uint64_t a1, uiox_uint64_t a2,
                                    uiox_uint64_t a3, uiox_uint64_t a4)
 {
     (void)a1; (void)a2; (void)a3; (void)a4;
     if (s_psci_ctx && s_psci_ctx->platform_off)
         s_psci_ctx->platform_off();
     for (;;)
#if defined(__aarch64__) || defined(__arm__)
        __asm__ volatile("wfi" ::: "memory");
 #elif defined(__x86_64__) || defined(__i386__)
        __asm__ volatile("hlt" ::: "memory");
 #elif defined(__riscv)
        __asm__ volatile("wfi" ::: "memory");
 #else
     /* Generic fallback — spin without hardware hint */
        __asm__ volatile("" ::: "memory");
 #endif
 }
 
 uiox_int64_t uiox_soc_psci_system_reset(uiox_uint64_t a1, uiox_uint64_t a2,
                                      uiox_uint64_t a3, uiox_uint64_t a4)
 {
     (void)a1; (void)a2; (void)a3; (void)a4;
     if (s_psci_ctx) s_psci_ctx->reset_count++;
     if (s_psci_ctx && s_psci_ctx->platform_reset)
         s_psci_ctx->platform_reset();
     for (;;) 
#if defined(__aarch64__) || defined(__arm__)
        __asm__ volatile("wfi" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
        __asm__ volatile("hlt" ::: "memory");
#elif defined(__riscv)
        __asm__ volatile("wfi" ::: "memory");
#else
    /* Generic fallback — spin without hardware hint */
        __asm__ volatile("" ::: "memory");
#endif
 }
 
 uiox_int64_t uiox_soc_psci_features(uiox_uint64_t a1, uiox_uint64_t a2,
                                  uiox_uint64_t a3, uiox_uint64_t a4)
 {
     (void)a2; (void)a3; (void)a4;
     return uiox_soc_psci_supported((uiox_uint32_t)a1)
            ? PSCI_RET_SUCCESS
            : PSCI_RET_NOT_SUPPORTED;
 }
 
 /* =========================================================================
  * Dispatch table
  * ====================================================================== */
 
 typedef struct {
    uiox_uint32_t                   fn_id;
     const char                *name;
     uiox_soc_psci_handler_t    handler;
 } psci_entry_t;
 
 static const psci_entry_t s_psci_table[] = {
     { PSCI_FN_VERSION,       "PSCI_VERSION",      uiox_soc_psci_version      },
     { PSCI_FN32_CPU_SUSPEND, "CPU_SUSPEND",       uiox_soc_psci_cpu_suspend  },
     { PSCI_FN64_CPU_SUSPEND, "CPU_SUSPEND_64",    uiox_soc_psci_cpu_suspend  },
     { PSCI_FN32_CPU_OFF,     "CPU_OFF",           uiox_soc_psci_cpu_off      },
     { PSCI_FN32_CPU_ON,      "CPU_ON",            uiox_soc_psci_cpu_on       },
     { PSCI_FN64_CPU_ON,      "CPU_ON_64",         uiox_soc_psci_cpu_on       },
     { PSCI_FN32_AFFINITY_INFO,"AFFINITY_INFO",    uiox_soc_psci_affinity_info},
     { PSCI_FN64_AFFINITY_INFO,"AFFINITY_INFO_64", uiox_soc_psci_affinity_info},
     { PSCI_FN_SYSTEM_OFF,    "SYSTEM_OFF",        uiox_soc_psci_system_off   },
     { PSCI_FN_SYSTEM_RESET,  "SYSTEM_RESET",      uiox_soc_psci_system_reset },
     { PSCI_FN_FEATURES,      "PSCI_FEATURES",     uiox_soc_psci_features     },
 };
 
 #define PSCI_TABLE_LEN  (sizeof(s_psci_table) / sizeof(s_psci_table[0]))
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_soc_err_t uiox_soc_psci_init(uiox_soc_psci_ctx_t *ctx,
                                     uiox_uint32_t             num_cpus,
                                     uiox_bool_t                 use_smc)
 {
     if (!ctx || num_cpus == 0u || num_cpus > UIOX_SOC_PSCI_MAX_CPUS)
         return UIOX_SOC_ERR_INVAL;
 
     soc_memset_ps(ctx, 0, sizeof(*ctx));
     ctx->num_cpus    = num_cpus;
     ctx->smc_enabled = use_smc;
     ctx->initialized = true;
     s_psci_ctx       = ctx;
 
     /* Mark CPU0 as ON; others as OFF */
     for (uiox_uint32_t i = 0u; i < num_cpus; i++) {
 #if defined(__aarch64__)
         ctx->cpus[i].affinity = (uiox_uint64_t)i;
 #endif
         ctx->cpus[i].state = (i == 0u)
                              ? UIOX_SOC_CPU_STATE_ON
                              : UIOX_SOC_CPU_STATE_OFF;
     }
 
     soc_puts_ps("[SOC] PSCI 1.1 init OK — ");
     char buf[12];
     soc_puts_ps(soc_u32_dec_ps(num_cpus, buf));
     soc_puts_ps(" CPUs\n");
     return UIOX_SOC_OK;
 }
 
 void uiox_soc_psci_set_cpu_on(uiox_soc_psci_ctx_t *ctx,
                                 void (*fn)(uiox_uint32_t, uiox_uintptr_t))
 {
     if (ctx) ctx->platform_cpu_on = fn;
 }
 
 void uiox_soc_psci_set_reset(uiox_soc_psci_ctx_t *ctx,
     void __attribute__((noreturn)) (*fn)(void))
 {
     if (ctx) ctx->platform_reset = fn;
 }
 
 void uiox_soc_psci_set_off(uiox_soc_psci_ctx_t *ctx,
     void __attribute__((noreturn)) (*fn)(void))
 {
     if (ctx) ctx->platform_off = fn;
 }
 
 uiox_int64_t uiox_soc_psci_dispatch(uiox_soc_psci_ctx_t *ctx,
                                  uiox_uint64_t fn_id,
                                  uiox_uint64_t a1, uiox_uint64_t a2,
                                  uiox_uint64_t a3)
 {
     (void)ctx;
     for (uiox_size_t i = 0u; i < PSCI_TABLE_LEN; i++) {
         if (s_psci_table[i].fn_id == (uiox_uint32_t)fn_id)
             return s_psci_table[i].handler(a1, a2, a3, 0u);
     }
     if (s_psci_ctx) s_psci_ctx->unknown_fn_count++;
     return PSCI_RET_NOT_SUPPORTED;
 }
 
 uiox_bool_t uiox_soc_psci_supported(uiox_uint32_t fn_id)
 {
     for (uiox_size_t i = 0u; i < PSCI_TABLE_LEN; i++) {
         if (s_psci_table[i].fn_id == fn_id) return true;
     }
     return false;
 }
 
 uiox_soc_cpu_state_t uiox_soc_psci_cpu_state(
         const uiox_soc_psci_ctx_t *ctx, uiox_uint32_t cpu_id)
 {
     if (!ctx || cpu_id >= ctx->num_cpus) return UIOX_SOC_CPU_STATE_OFF;
     return ctx->cpus[cpu_id].state;
 }
 
 void uiox_soc_psci_print(const uiox_soc_psci_ctx_t *ctx)
 {
     if (!ctx) return;
     char buf[20];
     soc_puts_ps("[SOC] PSCI context:\n");
     soc_puts_ps("  num_cpus     : ");
     soc_puts_ps(soc_u32_dec_ps(ctx->num_cpus, buf));
     soc_puts_ps("\n  cpu_on_count : ");
     soc_puts_ps(soc_u32_dec_ps(ctx->cpu_on_count, buf));
     soc_puts_ps("\n  cpu_off_cnt  : ");
     soc_puts_ps(soc_u32_dec_ps(ctx->cpu_off_count, buf));
     soc_puts_ps("\n  suspend_cnt  : ");
     soc_puts_ps(soc_u32_dec_ps(ctx->suspend_count, buf));
     soc_puts_ps("\n  reset_cnt    : ");
     soc_puts_ps(soc_u32_dec_ps(ctx->reset_count, buf));
     soc_puts_ps("\n  CPU states   :\n");
     static const char *state_names[] = {
         "OFF", "ON", "SUSPEND", "ON_PEND"
     };
     for (uiox_uint32_t i = 0u; i < ctx->num_cpus; i++) {
         soc_puts_ps("    CPU");
         soc_puts_ps(soc_u32_dec_ps(i, buf));
         soc_puts_ps(" affinity=0x");
         char hbuf[20];
         soc_puts_ps(soc_u64_hex_ps(ctx->cpus[i].affinity, hbuf));
         soc_puts_ps("  state=");
         uiox_uint32_t st = (uiox_uint32_t)ctx->cpus[i].state;
         soc_puts_ps(st < 4u ? state_names[st] : "?");
         soc_puts_ps("\n");
     }
 }
 