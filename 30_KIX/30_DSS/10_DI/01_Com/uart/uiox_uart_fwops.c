/**
 * @file  uiox_uart_fwops.c
 * @brief UIOX UART HAL — transport, register access, ring, ISR body.
 *
 * PATH  30_KIX/30_DSS/10_DI/01_Com/uart/uiox_uart_fwops.c
 *
 * CR / MCR / LCR are never rewritten here.  The BSP programmed the UART
 * at boot (uiox_soc_arm64.c:73 sets CR = 0x301; the 16550 files set
 * MCR/LCR/FCR) and early_puts() is still printing through it.
 * Re-enabling would reset the FIFO and drop console output.
 *
 * @version 1.0.0  @date 2026-10-21
 */
#include "uiox_uart_hw.h"
#include "uiox_soc_irq.h"

/* ── the MMIO pair the BSP passes for ARM and riscv ─────────────────
 * 32-bit wide reads/writes on the mapped window; the caller masks to 8
 * bits where it matters (the 16550 registers are byte-wide in a word). */
uint8_t uiox_uart_mmio_rd8(uintptr_t base, uint32_t off)
{ return (uint8_t)(*((volatile uint32_t *)(base + off)) & 0xFFu); }

void uiox_uart_mmio_wr8(uintptr_t base, uint32_t off, uint8_t v)
{ *((volatile uint32_t *)(base + off)) = (uint32_t)v; }

/* ── TX ─────────────────────────────────────────────────────────────
 * Bounded: a dead TX must not spin forever inside a kernel path. */
void uiox_uart_hw_putc(uiox_uart_hw_t *hw, char c)
{
    if (!hw || !hw->initialized) return;
    for (uint32_t i = 0u; i < UIOX_UART_POLL_SPINS; i++)
        if (uiox_uart_tx_ready(hw)) break;
    if (hw->ip == UIOX_UART_IP_PL011)
        *((volatile uint32_t *)(hw->base + UIOX_PL011_DR)) = (uint32_t)(uint8_t)c;
    else
        hw->wr8(hw->base, UIOX_16550_RBR_THR, (uint8_t)c);
}

/* ── RX: path (a) ─────────────────────────────────────────────────── */
int uiox_uart_hw_getc_poll(uiox_uart_hw_t *hw, uint8_t *c, uint32_t spins)
{
    if (!hw || !c || !hw->initialized) return -1;
    for (uint32_t i = 0u; i < spins; i++) {
        if (uiox_uart_rx_ready(hw)) {
            *c = (hw->ip == UIOX_UART_IP_PL011)
               ? (uint8_t)(*((volatile uint32_t *)(hw->base + UIOX_PL011_DR))
                           & 0xFFu)
               : hw->rd8(hw->base, UIOX_16550_RBR_THR);
            return 0;
        }
    }
    return 1;                    /* EAGAIN — nothing arrived */
}

/* ── the ring: single producer (ISR) / single consumer (read) ─────── */
static void ring_push(uiox_uart_hw_t *hw, uint8_t c)
{
    uint32_t next = (hw->rx.head + 1u) % UIOX_UART_RX_RING;
    if (next == hw->rx.tail) { hw->rx.overruns++; return; }  /* full: drop */
    hw->rx.buf[hw->rx.head] = c;
    hw->rx.head = next;          /* publish AFTER the store */
}

bool uiox_uart_hw_rx_pending(uiox_uart_hw_t *hw)
{ return hw && hw->rx.head != hw->rx.tail; }

int uiox_uart_hw_rx_pop(uiox_uart_hw_t *hw, uint8_t *c)
{
    if (!hw || !c || hw->rx.head == hw->rx.tail) return 1;
    *c = hw->rx.buf[hw->rx.tail];
    hw->rx.tail = (hw->rx.tail + 1u) % UIOX_UART_RX_RING;
    return 0;
}

/* ── RX: path (b), the ISR body ────────────────────────────────────
 * One byte (or drain the FIFO) into the ring, then ack.  NO echo and NO
 * putc here: putc is a bounded busy-wait, which is fine in process
 * context and wrong inside an interrupt.  Echo happens in read().
 *
 * The dispatcher acks for us — uiox_soc_irq.h:90, "invokes handler,
 * acknowledges the interrupt" — so the ICR write below is belt-and-
 * braces for a level-triggered line that stays asserted while RX holds
 * data.  16550 acks by reading IIR instead. */
