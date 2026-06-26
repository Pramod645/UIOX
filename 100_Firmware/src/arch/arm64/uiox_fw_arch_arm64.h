/**
 * @file  uiox_fw_arch_arm64.h
 * @brief UIOX Firmware — ARM64 (AArch64) architecture-specific definitions.
 *
 * Covers:
 *   - GIC-400 (GICv2) base addresses for QEMU virt machine
 *   - PL011 UART register set and baud-rate constants
 *   - SP804 / ARM Generic Timer definitions
 *   - PL061 GPIO register offsets
 *   - PSCI function IDs (HVC calling convention)
 *   - DAIF mask bits, barrier macros, system register accessors
 *   - IRQ line assignments (matches uiox.md arch_defs.h)
 *   - AArch64 SCTLR_EL1 / TCR_EL1 / MAIR_EL1 constants for early MMU
 *
 * This header is included ONLY by ARM64 arch source files.
 * All other firmware layers access hardware via uiox_fw_hw.h.
 *
 * @version 1.0.0
 * @date    2026-06-24
 */

 #ifndef UIOX_FW_ARCH_ARM64_H
 #define UIOX_FW_ARCH_ARM64_H
 
 #include "../../include/uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * QEMU virt machine peripheral base addresses
  * (matches uiox.md arch/arm64/include/arch_defs.h)
  * ====================================================================== */
 
 #define ARM64_FLASH_BASE            0x00000000ULL
 #define ARM64_FLASH_SIZE            0x04000000ULL  /**< 64 MB flash        */
 
 /* GIC-400 (GICv2) */
 #define ARM64_GIC_DIST_BASE         0x08000000ULL  /**< GICD base          */
 #define ARM64_GIC_CPU_BASE          0x08010000ULL  /**< GICC base          */
 #define ARM64_GIC_VCPU_BASE         0x08020000ULL  /**< GICV base          */
 #define ARM64_GIC_VIRT_BASE         0x08040000ULL  /**< GICH base          */
 #define ARM64_GIC_DIST_SIZE         0x00010000ULL
 #define ARM64_GIC_CPU_SIZE          0x00010000ULL
 
 /* PL011 UARTs */
 #define ARM64_UART0_BASE            0x09000000ULL
 #define ARM64_UART1_BASE            0x09001000ULL
 #define ARM64_UART_CLOCK_HZ         24000000u   /**< 24 MHz PL011 ref clk  */
 #define ARM64_UART_BAUD_DEFAULT     115200u
 
 /* PL031 RTC */
 #define ARM64_RTC_BASE              0x09010000ULL
 
 /* PL061 GPIO */
 #define ARM64_GPIO_BASE             0x09020000ULL
 #define ARM64_GPIO_SIZE             0x00001000ULL
 #define ARM64_GPIO_NUM_PINS         8u
 
 /* ARM Generic Timer (no MMIO — uses system registers) */
 #define ARM64_TIMER_CNTFRQ_DEFAULT  62500000u  /**< 62.5 MHz on QEMU virt */
 
 /* PCIe / VirtIO MMIO */
 #define ARM64_PCIE_ECAM_BASE        0x3F000000ULL
 #define ARM64_PCIE_MMIO_BASE        0x10000000ULL
 #define ARM64_PCIE_MMIO_SIZE        0x2EFF0000ULL
 #define ARM64_VIRTIO_MMIO_BASE      0x0A000000ULL  /**< 32 VirtIO devices  */
 #define ARM64_VIRTIO_MMIO_STRIDE    0x00000200ULL
 
 /* DRAM */
 #define ARM64_DRAM_BASE             0x40000000ULL
 #define ARM64_DRAM_SIZE_DEFAULT     0x08000000ULL  /**< 128 MB default     */
 
 /* SMMU (optional) */
 #define ARM64_SMMU_BASE             0x09050000ULL
 
 /* =========================================================================
  * GICv2 register offsets
  * ====================================================================== */
 
 /* Distributor */
 #define GICD_CTLR                   0x0000u
 #define GICD_TYPER                  0x0004u
 #define GICD_IIDR                   0x0008u
 #define GICD_IGROUPR(n)             (0x0080u + (n) * 4u)
 #define GICD_ISENABLER(n)           (0x0100u + (n) * 4u)
 #define GICD_ICENABLER(n)           (0x0180u + (n) * 4u)
 #define GICD_ISPENDR(n)             (0x0200u + (n) * 4u)
 #define GICD_ICPENDR(n)             (0x0280u + (n) * 4u)
 #define GICD_ISACTIVER(n)           (0x0300u + (n) * 4u)
 #define GICD_ICACTIVER(n)           (0x0380u + (n) * 4u)
 #define GICD_IPRIORITYR(n)          (0x0400u + (n) * 4u)
 #define GICD_ITARGETSR(n)           (0x0800u + (n) * 4u)
 #define GICD_ICFGR(n)               (0x0C00u + (n) * 4u)
 #define GICD_SGIR                   0x0F00u
 #define GICD_CTLR_EN                (1u << 0)
 
 /* CPU interface */
 #define GICC_CTLR                   0x0000u
 #define GICC_PMR                    0x0004u
 #define GICC_BPR                    0x0008u
 #define GICC_IAR                    0x000Cu
 #define GICC_EOIR                   0x0010u
 #define GICC_RPR                    0x0014u
 #define GICC_HPPIR                  0x0018u
 #define GICC_CTLR_EN                (1u << 0)
 #define GICC_IAR_INTID_MASK         0x3FFu
 #define GICC_IAR_SPURIOUS           0x3FFu
 
 /* =========================================================================
  * IRQ numbers — QEMU virt (matches uiox.md arch/arm64/include/arch_defs.h)
  * ====================================================================== */
 
 /* SGIs (Software Generated Interrupts) 0–15 */
 #define ARM64_SGI_BASE              0u
 #define ARM64_SGI_RESCHEDULE        0u
 #define ARM64_SGI_CALL_FUNC         1u
 #define ARM64_SGI_CPU_STOP          2u
 
 /* PPIs (Private Peripheral Interrupts) 16–31 */
 #define ARM64_PPI_BASE              16u
 #define ARM64_IRQ_VIRT_TIMER        (ARM64_PPI_BASE + 11u)  /**< vTimer  */
 #define ARM64_IRQ_PHYS_TIMER        (ARM64_PPI_BASE + 14u)  /**< pTimer  */
 #define ARM64_IRQ_NS_PHYS_TIMER     (ARM64_PPI_BASE + 13u)  /**< nsTIMER */
 
 /* SPIs (Shared Peripheral Interrupts) base at 32 */
 #define ARM64_SPI_BASE              32u
 #define ARM64_IRQ_UART0             (ARM64_SPI_BASE +  1u)  /**< 33      */
 #define ARM64_IRQ_UART1             (ARM64_SPI_BASE +  2u)  /**< 34      */
 #define ARM64_IRQ_RTC               (ARM64_SPI_BASE +  2u)  /**< 34      */
 #define ARM64_IRQ_GPIO              (ARM64_SPI_BASE +  4u)  /**< 36      */
 #define ARM64_IRQ_PCIE_A            (ARM64_SPI_BASE +  8u)  /**< 40      */
 #define ARM64_IRQ_PCIE_B            (ARM64_SPI_BASE +  9u)
 #define ARM64_IRQ_PCIE_C            (ARM64_SPI_BASE + 10u)
 #define ARM64_IRQ_PCIE_D            (ARM64_SPI_BASE + 11u)
 #define ARM64_IRQ_ETH               (ARM64_SPI_BASE + 10u)  /**< 42      */
 #define ARM64_IRQ_VIRTIO_BASE       (ARM64_SPI_BASE + 16u)  /**< 48–63   */
 #define ARM64_IRQ_VIRTIO(n)         (ARM64_IRQ_VIRTIO_BASE + (n))
 
 /* =========================================================================
  * PL011 UART register offsets and bit fields
  * ====================================================================== */
 
 #define PL011_DR                    0x000u  /**< Data register             */
 #define PL011_RSR_ECR               0x004u  /**< Receive status/err clear  */
 #define PL011_FR                    0x018u  /**< Flag register             */
 #define PL011_ILPR                  0x020u  /**< IrDA low-power counter    */
 #define PL011_IBRD                  0x024u  /**< Integer baud-rate divisor */
 #define PL011_FBRD                  0x028u  /**< Fractional baud-rate div  */
 #define PL011_LCR_H                 0x02Cu  /**< Line control register     */
 #define PL011_CR                    0x030u  /**< Control register          */
 #define PL011_IFLS                  0x034u  /**< FIFO level select         */
 #define PL011_IMSC                  0x038u  /**< Interrupt mask set/clear  */
 #define PL011_RIS                   0x03Cu  /**< Raw interrupt status      */
 #define PL011_MIS                   0x040u  /**< Masked interrupt status   */
 #define PL011_ICR                   0x044u  /**< Interrupt clear register  */
 #define PL011_DMACR                 0x048u  /**< DMA control               */
 #define PL011_PERIPH_ID0            0xFE0u
 #define PL011_PERIPH_ID1            0xFE4u
 
 /* FR (Flag Register) bits */
 #define PL011_FR_RI                 (1u << 8)   /**< Ring indicator        */
 #define PL011_FR_TXFE               (1u << 7)   /**< TX FIFO empty         */
 #define PL011_FR_RXFF               (1u << 6)   /**< RX FIFO full          */
 #define PL011_FR_TXFF               (1u << 5)   /**< TX FIFO full          */
 #define PL011_FR_RXFE               (1u << 4)   /**< RX FIFO empty         */
 #define PL011_FR_BUSY               (1u << 3)   /**< UART busy             */
 #define PL011_FR_DCD                (1u << 2)   /**< Data carrier detect   */
 #define PL011_FR_DSR                (1u << 1)   /**< Data set ready        */
 #define PL011_FR_CTS                (1u << 0)   /**< Clear to send         */
 
 /* LCR_H bits */
 #define PL011_LCR_SPS               (1u << 7)   /**< Stick parity select   */
 #define PL011_LCR_WLEN8             (0x3u << 5) /**< 8-bit word length     */
 #define PL011_LCR_WLEN7             (0x2u << 5)
 #define PL011_LCR_WLEN6             (0x1u << 5)
 #define PL011_LCR_WLEN5             (0x0u << 5)
 #define PL011_LCR_FEN               (1u << 4)   /**< FIFO enable           */
 #define PL011_LCR_STP2              (1u << 3)   /**< Two stop bits         */
 #define PL011_LCR_EPS               (1u << 2)   /**< Even parity select    */
 #define PL011_LCR_PEN               (1u << 1)   /**< Parity enable         */
 #define PL011_LCR_BRK               (1u << 0)   /**< Send break            */
 
 /* CR bits */
 #define PL011_CR_CTSEN              (1u << 15)
 #define PL011_CR_RTSEN              (1u << 14)
 #define PL011_CR_OUT2               (1u << 13)
 #define PL011_CR_OUT1               (1u << 12)
 #define PL011_CR_RTS                (1u << 11)
 #define PL011_CR_DTR                (1u << 10)
 #define PL011_CR_RXE                (1u << 9)   /**< Receive enable        */
 #define PL011_CR_TXE                (1u << 8)   /**< Transmit enable       */
 #define PL011_CR_LBE                (1u << 7)   /**< Loopback enable       */
 #define PL011_CR_UARTEN             (1u << 0)   /**< UART enable           */
 
 /* IMSC / RIS / MIS / ICR interrupt bits */
 #define PL011_INT_OEI               (1u << 10)  /**< Overrun error         */
 #define PL011_INT_BEI               (1u << 9)   /**< Break error           */
 #define PL011_INT_PEI               (1u << 8)   /**< Parity error          */
 #define PL011_INT_FEI               (1u << 7)   /**< Framing error         */
 #define PL011_INT_RTI               (1u << 6)   /**< Receive timeout       */
 #define PL011_INT_TXI               (1u << 5)   /**< Transmit FIFO         */
 #define PL011_INT_RXI               (1u << 4)   /**< Receive FIFO          */
 #define PL011_INT_ALL               0x7FFu
 
 /* =========================================================================
  * ARM Generic Timer system registers (AArch64)
  * ====================================================================== */
 
 /* Read counter */
 #define ARM64_CNTPCT_EL0_READ(v) \
     __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(v) :: "memory")
 
 /* Read frequency */
 #define ARM64_CNTFRQ_EL0_READ(v) \
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v))
 
 /* Set physical timer compare value (tval form) */
 #define ARM64_CNTP_TVAL_EL0_WRITE(v) \
     __asm__ volatile("msr cntp_tval_el0, %0" :: "r"((uint64_t)(v)))
 
 /* Control: EN=bit0, IMASK=bit1, ISTATUS=bit2 */
 #define ARM64_CNTP_CTL_EL0_WRITE(v) \
     __asm__ volatile("msr cntp_ctl_el0, %0" :: "r"((uint64_t)(v)))
 
 #define ARM64_TIMER_CTL_EN          (1ULL << 0)
 #define ARM64_TIMER_CTL_IMASK       (1ULL << 1)
 #define ARM64_TIMER_CTL_ISTATUS     (1ULL << 2)
 
 /* =========================================================================
  * SCTLR_EL1 bits (System Control Register)
  * ====================================================================== */
 
 #define SCTLR_EL1_M                 (1ULL << 0)   /**< MMU enable          */
 #define SCTLR_EL1_A                 (1ULL << 1)   /**< Alignment check     */
 #define SCTLR_EL1_C                 (1ULL << 2)   /**< D-cache enable      */
 #define SCTLR_EL1_SA                (1ULL << 3)   /**< SP alignment check  */
 #define SCTLR_EL1_SA0               (1ULL << 4)
 #define SCTLR_EL1_I                 (1ULL << 12)  /**< I-cache enable      */
 #define SCTLR_EL1_WXN               (1ULL << 19)  /**< Write implies XN    */
 #define SCTLR_EL1_UCI               (1ULL << 26)
 
 /* =========================================================================
  * TCR_EL1 bits (Translation Control Register)
  * ====================================================================== */
 
 #define TCR_EL1_T0SZ(x)            ((uint64_t)(x))
 #define TCR_EL1_T1SZ(x)            ((uint64_t)(x) << 16)
 #define TCR_EL1_IRGN0_WB_WA        (1ULL << 8)
 #define TCR_EL1_ORGN0_WB_WA        (1ULL << 10)
 #define TCR_EL1_SH0_IS             (3ULL << 12)
 #define TCR_EL1_TG0_4K             (0ULL << 14)
 #define TCR_EL1_TG1_4K             (2ULL << 30)
 #define TCR_EL1_IPS_48BIT          (5ULL << 32)
 #define TCR_EL1_AS                 (1ULL << 36)   /**< 16-bit ASID         */
 #define TCR_EL1_TBI0               (1ULL << 37)   /**< Top-byte ignore     */
 
 /* Typical 48-bit VA, 4KB granule */
 #define TCR_EL1_BOOT_VALUE \
     (TCR_EL1_T0SZ(16u)    | TCR_EL1_T1SZ(16u) | \
      TCR_EL1_IRGN0_WB_WA  | TCR_EL1_ORGN0_WB_WA | \
      TCR_EL1_SH0_IS        | TCR_EL1_TG0_4K       | \
      TCR_EL1_TG1_4K        | TCR_EL1_IPS_48BIT)
 
 /* =========================================================================
  * MAIR_EL1 attribute indices
  * ====================================================================== */
 
 #define MAIR_IDX_NORMAL_WB          0u   /**< Normal Write-Back Cacheable */
 #define MAIR_IDX_NORMAL_NC          1u   /**< Normal Non-Cacheable        */
 #define MAIR_IDX_DEVICE_nGnRnE      2u   /**< Device nGnRnE (strict)      */
 #define MAIR_IDX_DEVICE_nGnRE       3u   /**< Device nGnRE                */
 
 #define MAIR_NORMAL_WB_ATTR         0xFFu
 #define MAIR_NORMAL_NC_ATTR         0x44u
 #define MAIR_DEVICE_nGnRnE_ATTR     0x00u
 #define MAIR_DEVICE_nGnRE_ATTR      0x04u
 
 #define MAIR_EL1_BOOT_VALUE \
     ((uint64_t)MAIR_NORMAL_WB_ATTR      << (MAIR_IDX_NORMAL_WB     * 8u)) | \
     ((uint64_t)MAIR_NORMAL_NC_ATTR      << (MAIR_IDX_NORMAL_NC     * 8u)) | \
     ((uint64_t)MAIR_DEVICE_nGnRnE_ATTR  << (MAIR_IDX_DEVICE_nGnRnE * 8u)) | \
     ((uint64_t)MAIR_DEVICE_nGnRE_ATTR   << (MAIR_IDX_DEVICE_nGnRE  * 8u))
 
 /* =========================================================================
  * DAIF mask bits (interrupt masking)
  * ====================================================================== */
 
 #define DAIF_FIQ                    (1u << 0)
 #define DAIF_IRQ                    (1u << 1)
 #define DAIF_SERR                   (1u << 2)
 #define DAIF_DEBUG                  (1u << 3)
 
 /* Mask all async exceptions */
 #define ARM64_IRQ_DISABLE() \
     __asm__ volatile("msr daifset, #0xf" ::: "memory")
 
 /* Unmask IRQ and FIQ */
 #define ARM64_IRQ_ENABLE() \
     __asm__ volatile("msr daifclr, #0xf" ::: "memory")
 
 /* =========================================================================
  * Memory barrier macros
  * ====================================================================== */
 
 #define ARM64_DSB_SY()   __asm__ volatile("dsb sy"  ::: "memory")
 #define ARM64_DSB_ISH()  __asm__ volatile("dsb ish" ::: "memory")
 #define ARM64_DMB_SY()   __asm__ volatile("dmb sy"  ::: "memory")
 #define ARM64_ISB()      __asm__ volatile("isb"     ::: "memory")
 #define ARM64_WFI()      __asm__ volatile("wfi"     ::: "memory")
 #define ARM64_WFE()      __asm__ volatile("wfe"     ::: "memory")
 #define ARM64_SEV()      __asm__ volatile("sev"     ::: "memory")
 
 /* =========================================================================
  * Cache maintenance macros
  * ====================================================================== */
 
 /** Clean and invalidate D-cache line by virtual address */
 #define ARM64_DC_CIVAC(addr) \
     __asm__ volatile("dc civac, %0" :: "r"(addr) : "memory")
 
 /** Invalidate I-cache to PoU (inner shareable) */
 #define ARM64_IC_IALLUIS() \
     __asm__ volatile("ic ialluis" ::: "memory")
 
 /** Invalidate TLB — all entries, inner shareable */
 #define ARM64_TLBI_VMALLE1IS() \
     __asm__ volatile("tlbi vmalle1is" ::: "memory")
 
 /** Full cache + TLB flush sequence */
 #define ARM64_FULL_BARRIER() \
     do { ARM64_DSB_SY(); ARM64_IC_IALLUIS(); \
          ARM64_TLBI_VMALLE1IS(); ARM64_DSB_SY(); ARM64_ISB(); } while(0)
 
 /* =========================================================================
  * PSCI function IDs (HVC / SMC calling convention, 32-bit SMCCC)
  * ====================================================================== */
 
 #define PSCI_FN_VERSION             0x84000000u
 #define PSCI_FN_CPU_SUSPEND_32      0x84000001u
 #define PSCI_FN_CPU_OFF             0x84000002u
 #define PSCI_FN_CPU_ON_32           0x84000003u
 #define PSCI_FN_AFFINITY_INFO_32    0x84000004u
 #define PSCI_FN_MIGRATE_32          0x84000005u
 #define PSCI_FN_MIGRATE_INFO_TYPE   0x84000006u
 #define PSCI_FN_SYSTEM_OFF          0x84000008u
 #define PSCI_FN_SYSTEM_RESET        0x84000009u
 #define PSCI_FN_CPU_ON_64           0xC4000003u  /**< 64-bit variant       */
 #define PSCI_FN_SYSTEM_RESET2       0x84000012u
 
 #define PSCI_RET_SUCCESS            0
 #define PSCI_RET_NOT_SUPPORTED     (-1)
 #define PSCI_RET_INVALID_PARAMS    (-2)
 #define PSCI_RET_DENIED            (-3)
 #define PSCI_RET_ALREADY_ON        (-4)
 #define PSCI_RET_ON_PENDING        (-5)
 #define PSCI_RET_INTERNAL_FAILURE  (-6)
 #define PSCI_RET_NOT_PRESENT       (-7)
 #define PSCI_RET_DISABLED          (-8)
 
 /** Invoke PSCI via HVC (hypervisor call) */
 static inline int64_t arm64_psci_hvc(uint64_t fn,
                                        uint64_t a0, uint64_t a1,
                                        uint64_t a2)
 {
     register uint64_t x0 __asm__("x0") = fn;
     register uint64_t x1 __asm__("x1") = a0;
     register uint64_t x2 __asm__("x2") = a1;
     register uint64_t x3 __asm__("x3") = a2;
     __asm__ volatile("hvc #0"
                      : "=r"(x0)
                      : "r"(x0), "r"(x1), "r"(x2), "r"(x3)
                      : "memory");
     return (int64_t)x0;
 }
 
 /** Invoke PSCI via SMC (secure monitor call) */
 static inline int64_t arm64_psci_smc(uint64_t fn,
                                        uint64_t a0, uint64_t a1,
                                        uint64_t a2)
 {
     register uint64_t x0 __asm__("x0") = fn;
     register uint64_t x1 __asm__("x1") = a0;
     register uint64_t x2 __asm__("x2") = a1;
     register uint64_t x3 __asm__("x3") = a2;
     __asm__ volatile("smc #0"
                      : "=r"(x0)
                      : "r"(x0), "r"(x1), "r"(x2), "r"(x3)
                      : "memory");
     return (int64_t)x0;
 }
 
 /* =========================================================================
  * EL (Exception Level) helpers
  * ====================================================================== */
 
 /** Read current exception level (0–3) */
 static inline uint32_t arm64_current_el(void)
 {
     uint64_t el;
     __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
     return (uint32_t)((el >> 2u) & 0x3u);
 }
 
 /** Read SPSR_EL2 */
 static inline uint64_t arm64_read_spsr_el2(void)
 {
     uint64_t v;
     __asm__ volatile("mrs %0, spsr_el2" : "=r"(v));
     return v;
 }
 
 /** SPSR value for dropping to EL1h (AArch64, DAIF masked) */
 #define ARM64_SPSR_EL1H_MASKED      0x3C5ULL
 
 /* =========================================================================
  * ARM64 platform registration
  * ====================================================================== */
 
 /** Called by uiox_fw_main() to register the ARM64 vtable. */
 void uiox_fw_arch_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_ARCH_ARM64_H */
 