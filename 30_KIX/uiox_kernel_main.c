/*
 * uiox_kernel_main.c
 *
 * UIOX Kernel Entry Point — all four architectures.
 *
 * Called by the bootloader (01_uBoot) via uiox_boot_arch_jump().
 * This is the FIRST C function the kernel executes after the
 * bootloader hands off control.
 *
 * Entry register conventions (set by uiox_boot_arch_jump):
 *
 *   ARM64  (AArch64):
 *     x0 = dtb_pa    — Physical address of Device Tree Blob
 *     x1 = args_pa   — Physical address of uiox_boot_args_t
 *     x2 = x3 = 0   — Reserved, must be zero
 *
 *   ARM32  (ARMv7-A):
 *     r0 = 0         — Machine type (unused with DTB)
 *     r1 = 0         — Reserved
 *     r2 = dtb_pa    — Physical address of Device Tree Blob
 *     r3 = args_pa   — Physical address of uiox_boot_args_t
 *
 *   x86-64 (AMD64):
 *     rdi = args_pa  — SysV ABI first arg
 *     rsi = dtb_pa   — SysV ABI second arg
 *
 *   RISC-V 64 (RV64):
 *     a0 = dtb_pa    — Physical address of Device Tree Blob
 *     a1 = args_pa   — Physical address of uiox_boot_args_t
 *
 * BSP_MODE — controlled by the build system (-DUIOX_BSP_DYNAMIC_BOOT):
 *
 *   Static  (default, no define):
 *     BSP is linked into this ELF.  uiox_kernel_main() calls arch_init()
 *     and uiox_soc_init() itself via uiox_bsp_init().
 *     Build:  make arm64 LINK=yes
 *
 *   Dynamic (UIOX_BSP_DYNAMIC_BOOT defined):
 *     BSP ran as a standalone secondary bootloader before the kernel.
 *     arch_init() and uiox_soc_init() are already complete.
 *     uiox_kernel_main() skips those steps and proceeds directly to
 *     subsystem init.
 *     Build:  make kernel-dynamic-arm64
 *
 * Call chain:
 *
 *   uiox_kernel_main()
 *       ├─▶ early_console_init()          — UART output before MMU
 *       ├─▶ bss_zero()                    — clear BSS segment
 *       ├─▶ stack_setup()                 — set kernel stack pointer
 *       ├─▶ [static only] arch_init()     — GIC/APIC/PLIC, cache, MMU on
 *       │       └─▶ uiox_soc_init()       — SoC detect + clock + PM
 *       ├─▶ uiox_ks_boot_entry()          — 33_PCS/03_ksign: verify + PCR extend
 *       ├─▶ uiox_fb_shell_ready()         — 33_PCS/04_fboot: timing milestone
 *       ├─▶ uiox_proc_init()              — 33_PCS: scheduler + process table
 *       │       ├─▶ uiox_sched_init()     — 33_PCS/01_schedular
 *       │       ├─▶ uiox_timer_init()     — 33_PCS timer
 *       │       ├─▶ uiox_kp_engine_init() — 33_PCS/06_kpatch: live patch engine
 *       │       └─▶ uiox_sec_init()       — 33_PCS/05_sec: ASLR + MAC
 *       └─▶ uiox_shell_start()            — 50_UIX/01_shell: first prompt
 *
 * Subsystems now in kernel (moved from 50_UIX):
 *   33_PCS/03_ksign   — kernel image signing, verification, runtime integrity
 *   33_PCS/04_fboot   — fast-boot snapshot + deferred-init scheduling
 *   33_PCS/05_sec     — ASLR + MAC security policy
 *   33_PCS/06_kpatch  — live kernel text patching engine
 *   32_FS/02_journal  — filesystem journal (transaction, commit, checkpoint)
 *   32_FS/03_netfs    — in-kernel NFS/RPC client
 *
 * @version 1.2.0
 * @date    2026-07-26
 */

