/**
 * @file  uiox_fw_arch_arm32.h
 * @brief UIOX Firmware — ARM32 (ARMv7-A) architecture-specific definitions.
 *
 * Covers:
 *   - QEMU versatilepb peripheral base addresses
 *   - PL190 VIC (Vectored Interrupt Controller) register map
 *   - PL011 UART registers (same offsets as arm64, included here for
 *     self-containment)
 *   - SP804 dual-timer register offsets and control bits
 *   - PL061 GPIO register offsets
 *   - CPSR / SPSR mode bits, Thumb flag, interrupt masks
 *   - CP15 (System Control) coprocessor register accessors
 *   - ARMv7-A cache maintenance operations (MCR p15)
 *   - IRQ line assignments (matches uiox.md arch/arm32/include/arch_defs.h)
 *   - Hard-float (VFPv3 / NEON) enable macros
 *
 * @version 1.0.0
 * @date    2026-06-24
 */

 #ifndef UIOX_FW_ARCH_ARM32_H
 #define UIOX_FW_ARCH_ARM32_H
 
 #include "../../include/uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * QEMU versatilepb peripheral base addresses
  * (matches uiox.md arch/arm32/include/arch_defs.h)
  * ====================================================================== */
 
 /* Flash / ROM */
 #define ARM32_ROM_BASE              0x00000000u
 #define ARM32_ROM_SIZE              0x00100000u  /**< 1 MB ROM             */
 
 /* DRAM */
 #define ARM32_RAM_BASE              0x00100000u  /**< RAM starts at 1 MB   */
 #define ARM32_RAM_SIZE              0x07F00000u  /**< ~127 MB              */
 
 /* System registers */
 #define ARM32_SYSREGS_BASE          0x10000000u
 #define ARM32_SYS_ID                (ARM32_SYSREGS_BASE + 0x000u)
 #define ARM32_SYS_SW                (ARM32_SYSREGS_BASE + 0x004u)
 #define ARM32_SYS_LED               (ARM32_SYSREGS_BASE + 0x008u)
 #define ARM32_SYS_OSC0              (ARM32_SYSREGS_BASE + 0x00Cu)
 #define ARM32_SYS_LOCK              (ARM32_SYSREGS_BASE + 0x020u)
 #define ARM32_SYS_RESETCTL          (ARM32_SYSREGS_BASE + 0x040u)
 #define ARM32_SYS_PCICTL            (ARM32_SYSREGS_BASE + 0x044u)
 #define ARM32_SYS_RESETCTL_RESET    0x00000100u  /**< Trigger soft reset   */
 #define ARM32_SYS_LOCK_MAGIC        0x0000A05Fu  /**< Unlock system ctrl   */
 
 /* PL190 Vectored Interrupt Controller (VIC) */
 #define ARM32_VIC_BASE              0x10140000u
 #define ARM32_VIC_SIZE              0x00001000u
 
 /* Secondary Interrupt Controller (SIC) */
 #define ARM32_SIC_BASE              0x10003000u
 
 /* SP804 Dual Timers */
 #define ARM32_TIMER01_BASE          0x101E2000u  /**< Timer 0 and 1        */
 #define ARM32_TIMER23_BASE          0x101E3000u  /**< Timer 2 and 3        */
 #define ARM32_TIMER_CLOCK_HZ        1000000u     /**< 1 MHz SP804 ref clk  */
 
 /* PL011 UARTs */
 #define ARM32_UART0_BASE            0x101F1000u
 #define ARM32_UART1_BASE            0x101F2000u
 #define ARM32_UART2_BASE            0x101F3000u
 #define ARM32_UART_CLOCK_HZ         24000000u    /**< 24 MHz PL011 ref clk */
 #define ARM32_UART_BAUD_DEFAULT     115200u
 
 /* PL031 RTC */
 #define ARM32_RTC_BASE              0x101E8000u
 
 /* PL061 GPIO */
 #define ARM32_GPIO0_BASE            0x101E4000u
 #define ARM32_GPIO1_BASE            0x101E5000u
 #define ARM32_GPIO2_BASE            0x101E6000u
 #define ARM32_GPIO3_BASE            0x101E7000u
 #define ARM32_GPIO_NUM_PINS         8u
 
 /* PL181 SD/MMC */
 #define ARM32_MMCI_BASE             0x10005000u
 
 /* SMC / NOR flash */
 #define ARM32_SMC_BASE              0x10100000u
 
 /* CLCD / framebuffer */
 #define ARM32_CLCD_BASE             0x10120000u
 
 /* Ethernet (SMSC LAN91C111) */
 #define ARM32_ETH_BASE              0x10010000u
 
 /* IDE */
 #define ARM32_IDE_BASE              0x1000D000u
 
 /* =========================================================================
  * PL190 VIC register offsets
  * ====================================================================== */
 
 #define VIC_IRQSTATUS               0x000u  /**< IRQ status (read)        */
 #define VIC_FIQSTATUS               0x004u  /**< FIQ status (read)        */
 #define VIC_RAWINTR                 0x008u  /**< Raw interrupt status      */
 #define VIC_INTSELECT               0x00Cu  /**< IRQ / FIQ select         */
 #define VIC_INTENABLE               0x010u  /**< Interrupt enable set      */
 #define VIC_INTENCLEAR              0x014u  /**< Interrupt enable clear    */
 #define VIC_SOFTINT                 0x018u  /**< Software interrupt        */
 #define VIC_SOFTINTCLEAR            0x01Cu  /**< Software interrupt clear  */
 #define VIC_PROTECTION              0x020u  /**< Protection enable         */
 #define VIC_SWPRIORITYMASK          0x024u  /**< SW priority mask          */
 #define VIC_PRIORITYDAISY           0x028u  /**< Daisy chain priority      */
 #define VIC_VECTADDR(n)             (0x100u + (n) * 4u)  /**< Vector addr  */
 #define VIC_VECTPRIORITY(n)         (0x200u + (n) * 4u)  /**< Priority     */
 #define VIC_ADDRESS                 0xF00u  /**< Vector address register   */
 #define VIC_DEFVECTADDR             0xF04u  /**< Default vector address    */
 #define VIC_PERIPH_ID0              0xFE0u
 #define VIC_CELL_ID0                0xFF0u
 
 /* =========================================================================
  * IRQ numbers — QEMU versatilepb
  * (matches uiox.md arch/arm32/include/arch_defs.h)
  * ====================================================================== */
 
 #define ARM32_IRQ_WATCHDOG          0u
 #define ARM32_IRQ_SOFTINT           1u
 #define ARM32_IRQ_COMMS_RX          2u
 #define ARM32_IRQ_COMMS_TX          3u
 #define ARM32_IRQ_TIMER01           4u   /**< SP804 Timer 0/1             */
 #define ARM32_IRQ_TIMER23           5u   /**< SP804 Timer 2/3             */
 #define ARM32_IRQ_GPIO0             6u
 #define ARM32_IRQ_GPIO1             7u
 #define ARM32_IRQ_GPIO2             8u
 #define ARM32_IRQ_GPIO3             9u
 #define ARM32_IRQ_RTC               10u
 #define ARM32_IRQ_SSP               11u
 #define ARM32_IRQ_UART0             12u
 #define ARM32_IRQ_UART1             13u
 #define ARM32_IRQ_UART2             14u
 #define ARM32_IRQ_SCI               15u
 #define ARM32_IRQ_CLCD              16u
 #define ARM32_IRQ_DMA               17u
 #define ARM32_IRQ_PWRFAIL           18u
 #define ARM32_IRQ_MBX               19u
 #define ARM32_IRQ_GND               20u
 /* Secondary interrupt controller (routed through VIC IRQ 31) */
 #define ARM32_IRQ_SIC_START         21u
 #define ARM32_IRQ_ETH               25u
 #define ARM32_IRQ_USB               27u
 #define ARM32_IRQ_IDE               22u
 
 /* =========================================================================
  * SP804 Dual-Timer register offsets
  * ====================================================================== */
 
 /* Timer 1 (base + 0x000) */
 #define SP804_TIMER1_LOAD           0x000u
 #define SP804_TIMER1_VALUE          0x004u  /**< Current count (read-only) */
 #define SP804_TIMER1_CTRL           0x008u
 #define SP804_TIMER1_INTCLR         0x00Cu  /**< Write any to clear IRQ    */
 #define SP804_TIMER1_RIS            0x010u  /**< Raw interrupt status      */
 #define SP804_TIMER1_MIS            0x014u  /**< Masked interrupt status   */
 #define SP804_TIMER1_BGLOAD         0x018u  /**< Background load           */
 
 /* Timer 2 (base + 0x020) */
 #define SP804_TIMER2_LOAD           0x020u
 #define SP804_TIMER2_VALUE          0x024u
 #define SP804_TIMER2_CTRL           0x028u
 #define SP804_TIMER2_INTCLR         0x02Cu
 #define SP804_TIMER2_RIS            0x030u
 #define SP804_TIMER2_MIS            0x034u
 #define SP804_TIMER2_BGLOAD         0x038u
 
 /* Control register bits */
 #define SP804_CTRL_EN               (1u << 7)  /**< Timer enable            */
 #define SP804_CTRL_PERIODIC         (1u << 6)  /**< Periodic / free-running */
 #define SP804_CTRL_IE               (1u << 5)  /**< Interrupt enable        */
 #define SP804_CTRL_DIV1             (0u << 2)  /**< No prescale             */
 #define SP804_CTRL_DIV16            (1u << 2)  /**< Divide by 16            */
 #define SP804_CTRL_DIV256           (2u << 2)  /**< Divide by 256           */
 #define SP804_CTRL_32BIT            (1u << 1)  /**< 32-bit counter mode     */
 #define SP804_CTRL_ONESHOT          (1u << 0)  /**< One-shot / wrapping     */
 
 /* Default configuration: 100 Hz from 1 MHz clock */
 #define SP804_LOAD_100HZ            (ARM32_TIMER_CLOCK_HZ / 100u)
 
 /* =========================================================================
  * PL011 UART register offsets and bits (ARM32 — same as ARM64 PL011)
  * ====================================================================== */
 
 #define PL011_DR                    0x000u
 #define PL011_RSR_ECR               0x004u
 #define PL011_FR                    0x018u
 #define PL011_FR_TXFF               (1u << 5)
 #define PL011_FR_RXFE               (1u << 4)
 #define PL011_FR_BUSY               (1u << 3)
 #define PL011_IBRD                  0x024u
 #define PL011_FBRD                  0x028u
 #define PL011_LCR_H                 0x02Cu
 #define PL011_LCR_WLEN8             (0x3u << 5)
 #define PL011_LCR_FEN               (1u << 4)
 #define PL011_CR                    0x030u
 #define PL011_CR_UARTEN             (1u << 0)
 #define PL011_CR_TXE                (1u << 8)
 #define PL011_CR_RXE                (1u << 9)
 #define PL011_IMSC                  0x038u
 #define PL011_ICR                   0x044u
 #define PL011_INT_ALL               0x7FFu
 
 /* =========================================================================
  * CPSR / SPSR processor mode bits
  * ====================================================================== */
 
 #define ARM32_MODE_USR              0x10u  /**< User mode                 */
 #define ARM32_MODE_FIQ              0x11u  /**< FIQ mode                  */
 #define ARM32_MODE_IRQ              0x12u  /**< IRQ mode                  */
 #define ARM32_MODE_SVC              0x13u  /**< Supervisor (SVC) mode     */
 #define ARM32_MODE_MON              0x16u  /**< Monitor mode              */
 #define ARM32_MODE_ABT              0x17u  /**< Abort mode                */
 #define ARM32_MODE_HYP              0x1Au  /**< Hyp mode                  */
 #define ARM32_MODE_UND              0x1Bu  /**< Undefined mode            */
 #define ARM32_MODE_SYS              0x1Fu  /**< System mode               */
 
 #define ARM32_CPSR_MODE_MASK        0x1Fu
 #define ARM32_CPSR_T                (1u << 5)   /**< Thumb state           */
 #define ARM32_CPSR_F                (1u << 6)   /**< FIQ disable           */
 #define ARM32_CPSR_I                (1u << 7)   /**< IRQ disable           */
 #define ARM32_CPSR_A                (1u << 8)   /**< Async abort disable   */
 #define ARM32_CPSR_E                (1u << 9)   /**< Endianness (0=LE)     */
 #define ARM32_CPSR_J                (1u << 24)  /**< Jazelle state         */
 #define ARM32_CPSR_Q                (1u << 27)  /**< Overflow/saturation   */
 #define ARM32_CPSR_V                (1u << 28)  /**< Overflow flag         */
 #define ARM32_CPSR_C                (1u << 29)  /**< Carry flag            */
 #define ARM32_CPSR_Z                (1u << 30)  /**< Zero flag             */
 #define ARM32_CPSR_N                (1u << 31)  /**< Negative flag         */
 
 /* Disable IRQ + FIQ: OR into CPSR */
 #define ARM32_CPSR_NOIRQ            (ARM32_CPSR_I | ARM32_CPSR_F)
 
 /* =========================================================================
  * SCTLR bits (CP15 c1, CRm 0, opcode2 0)
  * ====================================================================== */
 
 #define ARM32_SCTLR_M               (1u << 0)   /**< MMU enable            */
 #define ARM32_SCTLR_A               (1u << 1)   /**< Alignment check       */
 #define ARM32_SCTLR_C               (1u << 2)   /**< D-cache enable        */
 #define ARM32_SCTLR_W               (1u << 3)   /**< Write buffer enable   */
 #define ARM32_SCTLR_B               (1u << 7)   /**< Big endian            */
 #define ARM32_SCTLR_S               (1u << 8)   /**< System protection     */
 #define ARM32_SCTLR_R               (1u << 9)   /**< ROM protection        */
 #define ARM32_SCTLR_SW              (1u << 10)  /**< SWP/SWPB enable       */
 #define ARM32_SCTLR_Z               (1u << 11)  /**< Branch prediction     */
 #define ARM32_SCTLR_I               (1u << 12)  /**< I-cache enable        */
 #define ARM32_SCTLR_V               (1u << 13)  /**< High vectors          */
 #define ARM32_SCTLR_RR              (1u << 14)  /**< Round-robin replace   */
 #define ARM32_SCTLR_XP              (1u << 23)  /**< Subpage AP disable    */
 #define ARM32_SCTLR_VE              (1u << 24)  /**< IRQ vectors enable    */
 #define ARM32_SCTLR_TRE             (1u << 28)  /**< TEX remap enable      */
 #define ARM32_SCTLR_AFE             (1u << 29)  /**< Access flag enable    */
 #define ARM32_SCTLR_TE              (1u << 30)  /**< Thumb exception       */
 
 /* =========================================================================
  * CP15 system register accessors (inline assembly)
  * ====================================================================== */
 
 /** Read SCTLR */
 #define ARM32_SCTLR_READ(v) \
     __asm__ volatile("mrc p15,0,%0,c1,c0,0" : "=r"(v))
 
 /** Write SCTLR */
 #define ARM32_SCTLR_WRITE(v) \
     __asm__ volatile("mcr p15,0,%0,c1,c0,0; isb" :: "r"(v) : "memory")
 
 /** Read CPSR */
 #define ARM32_CPSR_READ(v) \
     __asm__ volatile("mrs %0, cpsr" : "=r"(v))
 
 /** Write CPSR_c (control bits only) */
 #define ARM32_CPSR_WRITE_C(v) \
     __asm__ volatile("msr cpsr_c, %0" :: "r"(v) : "memory")
 
 /** Invalidate entire TLB (unified) */
 #define ARM32_TLB_FLUSH() \
     do { uint32_t _z = 0u; \
          __asm__ volatile("mcr p15,0,%0,c8,c7,0; dsb; isb" \
                           :: "r"(_z) : "memory"); } while(0)
 
 /** Invalidate I-cache */
 #define ARM32_ICACHE_INV() \
     do { uint32_t _z = 0u; \
          __asm__ volatile("mcr p15,0,%0,c7,c5,0; isb" \
                           :: "r"(_z) : "memory"); } while(0)
 
 /** Clean and invalidate D-cache line by MVA to PoC */
 #define ARM32_DC_CIVAC(addr) \
     __asm__ volatile("mcr p15,0,%0,c7,c14,1" :: "r"(addr) : "memory")
 
 /** Full data-sync barrier */
 #define ARM32_DSB() \
     __asm__ volatile("dsb" ::: "memory")
 
 /** Instruction-sync barrier */
 #define ARM32_ISB() \
     __asm__ volatile("isb" ::: "memory")
 
 /* =========================================================================
  * Interrupt control macros
  * ====================================================================== */
 
 /** Disable IRQ and FIQ */
 #define ARM32_IRQ_DISABLE() \
     __asm__ volatile("cpsid if" ::: "memory")
 
 /** Enable IRQ and FIQ */
 #define ARM32_IRQ_ENABLE() \
     __asm__ volatile("cpsie if" ::: "memory")
 
 /** Enter SVC mode with IRQ/FIQ disabled */
 #define ARM32_ENTER_SVC_NOIRQ() \
     __asm__ volatile( \
         "mrs r0, cpsr\n" \
         "bic r0, r0, #0x1F\n" \
         "orr r0, r0, #0xD3\n" \
         "msr cpsr_c, r0\n" \
         ::: "r0", "memory")
 
 /** Wait for interrupt */
 #define ARM32_WFI() \
     __asm__ volatile("wfi" ::: "memory")
 
 /* =========================================================================
  * VFPv3 / NEON enable macros (hard-float ABI)
  * ====================================================================== */
 
 /**
  * Enable VFP by setting FPEXC.EN (bit 30).
  * Must be called before any floating-point instruction.
  */
 #define ARM32_VFP_ENABLE() \
     do { \
         uint32_t _fpexc; \
         __asm__ volatile("fmrx %0, fpexc" : "=r"(_fpexc)); \
         _fpexc |= (1u << 30); \
         __asm__ volatile("fmxr fpexc, %0" :: "r"(_fpexc)); \
     } while(0)
 
 /** Disable VFP */
 #define ARM32_VFP_DISABLE() \
     do { \
         uint32_t _fpexc; \
         __asm__ volatile("fmrx %0, fpexc" : "=r"(_fpexc)); \
         _fpexc &= ~(1u << 30); \
         __asm__ volatile("fmxr fpexc, %0" :: "r"(_fpexc)); \
     } while(0)
 
 /* =========================================================================
  * ARM32 platform registration
  * ====================================================================== */
 
 /** Called by uiox_fw_main() to register the ARM32 vtable. */
 void uiox_fw_arch_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_ARCH_ARM32_H */
 