/**
 * @file    uiox_uart_probe.c
 * @brief   Bind the console to the device registry — family 5, (CMD,15).
 *
 * PATH    30_KIX/30_DSS/20_DD/01_Com/uart/uiox_uart_probe.c
 *
 * uiox_dss_init.c calls this in the family-5 group, beside gpu/hdmi/mon.
 * UIOX_DEVCLASS_UART is (CMD, 15) — uiox_devclass.h:132 keeps index 15
 * clear of the GPU/HDMI/monitor slots 0–2 "because the kernel prints
 * through it".
 *
 * ORDER — read off the two .c files, not invented
 *
 *   1. uiox_uart_hw_init(&s_uart_hw, base, irq, ip, rd8, wr8)
 *        transport + register access.  Does NOT re-enable the UART: the
 *        BSP already did that and early_puts() is printing through it.
 *
 *   2. uiox_uart_if_config(&s_uart_if, &s_uart_hw)
 *        defaults, winsize, primed = true.  No hardware I/O — so the
 *        two calls are adjacent: nothing may run between them.
 *
 *   3. uiox_uart_if_start(&s_uart_if)
 *        registers the RX handler and enables the line.  IRQ path only;
 *        in POLLED mode this is a no-op returning 0.
 *
 *   4. fill the static descriptor, then uiox_dev_bind().
 *
 * @version 1.0.0  @date 2026-10-21
 */
#include "uiox_uart_hw.h"
#include "uiox_uart_if.h"
#include "uiox_uart_ioctl.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"
#include "uiox_soc_hw.h"
#include "uiox_soc_map.h"        /* SOC_UART0_BASE / SOC_UART0_PORT */
#include "uiox_klibc.h"

/* Per-arch IRQ line.  UART0_IRQ is defined per arch in
 * 10_BSP/10_Arch/<arch>/include/irq.h — arm64 33, arm32 69,
 * riscv64 10, x86_64 36 — and reaches us through the arch include
 * path the build already adds (-I$(_ARCH_INC)). */
#include "irq.h"

/* The tty vtable from uiox_uart_blk.c — non-static there, so extern. */
extern const uiox_tty_ops_t s_uart_tty_ops;

/* ── statics ────────────────────────────────────────────────────────
 * uiox_dev_bind() stores the POINTER it is given, not a copy
 * (uiox_dss_init.c:27), so the descriptor must outlive this call.  hw
 * and if are static for the same reason: the descriptor points at both.
 * ─────────────────────────────────────────────────────────────────── */
static uiox_uart_hw_t   s_uart_hw;
static uiox_uart_if_t   s_uart_if;
static uiox_dev_dev_t   s_uart_dev;

/* ── transport selection ────────────────────────────────────────────
 * x86_inb/x86_outb are static inline in uiox_soc_x86.c and in NO header,
 * so this layer cannot name them.  Where the port pair is needed the BSP
 * must supply it — see the note at the bottom of this file.  For the two
 * MMIO transports (PL011, 16550-MMIO) the HAL's own pair is used. */
static uiox_uart_ip_t uart_pick_ip(void)
{
    const uiox_soc_platform_t *p = uiox_soc_platform_get();
    if (p && (p->caps & UIOX_SOC_HW_CAP_PL011)) return UIOX_UART_IP_PL011;
#if defined(__x86_64__)
    return UIOX_UART_IP_16550_PORT;
#else
    return UIOX_UART_IP_16550_MMIO;          /* riscv64 */
#endif
}

int uiox_uart_probe(void)
{
    int rc;
    uiox_uart_ip_t ip = uart_pick_ip();

    /* Discovered beats declared; a literal is the honest fallback —
     * the same rule kernel_main.c uses for the NIC address. */
    uintptr_t base;
    if (ip == UIOX_UART_IP_16550_PORT) {
        base = (uintptr_t)SOC_UART0_PORT;    /* 0x03F8, COM1 */
    } else {
        const uiox_soc_platform_t *p = uiox_soc_platform_get();
        base = (p && p->uart_base) ? (uintptr_t)p->uart_base
                                   : (uintptr_t)SOC_UART0_BASE;
    }

    /* ── 1. transport + registers ─────────────────────────────────── */
    rc = uiox_uart_hw_init(&s_uart_hw, base, UART0_IRQ, ip,
                           uiox_uart_mmio_rd8, uiox_uart_mmio_wr8);
    if (rc < 0) return rc;

    /* ── 2. driver → hardware ─────────────────────────────────────── */
    rc = uiox_uart_if_config(&s_uart_if, &s_uart_hw);
    if (rc < 0) return rc;

    /* ── 3. the interrupt line ────────────────────────────────────── */
    rc = uiox_uart_if_start(&s_uart_if);
    if (rc < 0) {
        /* Registration failed.  Fall back to polled RX rather than
         * refusing to bind: the console is more useful busy-waiting
         * than absent, and the state is visible in hw->irq_owned. */
        s_uart_hw.irq_owned = false;
    }

    /* ── 4. descriptor — every field sourced, none invented ─────────
     *   .cls      UIOX_DEVCLASS_UART = UIOX_DEVCLASS_(CMD, 15)
     *   .unit     0 — first instance; a second would be unit 1
     *   .bus      UIOX_BUS_MMIO — the window is memory-mapped (x86's
     *             port window is mapped the same way for registry
     *             purposes; the accessor pair carries the difference)
     *   .name     "ttyS0" — uiox_devparse() parses that prefix, which
     *             is how mknod resolves a node name back to (cls, unit)
     *   .ops      written once, never reassigned
     *   .drv_priv the driver's own state — the DRIVER owns this slot,
     *             which is what makes uiox_uart_blk.c's cast safe */
    s_uart_dev.cls      = UIOX_DEVCLASS_UART;
    s_uart_dev.unit     = 0u;
    s_uart_dev.bus      = UIOX_BUS_MMIO;
    s_uart_dev.base     = base;
    s_uart_dev.irq      = UART0_IRQ;
    s_uart_dev.name     = "ttyS0";
    s_uart_dev.ops      = (const uiox_dev_ops_t *)&s_uart_tty_ops;
    s_uart_dev.drv_priv = &s_uart_if;

    /* ── 5. bind ────────────────────────────────────────────────────
     * Stores the pointer.  0, or -EEXIST if (cls, unit) is already
     * bound, -ENOSPC past UIOX_DEV_MAX (32). */
    return uiox_dev_bind(&s_uart_dev);
}

/* ═══════════════════════════════════════════════════════════════════
 * BSP follow-up for the x86 transport
 *
 * uiox_soc_x86.c holds x86_inb/x86_outb as `static inline` (lines
 * 35–45), so they are unreachable from 10_DI.  To drive COM1 with the
 * port transport, uiox_soc_x86.c should export a pair in the shape
 * uiox_uart_rd8_t/uiox_uart_wr8_t expect:
 *
 *     extern uint8_t uiox_soc_x86_inb(uintptr_t base, uint32_t off);
 *     extern void    uiox_soc_x86_outb(uintptr_t base, uint32_t off,
 *                                      uint8_t v);
 *
 * and the probe passes those in place of uiox_uart_mmio_* when
 * ip == UIOX_UART_IP_16550_PORT.  Until then the port path is declared
 * and not wired — visible rather than silent.
 * ═══════════════════════════════════════════════════════════════════ */
