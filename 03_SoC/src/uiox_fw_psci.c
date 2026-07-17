/**
 * @file  uiox_fw_psci.c
 * @brief UIOX Firmware — PSCI 1.1 registration and dispatch.
 *        Zero libc dependency — no string.h or stdio.h.
 * @date  2026-07-07
 */

 #include "../include/uiox_fw_psci.h"
 #include "../include/uiox_fw_hw.h"
 
 /* =========================================================================
  * Bare-metal helpers
  * ====================================================================== */
 
 static void fw_memset_ps(void *dst, int val, size_t n)
 { uint8_t *d = (uint8_t *)dst; while (n--) *d++ = (uint8_t)val; }
 
 static void fw_puts_ps(const char *s)
{
    /* uiox_fw_hw_ops() is declared in uiox_fw_hw.h (already included) */
    const uiox_fw_hw_ops_t *ops = uiox_fw_hw_ops();
    if (!ops || !ops->uart_putc) return;
    while (*s) {
        if (*s == '\n') ops->uart_putc('\r');
        ops->uart_putc(*s++);
    }
}


 static char *fw_u32_dec_ps(uint32_t v, char *buf)
 {
     char tmp[12]; int n=0;
     if (v==0u){buf[0]='0';buf[1]='\0';return buf;}
     while(v&&n<11){tmp[n++]=(char)('0'+v%10u);v/=10u;}
     int j=0; for(int i=n-1;i>=0;i--) buf[j++]=tmp[i]; buf[j]='\0';
     return buf;
 }
 
 static char *fw_u64_hex_ps(uint64_t v, char *buf)
 {
     static const char h[] = "0123456789abcdef";
     for (int i=15;i>=0;i--){buf[i]=h[v&0xFu];v>>=4;}
     buf[16]='\0'; return buf;
 }
 
 /* =========================================================================
  * Global context
  * ====================================================================== */
 
 static uiox_psci_ctx_t *s_psci = NULL;
 
 /* =========================================================================
  * CPU lookup helpers
  * ====================================================================== */
 
 static int psci_find_cpu(uint64_t affinity)
 {
     if (!s_psci) return -1;
     for (uint32_t i = 0u; i < s_psci->num_cpus; i++)
         if (s_psci->cpus[i].affinity == affinity)
             return (int)i;
     return -1;
 }
 
 /* =========================================================================
  * PSCI handler implementations
  * ====================================================================== */
 
 int64_t uiox_psci_version(uint64_t a1, uint64_t a2,
                             uint64_t a3, uint64_t a4)
 {
     (void)a1;(void)a2;(void)a3;(void)a4;
     return (int64_t)PSCI_VERSION_VALUE;
 }
 
 int64_t uiox_psci_features(uint64_t a1, uint64_t a2,
                              uint64_t a3, uint64_t a4)
 {
     (void)a2;(void)a3;(void)a4;
     return uiox_fw_psci_supported((uint32_t)a1)
            ? PSCI_RET_SUCCESS : PSCI_RET_NOT_SUPPORTED;
 }
 
 int64_t uiox_psci_cpu_on(uint64_t a1, uint64_t a2,
                            uint64_t a3, uint64_t a4)
 {
     (void)a4;
     if (!s_psci) return PSCI_RET_INTERNAL_FAILURE;
     uint64_t  affinity = a1;
     uintptr_t entry    = (uintptr_t)a2;
     uint64_t  ctx_id   = a3;
     int idx = psci_find_cpu(affinity);
     if (idx < 0)                       return PSCI_RET_INVALID_PARAMS;
     uiox_psci_cpu_t *cpu = &s_psci->cpus[(uint32_t)idx];
     if (cpu->state == UIOX_CPU_STATE_ON)      return PSCI_RET_ALREADY_ON;
     if (cpu->state == UIOX_CPU_STATE_ON_PEND) return PSCI_RET_ON_PENDING;
     if (entry == 0u)                           return PSCI_RET_INVALID_ADDRESS;
     cpu->warm_entry = entry;
     cpu->context_id = ctx_id;
     cpu->state      = UIOX_CPU_STATE_ON_PEND;
     if (s_psci->platform_cpu_on)
         s_psci->platform_cpu_on((uint32_t)idx, entry);
     cpu->state = UIOX_CPU_STATE_ON;
     s_psci->cpu_on_count++;
     return PSCI_RET_SUCCESS;
 }
 
 int64_t uiox_psci_cpu_off(uint64_t a1, uint64_t a2,
                             uint64_t a3, uint64_t a4)
 {
     (void)a1;(void)a2;(void)a3;(void)a4;
     if (!s_psci) return PSCI_RET_INTERNAL_FAILURE;
 #if defined(__aarch64__)
     uint64_t mpidr;
     __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
     mpidr &= 0xFF00FFFFFFULL;
 #elif defined(__arm__)
     uint32_t mpidr_r;
     __asm__ volatile("mrc p15,0,%0,c0,c0,5" : "=r"(mpidr_r));
     uint64_t mpidr = (uint64_t)(mpidr_r & 0x00FFFFFFu);
 #else
     uint64_t mpidr = 0u;
 #endif
     int idx = psci_find_cpu(mpidr);
     if (idx < 0 || idx == 0) return PSCI_RET_DENIED;
     uiox_psci_cpu_t *cpu = &s_psci->cpus[(uint32_t)idx];
     cpu->state = UIOX_CPU_STATE_OFF;
     if (s_psci->platform_cpu_off)
         s_psci->platform_cpu_off((uint32_t)idx);
     s_psci->cpu_off_count++;
     for (;;) {
 #if defined(__aarch64__) || defined(__arm__)
         __asm__ volatile("wfi" ::: "memory");
 #else
         __asm__ volatile("hlt" ::: "memory");
 #endif
     }
 }
 
 int64_t uiox_psci_cpu_suspend(uint64_t a1, uint64_t a2,
                                uint64_t a3, uint64_t a4)
 {
     (void)a4;
     if (!s_psci) return PSCI_RET_INTERNAL_FAILURE;
     uint64_t  power_state = a1;
     uintptr_t entry       = (uintptr_t)a2;
     (void)a3;
 #if defined(__aarch64__)
     uint64_t mpidr;
     __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
     mpidr &= 0xFF00FFFFFFULL;
 #else
     uint64_t mpidr = 0u;
 #endif
     int idx = psci_find_cpu(mpidr);
     if (idx < 0) return PSCI_RET_INVALID_PARAMS;
     uiox_psci_cpu_t *cpu = &s_psci->cpus[(uint32_t)idx];
     cpu->suspend_state = power_state;
     cpu->warm_entry    = entry;
     cpu->state         = UIOX_CPU_STATE_SUSPEND;
     s_psci->suspend_count++;
 #if defined(__aarch64__)
     __asm__ volatile("dsb sy; wfi" ::: "memory");
 #elif defined(__arm__)
     __asm__ volatile("dsb; wfi"    ::: "memory");
 #else
     __asm__ volatile("hlt"         ::: "memory");
 #endif
     cpu->state = UIOX_CPU_STATE_ON;
     return PSCI_RET_SUCCESS;
 }
 
 int64_t uiox_psci_affinity_info(uint64_t a1, uint64_t a2,
                                   uint64_t a3, uint64_t a4)
 {
     (void)a2;(void)a3;(void)a4;
     if (!s_psci) return PSCI_RET_INTERNAL_FAILURE;
     int idx = psci_find_cpu(a1);
     if (idx < 0) return PSCI_RET_INVALID_PARAMS;
     switch (s_psci->cpus[(uint32_t)idx].state) {
     case UIOX_CPU_STATE_ON:      return (int64_t)PSCI_AFFINITY_LEVEL_ON;
     case UIOX_CPU_STATE_OFF:     return (int64_t)PSCI_AFFINITY_LEVEL_OFF;
     case UIOX_CPU_STATE_ON_PEND: return (int64_t)PSCI_AFFINITY_LEVEL_ON_PEND;
     case UIOX_CPU_STATE_SUSPEND: return (int64_t)PSCI_AFFINITY_LEVEL_OFF;
     default:                      return (int64_t)PSCI_AFFINITY_LEVEL_OFF;
     }
 }
 
 int64_t uiox_psci_system_off(uint64_t a1, uint64_t a2,
                                uint64_t a3, uint64_t a4)
 {
     (void)a1;(void)a2;(void)a3;(void)a4;
     if (s_psci && s_psci->platform_off)
         s_psci->platform_off();
     for (;;) __asm__ volatile("wfi" ::: "memory");
 }
 
 int64_t uiox_psci_system_reset(uint64_t a1, uint64_t a2,
                                  uint64_t a3, uint64_t a4)
 {
     (void)a1;(void)a2;(void)a3;(void)a4;
     if (s_psci) s_psci->reset_count++;
     if (s_psci && s_psci->platform_reset)
         s_psci->platform_reset();
     for (;;) __asm__ volatile("wfi" ::: "memory");
 }
 
 /* =========================================================================
  * Supported function table
  * ====================================================================== */
 
 static const uint32_t s_supported_fns[] = {
     PSCI_FN_VERSION,
     PSCI_FN_FEATURES,
     PSCI_FN32_CPU_SUSPEND,  PSCI_FN64_CPU_SUSPEND,
     PSCI_FN32_CPU_OFF,
     PSCI_FN32_CPU_ON,       PSCI_FN64_CPU_ON,
     PSCI_FN32_AFFINITY_INFO,PSCI_FN64_AFFINITY_INFO,
     PSCI_FN_SYSTEM_OFF,
     PSCI_FN_SYSTEM_RESET,   PSCI_FN_SYSTEM_RESET2,
 };
 
 bool uiox_fw_psci_supported(uint32_t fn_id)
 {
     for (uint32_t i = 0u;
          i < sizeof(s_supported_fns)/sizeof(s_supported_fns[0]); i++)
         if (s_supported_fns[i] == fn_id) return true;
     return false;
 }
 
 /* =========================================================================
  * Dispatch table
  * ====================================================================== */
 
 static const uiox_psci_entry_t s_psci_table[] = {
     { PSCI_FN_VERSION,          "VERSION",          uiox_psci_version       },
     { PSCI_FN_FEATURES,         "FEATURES",         uiox_psci_features      },
     { PSCI_FN32_CPU_SUSPEND,    "CPU_SUSPEND_32",   uiox_psci_cpu_suspend   },
     { PSCI_FN64_CPU_SUSPEND,    "CPU_SUSPEND_64",   uiox_psci_cpu_suspend   },
     { PSCI_FN32_CPU_OFF,        "CPU_OFF",          uiox_psci_cpu_off       },
     { PSCI_FN32_CPU_ON,         "CPU_ON_32",        uiox_psci_cpu_on        },
     { PSCI_FN64_CPU_ON,         "CPU_ON_64",        uiox_psci_cpu_on        },
     { PSCI_FN32_AFFINITY_INFO,  "AFFINITY_INFO_32", uiox_psci_affinity_info },
     { PSCI_FN64_AFFINITY_INFO,  "AFFINITY_INFO_64", uiox_psci_affinity_info },
     { PSCI_FN_SYSTEM_OFF,       "SYSTEM_OFF",       uiox_psci_system_off    },
     { PSCI_FN_SYSTEM_RESET,     "SYSTEM_RESET",     uiox_psci_system_reset  },
     { PSCI_FN_SYSTEM_RESET2,    "SYSTEM_RESET2",    uiox_psci_system_reset  },
 };
 #define PSCI_TABLE_SIZE  (sizeof(s_psci_table)/sizeof(s_psci_table[0]))
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_psci_init(uiox_psci_ctx_t *ctx,
                                   uint32_t num_cpus,
                                   bool use_smc)
 {
     if (!ctx || num_cpus == 0u || num_cpus > UIOX_PSCI_MAX_CPUS)
         return UIOX_FW_ERR_INVAL;
     fw_memset_ps(ctx, 0, sizeof(*ctx));
     ctx->num_cpus    = num_cpus;
     ctx->smc_enabled = use_smc;
     for (uint32_t i = 0u; i < num_cpus; i++) {
         ctx->cpus[i].state    = (i == 0u)
                                  ? UIOX_CPU_STATE_ON
                                  : UIOX_CPU_STATE_OFF;
         ctx->cpus[i].affinity = (uint64_t)i;
     }
     ctx->initialized = true;
     s_psci = ctx;
     return UIOX_FW_OK;
 }
 
 void uiox_fw_psci_set_cpu_on(uiox_psci_ctx_t *ctx,
                                void (*fn)(uint32_t, uintptr_t))
 { if (ctx) ctx->platform_cpu_on = fn; }
 
