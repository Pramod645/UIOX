/*
 * 10_Arch/riscv64/src/arch_init.c
 * RISC-V RV64 architecture initialisation.
 *
 * Scope — architecture layer ONLY (ISA-defined operations):
 *   1. CPU identification via misa / mvendorid CSRs
 *   2. Configure mstatus / sstatus (ISA register)
 *   3. Install stvec (supervisor trap vector — ISA register)
 *   4. Configure PLIC for the architecture-default UART IRQ
 *      (PLIC register layout is RISC-V architecture-defined;
 *       base addresses from arch_defs.h QEMU default)
 *   5. Configure CLINT to start the timer tick
 *      (CLINT is RISC-V architecture — same layout on all RV64 cores)
 *   6. Register IRQ handlers via 20_DriverInterfaces
 *   7. Enable S-mode external interrupts (sie.SEIE — ISA register)
 *
 * NOT in this file — belongs in 03_SoC:
 *   NS16550A baud rate config    → 03_SoC/uiox_soc_riscv64.c
 *   VirtIO device init           → 03_SoC/uiox_soc_riscv64.c
 *   SBI feature probing          → 03_SoC/uiox_soc_riscv64.c
 *   Clock / PLL setup            → 03_SoC/uiox_soc_clk.c
 *   uiox_soc_init() / soc_fini() → called by uiox_kernel_main(), NOT here
 *
 * CRITICAL FIX vs previous version:
 *   Old: arch_init() called uiox_soc_init() directly — WRONG layer order.
 *   New: arch_init() does ISA setup only.
 *        uiox_kernel_main() calls uiox_soc_init() after arch_init() returns.
 *
 * Kernel call chain:
 *   uiox_kernel_main()
 *     → arch_init()       ← THIS FILE
 *     → uiox_soc_init()   ← 03_SoC (called next by kernel, NOT from here)
 */

 #include "arch_defs.h"
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"   /* replaces <stdio.h>  */
 #include "../../../03_SoC/include/uiox_soc_string.h"  /* replaces <string.h> */
 
 /* ── Forward declarations of IRQ handlers ────────────────────────────
  * Implementations live in 20_DriverInterfaces or 03_SoC.
  * ──────────────────────────────────────────────────────────────────── */
 extern void uart0_irq_handler (int irq, hw_context_t *ctx, void *id);
 extern void timer0_irq_handler(int irq, hw_context_t *ctx, void *id);
 
 /* ── PLIC context: S-mode context for hart N = 2*N+1 ─────────────── */
 #define PLIC_CTX_SMODE(hart)  (2u * (hart) + 1u)
 
 /* =========================================================================
  * 1. CPU identification
  *    ISA CSRs: misa, mvendorid, marchid — same layout on every RV64 core.
  *    Uses arch_csrr_* inline functions from arch_defs.h (no statement
  *    expressions — pedantic-clean).
  * ====================================================================== */
 static void riscv_cpu_identify(void)
 {
     unsigned long misa      = arch_csrr_misa();
     unsigned long mvendorid = arch_csrr_mvendorid();
     unsigned long marchid   = arch_csrr_marchid();
     unsigned long mhartid   = arch_csrr_mhartid();
 
     printf("[riscv] hart=%lu misa=0x%016lx vendor=0x%lx arch=0x%lx\n",
            mhartid, misa, mvendorid, marchid);
 
     /* Decode misa extension bits */
     if (misa & (1UL << ('I' - 'A'))) printf("[riscv]   +I (Base integer)\n");
     if (misa & (1UL << ('M' - 'A'))) printf("[riscv]   +M (Multiply)\n");
     if (misa & (1UL << ('A' - 'A'))) printf("[riscv]   +A (Atomic)\n");
     if (misa & (1UL << ('F' - 'A'))) printf("[riscv]   +F (Float)\n");
     if (misa & (1UL << ('D' - 'A'))) printf("[riscv]   +D (Double)\n");
     if (misa & (1UL << ('C' - 'A'))) printf("[riscv]   +C (Compressed)\n");
     if (misa & (1UL << ('S' - 'A'))) printf("[riscv]   +S (Supervisor)\n");
     if (misa & (1UL << ('U' - 'A'))) printf("[riscv]   +U (User)\n");
 }