/*
 * Include order: uiox_fw_types.h (via uiox_soc.h) must come first so its
 * compiler-builtin typedefs (__INT64_TYPE__ etc.) win over the "long long"
 * aliases in uiox_boot_types.h.  Everything else follows.
 *
 * NOTE: uiox_bsp.h is intentionally NOT included here.  It re-declares
 * types already defined by uiox_boot_handoff.h and uiox_fboot.h with
 * conflicting struct layouts and return types, causing -Werror failures.
 * The two BSP symbols actually needed (uiox_bsp_config_t + uiox_bsp_init)
 * are forward-declared below so the static build can call uiox_bsp_init()
 * without pulling in the conflicting header.
 *
 * Headers for the moved subsystems come from 33_PCS/include and
 * 32_FS/include — already in KERNEL_CFLAGS_BASE via the Makefile.
 */
#include "uiox_soc.h"           /* pulls uiox_fw_types.h — primitive types  */
#include "uiox_fw_uart.h"       /* PL011/UART macros — after uiox_soc.h     */
#include "uiox_boot_handoff.h"  /* uiox_boot_args_t, uiox_boot_handoff_*    */
#include "uiox_boot_types.h"    /* remaining boot enums/structs/macros       */
#include "uiox_fboot.h"         /* uiox_fb_master_ctx_t, fb_init/ready/report*/


/* Add near the top of uiox_kernel_main.c includes: */
#include "uiox_syscall.h"
#include "uiox_uaccess.h"

/* In kernel_common_init(), after uiox_proc_init(): */
early_puts("[kernel] uiox_syscall_dispatch ready\r\n");
/*
 * The arch vector table calls uiox_syscall_dispatch() directly
 * via arch_syscall_entry() — no registration needed here.
 * This line confirms the symbol is linked and the table is live.
 */


/*
 * Static build only: forward-declare the two BSP symbols we need without
 * including uiox_bsp.h (which conflicts with the headers above).
 * Definitions live in 10_BSP/src/uiox_bsp_main.c, linked via libbsp.a.
 */
#if !defined(UIOX_BSP_DYNAMIC_BOOT)
typedef struct {
    uint32_t flags;
    uint64_t dtb_pa;
    uint64_t args_pa;
    uint64_t kernel_load_pa;
} uiox_bsp_config_t;

#define UIOX_BSP_OK           0
#define UIOX_BSP_FL_DYN_LOAD  (1u << 0)

extern int uiox_bsp_init(const uiox_bsp_config_t *cfg);
#endif

/* ── Forward declarations of subsystem init functions ────────────────── */
extern int  arch_init(void);          /* 10_Arch/<arch>/src/arch_init.c     */
extern void uiox_proc_init(void);     /* 33_PCS — scheduler + process table */

/* Forward declaration so weak stubs below can call early_puts()
 * before its static definition appears later in this file.        */
static void early_puts(const char *s);

/*
 * Weak stub implementations for subsystems not yet built.
 *
 * __attribute__((weak)) — the linker replaces these automatically when the
 * real implementations appear in a linked library. No source changes needed.
 *
 * 33_PCS/03_ksign  — kernel image signing / verification
 * 33_PCS/04_fboot  — fast-boot timing milestones
 * 33_PCS/05_sec    — ASLR + MAC security (init called from uiox_proc_init)
 * 33_PCS/06_kpatch — live patch engine (init called from uiox_proc_init)
 * 50_UIX/01_shell  — first user shell
 * 33_PCS internal  — scheduler and timer sub-modules
 */
__attribute__((weak))
void uiox_ks_boot_entry(const void *image,
                         size_t      image_size,
                         uintptr_t   text_base,
                         size_t      text_size,
                         uintptr_t   rodata_base,
                         size_t      rodata_size)
{
    (void)image; (void)image_size;
    (void)text_base; (void)text_size;
    (void)rodata_base; (void)rodata_size;
    early_puts("[kernel]   uiox_ks_boot_entry: stub (33_PCS/03_ksign not built)\r\n");
}

__attribute__((weak))
uiox_fb_err_t uiox_fb_init(uiox_fb_master_ctx_t *ctx,
                             uiox_fb_mode_t        mode,
                             uint64_t              budget_ns)
{
    (void)ctx; (void)mode; (void)budget_ns;
    early_puts("[kernel]   uiox_fb_init: stub (33_PCS/04_fboot not built)\r\n");
    return 0;
}

__attribute__((weak))
uiox_fb_err_t uiox_fb_shell_ready(uiox_fb_master_ctx_t *ctx)
{
    (void)ctx;
    early_puts("[kernel]   uiox_fb_shell_ready: stub (33_PCS/04_fboot not built)\r\n");
    return 0;
}

