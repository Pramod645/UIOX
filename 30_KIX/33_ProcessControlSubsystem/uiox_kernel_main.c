/*
 * 33_ProcessControlSubsystem/uiox_kernel_main.c
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
 * Call chain after this function:
 *
 *   uiox_kernel_main()
 *       ├─▶ early_console_init()          — UART output before MMU
 *       ├─▶ bss_zero()                    — clear BSS segment
 *       ├─▶ stack_setup()                 — set kernel stack pointer
 *       ├─▶ arch_init()                   — GIC/APIC/PLIC, cache, MMU on
 *       │       └─▶ uiox_soc_init()       — SoC detect + clock + PM
 *       ├─▶ uiox_ks_boot_entry()          — 12_ksign verify + PCR extend
 *       ├─▶ uiox_fb_shell_ready()         — 13_fboot timing milestone
 *       ├─▶ uiox_proc_init()              — 33_PCS scheduler + process table
 *       └─▶ uiox_shell_start()            — 50_UIX/01_shell first prompt
 *
 * @version 1.0.0
 * @date    2026-07-16
 */

#include "../01_uBoot/include/uiox_boot_types.h"
#include "../01_uBoot/include/uiox_boot_handoff.h"
#include "../02_FwHal/include/uiox_soc.h"
#include "../02_FwHal/include/uiox_fw_uart.h"
#include "../50_UIX/12_ksign/include/uiox_aslr.h"

/* ── Forward declarations of subsystem init functions ────────────────── */
extern int  arch_init(void);                /* 10_Arch/<arch>/src/arch_init.c  */
extern void uiox_ks_boot_entry(            /* 50_UIX/12_ksign/src/uiox_ksign.c*/
                const void *image,
                size_t      image_size,
                uintptr_t   text_base,
                size_t      text_size,
                uintptr_t   rodata_base,
                size_t      rodata_size);
extern void uiox_fb_shell_ready(           /* 50_UIX/13_fboot                  */
                uiox_fb_master_ctx_t *ctx);
extern void uiox_proc_init(void);          /* 33_ProcessControlSubsystem       */
extern void uiox_shell_start(void);        /* 50_UIX/01_shell                  */

/* ── Kernel BSS symbols (provided by the linker script) ──────────────── */
extern uint8_t _bss_start[];
extern uint8_t _bss_end[];
extern uint8_t _stack_top[];               /* top of kernel stack region        */

/* ── Global boot-args pointer — set once, read-only thereafter ────────── */
static const uiox_boot_args_t *g_boot_args  = NULL;
static uint64_t                g_dtb_pa     = 0u;

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/* ── Zero BSS without libc ─────────────────────────────────────────── */
static void bss_zero(void)
{
    uint8_t *p = _bss_start;
    while (p < _bss_end) *p++ = 0u;
}