/* =========================================================================
 * 2. mstatus / sstatus setup  (ISA register — same on every RV64 core)
 *    Set SUM=0 (supervisor cannot access user pages by default).
 *    MXR=0 (executable pages not readable as data).
 * ====================================================================== */
static void riscv_mstatus_init(void)
{
    /*
     * We run in S-mode after SBI hands off.
     * Clear SIE (supervisor interrupt enable) while we configure —
     * it will be set at the end of arch_init via arch_csrs_sie().
     */
    arch_csrc_sstatus(MSTATUS_SIE);
    arch_dsb_sy();
    printf("[riscv] sstatus.SIE cleared for safe configuration\n");
}

/* =========================================================================
 * 3. stvec — Supervisor Trap Vector  (ISA register)
 *    Install the kernel exception/interrupt dispatch table.
 *    Vector table symbol provided by the kernel linker script.
 * ====================================================================== */
extern unsigned long _vector_table[];   /* kernel linker symbol */

static void riscv_stvec_install(void)
{
    /*
     * stvec mode bits [1:0]:
     *   00 = Direct   — all traps jump to BASE
     *   01 = Vectored — async interrupts jump to BASE + cause*4
     * Use Direct mode (0) — kernel dispatch handles routing.
     */
    unsigned long vbase = (unsigned long)_vector_table & ~0x3UL;
    arch_csrw_stvec(vbase);   /* mode = Direct */
    arch_isb();
    printf("[riscv] stvec installed @ 0x%016lx (Direct mode)\n", vbase);
}

/* =========================================================================
 * 4. PLIC configuration  (RISC-V architecture-defined)
 *    PLIC register layout is specified by the RISC-V PLIC specification.
 *    Base address PLIC_BASE comes from arch_defs.h (QEMU virt default).
 *    SoC backends in 03_SoC may use a different base via uiox_soc_map.h
 *    but the register offsets are the same on every RISC-V platform.
 * ====================================================================== */
static void riscv_plic_init(void)
{
    unsigned int ctx = PLIC_CTX_SMODE(0u);   /* hart 0, S-mode */

    /* Set UART0 IRQ priority = 1 (above threshold 0) */
    mmio_write32(PLIC_PRIORITY(UART0_IRQ), 1u);

    /* Enable UART0 IRQ in the S-mode enable register */
    unsigned int word = (unsigned int)UART0_IRQ / 32u;
    unsigned int bit  = (unsigned int)UART0_IRQ % 32u;
    unsigned int en   = mmio_read32(PLIC_ENABLE(ctx, word));
    mmio_write32(PLIC_ENABLE(ctx, word), en | (1u << bit));

    /* Threshold = 0 — allow all priorities >= 1 */
    mmio_write32(PLIC_THRESHOLD(ctx), 0u);

    printf("[riscv] PLIC @ 0x%08lx  UART0_IRQ=%u S-mode ctx=%u\n",
           (unsigned long)PLIC_BASE, (unsigned int)UART0_IRQ, ctx);
}

/* =========================================================================
 * 5. CLINT timer  (RISC-V architecture-defined)
 *    CLINT register layout (MSIP, MTIMECMP, MTIME) is specified by the
 *    RISC-V privileged specification.  Same offsets on all RV64 platforms.
 *    We set MTIMECMP for hart 0 to fire at MTIME + freq/100 (100 Hz).
 * ====================================================================== */
