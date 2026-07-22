/*
 * 10_Arch/x86_64/src/uiox_soc_x86_init.c
 * UIOX x86-64 SoC — extended APIC, IOAPIC, HPET, and PIT init.
 *
 * Called by arch_init() after uiox_soc_init_x86() completes.
 */
#include "uiox_soc_x86.h"
#include "uiox_soc.h"
//#include "../../../20_DriverInterfaces/include/mmio.h"
//#include "../../../20_DriverInterfaces/include/irq.h"
#include "uiox_soc_stdio.h"
#include "uiox_soc_types.h"

/* ── Port I/O ────────────────────────────────────────────────────────── */
static inline void _outb(uiox_uint16_t p, uiox_uint8_t  v)
{ __asm__ volatile("outb %0,%1" :: "a"(v),  "Nd"(p)); }
static inline void _outw(uiox_uint16_t p, uiox_uint16_t v)
{ __asm__ volatile("outw %0,%1" :: "a"(v),  "Nd"(p)); }
static inline uiox_uint8_t _inb(uiox_uint16_t p)
{ uiox_uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(p)); return v; }

/* ── Mask all 16 legacy PIC IRQs (we use APIC) ───────────────────────── */
static void x86_pic_mask_all(void)
{
    _outb(0x21u, 0xFFu);   /* Master PIC: mask all 8 IRQs */
    _outb(0xA1u, 0xFFu);   /* Slave  PIC: mask all 8 IRQs */
    printf("[soc/x86]  Legacy PIC masked (using LAPIC/IOAPIC)\n");
}

/* ── IOAPIC init: route and unmask UART IRQ ──────────────────────────── */
static void x86_ioapic_init(void)
{
    uiox_uint32_t id  = ioapic_read(IOAPIC_REG_ID);
    uiox_uint32_t ver = ioapic_read(IOAPIC_REG_VER);
    uiox_uint32_t max_redir = (ver >> 16u) & 0xFFu;

    printf("[soc/x86]  IOAPIC @ 0x%08lx  id=0x%x  max_redir=%u\n",
           (unsigned long)SOC_IOAPIC_BASE, id >> 24u, max_redir);

    /* Mask all redirection table entries first */
    for (uiox_uint32_t i = 0u; i <= max_redir; i++)
        ioapic_write((uiox_uint8_t)IOAPIC_REG_REDTBL(i),
                     IOAPIC_RED_MASKED | (SOC_IOAPIC_IRQ_BASE + i));

    /* Unmask COM1 (IRQ4) → vector SOC_IOAPIC_IRQ_BASE + 4 */
    uiox_uint32_t uart_vec = (uiox_uint32_t)SOC_IOAPIC_IRQ_BASE + SOC_UART_IRQ;
    ioapic_write((uiox_uint8_t)IOAPIC_REG_REDTBL(SOC_UART_IRQ), uart_vec);

    printf("[soc/x86]  IOAPIC UART IRQ%u => vector %u unmasked\n",
           SOC_UART_IRQ, uart_vec);
}

/* ── LAPIC timer calibration using HPET ─────────────────────────────── */
static void x86_lapic_timer_init_hpet(uiox_uint32_t hz)
{
    /* 1. Program HPET timer 0 in one-shot mode for 10 ms */
    uiox_uint64_t hpet_period_fs =
        ((uiox_uint64_t)soc_mmio_read32(HPET_GCAP_ID + 4u)) & 0xFFFFFFFFull;
    if (hpet_period_fs == 0u) {
        printf("[soc/x86]  HPET period unreadable — skipping calibration\n");
        return;
    }

    /* ticks per 10 ms = 10_000_000_000_000 fs / period_fs */
    uiox_uint64_t hpet_10ms = 10000000000000ull / hpet_period_fs;

    /* Disable LAPIC timer, set divisor to 16 */
    soc_mmio_write32(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);
    soc_mmio_write32(LAPIC_TIMER_DCR, 0x3u);   /* divide by 16               */
    soc_mmio_write32(LAPIC_TIMER_ICR, 0xFFFFFFFFu);

    /* Start HPET */
    uiox_uint64_t start_ctr = soc_mmio_read64(HPET_MAIN_CTR);
    soc_mmio_write32(HPET_GEN_CFG, HPET_CFG_ENABLE);

    /* Wait 10 ms */
    while ((soc_mmio_read64(HPET_MAIN_CTR) - start_ctr) < hpet_10ms)
        __asm__ volatile("pause");

    /* Read how many LAPIC ticks elapsed in 10 ms */
    uiox_uint32_t ticks_10ms = 0xFFFFFFFFu - soc_mmio_read32(LAPIC_TIMER_CCR);
    uiox_uint32_t ticks_per_sec = ticks_10ms * 100u;
    uiox_uint32_t lapic_period  = ticks_per_sec / hz;

    printf("[soc/x86]  LAPIC timer: %u ticks/s => period=%u ticks @ %u Hz\n",
           ticks_per_sec, lapic_period, hz);

    /* Program LAPIC periodic timer */
    soc_mmio_write32(LAPIC_TIMER_DCR, 0x3u);
    soc_mmio_write32(LAPIC_LVT_TIMER, LAPIC_TIMER_PERIODIC
                                 | (SOC_IOAPIC_IRQ_BASE + 0u));
    soc_mmio_write32(LAPIC_TIMER_ICR, lapic_period);
}

/* ── 8253/8254 PIT — legacy init (10 ms period, channel 0) ─────────── */
static void x86_pit_init(uiox_uint32_t hz)
{
    uiox_uint32_t divisor = 1193182u / hz;  /* PIT input = 1.193182 MHz        */
    _outb(SOC_PIT_PORT + 3u, 0x34u);  /* Channel 0, rate generator, lo/hi */
    _outb(SOC_PIT_PORT,      (uiox_uint8_t)(divisor & 0xFFu));
    _outb(SOC_PIT_PORT,      (uiox_uint8_t)((divisor >> 8u) & 0xFFu));
    printf("[soc/x86]  PIT @ port 0x%02x: %u Hz (divisor=%u)\n",
           SOC_PIT_PORT, hz, divisor);
}

/* =========================================================================
 * uiox_soc_x86_init — extended x86-64 SoC init
 * ====================================================================== */
int uiox_soc_x86_init(void)
{
    printf("[soc/x86]  Extended x86-64 SoC init (APIC, IOAPIC, HPET)\n");

    x86_pic_mask_all();
    x86_ioapic_init();
    x86_lapic_timer_init_hpet(100u);
    x86_pit_init(100u);

    printf("[soc/x86]  Extended x86-64 SoC init complete.\n");
    return UIOX_SOC_OK;
}

void uiox_soc_x86_fini(void)
{
    soc_mmio_write32(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);
    printf("[soc/x86]  x86-64 SoC torn down.\n");
}
