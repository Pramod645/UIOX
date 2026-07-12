5_RegOffsets
============

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Peripheral (ARM64 base)
     - Register Name
     - Offset (hex)
     - Width
     - Access
     - Reset Value
     - Description
     - Source File
   * - UART0 PL011 @ 0xFC020000
     - UARTDR
     - 0x000
     - 32
     - R/W
     - 0x0000
     - Data Register: [10:8]=error flags; [7:0]=data
     - usb_hw/cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTRSR
     - 0x004
     - 32
     - R/W
     - 0x0000
     - Receive Status/Error Clear
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTFR
     - 0x018
     - 32
     - R
     - 0x0090
     - Flag Reg: [7]=TXFE [6]=RXFF [5]=TXFF [4]=RXFE [3]=BUSY
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTIBRD
     - 0x024
     - 32
     - R/W
     - 0x0000
     - Integer Baud Rate Divisor
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTFBRD
     - 0x028
     - 32
     - R/W
     - 0x0000
     - Fractional Baud Rate Divisor
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTLCR_H
     - 0x02C
     - 32
     - R/W
     - 0x0000
     - Line Ctrl: [6:5]=WLEN [4]=FEN [1]=PEN
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTCR
     - 0x030
     - 32
     - R/W
     - 0x0300
     - Ctrl: [15]=CTSEN [14]=RTSEN [9]=RXE [8]=TXE [0]=UARTEN
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTIMSC
     - 0x038
     - 32
     - R/W
     - 0x0000
     - Interrupt Mask Set/Clear
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTMIS
     - 0x040
     - 32
     - R
     - 0x0000
     - Masked Interrupt Status
     - cpu_hw
   * - UART0 PL011 @ 0xFC020000
     - UARTICR
     - 0x044
     - 32
     - W
     - —
     - Interrupt Clear Register
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_CTLR
     - 0x000
     - 32
     - R/W
     - 0x0
     - Distributor Ctrl; ARE_S/NS bits
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_TYPER
     - 0x004
     - 32
     - R
     - —
     - Interrupt type; ITLinesNumber field
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_IGROUPR0
     - 0x080
     - 32
     - R/W
     - 0x0
     - IRQ Group 0 (SPIs 0-31)
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_ISENABLER0
     - 0x100
     - 32
     - W
     - —
     - Enable Set; bit n=SPI n
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_ICENABLER0
     - 0x180
     - 32
     - W
     - —
     - Enable Clear
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_ISPENDR0
     - 0x200
     - 32
     - W
     - —
     - Pending Set
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_ICPENDR0
     - 0x280
     - 32
     - W
     - —
     - Pending Clear
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_IPRIORITYR0
     - 0x400
     - 32
     - R/W
     - 0x0
     - Priority (8b/SPI)
     - cpu_hw
   * - GIC GICD @ 0xFE000000
     - GICD_IROUTER0
     - 0x6000
     - 64
     - R/W
     - 0x0
     - Affinity routing (GICv3)
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1LOAD
     - 0x000
     - 32
     - R/W
     - 0x0
     - Load value; reloaded on wrap
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1VALUE
     - 0x004
     - 32
     - R
     - —
     - Current counter value
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1CTRL
     - 0x008
     - 32
     - R/W
     - 0x20
     - [7]=En [6]=Periodic [5]=IntEn [3]=Size [2:1]=Pre [0]=Wrap
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1INTCLR
     - 0x00C
     - 32
     - W
     - —
     - Interrupt clear (write any)
     - cpu_hw
   * - SP804 Timer @ 0xFC010000
     - TIMER1MIS
     - 0x014
     - 32
     - R
     - 0x0
     - Masked interrupt status
     - cpu_hw
   * - USB3 xHCI @ 0xFC100000
     - CAPLENGTH
     - 0x000
     - 8
     - R
     - —
     - Capability regs length
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - HCIVERSION
     - 0x002
     - 16
     - R
     - —
     - HCI version (0x0110=USB 3.1)
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - HCSPARAMS1
     - 0x004
     - 32
     - R
     - —
     - [31:24]=MaxPorts [16:8]=MaxIntrs [7:0]=MaxSlots
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - HCSPARAMS2
     - 0x008
     - 32
     - R
     - —
     - [7:4]=IST [11:8]=ERST Max
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - HCCPARAMS1
     - 0x010
     - 32
     - R
     - —
     - [0]=AC64 [1]=BNC [2]=CSZ [3]=PPC
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - USBCMD
     - 0x020
     - 32
     - R/W
     - 0x0
     - [0]=RS [1]=HCRST [2]=INTE [3]=HSEE
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - USBSTS
     - 0x024
     - 32
     - R/W
     - 0x0
     - [0]=HCH [2]=HSE [3]=EINT [4]=PCD
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - CRCR
     - 0x038
     - 64
     - R/W
     - 0x0
     - Command Ring Control (DMA addr + RCS)
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - DCBAAP
     - 0x050
     - 64
     - R/W
     - 0x0
     - Device Context Base Addr Array Ptr
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - CONFIG
     - 0x058
     - 32
     - R/W
     - 0x0
     - [7:0]=MaxSlotsEn
     - usb_hw
   * - USB3 xHCI @ 0xFC100000
     - PORTSC(n)
     - 0x420+n*0x10
     - 32
     - R/W
     - —
     - Port n status/ctrl; CCS/PED/PR/PLS/PP/PIC/LWS
     - usb_hw
   * - Crypto @ 0xFC200000
     - CTRL
     - 0x000
     - 32
     - R/W
     - 0x0
     - [3:0]=ALG_SEL: 1=RSA2048,2=RSA4096,3=ECDSA,4=ED25519; [4]=START; [5]=INT_EN
     - ksign_img
   * - Crypto @ 0xFC200000
     - STATUS
     - 0x004
     - 32
     - R
     - 0x0
     - [0]=BUSY [1]=DONE [2]=ERR
     - ksign_img
   * - Crypto @ 0xFC200000
     - SHA_IN
     - 0x010
     - 32
     - W
     - —
     - SHA-256/384 input data FIFO
     - ksign_img
   * - Crypto @ 0xFC200000
     - SHA_LEN
     - 0x014
     - 32
     - R/W
     - 0x0
     - Message byte length for final SHA block
     - ksign_img
   * - Crypto @ 0xFC200000
     - SHA_DIGEST
     - 0x020
     - 32
     - R
     - —
     - SHA digest output [0..7]=SHA-256; [0..11]=SHA-384
     - ksign_img
   * - Crypto @ 0xFC200000
     - RSA_MOD
     - 0x100
     - 32
     - W
     - —
     - RSA modulus input (256B/512B for 2048/4096)
     - ksign_img
   * - Crypto @ 0xFC200000
     - RSA_EXP
     - 0x200
     - 32
     - W
     - —
     - RSA exponent input
     - ksign_img
   * - Crypto @ 0xFC200000
     - RSA_SIG
     - 0x300
     - 32
     - W
     - —
     - RSA signature input
     - ksign_img
   * - Crypto @ 0xFC200000
     - RSA_OUT
     - 0x400
     - 32
     - R
     - —
     - RSA result (PKCS#1 v1.5 padded plaintext)
     - ksign_img
   * - Crypto @ 0xFC200000
     - EC_CURVE
     - 0x500
     - 32
     - R/W
     - 0x3
     - 0=none 3=P-256 4=Ed25519
     - ksign_img
   * - CLINT @ 0x02000000
     - msip[0]
     - 0x0000
     - 32
     - R/W
     - 0x0
     - hart0 SW IRQ; write 1=pend MIP.MSIP
     - cpu_hw
   * - CLINT @ 0x02000000
     - msip[1]
     - 0x0004
     - 32
     - R/W
     - 0x0
     - hart1 SW IRQ
     - cpu_hw
   * - CLINT @ 0x02000000
     - mtimecmp[0]
     - 0x4000
     - 64
     - R/W
     - 0xFFFFFFFFFFFFFFFF
     - hart0 timer compare
     - cpu_hw
   * - CLINT @ 0x02000000
     - mtimecmp[1]
     - 0x4008
     - 64
     - R/W
     - 0xFFFFFFFFFFFFFFFF
     - hart1 timer compare
     - cpu_hw
   * - CLINT @ 0x02000000
     - mtime
     - 0xBFF8
     - 64
     - R/W
     - 0x0
     - Global monotonic timer
     - cpu_hw
   * - OTP @ 0xFC300000
     - CTRL
     - 0x000
     - 32
     - R/W
     - 0x0
     - [0]=PROG_EN [1]=READ_EN [2]=LOCK
     - ksign_key
   * - OTP @ 0xFC300000
     - ADDR
     - 0x004
     - 32
     - R/W
     - 0x0
     - Word address (byte addr / 4)
     - ksign_key
   * - OTP @ 0xFC300000
     - WDATA
     - 0x008
     - 32
     - W
     - —
     - Write data (one 32-bit word)
     - ksign_key
   * - OTP @ 0xFC300000
     - RDATA
     - 0x00C
     - 32
     - R
     - —
     - Read data
     - ksign_key
   * - OTP @ 0xFC300000
     - STATUS
     - 0x010
     - 32
     - R
     - 0x0
     - [0]=BUSY [1]=FAIL [2]=LOCKED
     - ksign_key
   * - OTP @ 0xFC300000
     - ROOT_KEY_BASE
     - 0x100
     - —
     - R
     - —
     - ksign Root CA pubkey (256 B = 64 words)
     - ksign_key
