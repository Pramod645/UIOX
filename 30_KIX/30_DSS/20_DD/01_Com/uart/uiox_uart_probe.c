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
 *   3. hw->priv = &s_uart_if
 *        the driver's back-pointer.  Nothing above the class walks it
 *        (family 5 reaches ioctl through dev->ops), but the ISR and any
 *        in-kernel caller find the uiox_uart_if_t from the hw this way.
 *
 *   4. uiox_uart_if_start(&s_uart_if)
 *        registers the RX handler and enables the line.
 *
 *   5. fill the static descriptor, then uiox_dev_bind().
 *
 * @version 1.0.0  @date 2026-10-24
 */
#include "uiox_uart_hw.h"
#include "uiox_uart_if.h"
#include "uiox_uart_ioctl.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"
#include "uiox_soc_hw.h"
#include "uiox_soc_map.h"        /* SOC_UART0_BASE / SOC_UART0_PORT */
#include "uiox_klibc.h"

/* Per-arch UART IRQ line.  The arch header that carries it
 * (10_BSP/10_Arch/<arch>/include/irq.h — UART0_IRQ 33 on arm64) is not
 * on this layer's include path, and the SoC map holds no UART IRQ
 * (grep 'UART_IRQ' uiox_soc_map.h is empty).  So the line is named here
 * with the same "discovered beats declared, a literal is the honest
 * fallback" rule kernel_main.c uses for the NIC address. */
#if defined(__aarch64__)
#  define UIOX_UART0_IRQ   33u   /* GIC SPI  1 — QEMU virt PL011 */
#elif defined(__arm__)
#  define UIOX_UART0_IRQ   69u   /* GIC SPI 37 — versatilepb PL011 */
#elif defined(__riscv)
#  define UIOX_UART0_IRQ   10u   /* PLIC source 10 — NS16550A */
#else
#  define UIOX_UART0_IRQ   36u   /* APIC vector 36 — COM1 */
#endif

/* Both vtables from uiox_uart_blk.c — non-static there, so extern.
 * dev->ops gets the family one; the class ops are the class's own,
 * declared in uiox_uart_if.h, reached in-kernel. */
extern const uiox_devctl_ops_t     s_uart_devctl_ops;
extern const uiox_uart_class_ops_t s_uart_class_ops;

/* ── statics ────────────────────────────────────────────────────────
 * uiox_dev_bind() stores the POINTER it is given, not a copy
 * (uiox_dss_init.c:27), so the descriptor must outlive this call.  hw
 * and if are static for the same reason: the descriptor points at both.
 * ─────────────────────────────────────────────────────────────────── */
static uiox_uart_hw_t   s_uart_hw;
static uiox_uart_if_t   s_uart_if;
static uiox_dev_dev_t   s_uart_dev;

/* ── transport selection ────────────────────────────────────────────
 * Pure compile-time selection on the target arch:
 *   x86_64  16550 on port I/O (COM1) — no PL011 exists.
 *   others  PL011 MMIO at SOC_UART0_BASE (arm64, arm32).
 *
 * The runtime platform accessor (uiox_soc_platform_get) is NOT used
 * here: it is not declared in any header this layer reaches, and the
 * arm64 boot path does not populate it (kernel_main.c's own comment
 * notes uiox_kernel_get_soc() returns NULL without a /soc node). */
static uiox_uart_ip_t uart_pick_ip(void)
{
#if defined(__x86_64__)
    return UIOX_UART_IP_16550_PORT;
#else
    return UIOX_UART_IP_PL011;
#endif
}

int uiox_uart_probe(void)
{
    int rc;
    uiox_uart_ip_t ip = uart_pick_ip();

    /* Discovered beats declared; a literal is the honest fallback —
     * the same rule kernel_main.c uses for the NIC address.
     *
     * arm64/arm32: SOC_UART0_BASE (uiox_soc_map.h:147, 0x101F1000).
     * x86_64:      SOC_UART0_PORT (0x03F8, COM1).  Each arch's macro is
     *              inside its own #if in uiox_soc_map.h, so both cannot
     *              be named at once. */
    uintptr_t base;
#if defined(__x86_64__)
    base = (uintptr_t)SOC_UART0_PORT;    /* 0x03F8, COM1 */
#else
    base = (uintptr_t)SOC_UART0_BASE;    /* PL011, per uiox_soc_map.h:147 */
#endif

    /* ── 1. transport + registers ─────────────────────────────────── */
    rc = uiox_uart_hw_init(&s_uart_hw, base, UIOX_UART0_IRQ, ip,
                           uiox_uart_mmio_rd8, uiox_uart_mmio_wr8);
    if (rc < 0) return rc;

    /* ── 2. driver → hardware ─────────────────────────────────────── */
    rc = uiox_uart_if_config(&s_uart_if, &s_uart_hw);
    if (rc < 0) return rc;

    /* ── 3. the driver's back-pointer ─────────────────────────────── */
    s_uart_hw.priv = &s_uart_if;

    /* ── 4. the interrupt line ────────────────────────────────────── */
    rc = uiox_uart_if_start(&s_uart_if);
    if (rc < 0) {
        /* Registration failed.  Fall back to polled RX rather than
         * refusing to bind: the console is more useful busy-waiting
         * than absent, and the state is visible in hw->irq_owned. */
        s_uart_hw.irq_owned = false;
    }

    /* ── 5. descriptor — every field sourced, none invented ─────────
     *   .cls      UIOX_DEVCLASS_UART = UIOX_DEVCLASS_(CMD, 15)
     *   .unit     0 — first instance; a second would be unit 1
     *   .bus      UIOX_BUS_MMIO — the window is memory-mapped (x86's
     *             port window is mapped the same way for registry
     *             purposes; the accessor pair carries the difference)
     *   .name     "ttyS0" — uiox_devparse() parses that prefix, which
     *             is how mknod resolves a node name back to (cls, unit)
     *   .ops      the FAMILY vtable, uiox_devctl_ops_t — one slot
     *   .drv_priv the driver's own state — the DRIVER owns this slot,
     *             which is what makes uiox_uart_blk.c's cast safe */
    s_uart_dev.cls      = UIOX_DEVCLASS_UART;
    s_uart_dev.unit     = 0u;
    s_uart_dev.bus      = UIOX_BUS_MMIO;
    s_uart_dev.base     = base;
    s_uart_dev.irq      = UIOX_UART0_IRQ;
    s_uart_dev.name     = "ttyS0";
    s_uart_dev.ops      = &s_uart_devctl_ops;
    s_uart_dev.drv_priv = &s_uart_if;

    /* ── 6. bind ────────────────────────────────────────────────────
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
