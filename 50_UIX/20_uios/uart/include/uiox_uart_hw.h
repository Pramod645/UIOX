/**
 * @file  uiox_uart_hw.h
 * @brief UIOX UART Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   ARM64/ARM32 — ARM PrimeCell PL011 (QEMU virt / versatilepb)
 *   x86_64      — NS16550A / 8250 (COM1 0x3F8 / COM2 0x2F8)
 *   RISC-V      — SiFive UART0 (future)
 *
 * Owns:
 *   - MMIO register access (PL011) and I/O port access (16550)
 *   - Baud-rate divisor calculation
 *   - GPIO: CTS / RTS / DTR / DSR (hardware flow control)
 *   - IRQ: RX data ready, TX empty, line status, modem status
 *   - FIFO depth configuration
 *
 * Reference: uiox_fw_uart.c (02_FwHal/src/uiox_fw_uart.c)
 *
 * @version 1.0.0
 * @date    2026-07-05
 */

 #ifndef UIOX_UART_HW_H
 #define UIOX_UART_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * UART variant
  * ====================================================================== */
 
 typedef enum {
     UIOX_UART_PL011   = 0,  /**< ARM PrimeCell PL011 (MMIO)             */
     UIOX_UART_16550   = 1,  /**< NS16550A / 8250 (x86 I/O port)         */
     UIOX_UART_SIFIVE  = 2,  /**< SiFive UART (RISC-V MMIO)              */
 } uiox_uart_variant_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_UART_CAP_FIFO          (1u << 0)  /**< HW FIFO              */
 #define UIOX_UART_CAP_HW_FLOW       (1u << 1)  /**< CTS/RTS flow control */
 #define UIOX_UART_CAP_MODEM         (1u << 2)  /**< Full modem signals   */
 #define UIOX_UART_CAP_BREAK         (1u << 3)  /**< Break detection      */
 #define UIOX_UART_CAP_DMA           (1u << 4)  /**< DMA TX/RX            */
 #define UIOX_UART_CAP_AUTO_BAUD     (1u << 5)  /**< Auto baud detection  */
 #define UIOX_UART_CAP_9BIT          (1u << 6)  /**< 9-bit address mode   */
 #define UIOX_UART_CAP_IrDA          (1u << 7)  /**< IrDA SIR mode        */
 
 /* =========================================================================
  * PL011 register offsets (ARM PrimeCell UART)
  * ====================================================================== */
 
 #define PL011_DR                0x000u  /**< Data register               */
 #define PL011_RSR_ECR           0x004u  /**< Receive status / Error clear*/
 #define PL011_FR                0x018u  /**< Flag register               */
 #define PL011_ILPR              0x020u  /**< IrDA low-power counter      */
 #define PL011_IBRD              0x024u  /**< Integer baud-rate divisor   */
 #define PL011_FBRD              0x028u  /**< Fractional baud-rate div    */
 #define PL011_LCR_H             0x02Cu  /**< Line control register       */
 #define PL011_CR                0x030u  /**< Control register            */
 #define PL011_IFLS              0x034u  /**< FIFO level select           */
 #define PL011_IMSC              0x038u  /**< Interrupt mask set/clear    */
 #define PL011_RIS               0x03Cu  /**< Raw interrupt status        */
 #define PL011_MIS               0x040u  /**< Masked interrupt status     */
 #define PL011_ICR               0x044u  /**< Interrupt clear register    */
 #define PL011_DMACR             0x048u  /**< DMA control                 */
 #define PL011_PERIPH_ID0        0xFE0u  /**< Peripheral ID 0             */
 
 /* PL011 FR (Flag Register) bits */
 #define PL011_FR_RI             (1u << 8)   /**< Ring indicator          */
 #define PL011_FR_TXFE           (1u << 7)   /**< TX FIFO empty           */
 #define PL011_FR_RXFF           (1u << 6)   /**< RX FIFO full            */
 #define PL011_FR_TXFF           (1u << 5)   /**< TX FIFO full            */
 #define PL011_FR_RXFE           (1u << 4)   /**< RX FIFO empty           */
 #define PL011_FR_BUSY           (1u << 3)   /**< UART busy transmitting  */
 #define PL011_FR_DCD            (1u << 2)   /**< Data carrier detect     */
 #define PL011_FR_DSR            (1u << 1)   /**< Data set ready          */
 #define PL011_FR_CTS            (1u << 0)   /**< Clear to send           */
 
 /* PL011 DR receive error bits [11:8] */
 #define PL011_DR_OE             (1u << 11)  /**< Overrun error           */
 #define PL011_DR_BE             (1u << 10)  /**< Break error             */
 #define PL011_DR_PE             (1u << 9)   /**< Parity error            */
 #define PL011_DR_FE             (1u << 8)   /**< Framing error           */
 
 /* PL011 LCR_H bits */
 #define PL011_LCR_SPS           (1u << 7)   /**< Stick parity select     */
 #define PL011_LCR_WLEN8         (3u << 5)   /**< 8-bit word              */
 #define PL011_LCR_WLEN7         (2u << 5)   /**< 7-bit word              */
 #define PL011_LCR_WLEN6         (1u << 5)   /**< 6-bit word              */
 #define PL011_LCR_WLEN5         (0u << 5)   /**< 5-bit word              */
 #define PL011_LCR_FEN           (1u << 4)   /**< FIFO enable             */
 #define PL011_LCR_STP2          (1u << 3)   /**< 2 stop bits             */
 #define PL011_LCR_EPS           (1u << 2)   /**< Even parity select      */
 #define PL011_LCR_PEN           (1u << 1)   /**< Parity enable           */
 #define PL011_LCR_BRK           (1u << 0)   /**< Send break              */
 
 /* PL011 CR bits */
 #define PL011_CR_CTSEN          (1u << 15)  /**< CTS flow ctrl enable    */
 #define PL011_CR_RTSEN          (1u << 14)  /**< RTS flow ctrl enable    */
 #define PL011_CR_OUT2           (1u << 13)  /**< OUT2 signal             */
 #define PL011_CR_OUT1           (1u << 12)  /**< OUT1 signal             */
 #define PL011_CR_RTS            (1u << 11)  /**< RTS                     */
 #define PL011_CR_DTR            (1u << 10)  /**< DTR                     */
 #define PL011_CR_RXE            (1u << 9)   /**< RX enable               */
 #define PL011_CR_TXE            (1u << 8)   /**< TX enable               */
 #define PL011_CR_LBE            (1u << 7)   /**< Loopback enable         */
 #define PL011_CR_UARTEN         (1u << 0)   /**< UART enable             */
 
 /* PL011 interrupt bits (IMSC / RIS / MIS / ICR) */
 #define PL011_INT_OEI           (1u << 10)  /**< Overrun error IRQ       */
 #define PL011_INT_BEI           (1u << 9)   /**< Break error IRQ         */
 #define PL011_INT_PEI           (1u << 8)   /**< Parity error IRQ        */
 #define PL011_INT_FEI           (1u << 7)   /**< Framing error IRQ       */
 #define PL011_INT_RTI           (1u << 6)   /**< RX timeout IRQ          */
 #define PL011_INT_TXI           (1u << 5)   /**< TX IRQ (FIFO ≤ level)  */
 #define PL011_INT_RXI           (1u << 4)   /**< RX IRQ (FIFO ≥ level)  */
 #define PL011_INT_DSRMI         (1u << 3)   /**< DSR modem IRQ           */
 #define PL011_INT_DCDMI         (1u << 2)   /**< DCD modem IRQ           */
 #define PL011_INT_CTSMI         (1u << 1)   /**< CTS modem IRQ           */
 #define PL011_INT_RIMI          (1u << 0)   /**< RI modem IRQ            */
 #define PL011_INT_ALL           0x7FFu      /**< All interrupts          */
 #define PL011_INT_ERROR         (PL011_INT_OEI | PL011_INT_BEI | \
                                   PL011_INT_PEI | PL011_INT_FEI)
 
 /* PL011 IFLS (FIFO level select) */
 #define PL011_IFLS_RX_1_8       (0u << 3)   /**< RX FIFO ≥ 1/8 full    */
 #define PL011_IFLS_RX_1_4       (1u << 3)   /**< RX FIFO ≥ 1/4 full    */
 #define PL011_IFLS_RX_1_2       (2u << 3)   /**< RX FIFO ≥ 1/2 full    */
 #define PL011_IFLS_RX_3_4       (3u << 3)   /**< RX FIFO ≥ 3/4 full    */
 #define PL011_IFLS_RX_7_8       (4u << 3)   /**< RX FIFO ≥ 7/8 full    */
 #define PL011_IFLS_TX_1_8       (0u << 0)   /**< TX FIFO ≤ 1/8 full    */
 #define PL011_IFLS_TX_1_4       (1u << 0)   /**< TX FIFO ≤ 1/4 full    */
 #define PL011_IFLS_TX_1_2       (2u << 0)   /**< TX FIFO ≤ 1/2 full    */
 #define PL011_IFLS_TX_3_4       (3u << 0)   /**< TX FIFO ≤ 3/4 full    */
 #define PL011_IFLS_TX_7_8       (4u << 0)   /**< TX FIFO ≤ 7/8 full    */
 
 /* =========================================================================
  * NS16550 register offsets (x86 I/O port relative)
  * ====================================================================== */
 
 #define UART16550_RBR           0u   /**< Receive buffer (DLAB=0, R)      */
 #define UART16550_THR           0u   /**< Transmit holding (DLAB=0, W)    */
 #define UART16550_DLL           0u   /**< Divisor latch low  (DLAB=1)     */
 #define UART16550_IER           1u   /**< Interrupt enable                */
 #define UART16550_DLM           1u   /**< Divisor latch high (DLAB=1)     */
 #define UART16550_IIR           2u   /**< Interrupt ID (read)             */
 #define UART16550_FCR           2u   /**< FIFO control (write)            */
 #define UART16550_LCR           3u   /**< Line control                    */
 #define UART16550_MCR           4u   /**< Modem control                   */
 #define UART16550_LSR           5u   /**< Line status                     */
 #define UART16550_MSR           6u   /**< Modem status                    */
 #define UART16550_SCR           7u   /**< Scratch register                */
 
 /* IER bits */
 #define UART16550_IER_ERBFI     (1u << 0)  /**< RX data available IRQ    */
 #define UART16550_IER_ETBEI     (1u << 1)  /**< TX holding empty IRQ     */
 #define UART16550_IER_ELSI      (1u << 2)  /**< RX line status IRQ       */
 #define UART16550_IER_EDSSI     (1u << 3)  /**< Modem status IRQ         */
 
 /* FCR bits */
 #define UART16550_FCR_FIFOEN    (1u << 0)  /**< FIFO enable              */
 #define UART16550_FCR_RXRST     (1u << 1)  /**< RX FIFO reset            */
 #define UART16550_FCR_TXRST     (1u << 2)  /**< TX FIFO reset            */
 #define UART16550_FCR_DMA       (1u << 3)  /**< DMA mode                 */
 #define UART16550_FCR_TRIG1     (0u << 6)  /**< RX trigger at 1 byte     */
 #define UART16550_FCR_TRIG4     (1u << 6)  /**< RX trigger at 4 bytes    */
 #define UART16550_FCR_TRIG8     (2u << 6)  /**< RX trigger at 8 bytes    */
 #define UART16550_FCR_TRIG14    (3u << 6)  /**< RX trigger at 14 bytes   */
 
 /* LCR bits */
 #define UART16550_LCR_WLS5      0x00u  /**< 5-bit word length            */
 #define UART16550_LCR_WLS6      0x01u  /**< 6-bit                        */
 #define UART16550_LCR_WLS7      0x02u  /**< 7-bit                        */
 #define UART16550_LCR_WLS8      0x03u  /**< 8-bit                        */
 #define UART16550_LCR_STB       (1u << 2)  /**< 2 stop bits              */
 #define UART16550_LCR_PEN       (1u << 3)  /**< Parity enable            */
 #define UART16550_LCR_EPS       (1u << 4)  /**< Even parity              */
 #define UART16550_LCR_SPS       (1u << 5)  /**< Stick parity             */
 #define UART16550_LCR_SBC       (1u << 6)  /**< Set break                */
 #define UART16550_LCR_DLAB      (1u << 7)  /**< Divisor latch access     */
 #define UART16550_LCR_8N1       (UART16550_LCR_WLS8)
 
 /* LSR bits */
 #define UART16550_LSR_DR        (1u << 0)  /**< Data ready               */
 #define UART16550_LSR_OE        (1u << 1)  /**< Overrun error            */
 #define UART16550_LSR_PE        (1u << 2)  /**< Parity error             */
 #define UART16550_LSR_FE        (1u << 3)  /**< Framing error            */
 #define UART16550_LSR_BI        (1u << 4)  /**< Break interrupt          */
 #define UART16550_LSR_THRE      (1u << 5)  /**< TX holding empty         */
 #define UART16550_LSR_TEMT      (1u << 6)  /**< TX empty                 */
 #define UART16550_LSR_FIFOE     (1u << 7)  /**< FIFO data error          */
 
 /* MCR bits */
 #define UART16550_MCR_DTR       (1u << 0)
 #define UART16550_MCR_RTS       (1u << 1)
 #define UART16550_MCR_OUT1      (1u << 2)
 #define UART16550_MCR_OUT2      (1u << 3)  /**< Enable IRQs              */
 #define UART16550_MCR_LOOP      (1u << 4)  /**< Loopback                 */
 
 /* MSR bits */
 #define UART16550_MSR_DCTS      (1u << 0)  /**< Delta CTS                */
 #define UART16550_MSR_DDSR      (1u << 1)  /**< Delta DSR                */
 #define UART16550_MSR_TERI      (1u << 2)  /**< Trailing edge RI         */
 #define UART16550_MSR_DDCD      (1u << 3)  /**< Delta DCD                */
 #define UART16550_MSR_CTS       (1u << 4)  /**< CTS                      */
 #define UART16550_MSR_DSR       (1u << 5)  /**< DSR                      */
 #define UART16550_MSR_RI        (1u << 6)  /**< Ring indicator           */
 #define UART16550_MSR_DCD       (1u << 7)  /**< Data carrier detect      */
 
 /* =========================================================================
  * SiFive UART register offsets (RISC-V)
  * ====================================================================== */
 
 #define SIFIVE_UART_TXDATA      0x00u  /**< TX data (bit31 = full)       */
 #define SIFIVE_UART_RXDATA      0x04u  /**< RX data (bit31 = empty)      */
 #define SIFIVE_UART_TXCTRL      0x08u  /**< TX control                   */
 #define SIFIVE_UART_RXCTRL      0x0Cu  /**< RX control                   */
 #define SIFIVE_UART_IE          0x10u  /**< Interrupt enable             */
 #define SIFIVE_UART_IP          0x14u  /**< Interrupt pending            */
 #define SIFIVE_UART_DIV         0x18u  /**< Baud rate divisor            */
 #define SIFIVE_UART_TXDATA_FULL (1u << 31)
 #define SIFIVE_UART_RXDATA_EMPTY (1u << 31)
 #define SIFIVE_UART_TXCTRL_TXEN (1u << 0)
 #define SIFIVE_UART_RXCTRL_RXEN (1u << 0)
 #define SIFIVE_UART_IE_TXWM     (1u << 0)  /**< TX watermark IRQ        */
 #define SIFIVE_UART_IE_RXWM     (1u << 1)  /**< RX watermark IRQ        */
 
 /* =========================================================================
  * Platform UART addresses (matches 10_Arch / 02_FwHal arch_defs)
  * ====================================================================== */
 
 #define UIOX_UART_ARM64_BASE    0x09000000u  /**< QEMU virt PL011 UART0  */
 #define UIOX_UART_ARM64_CLK     24000000u    /**< 24 MHz reference clock */
 #define UIOX_UART_ARM32_BASE    0x101F1000u  /**< versatilepb PL011 UART0*/
 #define UIOX_UART_ARM32_CLK     24000000u
 #define UIOX_UART_X86_COM1      0x3F8u       /**< COM1 I/O port base     */
 #define UIOX_UART_X86_COM2      0x2F8u
 #define UIOX_UART_BAUD_DEFAULT  115200u
 
 /* =========================================================================
  * Parity, stop bits, word length (IC-agnostic enumerations)
  * ====================================================================== */
 
 typedef enum {
     UIOX_UART_PARITY_NONE  = 0,
     UIOX_UART_PARITY_ODD   = 1,
     UIOX_UART_PARITY_EVEN  = 2,
     UIOX_UART_PARITY_MARK  = 3,  /**< Stick parity = 1                  */
     UIOX_UART_PARITY_SPACE = 4,  /**< Stick parity = 0                  */
 } uiox_uart_parity_t;
 
 typedef enum {
     UIOX_UART_STOP_1   = 0,
     UIOX_UART_STOP_1_5 = 1,  /**< 16550 only (5-bit word)               */
     UIOX_UART_STOP_2   = 2,
 } uiox_uart_stop_t;
 
 typedef enum {
     UIOX_UART_BITS_5 = 5,
     UIOX_UART_BITS_6 = 6,
     UIOX_UART_BITS_7 = 7,
     UIOX_UART_BITS_8 = 8,
 } uiox_uart_bits_t;
 
 typedef enum {
     UIOX_UART_FLOW_NONE   = 0,
     UIOX_UART_FLOW_HW_RTS = 1,  /**< RTS/CTS hardware flow control      */
     UIOX_UART_FLOW_SW_XON = 2,  /**< XON/XOFF software flow control     */
 } uiox_uart_flow_t;
 
 /* =========================================================================
  * UART configuration
  * ====================================================================== */
 
 typedef struct {
     uint32_t           baud;
     uiox_uart_bits_t   data_bits;
     uiox_uart_stop_t   stop_bits;
     uiox_uart_parity_t parity;
     uiox_uart_flow_t   flow_ctrl;
     bool               fifo_en;
     bool               loopback;
 } uiox_uart_cfg_t;
 
 #define UIOX_UART_CFG_DEFAULT \
     { .baud       = UIOX_UART_BAUD_DEFAULT, \
       .data_bits  = UIOX_UART_BITS_8, \
       .stop_bits  = UIOX_UART_STOP_1, \
       .parity     = UIOX_UART_PARITY_NONE, \
       .flow_ctrl  = UIOX_UART_FLOW_NONE, \
       .fifo_en    = true, \
       .loopback   = false }
 
 /* =========================================================================
  * UART error flags (bitmask — can be OR'd)
  * ====================================================================== */
 
 #define UIOX_UART_ERR_NONE      0u
 #define UIOX_UART_ERR_OVERRUN   (1u << 0)
 #define UIOX_UART_ERR_PARITY    (1u << 1)
 #define UIOX_UART_ERR_FRAMING   (1u << 2)
 #define UIOX_UART_ERR_BREAK     (1u << 3)
 #define UIOX_UART_ERR_FIFO      (1u << 4)
 #define UIOX_UART_ERR_TIMEOUT   (1u << 5)
 
 /* =========================================================================
  * UART hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_UART_MODEL_LEN     32u
 
 typedef struct {
     uintptr_t           base;        /**< MMIO base / I/O port base       */
     uint32_t            clk_hz;      /**< Input reference clock Hz        */
     uint32_t            irq;
     uint32_t            caps;
     uiox_uart_variant_t variant;
     char                model[UIOX_UART_MODEL_LEN];
     uiox_uart_cfg_t     cfg;
     volatile uint32_t   pending_irq; /**< Bitmask of pending IRQ sources  */
     uint32_t            error_flags; /**< Accumulated error bitmask       */
     void               *priv;        /**< Points to uiox_uart_hw_ops_t   */
 } uiox_uart_hw_t;
 
 /* Pending IRQ bits */
 #define UIOX_UART_IRQ_RX_DATA   (1u << 0)  /**< RX data available       */
 #define UIOX_UART_IRQ_TX_EMPTY  (1u << 1)  /**< TX FIFO / THR empty     */
 #define UIOX_UART_IRQ_LINE_ERR  (1u << 2)  /**< Line status error        */
 #define UIOX_UART_IRQ_MODEM     (1u << 3)  /**< Modem status change      */
 #define UIOX_UART_IRQ_RX_TIMEOUT (1u << 4) /**< RX FIFO timeout          */
 
 /* =========================================================================
  * Hardware operations vtable (16-op table)
  * ====================================================================== */
 
 typedef struct {
     /* Lifecycle */
     int  (*init)        (uiox_uart_hw_t *hw,
                          const uiox_uart_cfg_t *cfg);
     void (*deinit)      (uiox_uart_hw_t *hw);
 
     /* Basic I/O */
     void (*putc)        (uiox_uart_hw_t *hw, char c);
     int  (*getc)        (uiox_uart_hw_t *hw);   /**< -1 = no data        */
     bool (*rx_ready)    (uiox_uart_hw_t *hw);
     bool (*tx_ready)    (uiox_uart_hw_t *hw);
     void (*tx_flush)    (uiox_uart_hw_t *hw);   /**< Wait TX complete     */
 
     /* Configuration */
     int  (*set_baud)    (uiox_uart_hw_t *hw, uint32_t baud);
     int  (*set_format)  (uiox_uart_hw_t *hw,
                          uiox_uart_bits_t bits,
                          uiox_uart_stop_t stop,
                          uiox_uart_parity_t parity);
     int  (*set_flow)    (uiox_uart_hw_t *hw, uiox_uart_flow_t flow);
     int  (*set_loopback)(uiox_uart_hw_t *hw, bool enable);
 
     /* Break */
     void (*send_break)  (uiox_uart_hw_t *hw, uint32_t duration_ms);
 
     /* Interrupt control */
     void (*irq_enable)  (uiox_uart_hw_t *hw, uint32_t irq_mask);
     void (*irq_disable) (uiox_uart_hw_t *hw, uint32_t irq_mask);
     uint32_t (*irq_status)(uiox_uart_hw_t *hw); /**< Read+clear pending  */
 
     /* GPIO */
     void (*gpio_write)  (uiox_uart_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)   (uiox_uart_hw_t *hw, uint32_t pin);
 
     /* ISR */
     void (*isr)         (uiox_uart_hw_t *hw);
 } uiox_uart_hw_ops_t;
 
 /* =========================================================================
  * MMIO helper
  * ====================================================================== */
 
 static inline void uart_mmio_write32(uintptr_t addr, uint32_t val)
 { *((volatile uint32_t *)addr) = val; }
 
 static inline uint32_t uart_mmio_read32(uintptr_t addr)
 { return *((volatile uint32_t *)addr); }
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_uart_hw_init        (uiox_uart_hw_t *hw,
                                     const uiox_uart_hw_ops_t *ops,
                                     const uiox_uart_cfg_t *cfg);
 void     uiox_uart_hw_deinit      (uiox_uart_hw_t *hw);
 void     uiox_uart_hw_putc        (uiox_uart_hw_t *hw, char c);
 int      uiox_uart_hw_getc        (uiox_uart_hw_t *hw);
 bool     uiox_uart_hw_rx_ready    (uiox_uart_hw_t *hw);
 bool     uiox_uart_hw_tx_ready    (uiox_uart_hw_t *hw);
 void     uiox_uart_hw_tx_flush    (uiox_uart_hw_t *hw);
 int      uiox_uart_hw_set_baud    (uiox_uart_hw_t *hw, uint32_t baud);
 int      uiox_uart_hw_set_format  (uiox_uart_hw_t *hw,
                                     uiox_uart_bits_t bits,
                                     uiox_uart_stop_t stop,
                                     uiox_uart_parity_t parity);
 int      uiox_uart_hw_set_flow    (uiox_uart_hw_t *hw, uiox_uart_flow_t flow);
 void     uiox_uart_hw_send_break  (uiox_uart_hw_t *hw, uint32_t ms);
 void     uiox_uart_hw_irq_enable  (uiox_uart_hw_t *hw, uint32_t mask);
 void     uiox_uart_hw_irq_disable (uiox_uart_hw_t *hw, uint32_t mask);
 
 /* Platform-specific initialisation helpers */
 int uiox_uart_pl011_init  (uiox_uart_hw_t *hw, uintptr_t base,
                              uint32_t clk_hz, uint32_t irq,
                              const uiox_uart_cfg_t *cfg);
 int uiox_uart_16550_init  (uiox_uart_hw_t *hw, uintptr_t port,
                              uint32_t irq,
                              const uiox_uart_cfg_t *cfg);
 int uiox_uart_sifive_init (uiox_uart_hw_t *hw, uintptr_t base,
                              uint32_t clk_hz, uint32_t irq,
                              const uiox_uart_cfg_t *cfg);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_UART_HW_H */
 