static void riscv_clint_init(void)
{
    /* Clear any pending software interrupt for hart 0 */
    mmio_write32(CLINT_MSIP(0u), 0u);

    /* Read current MTIME and set MTIMECMP = MTIME + freq/100 */
    unsigned long mtime =
        *(volatile unsigned long *)(unsigned long)CLINT_MTIME;
    unsigned long freq = 10000000UL;   /* 10 MHz nominal (QEMU virt) */
    unsigned long cmp  = mtime + freq / 100u;

    /* Write 64-bit MTIMECMP as two 32-bit writes (little-endian) */
    mmio_write32(CLINT_MTIMECMP(0u) + 4u, (unsigned int)(cmp >> 32u));
    mmio_write32(CLINT_MTIMECMP(0u),       (unsigned int)(cmp & 0xFFFFFFFFu));

    printf("[riscv] CLINT @ 0x%08lx  MTIME=0x%lx  CMP=0x%lx (100 Hz)\n",
           (unsigned long)CLINT_BASE, mtime, cmp);
}

/* =========================================================================
 * arch_init — called by uiox_kernel_main() before uiox_soc_init()
 *
 * IMPORTANT: this function must NOT call uiox_soc_init() or
 * uiox_soc_riscv64_init().  Those are called by uiox_kernel_main()
 * after arch_init() returns.  Mixing layers here was the main bug
 * in the previous version.
 * ====================================================================== */
int arch_init(void)
{
    printf("\n[arch_init] *** RISC-V 64 (RV64IMAFDC_Zicsr) ***\n");
    printf("[riscv]   DRAM  0x%08lx  %lu MB\n",
           (unsigned long)PHYS_DRAM_BASE,
           (unsigned long)(PHYS_DRAM_SIZE >> 20));

    /* 1. Identify CPU via misa / mvendorid / marchid (ISA CSRs) */
    riscv_cpu_identify();

    /* 2. Configure sstatus — clear SIE during init (ISA register) */
    riscv_mstatus_init();

    /* 3. Install stvec exception/interrupt vector (ISA register) */
    riscv_stvec_install();

    /* 4. Configure PLIC for UART IRQ (RISC-V arch-defined layout) */
    riscv_plic_init();

    /* 5. Start CLINT timer tick at 100 Hz (RISC-V arch-defined) */
    riscv_clint_init();

    /* 6. Initialise driver-interface layer */
    mmio_init();
    irq_init();

    /* 7. Register IRQ handlers
     *    UART0_IRQ and ARCH_TIMER_IRQ_PHYS come from arch_defs.h.
     *    Handler bodies are in 20_DriverInterfaces or 03_SoC.        */
    irq_request(UART0_IRQ,           uart0_irq_handler,  NULL, "uart0");
    irq_request(ARCH_TIMER_IRQ_PHYS, timer0_irq_handler, NULL, "timer");

    irq_enable(UART0_IRQ);
    irq_enable(ARCH_TIMER_IRQ_PHYS);

    /* 8. Enable S-mode external (PLIC) and timer interrupts
     *    sie.SEIE = bit 9,  sie.STIE = bit 5  (ISA register)
     *    Uses arch_csrs_sie() inline function from arch_defs.h —
     *    no statement-expression macro, pedantic-clean.            */
    arch_csrs_sie(SIE_SEIE | SIE_STIE);

    /* 9. Unmask global S-mode interrupts via sstatus.SIE */
    arch_csrs_sstatus(MSTATUS_SIE);

    printf("[arch_init] RISC-V 64 architecture layer ready\n");
    /*
     * uiox_soc_init() is called NEXT by uiox_kernel_main().
     * It will handle NS16550A baud config, VirtIO, SBI probing, etc.
     */
    return 0;
}

/* =========================================================================
 * arch_fini — called at orderly shutdown
 * ====================================================================== */
void arch_fini(void)
{
    /* Mask all S-mode interrupts */
    arch_csrc_sstatus(MSTATUS_SIE);
    arch_csrc_sie(SIE_SEIE | SIE_STIE);

    irq_free(UART0_IRQ);
    irq_free(ARCH_TIMER_IRQ_PHYS);

    printf("[arch_fini] RISC-V 64 architecture layer torn down\n");
}
