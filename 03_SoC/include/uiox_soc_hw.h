/**
 * @file    uiox_soc_hw.h
 * @brief   UIOX SoC — platform HW abstraction (18-op vtable).
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_SOC_HW_H
 #define UIOX_SOC_HW_H
 
 /*
  * uiox_soc_types.h is the authoritative source for ALL UIOX_SOC_CAP_*
  * flags.  This header adds only the HW-vtable-specific aliases that are
  * NOT already defined there (GIC400, LAPIC, PIC8259, PL011, 16550,
  * SP804, PIT8254, ARM_GT, GPIO, I2C, SPI, VIRTIO).
  *
  * Flags removed from here to fix redefinition warnings:
  *   UIOX_SOC_CAP_PSCI  — defined in uiox_soc_types.h as (1u << 19)
  *   UIOX_SOC_CAP_ACPI  — defined in uiox_soc_types.h as (1u << 20)
  *   UIOX_SOC_CAP_SMMU  — defined in uiox_soc_types.h as (1u <<  9)
  *   UIOX_SOC_CAP_DTB   — defined in uiox_soc_types.h as (1u << 21)
  */
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Platform HW capability flags — HW-vtable-specific aliases only.
  * Flags already in uiox_soc_types.h are NOT repeated here.
  * ====================================================================== */
 
 /* ── Interrupt controllers ─────────────────────────────────────────── */
 #define UIOX_SOC_HW_CAP_GIC400   UIOX_SOC_BIT(0)  /**< GIC-400             */
 #define UIOX_SOC_HW_CAP_LAPIC    UIOX_SOC_BIT(1)  /**< x86 LAPIC           */
 #define UIOX_SOC_HW_CAP_PIC8259  UIOX_SOC_BIT(2)  /**< Legacy 8259A PIC    */
 
 /* ── UART controllers ──────────────────────────────────────────────── */
 #define UIOX_SOC_HW_CAP_PL011    UIOX_SOC_BIT(3)  /**< ARM PL011 UART      */
 #define UIOX_SOC_HW_CAP_16550    UIOX_SOC_BIT(4)  /**< 16550 UART          */
 
 /* ── Timer controllers ─────────────────────────────────────────────── */
 #define UIOX_SOC_HW_CAP_SP804    UIOX_SOC_BIT(5)  /**< ARM SP804 timer     */
 #define UIOX_SOC_HW_CAP_PIT8254  UIOX_SOC_BIT(6)  /**< x86 PIT 8254        */
 #define UIOX_SOC_HW_CAP_ARM_GT   UIOX_SOC_BIT(7)  /**< ARM generic timer   */
 
 /* ── Peripheral controllers ────────────────────────────────────────── */
 #define UIOX_SOC_HW_CAP_GPIO     UIOX_SOC_BIT(8)  /**< GPIO controller     */
 #define UIOX_SOC_HW_CAP_I2C      UIOX_SOC_BIT(9)  /**< I2C master          */
 #define UIOX_SOC_HW_CAP_SPI      UIOX_SOC_BIT(10) /**< SPI master          */
 #define UIOX_SOC_HW_CAP_VIRTIO   UIOX_SOC_BIT(11) /**< VirtIO devices      */
 
 /*
  * The following capabilities are used by uiox_soc_hw.h consumers
  * but are defined in uiox_soc_types.h — reference them directly:
  *
  *   UIOX_SOC_CAP_PSCI    (1u << 19)   ARM PSCI
  *   UIOX_SOC_CAP_ACPI    (1u << 20)   ACPI power mgmt
  *   UIOX_SOC_CAP_SMMU    (1u <<  9)   ARM SMMU / IOMMU
  *   UIOX_SOC_CAP_DTB     (1u << 21)   Device Tree Blob
  *   UIOX_SOC_CAP_GIC_V2  (1u <<  5)
  *   UIOX_SOC_CAP_GIC_V3  (1u <<  6)
  *   UIOX_SOC_CAP_APIC    (1u <<  8)
  */
 
 /* =========================================================================
  * Platform descriptor
  * ====================================================================== */
 #define UIOX_SOC_PLATFORM_NAME_LEN  48u
 
 typedef struct {
     uiox_uint32_t    magic;
     uiox_uint32_t    version;
     uiox_uint32_t    caps;              /**< UIOX_SOC_CAP_* bitmask         */
     char             name[UIOX_SOC_PLATFORM_NAME_LEN];
     uiox_uintptr_t   uart_base;         /**< Debug UART MMIO base           */
     uiox_uintptr_t   gic_dist_base;     /**< GIC Distributor / IOAPIC base  */
     uiox_uintptr_t   gic_cpu_base;      /**< GIC CPU interface / LAPIC base */
     uiox_uintptr_t   timer_base;        /**< SP804 / PIT base               */
     uiox_uintptr_t   gpio_base;
     uiox_uintptr_t   dtb_phys;          /**< Physical DTB address           */
     uiox_uint32_t    uart_irq;
     uiox_uint32_t    timer_irq;
     uiox_uint32_t    gpio_irq;
     uiox_uint32_t    num_cpus;
     uiox_uint64_t    ram_base;
     uiox_uint64_t    ram_size;
     void            *priv;
 } uiox_soc_platform_t;
 
 /* =========================================================================
  * HW operations vtable (18-op table)
  * ====================================================================== */
 typedef struct {
     /* Lifecycle */
     uiox_soc_err_t (*init)          (uiox_soc_platform_t *plat);
     void           (*deinit)        (uiox_soc_platform_t *plat);
 
     /* CPU / cache */
     void           (*cache_enable)  (void);
     void           (*cache_disable) (void);
     void           (*tlb_flush)     (void);
     void           (*barrier_dsb)   (void);
     void           (*barrier_isb)   (void);
 
     /* IRQ controller */
     void           (*irq_init)      (uiox_soc_platform_t *plat);
     void           (*irq_enable)    (uiox_uint32_t irq);
     void           (*irq_disable)   (uiox_uint32_t irq);
     void           (*irq_ack)       (uiox_uint32_t irq);
     void           (*irq_global_en) (void);
     void           (*irq_global_dis)(void);
 
     /* UART */
     void           (*uart_init)     (uiox_soc_platform_t *plat);
     void           (*uart_putc)     (char c);
 
     /* Timer */
     void           (*timer_init)    (uiox_soc_platform_t *plat,
                                      uiox_uint32_t hz);
     uiox_uint64_t  (*timer_tick)    (void);
 
     /* Power */
     void           (*reset)         (void) __attribute__((noreturn));
     void           (*shutdown)      (void) __attribute__((noreturn));
 } uiox_soc_hw_ops_t;
 
 /* =========================================================================
  * HAL registration API
  * ====================================================================== */
 void                     uiox_soc_hw_register    (const uiox_soc_hw_ops_t *ops,
                                                    uiox_soc_platform_t    *plat);
 const uiox_soc_hw_ops_t *uiox_soc_hw_ops         (void);
 uiox_soc_platform_t     *uiox_soc_hw_platform    (void);
 
 /* ── Convenience wrappers ──────────────────────────────────────────── */
 uiox_soc_err_t uiox_soc_hw_init         (void);
 void           uiox_soc_hw_irq_init     (void);
 void           uiox_soc_hw_irq_enable   (uiox_uint32_t irq);
 void           uiox_soc_hw_irq_disable  (uiox_uint32_t irq);
 void           uiox_soc_hw_irq_ack      (uiox_uint32_t irq);
 void           uiox_soc_hw_irq_global_en (void);
 void           uiox_soc_hw_irq_global_dis(void);
 void           uiox_soc_hw_uart_putc    (char c);
 void           uiox_soc_hw_cache_enable (void);
 void           uiox_soc_hw_cache_disable(void);
 void           uiox_soc_hw_tlb_flush    (void);
 void           uiox_soc_hw_dsb          (void);
 void           uiox_soc_hw_isb          (void);
 void           uiox_soc_hw_barrier      (void);
 uiox_uint64_t  uiox_soc_hw_tick         (void);
 void __attribute__((noreturn)) uiox_soc_hw_reset    (void);
 void __attribute__((noreturn)) uiox_soc_hw_shutdown (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_HW_H */
 