/**
 * @file    uiox_soc_irq.c
 * @brief   UIOX SoC — IRQ manager implementation.
 * @date    2026-06-21
 */

/*
 * Include order:
 *   1. uiox_soc_irq.h  — IRQ manager types and API declarations
 *                        (now includes uiox_soc_irq_fire prototype)
 *   2. uiox_soc_hw.h   — HW vtable wrappers: uiox_soc_hw_irq_global_dis,
 *                        uiox_soc_hw_irq_init, uiox_soc_hw_irq_enable,
 *                        uiox_soc_hw_irq_disable, uiox_soc_hw_irq_ack,
 *                        uiox_soc_hw_irq_global_en
 *                        (was MISSING — caused all implicit-declaration errors)
 *   3. uiox_soc.h      — uiox_soc_puts, uiox_soc_printf, SOC_LOG
 */
#include "uiox_soc_irq.h"
#include "uiox_soc_hw.h"   /* ← THIS was the missing include             */
#include "uiox_soc.h"

static uiox_soc_irq_desc_t s_irq[UIOX_SOC_IRQ_MAX];

uiox_soc_err_t uiox_soc_irq_init(void)
{
    uiox_soc_hw_irq_global_dis();   /* now visible from uiox_soc_hw.h    */

    for (uiox_uint32_t i = 0u; i < UIOX_SOC_IRQ_MAX; i++) {
        s_irq[i].handler = NULL;
        s_irq[i].priv    = NULL;
        s_irq[i].enabled = false;
        s_irq[i].count   = 0u;
    }

    uiox_soc_hw_irq_init();         /* now visible from uiox_soc_hw.h    */
    SOC_LOG("IRQ", "manager init OK (%u vectors)", UIOX_SOC_IRQ_MAX);
    return UIOX_SOC_OK;
}

uiox_soc_err_t uiox_soc_irq_register(uiox_uint32_t irq,
                                       uiox_soc_irq_handler_t handler,
                                       void *priv)
{
    if (irq >= UIOX_SOC_IRQ_MAX || !handler) return UIOX_SOC_ERR_INVAL;
    s_irq[irq].handler = handler;
    s_irq[irq].priv    = priv;
    s_irq[irq].count   = 0u;
    s_irq[irq].enabled = true;
    uiox_soc_hw_irq_enable(irq);    /* now visible from uiox_soc_hw.h    */
    return UIOX_SOC_OK;
}

uiox_soc_err_t uiox_soc_irq_unregister(uiox_uint32_t irq)
{
    if (irq >= UIOX_SOC_IRQ_MAX) return UIOX_SOC_ERR_INVAL;
    uiox_soc_hw_irq_disable(irq);   /* now visible from uiox_soc_hw.h    */
    s_irq[irq].handler = NULL;
    s_irq[irq].priv    = NULL;
    s_irq[irq].enabled = false;
    return UIOX_SOC_OK;
}

void uiox_soc_irq_dispatch(void)
{
    /*
     * Platform-specific IRQ identification happens in the arch layer.
     * This generic dispatcher is called by uiox_soc_irq_fire(irq_num)
     * from the arch ISR stub after the IRQ number is resolved.
     */
    (void)s_irq; /* suppress unused warning when fire() is used instead */
}

/* Called by arch ISR stub with the resolved IRQ number. */
void uiox_soc_irq_fire(uiox_uint32_t irq)   /* prototype now in irq.h   */
{
    if (irq >= UIOX_SOC_IRQ_MAX) return;
    s_irq[irq].count++;
    if (s_irq[irq].handler)
        s_irq[irq].handler(irq, s_irq[irq].priv);
    uiox_soc_hw_irq_ack(irq);       /* now visible from uiox_soc_hw.h    */
}

void uiox_soc_irq_enable(uiox_uint32_t irq)
{
    if (irq < UIOX_SOC_IRQ_MAX) {
        s_irq[irq].enabled = true;
        uiox_soc_hw_irq_enable(irq);
    }
}

void uiox_soc_irq_disable(uiox_uint32_t irq)
{
    if (irq < UIOX_SOC_IRQ_MAX) {
        s_irq[irq].enabled = false;
        uiox_soc_hw_irq_disable(irq);
    }
}

void uiox_soc_irq_global_enable (void) { uiox_soc_hw_irq_global_en();  }
void uiox_soc_irq_global_disable(void) { uiox_soc_hw_irq_global_dis(); }

uiox_uint32_t uiox_soc_irq_count(uiox_uint32_t irq)
{
    return irq < UIOX_SOC_IRQ_MAX ? s_irq[irq].count : 0u;
}

void uiox_soc_irq_print(void)
{
    uiox_soc_puts("[SOC] IRQ table:\n");
    for (uiox_uint32_t i = 0u; i < UIOX_SOC_IRQ_MAX; i++) {
        if (!s_irq[i].handler) continue;
        uiox_soc_printf("  IRQ %3u  count=%-8u  %s\n",
                         i, s_irq[i].count,
                         s_irq[i].enabled ? "enabled" : "masked");
    }
}