/* REPLACE with these — typedef matches the noreturn field exactly */
typedef void __attribute__((noreturn)) (*uiox_noreturn_fn_t)(void);

void uiox_fw_psci_set_reset(uiox_psci_ctx_t *ctx,
                              void __attribute__((noreturn)) (*fn)(void))
{
    if (ctx) ctx->platform_reset = (uiox_noreturn_fn_t)fn;
}

void uiox_fw_psci_set_off(uiox_psci_ctx_t *ctx,
                            void __attribute__((noreturn)) (*fn)(void))
{
    if (ctx) ctx->platform_off = (uiox_noreturn_fn_t)fn;
}

 
 int64_t uiox_fw_psci_dispatch(uiox_psci_ctx_t *ctx,
                                 uint64_t fn_id,
                                 uint64_t a1, uint64_t a2, uint64_t a3)
 {
     if (!ctx || !ctx->initialized) return PSCI_RET_NOT_SUPPORTED;
     s_psci = ctx;
     for (uint32_t i = 0u; i < (uint32_t)PSCI_TABLE_SIZE; i++) {
         if (s_psci_table[i].fn_id == (uint32_t)fn_id)
             return s_psci_table[i].handler(a1, a2, a3, 0u);
     }
     ctx->unknown_fn_count++;
     return PSCI_RET_NOT_SUPPORTED;
 }
 
 uiox_cpu_state_t uiox_fw_psci_cpu_state(const uiox_psci_ctx_t *ctx,
                                            uint32_t cpu_id)
 {
     if (!ctx || cpu_id >= ctx->num_cpus) return UIOX_CPU_STATE_OFF;
     return ctx->cpus[cpu_id].state;
 }
 
 /* =========================================================================
  * uiox_fw_psci_print — no printf
  * ====================================================================== */
 
 void uiox_fw_psci_print(const uiox_psci_ctx_t *ctx)
 {
     if (!ctx) return;
     char buf[20];
     static const char *cpu_states[] = {"OFF","ON","SUSPEND","ON_PEND"};
 
     fw_puts_ps("[PSCI] Version     : ");
     fw_u32_dec_ps(PSCI_VERSION_MAJOR, buf); fw_puts_ps(buf);
     fw_puts_ps(".");
     fw_u32_dec_ps(PSCI_VERSION_MINOR, buf); fw_puts_ps(buf);
     fw_puts_ps("\n");
 
     fw_puts_ps("[PSCI] Transport   : ");
     fw_puts_ps(ctx->smc_enabled ? "SMC\n" : "HVC\n");
 
     fw_puts_ps("[PSCI] CPUs        : ");
     fw_u32_dec_ps(ctx->num_cpus, buf); fw_puts_ps(buf); fw_puts_ps("\n");
 
     fw_puts_ps("[PSCI] CPU_ON cnt  : ");
     fw_u32_dec_ps(ctx->cpu_on_count, buf); fw_puts_ps(buf); fw_puts_ps("\n");
 
     fw_puts_ps("[PSCI] CPU_OFF cnt : ");
     fw_u32_dec_ps(ctx->cpu_off_count, buf); fw_puts_ps(buf); fw_puts_ps("\n");
 
     fw_puts_ps("[PSCI] SUSPEND cnt : ");
     fw_u32_dec_ps(ctx->suspend_count, buf); fw_puts_ps(buf); fw_puts_ps("\n");
 
     fw_puts_ps("[PSCI] RESET cnt   : ");
     fw_u32_dec_ps(ctx->reset_count, buf); fw_puts_ps(buf); fw_puts_ps("\n");
 
     fw_puts_ps("[PSCI] Unknown fn  : ");
     fw_u32_dec_ps(ctx->unknown_fn_count, buf); fw_puts_ps(buf); fw_puts_ps("\n");
 
     fw_puts_ps("[PSCI] Dispatch table:\n");
     for (uint32_t i = 0u; i < (uint32_t)PSCI_TABLE_SIZE; i++) {
         fw_puts_ps("  0x");
         fw_u64_hex_ps((uint64_t)s_psci_table[i].fn_id, buf);
         /* Print only 8 hex chars for 32-bit fn_id */
         buf[8] = '\0';
         fw_puts_ps(buf + 8u);  /* skip leading zeros: use last 8 chars */
         /* Actually print full 8: */
         char fn_hex[10];
         const char *hx = "0123456789abcdef";
         uint32_t fn = s_psci_table[i].fn_id;
         for (int j = 7; j >= 0; j--) { fn_hex[j]=hx[fn&0xFu]; fn>>=4; }
         fn_hex[8] = '\0';
         fw_puts_ps("  0x"); fw_puts_ps(fn_hex);
         fw_puts_ps("  "); fw_puts_ps(s_psci_table[i].name);
         fw_puts_ps("\n");
     }
 
     fw_puts_ps("[PSCI] CPU states:\n");
     for (uint32_t i = 0u; i < ctx->num_cpus; i++) {
         const uiox_psci_cpu_t *c = &ctx->cpus[i];
         uint8_t st = (uint8_t)c->state;
         fw_puts_ps("  CPU"); fw_u32_dec_ps(i, buf); fw_puts_ps(buf);
         fw_puts_ps("  aff=0x");
         fw_u64_hex_ps(c->affinity, buf); fw_puts_ps(buf);
         fw_puts_ps("  state=");
         fw_puts_ps(st < 4u ? cpu_states[st] : "?");
         fw_puts_ps("  entry=0x");
         fw_u64_hex_ps((uint64_t)c->warm_entry, buf); fw_puts_ps(buf);
         fw_puts_ps("\n");
     }
 }
 