/* ── Architecture-specific stack pointer setup ─────────────────────── */
static void stack_setup(void)
{
#if defined(__aarch64__)
    /*
     * Set SP_EL1 to the top of the kernel stack region defined by the
     * linker script.  Must be 16-byte aligned per AArch64 ABI.
     */
    __asm__ volatile(
        "mov  sp, %0\n\t"
        "msr  sp_el0, xzr\n\t"   /* clear user-mode stack (unused at init) */
        :: "r"((uint64_t)_stack_top & ~0xFull)
        : "memory"
    );
#elif defined(__arm__)
    __asm__ volatile(
        "mov  sp, %0\n\t"
        :: "r"((uint32_t)_stack_top & ~7u)  /* ARM32: 8-byte aligned */
        : "memory"
    );
#elif defined(__x86_64__)
    __asm__ volatile(
        "movq %0, %%rsp\n\t"
        :: "r"((uint64_t)_stack_top & ~0xFull)  /* x86: 16-byte aligned */
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

/* ── Minimal early UART output (before MMU/caches are on) ──────────── */
static void early_putc(char c)
{
#if defined(__aarch64__) || defined(__arm__)
    /* PL011 UART — poll FR.TXFF (bit 5) then write DR */
    volatile uint32_t *fr = (volatile uint32_t *)(
#  if defined(__aarch64__)
        0x09000000UL + 0x018u    /* ARM64 QEMU virt PL011 FR */
#  else
        0x10009000UL + 0x018u    /* ARM32 versatilepb PL011 FR */
#  endif
    );
    volatile uint32_t *dr = (volatile uint32_t *)(
#  if defined(__aarch64__)
        0x09000000UL + 0x000u
#  else
        0x10009000UL + 0x000u
#  endif
    );
    while (*fr & (1u << 5u)) {}   /* wait while TX FIFO full */
    *dr = (uint32_t)(uint8_t)c;

#elif defined(__x86_64__)
    /* NS16550A COM1 — poll LSR.THRE (bit 5) then write THR */
    while (!(({uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"((uint16_t)0x3FDu)); v;}) & (1u<<5u))) {}
    __asm__ volatile("outb %0,%1" :: "a"((uint8_t)c), "Nd"((uint16_t)0x3F8u));

#elif defined(__riscv)
    /* NS16550A UART — base 0x10000000, LSR offset 5, THR offset 0 */
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

/* Simple unsigned 64-bit hex print for early debug */
static void early_puthex(uint64_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    early_puts("0x");
    for (int i = 60; i >= 0; i -= 4)
        early_putc(hex[(v >> i) & 0xFu]);
}

/* =========================================================================
 * ── ARM64 kernel entry ────────────────────────────────────────────────
 *
 * uiox_boot_arch_jump() sets:
 *   x0 = dtb_pa
 *   x1 = args_pa
 *   x2 = x3 = 0
 *
 * The __asm__ register variables capture x0/x1 BEFORE any C prologue
 * can overwrite them.  This is the standard Linux kernel pattern.
 * ====================================================================== */
#if defined(__aarch64__)

void __attribute__((noreturn))
uiox_kernel_main(void)
{
    /*
     * Capture x0/x1 immediately — these hold dtb_pa and args_pa
     * as placed by uiox_boot_arch_jump().  Any C function call would
     * corrupt x0/x1, so we must pin them to named variables first.
     */
    register uint64_t dtb_pa  __asm__("x0");
    register uint64_t args_pa __asm__("x1");

    /* Prevent the compiler reordering anything before this read */
    __asm__ volatile("" : "=r"(dtb_pa), "=r"(args_pa));

    /* ── 1. Set up the kernel stack ────────────────────────────────── */
    stack_setup();

    /* ── 2. Zero BSS segment ───────────────────────────────────────── */
    bss_zero();

    /* ── 3. Save boot arguments to globals ────────────────────────── */
    g_dtb_pa    = dtb_pa;
    g_boot_args = (const uiox_boot_args_t *)(uintptr_t)args_pa;

    /* ── 4. Early console — UART output before MMU ─────────────────── */
    early_puts("\r\n[kernel] UIOX kernel entry (ARM64)\r\n");
    early_puts("[kernel]   dtb_pa   = ");
    early_puthex(dtb_pa);
    early_puts("\r\n[kernel]   args_pa  = ");
    early_puthex(args_pa);
    early_puts("\r\n");

    early_puts("[kernel]   arch     = ARM64 / AArch64\r\n");
    early_puts("[kernel]   MMU      = OFF (enabling in arch_init)\r\n");

    /* ── 5. Architecture init: GIC, MMU, caches, UART irq ─────────── */
    early_puts("[kernel] arch_init()...\r\n");
    int rc = arch_init();
    if (rc != 0) {
        early_puts("[kernel] FATAL: arch_init failed\r\n");
        for (;;) __asm__ volatile("wfi");
    }

    /* ── 6. SoC init (clock, PM) — called inside arch_init() ─────── */
    /* uiox_soc_init() is called from arch_init() →
     *   10_Arch/arm64/src/arch_init.c calls uiox_soc_init()
     *   which calls uiox_soc_init_arm64() from 02_FwHal              */

    /* ── 7. Kernel signature verification (12_ksign) ──────────────── */
    early_puts("[kernel] uiox_ks_boot_entry()...\r\n");
    /*
     * Pass kernel text section boundaries.
     * In a real build these come from linker symbols __text_start/__text_end.
     * Here we use the load address from boot args as a safe default.
     */
    extern uint8_t _text_start[];
    extern uint8_t _text_end[];
    extern uint8_t _rodata_start[];
    extern uint8_t _rodata_end[];

    uiox_ks_boot_entry(
        (const void *)(uintptr_t)g_boot_args->kernel_entry,
        0u,                            /* image_size — 0 = skip re-verify */
        (uintptr_t)_text_start,
        (size_t)(_text_end   - _text_start),
        (uintptr_t)_rodata_start,
        (size_t)(_rodata_end - _rodata_start)
    );

    /* ── 8. Fast-boot milestone: shell is ready ────────────────────── */
    early_puts("[kernel] uiox_fb_shell_ready()...\r\n");
    uiox_fb_master_ctx_t fb_ctx;
    uiox_fb_init(&fb_ctx, UIOX_FB_MODE_COLD, 3000000u);
    uiox_fb_shell_ready(&fb_ctx);
    uiox_fb_report(&fb_ctx);

    /* ── 9. Process control subsystem ─────────────────────────────── */
    early_puts("[kernel] uiox_proc_init()...\r\n");
    uiox_proc_init();

    /* ── 10. Spawn shell — first user-visible prompt ───────────────── */
    early_puts("[kernel] uiox_shell_start()...\r\n");
    uiox_shell_start();

    /* ── Should never reach here ─────────────────────────────────── */
    early_puts("[kernel] FATAL: shell returned — halting\r\n");
    for (;;) __asm__ volatile("wfi");
}

/* =========================================================================
 * ── ARM32 kernel entry ────────────────────────────────────────────────
 *
 * uiox_boot_arch_jump() sets:
 *   r0 = 0  (machine type — unused with DTB)
 *   r1 = 0  (reserved)
 *   r2 = dtb_pa
 *   r3 = args_pa
 * ====================================================================== */
#elif defined(__arm__)

void __attribute__((noreturn))
uiox_kernel_main(void)
{
    /* Capture r2/r3 before any C function corrupts them */
    register uint32_t dtb_pa  __asm__("r2");
    register uint32_t args_pa __asm__("r3");
    __asm__ volatile("" : "=r"(dtb_pa), "=r"(args_pa));

    /* ── 1. Stack and BSS ──────────────────────────────────────────── */
    stack_setup();
    bss_zero();

    /* ── 2. Save boot arguments ────────────────────────────────────── */
    g_dtb_pa    = (uint64_t)dtb_pa;
    g_boot_args = (const uiox_boot_args_t *)(uintptr_t)args_pa;

    /* ── 3. Early console ──────────────────────────────────────────── */
    early_puts("\r\n[kernel] UIOX kernel entry (ARM32)\r\n");
    early_puts("[kernel]   dtb_pa   = ");
    early_puthex((uint64_t)dtb_pa);
    early_puts("\r\n[kernel]   args_pa  = ");
    early_puthex((uint64_t)args_pa);
    early_puts("\r\n");
    early_puts("[kernel]   arch     = ARM32 / ARMv7-A\r\n");

    /* ── 4. Architecture init ──────────────────────────────────────── */
    early_puts("[kernel] arch_init()...\r\n");
    int rc = arch_init();
    if (rc != 0) {
        early_puts("[kernel] FATAL: arch_init failed\r\n");
        for (;;) __asm__ volatile("wfi");
    }

    /* ── 5. Signature verify, fast-boot, process init, shell ──────── */
    early_puts("[kernel] uiox_ks_boot_entry()...\r\n");
    extern uint8_t _text_start[], _text_end[], _rodata_start[], _rodata_end[];
    uiox_ks_boot_entry(
        (const void *)(uintptr_t)g_boot_args->kernel_entry,
        0u,
        (uintptr_t)_text_start,
        (size_t)(_text_end   - _text_start),
        (uintptr_t)_rodata_start,
        (size_t)(_rodata_end - _rodata_start)
    );

    early_puts("[kernel] uiox_fb_shell_ready()...\r\n");
    uiox_fb_master_ctx_t fb_ctx;
    uiox_fb_init(&fb_ctx, UIOX_FB_MODE_COLD, 3000000u);
    uiox_fb_shell_ready(&fb_ctx);
    uiox_fb_report(&fb_ctx);

    early_puts("[kernel] uiox_proc_init()...\r\n");
    uiox_proc_init();

    early_puts("[kernel] uiox_shell_start()...\r\n");
    uiox_shell_start();

    early_puts("[kernel] FATAL: shell returned — halting\r\n");
    for (;;) __asm__ volatile("wfi");
}

/* =========================================================================
 * ── x86-64 kernel entry ───────────────────────────────────────────────
 *
 * uiox_boot_arch_jump() jumps via JMPQ *RAX.
 * SysV AMD64 ABI on entry:
 *   rdi = args_pa
 *   rsi = dtb_pa
 *   rdx = 0
 *
 * Because this is called via JMPQ (not CALL), there is no return
 * address on the stack.  The function is __attribute__((noreturn)).
 * ====================================================================== */
#elif defined(__x86_64__)

void __attribute__((noreturn))
uiox_kernel_main(uiox_boot_args_t *args_pa_ptr,
                 uint64_t          dtb_pa_val)
{
    /*
     * On x86 the bootloader uses the SysV ABI:
     *   rdi → args_pa_ptr   (first argument)
     *   rsi → dtb_pa_val    (second argument)
     * The C function signature captures them directly.
     */

    /* ── 1. Stack and BSS ──────────────────────────────────────────── */
    stack_setup();
    bss_zero();

    /* ── 2. Save boot arguments ────────────────────────────────────── */
    g_dtb_pa    = dtb_pa_val;
    g_boot_args = (const uiox_boot_args_t *)args_pa_ptr;

    /* ── 3. Early console (COM1 16550A) ────────────────────────────── */
    early_puts("\r\n[kernel] UIOX kernel entry (x86-64)\r\n");
    early_puts("[kernel]   args_pa  = ");
    early_puthex((uint64_t)(uintptr_t)args_pa_ptr);
    early_puts("\r\n[kernel]   dtb_pa   = ");
    early_puthex(dtb_pa_val);
    early_puts("\r\n");
    early_puts("[kernel]   arch     = x86-64 / AMD64\r\n");

    /* ── 4. Architecture init ──────────────────────────────────────── */
    early_puts("[kernel] arch_init()...\r\n");
    int rc = arch_init();
    if (rc != 0) {
        early_puts("[kernel] FATAL: arch_init failed\r\n");
        for (;;) __asm__ volatile("hlt");
    }

    /* ── 5. Signature verify ───────────────────────────────────────── */
    early_puts("[kernel] uiox_ks_boot_entry()...\r\n");
    extern uint8_t _text_start[], _text_end[], _rodata_start[], _rodata_end[];
    uiox_ks_boot_entry(
        (const void *)(uintptr_t)g_boot_args->kernel_entry,
        0u,
        (uintptr_t)_text_start,
        (size_t)(_text_end   - _text_start),
        (uintptr_t)_rodata_start,
        (size_t)(_rodata_end - _rodata_start)
    );

    /* ── 6. Fast-boot milestone ────────────────────────────────────── */
    early_puts("[kernel] uiox_fb_shell_ready()...\r\n");
    uiox_fb_master_ctx_t fb_ctx;
    uiox_fb_init(&fb_ctx, UIOX_FB_MODE_COLD, 3000000u);
    uiox_fb_shell_ready(&fb_ctx);
    uiox_fb_report(&fb_ctx);

    /* ── 7. Process init and shell ─────────────────────────────────── */
    early_puts("[kernel] uiox_proc_init()...\r\n");
    uiox_proc_init();

    early_puts("[kernel] uiox_shell_start()...\r\n");
    uiox_shell_start();

    early_puts("[kernel] FATAL: shell returned — halting\r\n");
    for (;;) __asm__ volatile("hlt");
}

/* =========================================================================
 * ── RISC-V 64 kernel entry ────────────────────────────────────────────
 *
 * uiox_boot_arch_jump() (RISC-V variant) sets:
 *   a0 = dtb_pa
 *   a1 = args_pa
 *   then: jalr zero, 0(t0)  (jump to kernel entry)
 *
 * On entry the MMU is OFF (satp=0), S-mode, SIE=0.
 * ====================================================================== */
#elif defined(__riscv)

void __attribute__((noreturn))
uiox_kernel_main(void)
{
    /* Capture a0/a1 before any C function call corrupts them */
    register uint64_t dtb_pa  __asm__("a0");
    register uint64_t args_pa __asm__("a1");
    __asm__ volatile("" : "=r"(dtb_pa), "=r"(args_pa));

    /* ── 1. Stack and BSS ──────────────────────────────────────────── */
    stack_setup();
    bss_zero();

    /* ── 2. Save boot arguments ────────────────────────────────────── */
    g_dtb_pa    = dtb_pa;
    g_boot_args = (const uiox_boot_args_t *)(uintptr_t)args_pa;

    /* ── 3. Early console (NS16550A at 0x10000000) ─────────────────── */
    early_puts("\r\n[kernel] UIOX kernel entry (RISC-V 64)\r\n");
    early_puts("[kernel]   dtb_pa   = ");
    early_puthex(dtb_pa);
    early_puts("\r\n[kernel]   args_pa  = ");
    early_puthex(args_pa);
    early_puts("\r\n");
    early_puts("[kernel]   arch     = RV64IMAFDC\r\n");
    early_puts("[kernel]   satp     = 0 (bare, MMU off)\r\n");

    /* ── 4. Architecture init ──────────────────────────────────────── */
    early_puts("[kernel] arch_init()...\r\n");
    int rc = arch_init();
    if (rc != 0) {
        early_puts("[kernel] FATAL: arch_init failed\r\n");
        for (;;) __asm__ volatile("wfi");
    }

    /* ── 5. Signature verify ───────────────────────────────────────── */
    early_puts("[kernel] uiox_ks_boot_entry()...\r\n");
    extern uint8_t _text_start[], _text_end[], _rodata_start[], _rodata_end[];
    uiox_ks_boot_entry(
        (const void *)(uintptr_t)g_boot_args->kernel_entry,
        0u,
        (uintptr_t)_text_start,
        (size_t)(_text_end   - _text_start),
        (uintptr_t)_rodata_start,
        (size_t)(_rodata_end - _rodata_start)
    );

    /* ── 6. Fast-boot milestone ────────────────────────────────────── */
    early_puts("[kernel] uiox_fb_shell_ready()...\r\n");
    uiox_fb_master_ctx_t fb_ctx;
    uiox_fb_init(&fb_ctx, UIOX_FB_MODE_COLD, 3000000u);
    uiox_fb_shell_ready(&fb_ctx);
    uiox_fb_report(&fb_ctx);

    /* ── 7. Process init and shell ─────────────────────────────────── */
    early_puts("[kernel] uiox_proc_init()...\r\n");
    uiox_proc_init();

    early_puts("[kernel] uiox_shell_start()...\r\n");
    uiox_shell_start();

    early_puts("[kernel] FATAL: shell returned — halting\r\n");
    for (;;) __asm__ volatile("wfi");
}

#else
#  error "uiox_kernel_main.c: unsupported architecture"
#endif


/* =========================================================================
 * Public accessors — called by subsystems after kernel_main runs
 * ====================================================================== */

/**
 * @brief Return the physical address of the Device Tree Blob.
 *        Valid after uiox_kernel_main() has saved it.
 */
uint64_t uiox_kernel_get_dtb_pa(void)
{
    return g_dtb_pa;
}

/**
 * @brief Return a pointer to the boot arguments struct.
 *        Valid after uiox_kernel_main() has saved it.
 */
const uiox_boot_args_t *uiox_kernel_get_boot_args(void)
{
    return g_boot_args;
}
