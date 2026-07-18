/**
 * @file  uiox_fw_gpio.c
 * @brief UIOX Firmware — GPIO driver implementation.
 *
 * Supports:
 *   ARM64 / ARM32 — ARM PL061 GPIO controller
 *   x86_64        — Stub (no discrete GPIO on QEMU q35)
 *
 * Matches:
 *   30_DeviceDrivers/04_NonSecnsors
 *   20_DriverInterfaces/03_NonSecnsors
 *   uiox_fw_gpio.h public API
 *
 * @version 1.0.0
 * @date    2026-06-25
 */
//uioxfwgpio.c | PL061 GPIO init (direction register, AFSEL, interrupt sense/edge/level config via IS/IBE/IEV), masked-write data access, pin read, IRQ dispatch with per-pin callbacks, stub path for x86 (base == 0) |
 #include "uiox_fw.h"
#include "uiox_fw_gpio.h"
 /* =========================================================================
  * PL061 register offsets (also defined in uiox_fw_gpio.h for reference)
  * ====================================================================== */
 
 /**
  * PL061 uses a unique "masked write" scheme for the data register:
  *   Address bits [9:2] select which GPIO bits to affect.
  *   Write:  only bits set in the address mask are modified.
  *   Read:   only bits set in the address mask are returned.
  *
  *   To write pin N:  offset = (1u << (N + 2))
  *   To read  pin N:  offset = (1u << (N + 2))
  *   To read all:     offset = 0x3FCu (all 8 pins selected)
  */
 
 #define PL061_DATA_ALL              0x3FCu  /**< Select all 8 pins         */
 #define PL061_DIR                   0x400u  /**< Direction: 1=output       */
 #define PL061_IS                    0x404u  /**< Interrupt sense           */
 #define PL061_IBE                   0x408u  /**< Interrupt both edges      */
 #define PL061_IEV                   0x40Cu  /**< Interrupt event           */
 #define PL061_IE                    0x410u  /**< Interrupt mask            */
 #define PL061_RIS                   0x414u  /**< Raw interrupt status      */
 #define PL061_MIS                   0x418u  /**< Masked interrupt status   */
 #define PL061_IC                    0x41Cu  /**< Interrupt clear           */
 #define PL061_AFSEL                 0x420u  /**< Alternate function select */
 
 /* PL061 peripheral ID (bytes 0–3 of the ID block at 0xFE0) */
 #define PL061_PERIPH_ID0            0x061u
 #define PL061_PERIPH_ID1            0x010u
 
 /* =========================================================================
  * PL061 driver
  * ====================================================================== */
 
 static uiox_fw_err_t pl061_init(uiox_fw_gpio_t *g)
 {
     uintptr_t b = g->base;
 
     /* All pins input by default */
     fw_mmio_write32(b + PL061_DIR,   0x00u);
     /* All pins direct (no alternate function) */
     fw_mmio_write32(b + PL061_AFSEL, 0x00u);
     /* Disable all interrupts */
     fw_mmio_write32(b + PL061_IE,    0x00u);
     /* Clear any pending interrupts */
     fw_mmio_write32(b + PL061_IC,    0xFFu);
     /* Level-sensitive (default) */
     fw_mmio_write32(b + PL061_IS,    0x00u);
     fw_mmio_write32(b + PL061_IBE,   0x00u);
     fw_mmio_write32(b + PL061_IEV,   0x00u);
 
     return UIOX_FW_OK;
 }
 
 static void pl061_set_dir(uiox_fw_gpio_t *g, uint32_t pin,
                            uiox_fw_gpio_dir_t dir)
 {
     if (pin >= g->num_pins) return;
     uintptr_t b = g->base;
     uint32_t  d = fw_mmio_read32(b + PL061_DIR);
     if (dir == UIOX_FW_GPIO_OUT)
         d |=  (1u << pin);
     else
         d &= ~(1u << pin);
     fw_mmio_write32(b + PL061_DIR, d);
 }
 
 static void pl061_write(uiox_fw_gpio_t *g, uint32_t pin, bool val)
 {
     if (pin >= g->num_pins) return;
     /* Masked write: address bits [9:2] select the pin */
     uintptr_t mask_offset = (uintptr_t)(1u << (pin + 2u));
     fw_mmio_write32(g->base + mask_offset,
                      val ? (uint32_t)(1u << pin) : 0u);
 }
 
 static bool pl061_read(const uiox_fw_gpio_t *g, uint32_t pin)
 {
     if (pin >= g->num_pins) return false;
     uintptr_t mask_offset = (uintptr_t)(1u << (pin + 2u));
     return !!(fw_mmio_read32(g->base + mask_offset) & (1u << pin));
 }
 
 static uiox_fw_err_t pl061_irq_enable(uiox_fw_gpio_t *g, uint32_t pin,
                                         uiox_fw_gpio_irq_mode_t mode,
                                         uiox_fw_gpio_cb_t cb, void *priv)
 {
     if (pin >= g->num_pins || pin >= UIOX_FW_GPIO_MAX_PINS)
         return UIOX_FW_ERR_INVAL;
 
     g->cb[pin]      = cb;
     g->cb_priv[pin] = priv;
 
     uintptr_t b    = g->base;
     uint32_t  mask = 1u << pin;
 
     /* Configure interrupt sense/edge/level */
     uint32_t is_reg  = fw_mmio_read32(b + PL061_IS);
     uint32_t ibe_reg = fw_mmio_read32(b + PL061_IBE);
     uint32_t iev_reg = fw_mmio_read32(b + PL061_IEV);
 
     /* Clear existing config for this pin */
     is_reg  &= ~mask;
     ibe_reg &= ~mask;
     iev_reg &= ~mask;
 
     switch (mode) {
     case UIOX_FW_GPIO_IRQ_RISING:
         /* Edge, single, rising */
         iev_reg |= mask;
         break;
     case UIOX_FW_GPIO_IRQ_FALLING:
         /* Edge, single, falling */
         break;
     case UIOX_FW_GPIO_IRQ_BOTH:
         /* Edge, both */
         ibe_reg |= mask;
         break;
     case UIOX_FW_GPIO_IRQ_HIGH:
         /* Level, high */
         is_reg  |= mask;
         iev_reg |= mask;
         break;
     case UIOX_FW_GPIO_IRQ_LOW:
         /* Level, low */
         is_reg  |= mask;
         break;
     default:
         return UIOX_FW_ERR_INVAL;
     }
 
     fw_mmio_write32(b + PL061_IS,  is_reg);
     fw_mmio_write32(b + PL061_IBE, ibe_reg);
     fw_mmio_write32(b + PL061_IEV, iev_reg);
 
     /* Unmask interrupt for this pin */
     uint32_t ie = fw_mmio_read32(b + PL061_IE);
     ie |= mask;
     fw_mmio_write32(b + PL061_IE, ie);
 
     return UIOX_FW_OK;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_gpio_init(uiox_fw_gpio_t *g,
                                   uintptr_t base, uint32_t irq,
                                   uint32_t num_pins)
 {
     if (!g) return UIOX_FW_ERR_INVAL;
     uiox_fw_memset(g, 0, sizeof(*g));
 
     g->base     = base;
     g->irq      = irq;
     g->num_pins = (num_pins < UIOX_FW_GPIO_MAX_PINS)
                   ? num_pins : UIOX_FW_GPIO_MAX_PINS;
 
     /* x86 / no-GPIO platforms: base == 0 means stub only */
     if (base == 0u) {
         FW_LOG("GPIO", "stub (no GPIO controller on this platform)");
         return UIOX_FW_OK;
     }
 
     uiox_fw_err_t rc = pl061_init(g);
     if (rc != UIOX_FW_OK) {
         FW_ERR("PL061 init failed at base 0x%p", (void *)base);
         return rc;
     }
 
     FW_LOG("GPIO", "PL061 init OK  base=%p  pins=%u  irq=%u",
            (void *)base, g->num_pins, irq);
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_gpio_set_dir(uiox_fw_gpio_t *g, uint32_t pin,
                                      uiox_fw_gpio_dir_t dir)
 {
     if (!g || pin >= g->num_pins) return UIOX_FW_ERR_INVAL;
     if (g->base == 0u) return UIOX_FW_ERR_UNSUP;
     pl061_set_dir(g, pin, dir);
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_gpio_set_pull(uiox_fw_gpio_t *g, uint32_t pin,
                                       uiox_fw_gpio_pull_t pull)
 {
     /*
      * PL061 does not have configurable pull resistors — pull-up/down
      * is board-level (external resistors). This is a no-op stub.
      */
     UIOX_FW_UNUSED(g);
     UIOX_FW_UNUSED(pin);
     UIOX_FW_UNUSED(pull);
     return UIOX_FW_OK;
 }
 
 void uiox_fw_gpio_write(uiox_fw_gpio_t *g, uint32_t pin, bool val)
 {
     if (!g || g->base == 0u || pin >= g->num_pins) return;
     pl061_write(g, pin, val);
 }
 
 bool uiox_fw_gpio_read(const uiox_fw_gpio_t *g, uint32_t pin)
 {
     if (!g || g->base == 0u || pin >= g->num_pins) return false;
     return pl061_read(g, pin);
 }
 
 uiox_fw_err_t uiox_fw_gpio_irq_en(uiox_fw_gpio_t *g, uint32_t pin,
                                     uiox_fw_gpio_irq_mode_t mode,
                                     uiox_fw_gpio_cb_t cb, void *priv)
 {
     if (!g || g->base == 0u) return UIOX_FW_ERR_NODEV;
     return pl061_irq_enable(g, pin, mode, cb, priv);
 }
 
 /**
  * uiox_fw_gpio_irq — GPIO interrupt service routine.
  * Call this from the platform IRQ vector for the GPIO interrupt line.
  * Reads the masked interrupt status, invokes registered callbacks,
  * then clears the interrupt.
  */
 void uiox_fw_gpio_irq(uiox_fw_gpio_t *g)
 {
     if (!g || g->base == 0u) return;
 
     uint32_t mis = fw_mmio_read32(g->base + PL061_MIS);
     if (!mis) return;  /* Spurious */
 
     for (uint32_t pin = 0u; pin < g->num_pins && pin < 8u; pin++) {
         if (!(mis & (1u << pin))) continue;
         bool level = pl061_read(g, pin);
         if (g->cb[pin])
             g->cb[pin](pin, level, g->cb_priv[pin]);
     }
 
     /* Clear all fired interrupts */
     fw_mmio_write32(g->base + PL061_IC, mis);
 }
 