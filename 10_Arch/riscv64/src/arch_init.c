/*
 * 10_Arch/riscv64/src/arch_init.c
 * UIOX RISC-V 64-bit platform initialisation.
 *
 * Mirrors the structure of 10_Arch/arm64/src/arch_init.c.
 * Called by the UIOX kernel startup sequence.
 */
#include "arch_defs.h"
#include "../../../20_DriverInterfaces/include/hw_types.h"
#include "../../../20_DriverInterfaces/include/mmio.h"
#include "../../../20_DriverInterfaces/include/irq.h"
#include "../../../20_DriverInterfaces/include/cpu.h"
#include "../../../02_FwHal/include/uiox_soc.h"
#include <stdio.h>
#include <string.h>

/* ── UART IRQ handler ────────────────────────────────────────────────── */
static void riscv_uart_handler(int irq, hw_context_t *ctx, void *id)
{
    (void)ctx; (void)id;

    /* Read received byte from NS16550A RBR */
    uint8_t rx = (uint8_t)mmio_read32(UART0_BASE + 0x00u);
    printf("  [riscv/uart_irq] IRQ%d rx=0x%02x '%c'\n",
           irq, rx & 0xFFu,
           (rx >= 0x20u && rx < 0x7Fu) ? (char)rx : '.');

    /* Complete PLIC claim for hart 0 S-mode context */
    mmio_write32(PLIC_CLAIM(PLIC_CTX_SMODE(0u)), (uint32_t)UART0_IRQ);
}

/* ── Timer handler (CLINT MTIP via SBI) ─────────────────────────────── */
static void riscv_timer_handler(int irq, hw_context_t *ctx, void *id)
{
    (void)ctx; (void)id;
    printf("  [riscv/timer_irq] IRQ%d tick\n", irq);

    /* Re-arm timer: 10 ms from now */
    uint64_t now = mmio_read32(CLINT_MTIME);
    now |= ((uint64_t)mmio_read32(CLINT_MTIME + 4u)) << 32u;
    SBI_SET_TIMER(now + 1000000u);  /* assume 100 MHz mtime */

    /* Clear supervisor timer interrupt pending */
    CSR_CLEAR("sip", (1u << 5));
}

/* ── VirtIO block IRQ handler ────────────────────────────────────────── */
static void riscv_virtio_handler(int irq, hw_context_t *ctx, void *id)
{
    (void)ctx; (void)id;
    printf("  [riscv/virtio_irq] IRQ%d VirtIO complete\n", irq);
    mmio_write32(PLIC_CLAIM(PLIC_CTX_SMODE(0u)),
                 (uint32_t)VIRTIO_IRQ_BASE);
}

/* ── PLIC init ───────────────────────────────────────────────────────── */
static void riscv_plic_init(void)
{
    /* Set UART priority = 1 */
    mmio_write32(PLIC_PRIORITY(UART0_IRQ), 1u);
    /* Set VirtIO priority = 1 */
    mmio_write32(PLIC_PRIORITY(VIRTIO_IRQ_BASE), 1u);

    /* Enable UART + VirtIO for hart 0 S-mode (context 1) */
    uint32_t ctx = PLIC_CTX_SMODE(0u);
    mmio_write32(PLIC_ENABLE(ctx),
                 (1u << UART0_IRQ) | (1u << VIRTIO_IRQ_BASE));

    /* Threshold = 0 (allow all non-zero priorities) */
    mmio_write32(PLIC_THRESHOLD(ctx), 0u);

    printf("[riscv] PLIC initialised (UART_IRQ=%u VIRTIO_IRQ=%u)\n",
           UART0_IRQ, VIRTIO_IRQ_BASE);
}

/* ── UART init ───────────────────────────────────────────────────────── */
static void riscv_uart_init(void)
{
    mmio_write32(UART0_IER,  0x00u);   /* Disable interrupts              */
    mmio_write32(UART0_LCR,  0x83u);   /* 8N1 + DLAB                      */
    mmio_write32(UART0_DLL,  0x01u);   /* Divisor lo (115200 baud)         */
    mmio_write32(UART0_DLM,  0x00u);   /* Divisor hi                       */
    mmio_write32(UART0_LCR,  0x03u);   /* 8N1, DLAB off                   */
    mmio_write32(UART0_FCR,  0xC7u);   /* FIFO enable, 14-byte threshold   */
    mmio_write32(UART0_IER,  0x01u);   /* Enable RX interrupt              */
    printf("[riscv] UART0 @ 0x%08lx  115200 8N1\n",
           (unsigned long)UART0_BASE);
}

/* ── Timer init via SBI ──────────────────────────────────────────────── */
static void riscv_timer_init(void)
{
    /* Schedule first timer interrupt 10 ms from now */
    uint64_t now = mmio_read32(CLINT_MTIME);
    now |= ((uint64_t)mmio_read32(CLINT_MTIME + 4u)) << 32u;
    SBI_SET_TIMER(now + 1000000u);

    /* Enable supervisor timer interrupts */
    CSR_SET("sie", (1u << 5));  /* STIE bit */

    printf("[riscv] Timer initialised (SBI SET_TIMER, ~10 ms)\n");
}

/* =========================================================================
 * arch_init — called by UIOX kernel startup
 * ====================================================================== */
int arch_init(void)
{
    printf("\n[arch_init] *** RISC-V 64 (RV64IMAFDC) platform ***\n");
    printf("[riscv]   DRAM  0x%08lx  size %lu MB\n",
           (unsigned long)PHYS_DRAM_BASE,
           (unsigned long)(PHYS_DRAM_SIZE >> 20));
    printf("[riscv]   MMIO  0x%08lx\n",
           (unsigned long)PHYS_MMIO_BASE);

    mmio_init();
    irq_init();

    /* 1. SoC layer */
    uiox_soc_init();

    /* 2. Extended SoC init (CLINT, PLIC, cache) */
    uiox_soc_riscv64_init();

    /* 3. PLIC and peripheral init */
    riscv_plic_init();
    riscv_uart_init();
    riscv_timer_init();

    /* 4. Register IRQ handlers */
    irq_register(UART0_IRQ,      riscv_uart_handler,   NULL);
    irq_register(0 /* timer */,  riscv_timer_handler,  NULL);
    irq_register(VIRTIO_IRQ_BASE,riscv_virtio_handler, NULL);

    irq_enable(UART0_IRQ);
    irq_enable(VIRTIO_IRQ_BASE);

    /* 5. Enable S-mode external interrupts (SEIE) and global IRQ */
    CSR_SET("sie", (1u << 9));   /* SEIE: S-mode external interrupt enable */
    arch_irq_enable();

    printf("[arch_init] RISC-V 64 platform ready\n");
    return 0;
}

/* ── arch_fini ───────────────────────────────────────────────────────── */
void arch_fini(void)
{
    arch_irq_disable();
    irq_free(UART0_IRQ);
    irq_free(VIRTIO_IRQ_BASE);
    uiox_soc_fini();
    printf("[arch_fini] RISC-V 64 platform torn down\n");
}