void uiox_uart_hw_isr(uiox_uart_hw_t *hw)
{
    if (!hw || !hw->initialized) return;

    if (hw->ip == UIOX_UART_IP_PL011) {
        uint32_t mis = *((volatile uint32_t *)(hw->base + UIOX_PL011_MIS));
        if (mis & UIOX_PL011_INT_RXIM) {
            while (!(*((volatile uint32_t *)(hw->base + UIOX_PL011_FR))
                     & UIOX_PL011_FR_RXFE))
                ring_push(hw, (uint8_t)(*((volatile uint32_t *)
                              (hw->base + UIOX_PL011_DR)) & 0xFFu));
        }
        *((volatile uint32_t *)(hw->base + UIOX_PL011_ICR)) = UIOX_PL011_INT_ALL;
    } else {
        (void)hw->rd8(hw->base, UIOX_16550_IIR_FCR);   /* ack */
        while (uiox_uart_rx_ready(hw))
            ring_push(hw, hw->rd8(hw->base, UIOX_16550_RBR_THR));
    }
}

/* The registered callback: (irq, priv).  priv is the hw pointer the
 * probe passed, so the ISR needs no global lookup. */
static void uiox_uart_irq_handler(uiox_uint32_t irq, void *priv)
{ (void)irq; uiox_uart_hw_isr((uiox_uart_hw_t *)priv); }

/* ── interrupt-line ownership ─────────────────────────────────────── */
int uiox_uart_hw_irq_attach(uiox_uart_hw_t *hw)
{
    if (!hw) return -1;
    if (uiox_soc_irq_register(hw->irq, uiox_uart_irq_handler, hw) != UIOX_SOC_OK)
        return -1;
    uiox_soc_irq_enable(hw->irq);
    hw->irq_owned = true;
    return 0;
}

void uiox_uart_hw_irq_detach(uiox_uart_hw_t *hw)
{
    if (!hw || !hw->irq_owned) return;
    uiox_soc_irq_disable(hw->irq);
    uiox_soc_irq_unregister(hw->irq);
    hw->irq_owned = false;
}

/* ── baud ───────────────────────────────────────────────────────────
 * PL011: IBRD/FBRD from a 24 MHz reference (the QEMU virt default).
 * 16550: the DLAB dance — LCR |= 0x80, write DLL/DLM, LCR &= ~0x80 —
 * which is exactly what rv_uart_init() and x86_uart_init() do at boot
 * (they write LCR 0x83, then DLL/DLM, then LCR 0x03). */
int uiox_uart_hw_set_baud(uiox_uart_hw_t *hw, uint32_t hz)
{
    if (!hw || hz == 0u) return -1;

    if (hw->ip == UIOX_UART_IP_PL011) {
        uint32_t clk = 24000000u;
        uint32_t div = (8u * clk) / hz;
        *((volatile uint32_t *)(hw->base + UIOX_PL011_IBRD)) =
            (div >> 6) & 0xFFFFu;
        *((volatile uint32_t *)(hw->base + UIOX_PL011_FBRD)) = div & 0x3Fu;
    } else {
        uint32_t div = 115200u / hz;        /* DLL for a 1.8432 MHz ref */
        if (div == 0u) div = 1u;
        uint8_t lcr = hw->rd8(hw->base, UIOX_16550_LCR);
        hw->wr8(hw->base, UIOX_16550_LCR, (uint8_t)(lcr | UIOX_16550_LCR_DLAB));
        hw->wr8(hw->base, UIOX_16550_RBR_THR, (uint8_t)(div & 0xFFu));
        hw->wr8(hw->base, UIOX_16550_IER_DLM, (uint8_t)((div >> 8) & 0xFFu));
        hw->wr8(hw->base, UIOX_16550_LCR,
                (uint8_t)(lcr & (uint8_t)~UIOX_16550_LCR_DLAB));
    }
    hw->baud = hz;
    return 0;      /* CR / MCR untouched — see banner */
}

/* ── lifecycle ────────────────────────────────────────────────────── */
int uiox_uart_hw_init(uiox_uart_hw_t *hw, uintptr_t base, uint32_t irq,
                      uiox_uart_ip_t ip, uiox_uart_rd8_t rd8,
                      uiox_uart_wr8_t wr8)
{
    if (!hw || !base || !rd8 || !wr8) return -1;
    for (uiox_uint32_t i = 0u; i < sizeof(*hw); i++) ((uint8_t *)hw)[i] = 0u;
    hw->base        = base;
    hw->irq         = irq;
    hw->ip          = ip;
    hw->rd8         = rd8;
    hw->wr8         = wr8;
    hw->baud        = 115200u;
    hw->initialized = true;

    /* PL011: IMSC already has RXIM (arm64.c:72 sets 1u << 4).  Re-asserting
     * is harmless and makes the HAL self-sufficient if probe ever precedes
     * the BSP.  16550: enable only the received-data interrupt in IER. */
    if (ip == UIOX_UART_IP_PL011)
        *((volatile uint32_t *)(hw->base + UIOX_PL011_IMSC)) = UIOX_PL011_INT_RXIM;
    else
        hw->wr8(hw->base, UIOX_16550_IER_DLM, 0x01u);   /* RDA interrupt */

    return 0;
}

void uiox_uart_hw_deinit(uiox_uart_hw_t *hw)
{
    if (!hw) return;
    uiox_uart_hw_irq_detach(hw);
    hw->initialized = false;
}
