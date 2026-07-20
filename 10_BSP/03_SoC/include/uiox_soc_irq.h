/**
 * @file    uiox_soc_irq.h
 * @brief   UIOX SoC — IRQ manager
 *          (GIC-400 / 8259A / LAPIC abstraction).
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_SOC_IRQ_H
 #define UIOX_SOC_IRQ_H
 
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── ARM64 (QEMU virt) IRQ numbers ─────────────────────────────────── */
 #define UIOX_SOC_IRQ_ARM64_UART0        33u
 #define UIOX_SOC_IRQ_ARM64_UART1        34u
 #define UIOX_SOC_IRQ_ARM64_TIMER0       30u   /**< ARM generic timer PPI  */
 #define UIOX_SOC_IRQ_ARM64_TIMER1       27u
 #define UIOX_SOC_IRQ_ARM64_GPIO         36u
 #define UIOX_SOC_IRQ_ARM64_ETH          42u
 #define UIOX_SOC_IRQ_ARM64_PCIE         64u
 #define UIOX_SOC_IRQ_ARM64_VIRTIO_BASE  48u
 
 /* ── ARM32 (QEMU versatilepb) IRQ numbers ───────────────────────────── */
 #define UIOX_SOC_IRQ_ARM32_UART0        12u
 #define UIOX_SOC_IRQ_ARM32_UART1        13u
 #define UIOX_SOC_IRQ_ARM32_TIMER0        4u
 #define UIOX_SOC_IRQ_ARM32_TIMER1        5u
 #define UIOX_SOC_IRQ_ARM32_GPIO          6u
 #define UIOX_SOC_IRQ_ARM32_ETH          25u
 #define UIOX_SOC_IRQ_ARM32_IDE          22u
 
 /* ── x86_64 (QEMU q35) IRQ numbers ─────────────────────────────────── */
 #define UIOX_SOC_IRQ_X86_TIMER           0u   /**< PIT channel 0 → IRQ0   */
 #define UIOX_SOC_IRQ_X86_KBD             1u
 #define UIOX_SOC_IRQ_X86_COM2            3u
 #define UIOX_SOC_IRQ_X86_COM1            4u
 #define UIOX_SOC_IRQ_X86_FLOPPY          6u
 #define UIOX_SOC_IRQ_X86_RTC             8u
 #define UIOX_SOC_IRQ_X86_PS2MOUSE       12u
 #define UIOX_SOC_IRQ_X86_IDE0           14u
 #define UIOX_SOC_IRQ_X86_IDE1           15u
 #define UIOX_SOC_IRQ_X86_REMAP_BASE   0x20u   /**< 8259A remap offset     */
 
 /* Maximum IRQ count */
 #define UIOX_SOC_IRQ_MAX               256u
 
 /* ── IRQ handler type ───────────────────────────────────────────────── */
 typedef void (*uiox_soc_irq_handler_t)(uiox_uint32_t irq, void *priv);
 
 /* ── IRQ descriptor ─────────────────────────────────────────────────── */
 typedef struct {
     uiox_soc_irq_handler_t handler;
     void                  *priv;
     uiox_bool_t            enabled;
     uiox_uint32_t          count;   /**< Number of times fired            */
 } uiox_soc_irq_desc_t;
 
 /* ── IRQ Manager API ────────────────────────────────────────────────── */
 
 /** Initialise IRQ manager and configure underlying controller. */
 uiox_soc_err_t uiox_soc_irq_init        (void);
 
 /**
  * Register a handler for IRQ @irq.
  * @param irq      Platform IRQ number.
  * @param handler  Callback invoked from dispatch.
  * @param priv     Caller-private pointer passed to handler.
  */
 uiox_soc_err_t uiox_soc_irq_register    (uiox_uint32_t irq,
                                           uiox_soc_irq_handler_t handler,
                                           void *priv);
 
 /** Unregister handler for @irq. */
 uiox_soc_err_t uiox_soc_irq_unregister  (uiox_uint32_t irq);
 
 /**
  * Dispatch: call from the platform vector table.
  * Reads pending IRQ from controller, calls handler, acks.
  */
 void           uiox_soc_irq_dispatch     (void);
 
 /**
  * Fire a specific IRQ — called by arch ISR stub with the resolved
  * IRQ number after the interrupt controller has identified it.
  * Increments count, invokes handler, acknowledges the interrupt.
  *
  * This declaration was missing, causing the
  * "no previous prototype for uiox_soc_irq_fire" warning.
  */
 void           uiox_soc_irq_fire         (uiox_uint32_t irq);
 
 /** Enable / disable a specific IRQ line. */
 void           uiox_soc_irq_enable       (uiox_uint32_t irq);
 void           uiox_soc_irq_disable      (uiox_uint32_t irq);
 
 /** Global masking. */
 void           uiox_soc_irq_global_enable  (void);
 void           uiox_soc_irq_global_disable (void);
 
 /** Query fire count for an IRQ. */
 uiox_uint32_t  uiox_soc_irq_count         (uiox_uint32_t irq);
 
 /** Print IRQ table to SoC console. */
 void           uiox_soc_irq_print         (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_IRQ_H */
 