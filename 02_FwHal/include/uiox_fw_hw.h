/**
 * @file  uiox_fw_hw.h
 * @brief UIOX Firmware — platform HW abstraction (18-op vtable).
 *        Same vtable pattern as uiox_boot_hw / uiox_sata_hw etc.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_HW_H
 #define UIOX_FW_HW_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Platform HW capability flags
  * ====================================================================== */
 #define UIOX_FW_CAP_GIC400      UIOX_FW_BIT(0)   /**< GIC-400 interrupt ctrl */
 #define UIOX_FW_CAP_LAPIC       UIOX_FW_BIT(1)   /**< x86 LAPIC             */
 #define UIOX_FW_CAP_PIC8259     UIOX_FW_BIT(2)   /**< Legacy 8259A PIC      */
 #define UIOX_FW_CAP_PL011       UIOX_FW_BIT(3)   /**< ARM PL011 UART        */
 #define UIOX_FW_CAP_16550       UIOX_FW_BIT(4)   /**< 16550 UART            */
 #define UIOX_FW_CAP_SP804       UIOX_FW_BIT(5)   /**< ARM SP804 dual-timer  */
 #define UIOX_FW_CAP_PIT8254     UIOX_FW_BIT(6)   /**< x86 PIT 8254          */
 #define UIOX_FW_CAP_ARM_GT      UIOX_FW_BIT(7)   /**< ARM generic timer     */
 #define UIOX_FW_CAP_GPIO        UIOX_FW_BIT(8)   /**< GPIO controller       */
 #define UIOX_FW_CAP_I2C         UIOX_FW_BIT(9)   /**< I2C master            */
 #define UIOX_FW_CAP_SPI         UIOX_FW_BIT(10)  /**< SPI master            */
 #define UIOX_FW_CAP_PSCI        UIOX_FW_BIT(11)  /**< ARM PSCI power ops    */
 #define UIOX_FW_CAP_ACPI        UIOX_FW_BIT(12)  /**< ACPI power mgmt       */
 #define UIOX_FW_CAP_SMMU        UIOX_FW_BIT(13)  /**< ARM SMMU / IOMMU      */
 #define UIOX_FW_CAP_VIRTIO      UIOX_FW_BIT(14)  /**< VirtIO devices        */
 #define UIOX_FW_CAP_DTB         UIOX_FW_BIT(15)  /**< Device Tree Blob      */
 
 /* =========================================================================
  * Platform descriptor
  * ====================================================================== */
 #define UIOX_FW_PLATFORM_NAME_LEN  48u
 
 typedef struct {
     uint32_t         magic;
     uint32_t         version;
     uint32_t         caps;
     //uiox_fw_arch_t   arch;
     char             name[UIOX_FW_PLATFORM_NAME_LEN];
     uintptr_t        uart_base;       /**< Debug UART MMIO base            */
     uintptr_t        gic_dist_base;   /**< GIC Distributor / IOAPIC base   */
     uintptr_t        gic_cpu_base;    /**< GIC CPU interface / LAPIC base  */
     uintptr_t        timer_base;      /**< SP804 / PIT base                */
     uintptr_t        gpio_base;
     uintptr_t        dtb_phys;        /**< Physical DTB address            */
     uint32_t         uart_irq;
     uint32_t         timer_irq;
     uint32_t         gpio_irq;
     uint32_t         num_cpus;
     uint64_t         ram_base;
     uint64_t         ram_size;
     /* Private (ops pointer) */
     void            *priv;
 } uiox_fw_platform_t;
 
 /* =========================================================================
  * HW operations vtable (18-op table, same pattern as rest of UIOX)
  * ====================================================================== */
 typedef struct {
     /* Lifecycle */
     uiox_fw_err_t (*init)          (uiox_fw_platform_t *plat);
     void          (*deinit)        (uiox_fw_platform_t *plat);
 
     /* CPU / cache */
     void          (*cache_enable)  (void);
     void          (*cache_disable) (void);
     void          (*tlb_flush)     (void);
     void          (*barrier_dsb)   (void);
     void          (*barrier_isb)   (void);
 
     /* IRQ controller */
     void          (*irq_init)      (uiox_fw_platform_t *plat);
     void          (*irq_enable)    (uint32_t irq);
     void          (*irq_disable)   (uint32_t irq);
     void          (*irq_ack)       (uint32_t irq);
     void          (*irq_global_en) (void);
     void          (*irq_global_dis)(void);
 
     /* UART */
     void          (*uart_init)     (uiox_fw_platform_t *plat);
     void          (*uart_putc)     (char c);
 
     /* Timer */
     void          (*timer_init)    (uiox_fw_platform_t *plat,
                                     uint32_t hz);
     uint64_t      (*timer_tick)    (void);
 
     /* Power */
     void          (*reset)         (void) __attribute__((noreturn));
     void          (*shutdown)      (void) __attribute__((noreturn));
 } uiox_fw_hw_ops_t;
 
 /* =========================================================================
  * HAL registration API
  * ====================================================================== */
 void                    uiox_fw_hw_register   (const uiox_fw_hw_ops_t *ops,
                                                 uiox_fw_platform_t *plat);
 const uiox_fw_hw_ops_t *uiox_fw_hw_ops        (void);
 uiox_fw_platform_t     *uiox_fw_hw_platform   (void);
 
 /* Convenience wrappers */
 uiox_fw_err_t  uiox_fw_hw_init           (void);
 void           uiox_fw_hw_irq_init       (void);
 void           uiox_fw_hw_irq_enable     (uint32_t irq);
 void           uiox_fw_hw_irq_disable    (uint32_t irq);
 void           uiox_fw_hw_irq_ack        (uint32_t irq);
 void           uiox_fw_hw_irq_global_en  (void);
 void           uiox_fw_hw_irq_global_dis (void);
 void           uiox_fw_hw_uart_putc      (char c);
 void           uiox_fw_hw_cache_enable   (void);
 void           uiox_fw_hw_cache_disable  (void);
 void           uiox_fw_hw_tlb_flush      (void);
 void           uiox_fw_hw_dsb            (void);
 void           uiox_fw_hw_isb            (void);
 void           uiox_fw_hw_barrier        (void);
 uint64_t       uiox_fw_hw_tick           (void);
 void __attribute__((noreturn)) uiox_fw_hw_reset    (void);
 void __attribute__((noreturn)) uiox_fw_hw_shutdown (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_HW_H */
 