__attribute__((weak))
void uiox_fb_report(const uiox_fb_master_ctx_t *ctx)
{
    (void)ctx;
    early_puts("[kernel]   uiox_fb_report: stub (33_PCS/04_fboot not built)\r\n");
}

__attribute__((weak))
void uiox_shell_start(void)
{
    early_puts("[kernel]   uiox_shell_start: stub (50_UIX/01_shell not built)\r\n");
    early_puts("[kernel]   System halted — shell not available.\r\n");
    for (;;) {
#if defined(__x86_64__)
        __asm__ volatile("hlt");
#else
        __asm__ volatile("wfi");
#endif
    }
}

__attribute__((weak))
void uiox_sched_init(void)
{
    early_puts("[kernel]   uiox_sched_init: stub (33_PCS/01_schedular not built)\r\n");
}

__attribute__((weak))
void uiox_timer_init(void)
{
    early_puts("[kernel]   uiox_timer_init: stub (33_PCS timer not built)\r\n");
}

__attribute__((weak))
void uiox_kp_engine_init(void)
{
    early_puts("[kernel]   uiox_kp_engine_init: stub (33_PCS/06_kpatch not built)\r\n");
}

__attribute__((weak))
void uiox_sec_init(void)
{
    early_puts("[kernel]   uiox_sec_init: stub (33_PCS/05_sec not built)\r\n");
}

/* ── Kernel BSS / stack symbols (provided by the linker script) ───────── */
extern uint8_t _bss_start[];
extern uint8_t _bss_end[];
extern uint8_t _stack_top[];
extern uint8_t _text_start[];
extern uint8_t _text_end[];
extern uint8_t _rodata_start[];
extern uint8_t _rodata_end[];

/* ── Global boot-args pointer — set once at entry, read-only thereafter ─ */
static const uiox_boot_args_t *g_boot_args = NULL;
static uint64_t                g_dtb_pa    = 0u;

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void bss_zero(void)
{
    uint8_t *p = _bss_start;
    while (p < _bss_end) *p++ = 0u;
}

static void stack_setup(void)
{
#if defined(__aarch64__)
    __asm__ volatile(
        "mov  sp, %0\n\t"
        "msr  sp_el0, xzr\n\t"
        :: "r"((uint64_t)_stack_top & ~0xFull)
        : "memory"
    );
#elif defined(__arm__)
    __asm__ volatile(
        "mov  sp, %0\n\t"
        :: "r"((uint32_t)_stack_top & ~7u)
        : "memory"
    );
#elif defined(__x86_64__)
    __asm__ volatile(
        "movq %0, %%rsp\n\t"
        :: "r"((uint64_t)_stack_top & ~0xFull)
        : "memory"
    );
#elif defined(__riscv)
    __asm__ volatile(
        "mv   sp, %0\n\t"
        :: "r"((uint64_t)_stack_top & ~0xFull)
        : "memory"
    );
#endif
}

