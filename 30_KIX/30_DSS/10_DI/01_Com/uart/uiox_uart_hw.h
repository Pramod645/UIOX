/**
 * @file    uiox_uart_hw.h
 * @brief   UIOX UART HAL — PL011 (ARM) and NS16550A (riscv64, x86_64).
 *
 * PATH    30_KIX/30_DSS/10_DI/01_Com/uart/uiox_uart_hw.h
 *
 * ── four architectures, three transports ────────────────────────────
 *   arm64  PL011 MMIO         SOC_UART0_BASE 0x101F1000   IRQ 33
 *   arm32  PL011 MMIO         per uiox_soc_map.h           IRQ 69
 *   riscv64 NS16550A MMIO     0x10000000                   IRQ 10
 *   x86_64  NS16550A port I/O 0x03F8                       IRQ 36
 *
 * The PL011 offsets are the ones uiox_soc_arm64.c:68–73 and
 * uiox_soc_arm32.c:75–80 already drive.  The 16550 ones are the ones
 * uiox_soc_riscv64_init.c and uiox_soc_x86.c already drive (0x00 THR/RBR,
 * 0x01 IER/DLM, 0x02 IIR/FCR, 0x03 LCR, 0x04 MCR).  Copied to MATCH,
 * not re-derived.
 *
 * ── why the transport is function pointers ──────────────────────────
 * x86_inb/x86_outb are `static inline` in uiox_soc_x86.c:35–45 and in
 * NO header, so this layer cannot name them.  The BSP supplies rd8/wr8
 * and the HAL stays transport-blind — the same idea as the 18-op vtable
 * in uiox_soc_hw.h, one level down.
 *
 * ── the UART is already alive ───────────────────────────────────────
 * The BSP programs it at boot (CR = 0x301 on PL011, MCR/LCR on 16550)
 * because early_puts() prints through it.  This HAL never re-enables the
 * controller: writing CR would reset the FIFO and drop console output.
 *
 * ── no class-ops typedef here ───────────────────────────────────────
 * The console's byte-stream vtable (uiox_uart_class_ops_t) is declared
 * in the CLASS's own header — 20_DD/01_Com/uart/uiox_uart_if.h — the
 * same placement uiox_gpu_ops_t has in uiox_gpu_if.h.  A CMD class's
 * own vtable is private to its class; 10_DI declares the HAL and never
 * the class's dispatch surface.
 *
 * @version 1.0.0  @date 2026-10-24
 */
#ifndef UIOX_UART_HW_H
#define UIOX_UART_HW_H

#include "uiox_klibc.h"

/* ── PL011 register offsets ─────────────────────────────────────────
 * From uiox_soc_arm64.c:68–73 (identical in uiox_soc_arm32.c:75–80). */
#define UIOX_PL011_DR        0x000u
#define UIOX_PL011_FR        0x018u
#define UIOX_PL011_IBRD      0x024u
#define UIOX_PL011_FBRD      0x028u
#define UIOX_PL011_LCR_H     0x02Cu
#define UIOX_PL011_CR        0x030u
#define UIOX_PL011_IMSC      0x038u
#define UIOX_PL011_RIS       0x03Cu
#define UIOX_PL011_MIS       0x040u
#define UIOX_PL011_ICR       0x044u

#define UIOX_PL011_FR_TXFE   (1u << 7)   /* FIFO EMPTY  */
#define UIOX_PL011_FR_RXFE   (1u << 4)   /* FIFO EMPTY  */

#define UIOX_PL011_CR_BOOT   0x301u      /* TXE|RXE|UARTEN, set at boot */
#define UIOX_PL011_INT_RXIM  (1u << 4)
#define UIOX_PL011_INT_ALL   0x7FFu

/* ── NS16550A register offsets ──────────────────────────────────────
 * From uiox_soc_riscv64_init.c's rv_uart_init() and uiox_soc_x86.c's
 * x86_uart_init(): 0x04 MCR, 0x03 LCR (0x83 = DLAB|8N1), 0x00 DLL/THR,
 * 0x01 DLM/IER, 0x02 FCR/IIR. */
#define UIOX_16550_RBR_THR   0x000u
#define UIOX_16550_IER_DLM   0x001u
#define UIOX_16550_IIR_FCR   0x002u
#define UIOX_16550_LCR       0x003u
#define UIOX_16550_MCR       0x004u
#define UIOX_16550_LSR       0x005u

#define UIOX_16550_LCR_DLAB  0x80u
#define UIOX_16550_LCR_8N1   0x03u
#define UIOX_16550_FCR_ON    0xC7u       /* enable, clear, 14-byte trigger */
#define UIOX_16550_MCR_MMIO  0x01u       /* riscv writes 0x01 */
#define UIOX_16550_MCR_PORT  0x0Bu       /* x86 writes 0x0B: OUT2|DTR|RTS */
#define UIOX_16550_LSR_DR    (1u << 0)   /* data READY  */
#define UIOX_16550_LSR_THRE  (1u << 5)   /* TX holding empty */

