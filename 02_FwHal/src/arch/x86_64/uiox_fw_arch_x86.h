/**
 * @file  uiox_fw_arch_x86.h
 * @brief UIOX Firmware — x86_64 architecture-specific definitions.
 *
 * Covers:
 *   - QEMU q35 / PC-compatible peripheral I/O port map
 *   - 8259A PIC (Programmable Interrupt Controller) register map
 *   - 8254 PIT (Programmable Interval Timer) register map
 *   - NS16550A / 8250 UART (COM1/COM2) I/O ports and bit fields
 *   - LAPIC / IOAPIC MMIO register offsets
 *   - ACPI PM1 control / reset register
 *   - RFLAGS bit definitions
 *   - MSR (Model Specific Register) numbers
 *   - CPU control register bits (CR0, CR3, CR4)
 *   - Inline I/O port helpers (inb/outb/inw/outw/inl/outl)
 *   - CLI / STI / HLT / CPUID macros
 *   - IRQ line assignments (matches uiox.md arch/x86_64/include/arch_defs.h)
 *   - GDT / IDT descriptor types
 *
 * @version 1.0.0
 * @date    2026-06-24
 */

 #ifndef UIOX_FW_ARCH_X86_H
 #define UIOX_FW_ARCH_X86_H
 
 #include "../include/uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * QEMU q35 / PC-compatible physical memory layout
  * ====================================================================== */
 
 #define X86_RAM_BASE                0x00100000ULL  /**< RAM starts at 1 MB  */
 #define X86_RAM_SIZE_DEFAULT        0x03F00000ULL  /**< 63 MB default       */
 #define X86_LAPIC_PHYS              0xFEE00000ULL  /**< Local APIC MMIO     */
 #define X86_IOAPIC_PHYS             0xFEC00000ULL  /**< I/O APIC MMIO       */
 #define X86_HPET_PHYS               0xFED00000ULL  /**< HPET MMIO           */
 #define X86_PCIE_ECAM_BASE          0xB0000000ULL  /**< PCIe ECAM (q35)     */
 #define X86_BIOS_ROM_BASE           0xFFFF0000ULL  /**< 64 KB legacy BIOS   */
 #define X86_VGA_TEXT_BASE           0x000B8000ULL  /**< VGA text buffer     */
 #define X86_VGA_TEXT_SIZE           0x00004000ULL
 #define X86_EBDA_BASE               0x0009FC00ULL  /**< Extended BIOS data  */
 
 /* =========================================================================
  * I/O port assignments (x86 legacy and QEMU q35)
  * (matches uiox.md arch/x86_64/include/arch_defs.h)
  * ====================================================================== */
 
 /* 8259A PIC */
 #define X86_PIC1_CMD                0x20u  /**< Master PIC command         */
 #define X86_PIC1_DATA               0x21u  /**< Master PIC data/IMR        */
 #define X86_PIC2_CMD                0xA0u  /**< Slave PIC command          */
 #define X86_PIC2_DATA               0xA1u  /**< Slave PIC data/IMR         */
 
 /* 8254 PIT */
 #define X86_PIT_CH0                 0x40u  /**< Channel 0 data             */
 #define X86_PIT_CH1                 0x41u  /**< Channel 1 data             */
 #define X86_PIT_CH2                 0x42u  /**< Channel 2 data             */
 #define X86_PIT_CMD                 0x43u  /**< Mode/command register      */
 #define X86_PIT_BASE_FREQ           1193182u  /**< 1.193182 MHz            */
 
 /* NS16550 UART */
 #define X86_COM1_PORT               0x3F8u  /**< COM1 base I/O port        */
 #define X86_COM2_PORT               0x2F8u  /**< COM2 base I/O port        */
 #define X86_COM3_PORT               0x3E8u
 #define X86_COM4_PORT               0x2E8u
 #define X86_COM_BAUD_DEFAULT        115200u
 
 /* CMOS / RTC */
 #define X86_CMOS_ADDR               0x70u
 #define X86_CMOS_DATA               0x71u
 #define X86_CMOS_NMI_DISABLE        0x80u  /**< Bit 7 in CMOS_ADDR        */
 
 /* Keyboard / PS2 controller (8042) */
 #define X86_KBD_DATA                0x60u
 #define X86_KBD_STATUS              0x64u
 #define X86_KBD_CMD                 0x64u
 #define X86_KBD_RESET               0xFEu  /**< System reset via 8042     */
 
 /* Bochs / QEMU debug console */
 #define X86_BOCHS_DEBUG_PORT        0xE9u
 
 /* ACPI / Power management (QEMU q35) */
 #define X86_ACPI_PM1A_EVT_BASE      0xB000u
 #define X86_ACPI_PM1A_CNT_BASE      0xB004u
 #define X86_ACPI_PM_TMR_BASE        0xB008u
 #define X86_ACPI_RESET_REG          0x0CF9u  /**< CF9 system reset        */
 #define X86_ACPI_RESET_VAL          0x0Eu    /**< Hard reset: INIT + RST  */
 
 /* DMA controller */
 #define X86_DMA1_BASE               0x0000u
 #define X86_DMA2_BASE               0x00C0u
 
 /* =========================================================================
  * 8259A PIC register bit fields
  * ====================================================================== */
 
 /* ICW1 bits */
 #define PIC_ICW1_IC4                (1u << 0)  /**< ICW4 needed            */
 #define PIC_ICW1_SNGL               (1u << 1)  /**< Single PIC (no slave)  */
 #define PIC_ICW1_ADI                (1u << 2)  /**< Call addr interval = 4 */
 #define PIC_ICW1_LTIM               (1u << 3)  /**< Level triggered mode   */
 #define PIC_ICW1_INIT               (1u << 4)  /**< Initialisation command */
 
 /* ICW4 bits */
 #define PIC_ICW4_8086               (1u << 0)  /**< 8086/88 mode           */
 #define PIC_ICW4_AEOI               (1u << 1)  /**< Auto EOI               */
 #define PIC_ICW4_BUF_SLAVE          (2u << 2)
 #define PIC_ICW4_BUF_MASTER         (3u << 2)
 #define PIC_ICW4_SFNM               (1u << 4)  /**< Special fully nested   */
 
 /* OCW2: EOI */
 #define PIC_EOI                     0x20u      /**< Non-specific EOI       */
 #define PIC_SPECIFIC_EOI(irq)       (0x60u | (irq))
 
 /* OCW3: read ISR / IRR */
 #define PIC_OCW3_READ_IRR           0x0Au
 #define PIC_OCW3_READ_ISR           0x0Bu
 
 /* Standard remap: master 0x20–0x27, slave 0x28–0x2F */
 #define PIC_MASTER_VECTOR_BASE      0x20u
 #define PIC_SLAVE_VECTOR_BASE       0x28u
 
 /* =========================================================================
  * 8254 PIT mode/command bits
  * ====================================================================== */
 
 #define PIT_CH_SELECT(n)            ((n) << 6)
 #define PIT_ACCESS_LATCH            (0u << 4)  /**< Counter latch command */
 #define PIT_ACCESS_LO               (1u << 4)  /**< Low byte only         */
 #define PIT_ACCESS_HI               (2u << 4)  /**< High byte only        */
 #define PIT_ACCESS_LOHI             (3u << 4)  /**< Low then high byte    */
 #define PIT_MODE_0                  (0u << 1)  /**< Interrupt on term cnt  */
 #define PIT_MODE_1                  (1u << 1)  /**< Hardware retriggerable */
 #define PIT_MODE_2                  (2u << 1)  /**< Rate generator        */
 #define PIT_MODE_3                  (3u << 1)  /**< Square wave generator  */
 #define PIT_MODE_4                  (4u << 1)  /**< Software triggered    */
 #define PIT_MODE_5                  (5u << 1)  /**< Hardware triggered     */
 #define PIT_BINARY                  (0u << 0)
 #define PIT_BCD                     (1u << 0)
 
 /* 100 Hz rate generator on channel 0 */
 #define PIT_CMD_CH0_100HZ \
     (PIT_CH_SELECT(0) | PIT_ACCESS_LOHI | PIT_MODE_2 | PIT_BINARY)
 #define PIT_DIVISOR_100HZ           (X86_PIT_BASE_FREQ / 100u)
 
 /* =========================================================================
  * NS16550 UART register offsets (relative to COM base port)
  * ====================================================================== */
 
 #define UART16550_RBR               0u   /**< Receive buffer (DLAB=0 R)   */
 #define UART16550_THR               0u   /**< Transmit holding (DLAB=0 W) */
 #define UART16550_DLL               0u   /**< Divisor latch low (DLAB=1)  */
 #define UART16550_IER               1u   /**< Interrupt enable reg        */
 #define UART16550_DLM               1u   /**< Divisor latch high (DLAB=1) */
 #define UART16550_IIR               2u   /**< Interrupt ID (read)         */
 #define UART16550_FCR               2u   /**< FIFO control (write)        */
 #define UART16550_LCR               3u   /**< Line control                */
 #define UART16550_MCR               4u   /**< Modem control               */
 #define UART16550_LSR               5u   /**< Line status                 */
 #define UART16550_MSR               6u   /**< Modem status                */
 #define UART16550_SCR               7u   /**< Scratch register            */
 
 /* LCR bits */
 #define UART16550_LCR_WLS5          0x00u  /**< 5-bit word length         */
 #define UART16550_LCR_WLS6          0x01u
 #define UART16550_LCR_WLS7          0x02u
 #define UART16550_LCR_WLS8          0x03u  /**< 8-bit word length         */
 #define UART16550_LCR_STB           (1u << 2)  /**< 2 stop bits           */
 #define UART16550_LCR_PEN           (1u << 3)  /**< Parity enable         */
 #define UART16550_LCR_EPS           (1u << 4)  /**< Even parity           */
 #define UART16550_LCR_SPS           (1u << 5)  /**< Stick parity          */
 #define UART16550_LCR_SBC           (1u << 6)  /**< Set break control     */
 #define UART16550_LCR_DLAB          (1u << 7)  /**< Divisor latch access  */
 #define UART16550_LCR_8N1           (UART16550_LCR_WLS8)
 
 /* LSR bits */
 /* DELETE these lines from uiox_fw_arch_x86.h already in include/uiox_fw_uart.h*/
 //#define UART16550_LSR_DR            (1u << 0)  /**< Data ready            */
 #define UART16550_LSR_OE            (1u << 1)  /**< Overrun error         */
 #define UART16550_LSR_PE            (1u << 2)  /**< Parity error          */
 #define UART16550_LSR_FE            (1u << 3)  /**< Framing error         */
 #define UART16550_LSR_BI            (1u << 4)  /**< Break interrupt       */
 /* DELETE these lines from uiox_fw_arch_x86.h already in include/uiox_fw_uart.h*/
 //#define UART16550_LSR_THRE          (1u << 5)  /**< THR empty             */
 #define UART16550_LSR_TEMT          (1u << 6)  /**< Transmitter empty     */
 #define UART16550_LSR_FIFOE         (1u << 7)  /**< FIFO data error       */
 
 /* FCR bits */
 #define UART16550_FCR_FIFOEN        (1u << 0)  /**< FIFO enable           */
 #define UART16550_FCR_RXRST         (1u << 1)  /**< RX FIFO reset         */
 #define UART16550_FCR_TXRST         (1u << 2)  /**< TX FIFO reset         */
 #define UART16550_FCR_DMA           (1u << 3)  /**< DMA mode select       */
 #define UART16550_FCR_TRIG1         (0u << 6)  /**< RX trigger at 1 byte  */
 #define UART16550_FCR_TRIG4         (1u << 6)  /**< RX trigger at 4 bytes */
 #define UART16550_FCR_TRIG8         (2u << 6)  /**< RX trigger at 8 bytes */
 #define UART16550_FCR_TRIG14        (3u << 6)  /**< RX trigger at 14 bytes*/
 
 /* IER bits */
 #define UART16550_IER_ERDAI         (1u << 0)  /**< RX data available IRQ */
 #define UART16550_IER_ETHREI        (1u << 1)  /**< THR empty IRQ         */
 #define UART16550_IER_ELSI          (1u << 2)  /**< RX line status IRQ    */
 #define UART16550_IER_EDSSI         (1u << 3)  /**< Modem status IRQ      */
 
 /* MCR bits */
 #define UART16550_MCR_DTR           (1u << 0)
 #define UART16550_MCR_RTS           (1u << 1)
 #define UART16550_MCR_OUT1          (1u << 2)
 #define UART16550_MCR_OUT2          (1u << 3)  /**< Needed to enable IRQs */
 #define UART16550_MCR_LOOP          (1u << 4)
 
 /* Standard 115200 baud divisor (base clock = 1.8432 MHz) */
 #define UART16550_DIVISOR(baud)     (115200u / (baud))
 
 /* =========================================================================
  * IRQ assignments (after 8259A remap to base 0x20 / 0x28)
  * (matches uiox.md arch/x86_64/include/arch_defs.h)
  * ====================================================================== */
 
 /* Remapped vector = physical IRQ + PIC_MASTER_VECTOR_BASE */
 #define X86_IRQ_TIMER               (0u  + PIC_MASTER_VECTOR_BASE)  /* 0x20 */
 #define X86_IRQ_KBD                 (1u  + PIC_MASTER_VECTOR_BASE)  /* 0x21 */
 #define X86_IRQ_CASCADE             (2u  + PIC_MASTER_VECTOR_BASE)  /* 0x22 */
 #define X86_IRQ_COM2                (3u  + PIC_MASTER_VECTOR_BASE)  /* 0x23 */
 #define X86_IRQ_COM1                (4u  + PIC_MASTER_VECTOR_BASE)  /* 0x24 */
 #define X86_IRQ_LPT2                (5u  + PIC_MASTER_VECTOR_BASE)  /* 0x25 */
 #define X86_IRQ_FLOPPY              (6u  + PIC_MASTER_VECTOR_BASE)  /* 0x26 */
 #define X86_IRQ_LPT1                (7u  + PIC_MASTER_VECTOR_BASE)  /* 0x27 */
 /* Slave PIC (IRQ8–15 → vectors 0x28–0x2F) */
 #define X86_IRQ_RTC                 (8u  + PIC_MASTER_VECTOR_BASE)  /* 0x28 */
 #define X86_IRQ_FREE1               (9u  + PIC_MASTER_VECTOR_BASE)  /* 0x29 */
 #define X86_IRQ_FREE2               (10u + PIC_MASTER_VECTOR_BASE)  /* 0x2A */
 #define X86_IRQ_FREE3               (11u + PIC_MASTER_VECTOR_BASE)  /* 0x2B */
 #define X86_IRQ_PS2MOUSE            (12u + PIC_MASTER_VECTOR_BASE)  /* 0x2C */
 #define X86_IRQ_FPU                 (13u + PIC_MASTER_VECTOR_BASE)  /* 0x2D */
 #define X86_IRQ_IDE0                (14u + PIC_MASTER_VECTOR_BASE)  /* 0x2E */
 #define X86_IRQ_IDE1                (15u + PIC_MASTER_VECTOR_BASE)  /* 0x2F */
 
 /* CPU exception vectors (fixed by Intel architecture) */
 #define X86_EXC_DE                  0x00u  /**< Divide error              */
 #define X86_EXC_DB                  0x01u  /**< Debug                     */
 #define X86_EXC_NMI                 0x02u  /**< Non-maskable interrupt    */
 #define X86_EXC_BP                  0x03u  /**< Breakpoint                */
 #define X86_EXC_OF                  0x04u  /**< Overflow                  */
 #define X86_EXC_BR                  0x05u  /**< Bound range exceeded      */
 #define X86_EXC_UD                  0x06u  /**< Invalid opcode            */
 #define X86_EXC_NM                  0x07u  /**< No math coprocessor       */
 #define X86_EXC_DF                  0x08u  /**< Double fault              */
 #define X86_EXC_TS                  0x0Au  /**< Invalid TSS               */
 #define X86_EXC_NP                  0x0Bu  /**< Segment not present       */
 #define X86_EXC_SS                  0x0Cu  /**< Stack segment fault       */
 #define X86_EXC_GP                  0x0Du  /**< General protection fault  */
 #define X86_EXC_PF                  0x0Eu  /**< Page fault                */
 #define X86_EXC_MF                  0x10u  /**< x87 FP exception          */
 #define X86_EXC_AC                  0x11u  /**< Alignment check           */
 #define X86_EXC_MC                  0x12u  /**< Machine check             */
 #define X86_EXC_XF                  0x13u  /**< SIMD FP exception         */
 
 /* =========================================================================
  * LAPIC MMIO register offsets (relative to X86_LAPIC_PHYS)
  * ====================================================================== */
 
 #define LAPIC_ID                    0x020u  /**< Local APIC ID            */
 #define LAPIC_VERSION               0x030u
 #define LAPIC_TPR                   0x080u  /**< Task Priority Register   */
 #define LAPIC_APR                   0x090u
 #define LAPIC_PPR                   0x0A0u  /**< Processor Priority       */
 #define LAPIC_EOI                   0x0B0u  /**< End of interrupt         */
 #define LAPIC_RRD                   0x0C0u
 #define LAPIC_LDR                   0x0D0u  /**< Logical destination      */
 #define LAPIC_DFR                   0x0E0u  /**< Destination format       */
 #define LAPIC_SVR                   0x0F0u  /**< Spurious vector register */
 #define LAPIC_ISR(n)                (0x100u + (n) * 0x10u)
 #define LAPIC_TMR(n)                (0x180u + (n) * 0x10u)
 #define LAPIC_IRR(n)                (0x200u + (n) * 0x10u)
 #define LAPIC_ESR                   0x280u  /**< Error status             */
 #define LAPIC_ICR_LO                0x300u  /**< Interrupt command (lo)   */
 #define LAPIC_ICR_HI                0x310u  /**< Interrupt command (hi)   */
 #define LAPIC_LVT_TIMER             0x320u
 #define LAPIC_LVT_THERMAL           0x330u
 #define LAPIC_LVT_PMI               0x340u
 #define LAPIC_LVT_LINT0             0x350u
 #define LAPIC_LVT_LINT1             0x360u
 #define LAPIC_LVT_ERROR             0x370u
 #define LAPIC_TIMER_ICR             0x380u  /**< Initial count            */
 #define LAPIC_TIMER_CCR             0x390u  /**< Current count            */
 #define LAPIC_TIMER_DCR             0x3E0u  /**< Divide configuration     */
 
 /* SVR bits */
 #define LAPIC_SVR_ENABLE            (1u << 8)
 #define LAPIC_SVR_FOCUS_DISABLE     (1u << 9)
 
 /* LVT bits */
 #define LAPIC_LVT_MASKED            (1u << 16)
 #define LAPIC_LVT_PERIODIC          (1u << 17)
 #define LAPIC_LVT_DELIVERY_FIXED    (0u << 8)
 #define LAPIC_LVT_DELIVERY_NMI      (4u << 8)
 #define LAPIC_LVT_DELIVERY_EXTINT   (7u << 8)
 
 /* Timer DCR (divisor) values */
 #define LAPIC_DCR_DIV2              0x00u
 #define LAPIC_DCR_DIV4              0x01u
 #define LAPIC_DCR_DIV8              0x02u
 #define LAPIC_DCR_DIV16             0x03u
 #define LAPIC_DCR_DIV32             0x08u
 #define LAPIC_DCR_DIV64             0x09u
 #define LAPIC_DCR_DIV128            0x0Au
 #define LAPIC_DCR_DIV1              0x0Bu
 
 /* =========================================================================
  * I/O APIC MMIO (relative to X86_IOAPIC_PHYS)
  * ====================================================================== */
 
 #define IOAPIC_REGSEL               0x00u  /**< Register select           */
 #define IOAPIC_IOWIN                0x10u  /**< I/O window (data)         */
 #define IOAPIC_REG_ID               0x00u
 #define IOAPIC_REG_VER              0x01u
 #define IOAPIC_REG_ARB              0x02u
 #define IOAPIC_REG_REDTBL(n)        (0x10u + (n) * 2u)
 
 /* =========================================================================
  * ACPI power management bits
  * ====================================================================== */
 
 #define ACPI_PM1_SLP_EN             (1u << 13)  /**< Sleep enable         */
 #define ACPI_PM1_SLP_TYP_S5         (0x07u << 10) /**< S5 = power off     */
 #define ACPI_PM1_SLP_TYP_S3         (0x05u << 10) /**< S3 = suspend-to-RAM*/
 
 /* QEMU q35 soft-off */
 #define X86_QEMU_SHUTDOWN() \
     do { \
         uint16_t _v = (uint16_t)(ACPI_PM1_SLP_TYP_S5 | ACPI_PM1_SLP_EN); \
         __asm__ volatile("outw %0,%1" :: "a"(_v), \
                          "dN"((uint16_t)X86_ACPI_PM1A_CNT_BASE)); \
     } while(0)
 
 /* =========================================================================
  * RFLAGS bit definitions
  * ====================================================================== */
 
 #define X86_RFLAGS_CF               (1ULL << 0)   /**< Carry             */
 #define X86_RFLAGS_PF               (1ULL << 2)   /**< Parity            */
 #define X86_RFLAGS_AF               (1ULL << 4)   /**< Auxiliary carry   */
 #define X86_RFLAGS_ZF               (1ULL << 6)   /**< Zero              */
 #define X86_RFLAGS_SF               (1ULL << 7)   /**< Sign              */
 #define X86_RFLAGS_TF               (1ULL << 8)   /**< Trap              */
 #define X86_RFLAGS_IF               (1ULL << 9)   /**< Interrupt enable  */
 #define X86_RFLAGS_DF               (1ULL << 10)  /**< Direction         */
 #define X86_RFLAGS_OF               (1ULL << 11)  /**< Overflow          */
 #define X86_RFLAGS_IOPL_MASK        (3ULL << 12)  /**< I/O privilege lvl */
 #define X86_RFLAGS_NT               (1ULL << 14)  /**< Nested task       */
 #define X86_RFLAGS_RF               (1ULL << 16)  /**< Resume            */
 #define X86_RFLAGS_VM               (1ULL << 17)  /**< Virtual 8086 mode */
 #define X86_RFLAGS_AC               (1ULL << 18)  /**< Alignment check   */
 #define X86_RFLAGS_VIF              (1ULL << 19)
 #define X86_RFLAGS_VIP              (1ULL << 20)
 #define X86_RFLAGS_ID               (1ULL << 21)  /**< CPUID available   */
 
 /* =========================================================================
  * MSR numbers
  * ====================================================================== */
 
 #define X86_MSR_IA32_APIC_BASE      0x0000001Bu
 #define X86_MSR_IA32_EFER           0xC0000080u  /**< Extended feature en */
 #define X86_MSR_IA32_STAR           0xC0000081u
 #define X86_MSR_IA32_LSTAR          0xC0000082u  /**< SYSCALL entry point  */
 #define X86_MSR_IA32_CSTAR          0xC0000083u
 #define X86_MSR_IA32_SFMASK         0xC0000084u
 #define X86_MSR_IA32_FS_BASE        0xC0000100u
 #define X86_MSR_IA32_GS_BASE        0xC0000101u
 #define X86_MSR_IA32_KERNEL_GS_BASE 0xC0000102u
 #define X86_MSR_IA32_TSC            0x00000010u  /**< Time-stamp counter   */
 #define X86_MSR_IA32_MISC_ENABLE    0x000001A0u
 
 /* IA32_EFER bits */
 #define X86_EFER_SCE                (1ULL << 0)  /**< SYSCALL enable       */
 #define X86_EFER_LME                (1ULL << 8)  /**< Long mode enable     */
 #define X86_EFER_LMA                (1ULL << 10) /**< Long mode active     */
 #define X86_EFER_NXE                (1ULL << 11) /**< No-execute enable    */
 
 /* =========================================================================
  * Control register bits
  * ====================================================================== */
 
 /* CR0 */
 #define X86_CR0_PE                  (1ULL << 0)   /**< Protection enable   */
 #define X86_CR0_MP                  (1ULL << 1)
 #define X86_CR0_EM                  (1ULL << 2)   /**< FPU emulation       */
 #define X86_CR0_TS                  (1ULL << 3)   /**< Task switched       */
 #define X86_CR0_ET                  (1ULL << 4)
 #define X86_CR0_NE                  (1ULL << 5)   /**< Numeric error       */
 #define X86_CR0_WP                  (1ULL << 16)  /**< Write protect       */
 #define X86_CR0_AM                  (1ULL << 18)  /**< Alignment mask      */
 #define X86_CR0_NW                  (1ULL << 29)  /**< Not write-through   */
 #define X86_CR0_CD                  (1ULL << 30)  /**< Cache disable       */
 #define X86_CR0_PG                  (1ULL << 31)  /**< Paging enable       */
 
 /* CR4 */
 #define X86_CR4_VME                 (1ULL << 0)
 #define X86_CR4_PVI                 (1ULL << 1)
 #define X86_CR4_TSD                 (1ULL << 2)
 #define X86_CR4_DE                  (1ULL << 3)
 #define X86_CR4_PSE                 (1ULL << 4)   /**< Page size extension */
 #define X86_CR4_PAE                 (1ULL << 5)   /**< Phys addr extension */
 #define X86_CR4_MCE                 (1ULL << 6)   /**< Machine check enable*/
 #define X86_CR4_PGE                 (1ULL << 7)   /**< Global page enable  */
 #define X86_CR4_PCE                 (1ULL << 8)
 #define X86_CR4_OSFXSR              (1ULL << 9)   /**< SSE enable          */
 #define X86_CR4_OSXMMEXCPT          (1ULL << 10)
 #define X86_CR4_SMEP                (1ULL << 20)
 #define X86_CR4_SMAP                (1ULL << 21)
 
 /* =========================================================================
  * Inline I/O port helper functions
  * ====================================================================== */
 
 static inline void x86_outb(uint16_t port, uint8_t val)
 { __asm__ volatile("outb %0,%1" :: "a"(val),  "dN"(port)); }
 
 static inline uint8_t x86_inb(uint16_t port)
 { uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "dN"(port)); return v; }
 
 static inline void x86_outw(uint16_t port, uint16_t val)
 { __asm__ volatile("outw %0,%1" :: "a"(val),  "dN"(port)); }
 
 static inline uint16_t x86_inw(uint16_t port)
 { uint16_t v; __asm__ volatile("inw %1,%0" : "=a"(v) : "dN"(port)); return v; }
 
 static inline void x86_outl(uint16_t port, uint32_t val)
 { __asm__ volatile("outl %0,%1" :: "a"(val),  "dN"(port)); }
 
 static inline uint32_t x86_inl(uint16_t port)
 { uint32_t v; __asm__ volatile("inl %1,%0" : "=a"(v) : "dN"(port)); return v; }
 
 /** Small I/O delay: write to port 0x80 (POST code port — harmless) */
 static inline void x86_io_delay(void)
 { __asm__ volatile("outb %%al, $0x80" :: "a"((uint8_t)0)); }
 
 /* =========================================================================
  * MSR read / write helpers
  * ====================================================================== */
 
 static inline uint64_t x86_rdmsr(uint32_t msr)
 {
     uint32_t lo, hi;
     __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
     return ((uint64_t)hi << 32u) | lo;
 }
 
 static inline void x86_wrmsr(uint32_t msr, uint64_t val)
 {
     __asm__ volatile("wrmsr"
                      :: "c"(msr),
                         "a"((uint32_t)(val & 0xFFFFFFFFu)),
                         "d"((uint32_t)(val >> 32u)));
 }
 
 /* =========================================================================
  * CPU control macros
  * ====================================================================== */
 
 #define X86_CLI()   __asm__ volatile("cli" ::: "memory")
 #define X86_STI()   __asm__ volatile("sti" ::: "memory")
 #define X86_HLT()   __asm__ volatile("hlt" ::: "memory")
 #define X86_NOP()   __asm__ volatile("nop" ::: "memory")
 #define X86_MFENCE() __asm__ volatile("mfence" ::: "memory")
 #define X86_SFENCE() __asm__ volatile("sfence" ::: "memory")
 #define X86_LFENCE() __asm__ volatile("lfence" ::: "memory")
 #define X86_PAUSE()  __asm__ volatile("pause"  ::: "memory")
 
 /** Read RFLAGS */
 static inline uint64_t x86_read_rflags(void)
 {
     uint64_t v;
     __asm__ volatile("pushfq; popq %0" : "=r"(v) :: "memory");
     return v;
 }
 
 /** CLFLUSH: flush a single cache line by address */
 #define X86_CLFLUSH(addr) \
     __asm__ volatile("clflush (%0)" :: "r"(addr) : "memory")
 
 /** WBINVD: write-back and invalidate all caches */
 #define X86_WBINVD() \
     __asm__ volatile("wbinvd" ::: "memory")
 
 /** Read TSC */
 static inline uint64_t x86_rdtsc(void)
 {
     uint32_t lo, hi;
     __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
     return ((uint64_t)hi << 32u) | lo;
 }
 
 /** CPUID: eax=function, fills eax/ebx/ecx/edx */
 static inline void x86_cpuid(uint32_t fn,
                                uint32_t *eax, uint32_t *ebx,
                                uint32_t *ecx, uint32_t *edx)
 {
     __asm__ volatile("cpuid"
                      : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                      : "0"(fn) : "memory");
 }
 
 /* =========================================================================
  * GDT / IDT descriptor types
  * ====================================================================== */
 
 #pragma pack(push, 1)
 
 /** GDT pointer (for LGDT instruction) */
 typedef struct {
     uint16_t limit;
     uint64_t base;
 } x86_gdtr_t;
 
 /** IDT pointer (for LIDT instruction) */
 typedef struct {
     uint16_t limit;
     uint64_t base;
 } x86_idtr_t;
 
 /** 64-bit GDT segment descriptor (8 bytes) */
 typedef struct {
     uint16_t limit_lo;
     uint16_t base_lo;
     uint8_t  base_mid;
     uint8_t  access;     /**< P | DPL | S | Type                         */
     uint8_t  granularity;/**< G | D/B | L | AVL | limit_hi[3:0]          */
     uint8_t  base_hi;
 } x86_gdt_entry_t;
 
 /** 64-bit IDT gate descriptor (16 bytes) */
 typedef struct {
     uint16_t offset_lo;
     uint16_t selector;   /**< Code segment selector                       */
     uint8_t  ist;        /**< Interrupt Stack Table index [2:0]           */
     uint8_t  type_attr;  /**< P | DPL | 0 | gate type                    */
     uint16_t offset_mid;
     uint32_t offset_hi;
     uint32_t reserved;
 } x86_idt_entry_t;
 
 #pragma pack(pop)
 
 /* GDT access byte values */
 #define GDT_ACCESS_PRESENT          (1u << 7)
 #define GDT_ACCESS_DPL0             (0u << 5)
 #define GDT_ACCESS_DPL3             (3u << 5)
 #define GDT_ACCESS_SYSTEM           (0u << 4)
 #define GDT_ACCESS_CODE_DATA        (1u << 4)
 #define GDT_ACCESS_CODE_X           (1u << 3)
 #define GDT_ACCESS_CODE_R           (1u << 1)
 #define GDT_ACCESS_DATA_W           (1u << 1)
 #define GDT_ACCESS_ACCESSED         (1u << 0)
 
 /* GDT granularity byte values */
 #define GDT_GRAN_4K                 (1u << 7)  /**< 4 KB page granularity */
 #define GDT_GRAN_32BIT              (1u << 6)  /**< 32-bit segment        */
 #define GDT_GRAN_64BIT              (1u << 5)  /**< Long mode (64-bit)    */
 
 /* Standard descriptor values for flat 64-bit kernel segments */
 #define GDT_KERNEL_CODE64_ACCESS    (GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | \
                                      GDT_ACCESS_CODE_DATA | GDT_ACCESS_CODE_X | \
                                      GDT_ACCESS_CODE_R)
 #define GDT_KERNEL_CODE64_GRAN      (GDT_GRAN_64BIT)
 
 #define GDT_KERNEL_DATA_ACCESS      (GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | \
                                      GDT_ACCESS_CODE_DATA | GDT_ACCESS_DATA_W)
 #define GDT_KERNEL_DATA_GRAN        (GDT_GRAN_32BIT)
 
 /* IDT gate type_attr values */
 #define IDT_GATE_INTERRUPT64        0x8Eu  /**< P=1, DPL=0, type=0xE     */
 #define IDT_GATE_TRAP64             0x8Fu  /**< P=1, DPL=0, type=0xF     */
 #define IDT_GATE_INTERRUPT64_DPL3   0xEEu  /**< P=1, DPL=3, type=0xE     */
 
 /* Macro to build an IDT entry */
 #define X86_IDT_ENTRY(offset, sel, ist_idx, attr) \
     { .offset_lo  = (uint16_t)((uint64_t)(offset) & 0xFFFFu), \
       .selector   = (uint16_t)(sel), \
       .ist        = (uint8_t)(ist_idx), \
       .type_attr  = (uint8_t)(attr), \
       .offset_mid = (uint16_t)(((uint64_t)(offset) >> 16u) & 0xFFFFu), \
       .offset_hi  = (uint32_t)(((uint64_t)(offset) >> 32u) & 0xFFFFFFFFu), \
       .reserved   = 0u }
 
 /* =========================================================================
  * x86_64 platform registration
  * ====================================================================== */
 
 /** Called by uiox_fw_main() to register the x86_64 vtable. */
 void uiox_fw_arch_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_ARCH_X86_H */
 