static void early_putc(char c)
{
#if defined(__aarch64__) || defined(__arm__)
    volatile uint32_t *fr = (volatile uint32_t *)(
#  if defined(__aarch64__)
        0x09000000UL + 0x018u
#  else
        0x10009000UL + 0x018u
#  endif
    );
    volatile uint32_t *dr = (volatile uint32_t *)(
#  if defined(__aarch64__)
        0x09000000UL + 0x000u
#  else
        0x10009000UL + 0x000u
#  endif
    );
    while (*fr & (1u << 5u)) {}
    *dr = (uint32_t)(uint8_t)c;
#elif defined(__x86_64__)
    while (!(({uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"((uint16_t)0x3FDu)); v;}) & (1u<<5u))) {}
    __asm__ volatile("outb %0,%1" :: "a"((uint8_t)c), "Nd"((uint16_t)0x3F8u));
#elif defined(__riscv)
    volatile uint32_t *lsr = (volatile uint32_t *)(0x10000000UL + 0x05u);
    volatile uint32_t *thr = (volatile uint32_t *)(0x10000000UL + 0x00u);
    while (!(*lsr & (1u << 5u))) {}
    *thr = (uint32_t)(uint8_t)c;
#endif
}

static void early_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') early_putc('\r');
        early_putc(*s++);
    }
}

static void early_puthex(uint64_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    early_puts("0x");
    for (int i = 60; i >= 0; i -= 4)
        early_putc(hex[(v >> i) & 0xFu]);
}

/* =========================================================================
 * arch_init wrapper — skipped in dynamic mode (BSP already ran it)
 * ====================================================================== */
static int kernel_arch_init(void)
{
#if defined(UIOX_BSP_DYNAMIC_BOOT)
    early_puts("[kernel]   arch_init: skipped (BSP dynamic boot)\r\n");
    return 0;
#else
    return arch_init();
#endif
}

/* =========================================================================
 * Common kernel init — called from every arch entry after boot args saved
 * ====================================================================== */
static void __attribute__((noreturn)) kernel_common_init(void)
{
    early_puts("[kernel] arch_init()...\r\n");
    int rc = kernel_arch_init();
    if (rc != 0) {
        early_puts("[kernel] FATAL: arch_init failed\r\n");
        for (;;) {
#if defined(__x86_64__)
            __asm__ volatile("hlt");
#else
            __asm__ volatile("wfi");
#endif
        }
    }

    /* 33_PCS/03_ksign — verify kernel image + extend PCR measurements */
    early_puts("[kernel] uiox_ks_boot_entry()...\r\n");
    uiox_ks_boot_entry(
        (const void *)(uintptr_t)g_boot_args->kernel_entry,
        0u,
        (uintptr_t)_text_start,
        (size_t)(_text_end   - _text_start),
        (uintptr_t)_rodata_start,
        (size_t)(_rodata_end - _rodata_start)
    );

    /* 33_PCS/04_fboot — fast-boot timing milestone */
    early_puts("[kernel] uiox_fb_shell_ready()...\r\n");
    uiox_fb_master_ctx_t fb_ctx;
    uiox_fb_init(&fb_ctx, UIOX_FB_MODE_COLD, 3000000u);
    uiox_fb_shell_ready(&fb_ctx);
    uiox_fb_report(&fb_ctx);

    /* 33_PCS — scheduler, process table, security, live patching */
    early_puts("[kernel] uiox_proc_init()...\r\n");
    uiox_proc_init();

    /* 50_UIX/01_shell — first user shell (only remaining 50_UIX entry) */
    early_puts("[kernel] uiox_shell_start()...\r\n");
    uiox_shell_start();

    early_puts("[kernel] FATAL: shell returned — halting\r\n");
    for (;;) {
#if defined(__x86_64__)
        __asm__ volatile("hlt");
#else
        __asm__ volatile("wfi");
#endif
    }
}

/* =========================================================================
 * ── ARM64 kernel entry ────────────────────────────────────────────────
 *
 * uiox_boot_arch_jump() sets:  x0 = dtb_pa,  x1 = args_pa
 * ====================================================================== */
#if defined(__aarch64__)

void __attribute__((noreturn))
uiox_kernel_main(void)
{
    register uint64_t dtb_pa  __asm__("x0");
    register uint64_t args_pa __asm__("x1");
    __asm__ volatile("" : "=r"(dtb_pa), "=r"(args_pa));

    stack_setup();
    bss_zero();

    g_dtb_pa    = dtb_pa;
    g_boot_args = (const uiox_boot_args_t *)(uintptr_t)args_pa;

    early_puts("\r\n[kernel] UIOX kernel entry (ARM64)\r\n");
    early_puts("[kernel]   dtb_pa   = "); early_puthex(dtb_pa);  early_puts("\r\n");
    early_puts("[kernel]   args_pa  = "); early_puthex(args_pa); early_puts("\r\n");
    early_puts("[kernel]   arch     = ARM64 / AArch64\r\n");
#if defined(UIOX_BSP_DYNAMIC_BOOT)
    early_puts("[kernel]   boot     = dynamic (BSP secondary bootloader)\r\n");
#else
    early_puts("[kernel]   boot     = static (BSP linked)\r\n");
#endif

    kernel_common_init();
}

/* =========================================================================
 * ── ARM32 kernel entry ────────────────────────────────────────────────
 *
 * uiox_boot_arch_jump() sets:  r2 = dtb_pa,  r3 = args_pa
 * ====================================================================== */
#elif defined(__arm__)

void __attribute__((noreturn))
uiox_kernel_main(void)
{
    register uint32_t dtb_pa  __asm__("r2");
    register uint32_t args_pa __asm__("r3");
    __asm__ volatile("" : "=r"(dtb_pa), "=r"(args_pa));

    stack_setup();
    bss_zero();

    g_dtb_pa    = (uint64_t)dtb_pa;
    g_boot_args = (const uiox_boot_args_t *)(uintptr_t)args_pa;

    early_puts("\r\n[kernel] UIOX kernel entry (ARM32)\r\n");
    early_puts("[kernel]   dtb_pa   = "); early_puthex((uint64_t)dtb_pa);  early_puts("\r\n");
    early_puts("[kernel]   args_pa  = "); early_puthex((uint64_t)args_pa); early_puts("\r\n");
    early_puts("[kernel]   arch     = ARM32 / ARMv7-A\r\n");
#if defined(UIOX_BSP_DYNAMIC_BOOT)
    early_puts("[kernel]   boot     = dynamic (BSP secondary bootloader)\r\n");
#else
    early_puts("[kernel]   boot     = static (BSP linked)\r\n");
#endif

    kernel_common_init();
}

/* =========================================================================
 * ── x86-64 kernel entry ───────────────────────────────────────────────
 *
 * SysV ABI:  rdi = args_pa,  rsi = dtb_pa
 * ====================================================================== */
#elif defined(__x86_64__)

void __attribute__((noreturn))
uiox_kernel_main(uiox_boot_args_t *args_pa_ptr,
                 uint64_t          dtb_pa_val)
{
    stack_setup();
    bss_zero();

    g_dtb_pa    = dtb_pa_val;
    g_boot_args = (const uiox_boot_args_t *)args_pa_ptr;

    early_puts("\r\n[kernel] UIOX kernel entry (x86-64)\r\n");
    early_puts("[kernel]   args_pa  = "); early_puthex((uint64_t)(uintptr_t)args_pa_ptr); early_puts("\r\n");
    early_puts("[kernel]   dtb_pa   = "); early_puthex(dtb_pa_val); early_puts("\r\n");
    early_puts("[kernel]   arch     = x86-64 / AMD64\r\n");
#if defined(UIOX_BSP_DYNAMIC_BOOT)
    early_puts("[kernel]   boot     = dynamic (BSP secondary bootloader)\r\n");
#else
    early_puts("[kernel]   boot     = static (BSP linked)\r\n");
#endif

    kernel_common_init();
}

/* =========================================================================
 * ── RISC-V 64 kernel entry ────────────────────────────────────────────
 *
 * uiox_boot_arch_jump() sets:  a0 = dtb_pa,  a1 = args_pa
 * ====================================================================== */
#elif defined(__riscv)

void __attribute__((noreturn))
uiox_kernel_main(void)
{
    register uint64_t dtb_pa  __asm__("a0");
    register uint64_t args_pa __asm__("a1");
    __asm__ volatile("" : "=r"(dtb_pa), "=r"(args_pa));

    stack_setup();
    bss_zero();

    g_dtb_pa    = dtb_pa;
    g_boot_args = (const uiox_boot_args_t *)(uintptr_t)args_pa;

    early_puts("\r\n[kernel] UIOX kernel entry (RISC-V 64)\r\n");
    early_puts("[kernel]   dtb_pa   = "); early_puthex(dtb_pa);  early_puts("\r\n");
    early_puts("[kernel]   args_pa  = "); early_puthex(args_pa); early_puts("\r\n");
    early_puts("[kernel]   arch     = RV64IMAFDC\r\n");
    early_puts("[kernel]   satp     = 0 (bare, MMU off)\r\n");
#if defined(UIOX_BSP_DYNAMIC_BOOT)
    early_puts("[kernel]   boot     = dynamic (BSP secondary bootloader)\r\n");
#else
    early_puts("[kernel]   boot     = static (BSP linked)\r\n");
#endif

    kernel_common_init();
}

#else
#  error "uiox_kernel_main.c: unsupported architecture"
#endif

/* =========================================================================
 * Public accessors — called by subsystems after kernel_main runs
 * ====================================================================== */

uint64_t uiox_kernel_get_dtb_pa(void)
{
    return g_dtb_pa;
}

const uiox_boot_args_t *uiox_kernel_get_boot_args(void)
{
    return g_boot_args;
}