/* ── RX strategy — the (a)/(b) choice, selectable at build time ─────
 *   POLLED  busy-waits on the status register.  No ISR, no
 *           registration; works before the GIC/PLIC is up.  Costs CPU.
 *   IRQ     a registered handler fills a ring; read() drains it. */
#define UIOX_UART_RX_POLLED  0
#define UIOX_UART_RX_IRQ     1
#ifndef UIOX_UART_RX_MODE
#define UIOX_UART_RX_MODE    UIOX_UART_RX_IRQ
#endif

#define UIOX_UART_RX_RING    256u
#define UIOX_UART_POLL_SPINS 100000u

typedef enum {
    UIOX_UART_IP_PL011 = 0,      /* arm64, arm32 — MMIO              */
    UIOX_UART_IP_16550_MMIO,     /* riscv64      — MMIO @ 0x10000000 */
    UIOX_UART_IP_16550_PORT,     /* x86_64       — port I/O @ 0x03F8 */
} uiox_uart_ip_t;

/* Transport accessor, supplied by the BSP. */
typedef uint8_t (*uiox_uart_rd8_t)(uintptr_t base, uint32_t off);
typedef void    (*uiox_uart_wr8_t)(uintptr_t base, uint32_t off, uint8_t v);

/* ── the ring — single producer (ISR), single consumer (read) ─────── */
typedef struct {
    volatile uint8_t  buf[UIOX_UART_RX_RING];
    volatile uint32_t head;      /* ISR advances    */
    volatile uint32_t tail;      /* read() advances */
    uint32_t          overruns;
} uiox_uart_ring_t;

typedef struct {
    uintptr_t          base;
    uint32_t           irq;
    uint32_t           baud;
    uiox_uart_ip_t     ip;
    uiox_uart_rd8_t    rd8;      /* BSP-supplied — see banner */
    uiox_uart_wr8_t    wr8;
    bool               initialized;
    bool               irq_owned;/* true when this driver registered */
    uiox_uart_ring_t   rx;
    void              *priv;     /* the driver's uiox_uart_if_t — set
                                  * by the probe, read in-kernel */
} uiox_uart_hw_t;

/* ── HAL API ──────────────────────────────────────────────────────── */

int  uiox_uart_hw_init  (uiox_uart_hw_t *hw, uintptr_t base, uint32_t irq,
                         uiox_uart_ip_t ip, uiox_uart_rd8_t rd8,
                         uiox_uart_wr8_t wr8);
void uiox_uart_hw_deinit(uiox_uart_hw_t *hw);

void uiox_uart_hw_putc  (uiox_uart_hw_t *hw, char c);

int  uiox_uart_hw_getc_poll (uiox_uart_hw_t *hw, uint8_t *c, uint32_t spins);

bool uiox_uart_hw_rx_pending(uiox_uart_hw_t *hw);
int  uiox_uart_hw_rx_pop    (uiox_uart_hw_t *hw, uint8_t *c);
void uiox_uart_hw_isr       (uiox_uart_hw_t *hw);

int  uiox_uart_hw_irq_attach(uiox_uart_hw_t *hw);
void uiox_uart_hw_irq_detach(uiox_uart_hw_t *hw);

int  uiox_uart_hw_set_baud  (uiox_uart_hw_t *hw, uint32_t hz);

uint8_t uiox_uart_mmio_rd8(uintptr_t base, uint32_t off);
void    uiox_uart_mmio_wr8(uintptr_t base, uint32_t off, uint8_t v);

/* ── shared helpers ────────────────────────────────────────────────
 * PL011's FR.RXFE is set when the FIFO is EMPTY; 16550's LSR.DR is set
 * when a byte is READY.  A single `& RXFE` check would make the console
 * work on ARM and hang on riscv and x86. */
static inline bool uiox_uart_rx_ready(uiox_uart_hw_t *hw)
{
    if (hw->ip == UIOX_UART_IP_PL011)
        return !(*((volatile uint32_t *)(hw->base + UIOX_PL011_FR))
                 & UIOX_PL011_FR_RXFE);
    return (hw->rd8(hw->base, UIOX_16550_LSR) & UIOX_16550_LSR_DR) != 0u;
}

static inline bool uiox_uart_tx_ready(uiox_uart_hw_t *hw)
{
    if (hw->ip == UIOX_UART_IP_PL011)
        return (*((volatile uint32_t *)(hw->base + UIOX_PL011_FR))
                & UIOX_PL011_FR_TXFE) != 0u;
    return (hw->rd8(hw->base, UIOX_16550_LSR) & UIOX_16550_LSR_THRE) != 0u;
}

#endif /* UIOX_UART_HW_H */
