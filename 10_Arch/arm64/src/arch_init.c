/*
 * arch/arm64/src/arch_init.c
 * ARMv8-A 64-bit platform initialisation.
 */
#include "arch_defs.h"
#include "../../../20_DriverInterfaces/include/hw_types.h"
#include "../../../20_DriverInterfaces/include/mmio.h"
#include "../../../20_DriverInterfaces/include/irq.h"
#include "../../../20_DriverInterfaces/include/cpu.h"
#include <stdio.h>
#include <string.h>

/* ── UART IRQ handler ────────────────────────────────────── */
static void arm64_uart_handler(int irq, hw_context_t *ctx, void *id)
{
    (void)ctx; (void)id;
    uint32_t rx = mmio_read32(UART0_DR);
    printf("  [arm64/uart_irq] IRQ%d rx=0x%02x '%c'\n",
           irq, rx & 0xFF,
           (rx >= 0x20 && rx < 0x7F) ? (char)rx : '.');
    mmio_write32(UART0_ICR, 0x7FFu);
}

/* ── Generic timer IRQ handler ───────────────────────────── */
static void arm64_timer_handler(int irq, hw_context_t *ctx, void *id)
{
    (void)ctx; (void)id;
    /*
     * Acknowledge the virtual timer by clearing CNTV_CTL_EL0.IMASK
     * or re-arming CNTV_CVAL_EL0. Stub: just log.
     */
    printf("  [arm64/timer_irq] IRQ%d tick\n", irq);
}

/* ── VirtIO block IRQ handler ────────────────────────────── */
static void arm64_virtio_handler(int irq, hw_context_t *ctx, void *id)
{
    (void)ctx; (void)id;
    printf("  [arm64/virtio_irq] IRQ%d VirtIO blk complete\n", irq);
}

/* ── GIC-400 initialisation ──────────────────────────────── */
static void arm64_gic_init(void)
{
    /* 1. Disable distributor */
    mmio_write32(GIC_DIST_CTLR, 0x0u);

    /* 2. Enable all SPIs in distributor (IRQs 32-63 for our map) */
    mmio_write32(GIC_DIST_ISENABLER0, 0xFFFFFFFFu);

    /* 3. Enable CPU interface and allow all priorities */
    mmio_write32(GIC_CPU_CTLR, 0x1u);

    /* 4. Re-enable distributor */
    mmio_write32(GIC_DIST_CTLR, 0x1u);
}

/* ── PL011 UART initialisation ───────────────────────────── */
static void arm64_uart_init(void)
{
    /* Disable UART */
    mmio_write32(UART0_CR, 0x0u);

    /*
     * 115200 baud @ 24 MHz reference:
     *   Divisor = 24e6 / (16 * 115200) = 13.020…
     *   IBRD = 13, FBRD = round(0.020 * 64) = 1
     */
    mmio_write32(UART0_IBRD,  13u);
    mmio_write32(UART0_FBRD,   1u);

    /* 8-bit, no parity, 1 stop, FIFOs enabled */
    mmio_write32(UART0_LCR_H, 0x70u);

    /* Unmask receive interrupt */
    mmio_write32(UART0_IMSC, (1u << 4));

    /* Enable TX, RX, UART */
    mmio_write32(UART0_CR, 0x301u);
}

/* ── Generic timer initialisation ────────────────────────── */
static void arm64_timer_init(void)
{
    /*
     * Set virtual timer to fire after ~10 ms.
     * CNTFRQ_EL0 gives the frequency; read via MRS in real code.
     * Stub: just register the handler.
     */
    (void)0;
}

/* ── arch_init() — called by UIOX kernel startup ─────────── */
int arch_init(void)
{
    printf("[arm64] arch_init: ARMv8-A 64-bit platform\n");
    printf("[arm64]   DRAM  0x%08lX  size %lu MB\n",
           (unsigned long)PHYS_DRAM_BASE,
           (unsigned long)(PHYS_DRAM_SIZE >> 20));
    printf("[arm64]   MMIO  0x%08lX\n",
           (unsigned long)PHYS_MMIO_BASE);

    /* 1. Initialise GIC */
    arm64_gic_init();
    printf("[arm64]   GIC-400 initialised\n");

    /* 2. Initialise UART and register its IRQ */
    arm64_uart_init();
    irq_register(UART0_IRQ, arm64_uart_handler, NULL);
    irq_enable(UART0_IRQ);
    printf("[arm64]   UART0 initialised (IRQ %d)\n", UART0_IRQ);

    /* 3. Initialise generic timer and register its PPI */
    arm64_timer_init();
    irq_register(TIMER0_IRQ, arm64_timer_handler, NULL);
    irq_enable(TIMER0_IRQ);
    printf("[arm64]   Generic timer initialised (IRQ %d)\n", TIMER0_IRQ);

    /* 4. Register VirtIO block IRQ */
    irq_register(VIRTIO_IRQ, arm64_virtio_handler, NULL);
    irq_enable(VIRTIO_IRQ);
    printf("[arm64]   VirtIO blk registered (IRQ %d)\n", VIRTIO_IRQ);

    /* 5. Enable interrupts at CPU level */
    arch_irq_enable();
    printf("[arm64]   IRQs enabled\n");

    return 0;
}
