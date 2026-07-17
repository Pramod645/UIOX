90_uBoot/
├── include/
│   ├── uiox_boot_types.h
│   ├── uiox_boot_hw.h
│   ├── uiox_boot_mem.h
│   ├── uiox_boot_console.h
│   ├── uiox_boot_fs.h
│   ├── uiox_boot_verify.h
│   ├── uiox_boot_handoff.h
│   └── uiox_boot.h
├── src/
│   ├── arch/
│   │   ├── arm64/
│   │   │   ├── uiox_boot_entry_arm64.S
│   │   │   └── uiox_boot_hw_arm64.c
│   │   ├── arm32/
│   │   │   ├── uiox_boot_entry_arm32.S
│   │   │   └── uiox_boot_hw_arm32.c
│   │   └── x86_64/
│   │       ├── uiox_boot_entry_x86.S
│   │       └── uiox_boot_hw_x86.c
│   ├── uiox_boot_mem.c
│   ├── uiox_boot_console.c
│   ├── uiox_boot_fs.c
│   ├── uiox_boot_verify.c
│   ├── uiox_boot_handoff.c
│   └── uiox_boot_main.c
├── linker/
│   ├── uiox_boot_arm64.ld
│   ├── uiox_boot_arm32.ld
│   └── uiox_boot_x86_64.ld
└── Makefile

==================================================================================
Layer	File	Purpose
Types	uiox_boot_types.h	Base integer types, magic numbers, error codes, arch enum
HW HAL	uiox_boot_hw.h + arch hw.c × 3	Ops vtable, MMIO helpers, GIC/PIC init, cache ops, reset
Memory	uiox_boot_mem.h/c	DTB/ATAG/E820 probe, region table, bump allocator
Console	uiox_boot_console.h/c	PL011 / 16550 / SiFive UART, uboot_printf
Storage	uiox_boot_fs.h/c	FAT32 BPB parse, cluster walk, file load by 8.3 name
Verify	uiox_boot_verify.h/c	Full RFC 6234 SHA-256, UIOX image header check
Handoff	uiox_boot_handoff.h/c	ELF64 segment loader, boot args struct, arch-specific kernel jump
Master	uiox_boot.h	Single include for all bootloader APIs
Entry	entry_arm64.S	EL2→EL1 drop, SCTLR disable, TLB flush, BSS zero, C call
Entry	entry_arm32.S	SVC mode, SCTLR, TLB flush, BSS zero, C call
Entry	entry_x86.S	Multiboot2 header, 32→64-bit mode, page tables, C call
Main	uiox_boot_main.c	7-stage pipeline: memory → storage → load → verify → ELF → args → jump
Linker	*.ld × 3	Per-arch memory layout (ARM64@0x40000000, ARM32@0x100000, x86@0x100000)
Build	Makefile	make all builds all 3; make ARCH=arm64/arm32/x86_64 for single
====================================================================================================

===============================================
Quick start:
# Install toolchains (Ubuntu/Debian)
sudo apt install gcc-aarch64-linux-gnu gcc-arm-linux-gnueabihf \
                 gcc binutils qemu-system-arm qemu-system-x86

# Build all three
cd 00_Boot
make all

# Run each in QEMU
make run_arm64   # PL011 UART output on terminal
make run_arm32
make run_x86
==========================================================
============================================
  UIOX Bootloader v1.0  (ARM64/ARM32/x86)
  [github.com](https://github.com/Pramod645/UIOX)
============================================
[BOOT] Stage 2: Memory
  Memory map (1 regions):
    base=0000000040000000  size=0000000004000000  USABLE
  Usable: 64 MB
  OK
[BOOT] Stage 3: Storage
  No storage — simulation mode
[BOOT] Stage 4: Load kernel
  No kernel file — QEMU simulation handoff
[BOOT] Stage 6: ELF
  Flat binary load
  Entry: 0000000040080000
[BOOT] Stage 7: Handoff
  args@0000000040070000
  entry=0000000040080000
  dtb=0000000040000000
  cmd: root=/dev/mmcblk0p2 rw quiet
[BOOT] Jumping to kernel...
=========================
The error has nothing to do with your C code. It is a macOS make recursion problem. The all target in the Makefile calls $(MAKE) three times in sequence, but macOS ships an ancient make (from Xcode) that hits a process limit when you call make from inside make without proper flags.

Here are all the fixes:

Root Cause:
# OLD — broken on macOS: recursive $(MAKE) hits process limit
all:
	$(MAKE) ARCH=arm64
	$(MAKE) ARCH=arm32
	$(MAKE) ARCH=x86_64
  macOS make (BSD make from Xcode) does not handle recursive $(MAKE) the same way GNU make does. It runs out of child process slots.
=======
Fix 1 — Install GNU Make (recommended)
brew install make

# Then use gmake instead of make
gmake all
gmake ARCH=arm64

Or add to your ~/.zshrc / ~/.bashrc:
export PATH="$(brew --prefix make)/libexec/gnubin:$PATH"
Then make will call GNU make automatically. added MAkefile heer


Fix 3 — macOS cross-compiler names differ
On macOS with Homebrew the compiler names are different from Linux:
# Install correct toolchains on macOS
brew install make
brew install aarch64-elf-gcc          # for ARM64
brew install arm-none-eabi-gcc        # for ARM32
brew install x86_64-elf-gcc           # for x86-64 bare-metal
brew install qemu

Then override the cross prefix when building:
# ARM64 on macOS
make ARCH=arm64 CROSS=aarch64-elf-

# ARM32 on macOS
make ARCH=arm32 CROSS=arm-none-eabi-

# x86-64 on macOS (native gcc works)
make ARCH=x86_64 CROSS=

To avoid typing this every time, add a macOS detect block at the top of the Makefile:
# -- Auto-detect macOS and use Homebrew toolchain names ------
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ifeq ($(ARCH),arm64)
    CROSS = aarch64-elf-
  else ifeq ($(ARCH),arm32)
    CROSS = arm-none-eabi-
  else
    CROSS =
  endif
endif
===========
Fix 4 — use GNU make from Homebrew
Add this line to ~/.zshrc (macOS default shell):
# Use GNU make instead of Xcode BSD make
export PATH="$(brew --prefix make)/libexec/gnubin:$PATH"
Then reload and build:
source ~/.zshrc
make --version      # should show GNU Make 4.x
make ARCH=arm64
make all_arches

=======
Error	Cause	Fix
Resource temporarily unavailable	    macOS BSD make hits fork limit with recursive $(MAKE)	Replace         recursive $(MAKE) with shell for loop in all_arches
wait: No child processes	            Same root cause — BSD make process table full	Use GNU make (brew install make)
Error 2	                              Child make failed propagated upward	Fixed by using shell loop — failure exits cleanly
Cross-compiler not found	           macOS uses aarch64-elf-gcc not aarch64-linux-gnu-gcc	Add macOS auto-detect block in Makefile
==========================================================
Expected Console Output
Matching the uBoot.md sample output exactly:

UIOX Bootloader v1.0 (ARM64) [github.com/Pramod645/UIOX]
[BOOT] Stage 1: HW init
OK
[BOOT] Stage 2: Memory
Memory map (1 regions):
  base=0000000040000000 size=0000000004000000 USABLE
Usable: 64 MB
OK
[BOOT] Stage 3: Storage
No storage — simulation mode
[BOOT] Stage 4: Load kernel
  No kernel file — QEMU simulation handoff
[BOOT] Stage 5: Verify
  No kernel file — QEMU simulation handoff
[BOOT] Stage 6: ELF
  Flat binary load
  Entry: 0000000040080000
OK
[BOOT] Stage 7: Handoff
args@0000000040070000
entry=0000000040080000
dtb=0000000040000000
cmd: root=/dev/mmcblk0p2 rw quiet console=ttyAMA0
[BOOT] Jumping to kernel...
==========================================================
File	                            Layer	              Role
uiox_boot_types.h	                Types	              Integer types, magic numbers, error codes, uiox_image_hdr_t, uiox_boot_args_t
uiox_boot_hw.h/.c + arch hw_*.c	  HW HAL	            8-op vtable, MMIO helpers, PL011/16550, GIC/8259A, cache, timer
uiox_boot_mem.h/.c	              Memory	            DTB FDT parser, E820 fallback, bump allocator, memset/memcpy
uiox_boot_console.h/.c	          Console	            UART putc/puts, minimal printf (%s %d %u %x %llx %p)
uiox_boot_fs.h/.c	                Storage	            FAT32 BPB mount, cluster chain walk, 8.3 file load
uiox_boot_verify.h/.c	            Verify	            Full RFC 6234 SHA-256, UIOX image header magic + arch + hash check
uiox_boot_handoff.h/.c	          Handoff	            ELF64 PT_LOAD mapper, flat binary loader, uiox_boot_args_t builder, arch jump
entry_arm64.S	                    ARM64 Entry	        EL2→EL1 drop, SCTLR disable, BSS zero, stack, C call
entry_arm32.S	                    ARM32 Entry	        SVC mode, SCTLR, TLB flush, BSS zero, C call
entry_x86.S	                      x86 Entry	          Multiboot2 header, 32→64-bit page tables, long mode, C call
uiox_boot_main.c	                7-Stage Pipeline	  Orchestrates all stages; simulation-mode fallback when no storage
linker/*.ld	                      Linker Scripts	    ARM64@0x40000000, ARM32@0x00000000/0x00100000, x86@0x00100000
Makefile	                        Build	              make all / make ARCH=arm64 / make run ARCH=arm64
==============================================================
Added RISCV in version 2:

├── src/arch/riscv64/
│   ├── uioxbootentryriscv64.S     ← NEW  (4.5 KB)
│   └── uioxboothwriscv64.c        ← NEW  (9.3 KB)
├── linker/
│   └── uioxbootriscv64.ld          ← NEW  (2.7 KB)
├── include/
│   ├── uioxboottypes.h             ← MERGED (v1.1.0)
│   └── uioxboothw.h                ← MERGED (v1.1.0)
├── Makefile                           ← MERGED (all 4 archs)

`

New files — what each does
uioxbootentryriscv64.S
Mirrors uioxbootentryarm64.S step-for-step:

| Step | RISC-V | ARM64 equivalent |
|---|---|---|
| 1 | csrr t0, mhartid → park hart ≠ 0 | Check CurrentEL, drop EL2→EL1 |
| 2 | csrci mstatus, MIE + csrwi mie, 0 | Disable caches via SCTLREL1 |
| 3 | la sp, bootstacktop | adrp x10, bootstacktop |
| 4 | BSS zero with sd zero 8-byte loop | BSS zero with str xzr 8-byte loop |
| 5 | call uioxboothwriscv64register | GIC init + uioxboothwregister |
| 6 | mv a0, s0; call uioxbootmain | mov x0, x20; bl uioxbootmain |
| 7 | wfi; j .Lhang | wfe; b .Lhang |

Stack: 8 KiB in .bss.stack, 16-byte aligned (RV64 ABI requirement).

uioxboothwriscv64.c
Implements all 8 uioxboothwopst fields, matching the pattern of uioxboothwarm64.c and uioxboothwx86.c:

| Op | Implementation | Source cross-ref |
|---|---|---|
| init | NS16550A @ 0x10000000, 115200 8N1, divisor=2 | DTB uart0, mirrors com1init() |
| uartputc | Poll LSRTHRE, write THR | Mirrors com1putc() |
| getticks | (volatile uint64t)0x0200BFF8 — CLINT mtime | uioxcpuhw.h clintbase |
| udelay | CLINT tick-based, 10 MHz rate | Mirrors arm32udelay() |
| dcacheflush | fence rw, rw | RISC-V base ISA (no explicit cache MMIO) |
| icacheinv | fence.i | Required after ksign image load |
| barrier | fence rw, rw | Mirrors arm64barrier() DSB |
| reset | Write 0x7777 to test finisher 0x100000 | QEMU virt; mirrors arm32reset() |

plicinit() zeros all 64 source priorities and sets S-mode threshold to 0 before registration.

linker/uioxbootriscv64.ld
Mirrors uioxbootarm64.ld section layout exactly:

| Symbol | Address | Notes |
|---|---|---|
| bootloadbase | 0x80200000 | OpenSBI FWJUMP S-mode target |
| kernloadbase | 0x80280000 | 512 KiB gap (same as ARM64) |
| argsbase | 0x80270000 | uioxbootargst |
| DRAM region | 64 MiB from 0x80200000 | OUTPUTARCH(riscv) |

Includes RISC-V-specific .sdata / .sbss (GP-relative small data) sections.

Merged files — exactly what changed
include/uioxboottypes.h (v1.0.0 → v1.1.0)
• UIOXARCHRV64 = 3 added to uioxarcht enum
• UIOXRVUARTBASE, UIOXRVCLINTBASE/MTIME/MTIMECMP0, UIOXRVPLICBASE, UIOXRVTESTBASE constants added alongside existing ARM/x86 defines

include/uioxboothw.h (v1.0.0 → v1.1.0)
• NS16550 register offset defines added (parallel to existing COM1 and PL011*)
• void uioxboothwriscv64register(void) prototype added alongside existing arm64register, arm32register, x86register

Makefile (328 → ~420 lines)
Every existing section extended with a parallel RISC-V block:
• Toolchain: RISCVCC := riscv64-elf-gcc, RISCVOBJCOPY/OBJDUMP
• Flags: -march=rv64gc -mabi=lp64d -mcmodel=medany -mno-relax -Driscv -Driscvxlen=64
• Build rules: .S + .c compile, link, objcopy → .bin
• $(BUILDDIR)/riscv64/arch/riscv64 mkdir rule
• -include $(RISCVOBJS:.o=.d) dependency tracking
• run ARCH=riscv64 → qemu-system-riscv64 -machine virt -bios default -kernel $(RISCV_ELF)
• all target now builds all four; help updated

Build & run

`bash
Install toolchain
brew install riscv64-elf-gcc qemu          # macOS
sudo apt install gcc-riscv64-linux-gnu \
                 qemu-system-misc          # Ubuntu/Debian

RISC-V only
make riscv64

All four architectures
make all

Run in QEMU (OpenSBI auto-loaded via -bios default)
make run ARCH=riscv64
``
======================================
╔══════════════════════════════════════════════════════════════════════════╗
║                    UIOX COMPLETE BOOT EXECUTION FLOW                    ║
║           Power-On → SoC Init → Arch Init → Peripheral Init             ║
║                    → Kernel Entry → Normal Execution                    ║
╚══════════════════════════════════════════════════════════════════════════╝

Power-On Reset
      │  CPU fetches reset vector from ROM
      │  ARM64: 0x00000000  ARM32: 0x00000000  x86: 0xFFFFFFF0  RISC-V: 0x1000
      │
      ▼
╔══════════════════════════════════════════════════════════════════════════╗
║  BOOTLOADER  (01_uBoot/src/uiox_boot_main.c)                            ║
║  Entry: uiox_boot_hw_register() → uiox_boot_main()                     ║
╚══════════════════════════════════════════════════════════════════════════╝
      │
      │ ┌─────────────────────────────────────────────────────────────┐
      │ │ STAGE 1 — uiox_boot_main.c  Stage 1: "HW init"             │
      ▼ └─────────────────────────────────────────────────────────────┘
      │
      ▼
① SoC Hardware Init  ─────────────────────────── [BOOTLOADER — Stage 1/2]
      │  "Power on the silicon — chip-wide, must be first"
      │  File: 02_FwHal/src/uiox_soc_arm64.c  (or arm32 / x86 / riscv64)
      │
      ├─ uiox_soc_init()            detect SoC from MIDR_EL1/CPUID/mvendorid
      │       └─▶ uiox_soc_init_arm64()   populate uiox_soc_desc_t
      │
      ├─ uiox_pm_init()             power domains ON
      │       └─▶ uiox_pm_domain_on(CPU0)    CPU power rail
      │       └─▶ uiox_pm_domain_on(DRAM)    DDR cells now have power
      │       └─▶ uiox_pm_domain_on(PERIPH)  peripheral bus powered
      │
      ├─ uiox_clk_init()            PLL → full CPU speed
      │       └─▶ uiox_clk_enable(UIOX_CLK_CPU)    1.5 GHz / 1.0 GHz
      │       └─▶ uiox_clk_enable(UIOX_CLK_BUS)    AXI/AHB bus clock
      │       └─▶ uiox_clk_enable(UIOX_CLK_UART0)  UART ref clock 24 MHz
      │
      ├─ DDR PHY training           RAM now usable for code/data
      │
      ├─ uiox_rst_deassert(PERIPH)  peripheral registers now accessible
      │
      └─ GIC/APIC/PLIC distributor ON   interrupt fabric ready
              ARM64:  mmio_write32(GIC_DIST_CTLR, 0x1u)
              x86:    LAPIC MSR enable
              RISC-V: PLIC threshold = 0
      │
      ▼
      │ ┌─────────────────────────────────────────────────────────────┐
      │ │ STAGE 1 continued — arch HW register init                  │
      └─┤ File: 10_Arch/<arch>/src/arch_init.c                        │
        └─────────────────────────────────────────────────────────────┘
      │
      ▼
② Architecture Init  ─────────────────────────── [BOOTLOADER — minimal]
      │  "Prepare the CPU core — privilege, stack, vectors"
      │  File: 10_Arch/arm64/src/arch_init.c  (arch_init called by BL)
      │
      ├─ arch_irq_disable()         mask all interrupts during init
      │       ARM64:  msr daifset, #0xF
      │       ARM32:  CPSR I+F bits set
      │       x86:    CLI
      │       RISC-V: csrc sstatus, SIE
      │
      ├─ Set privilege level
      │       ARM64:  already at EL1 (EL3→EL2→EL1 drop by secure world)
      │       ARM32:  CPSR mode bits = SVC (0b10011)
      │       x86:    Protected mode → Long mode (CR0.PE + EFER.LME)
      │       RISC-V: S-mode (after OpenSBI M-mode handoff)
      │
      ├─ Configure stack pointer
      │       ARM64:  MSP = _stack_top (16-byte aligned)
      │       ARM32:  SP  = _stack_top (8-byte aligned)
      │       x86:    RSP = _stack_top (16-byte aligned)
      │       RISC-V: sp  = _stack_top (16-byte aligned)
      │
      ├─ Install minimal exception vectors  [bootloader only]
      │       ARM64:  MSR VBAR_EL1, <bl_vector_table>
      │       ARM32:  MCR p15,0,r0,c12,c0,0 (VBAR)
      │       x86:    LIDT bl_idt
      │       RISC-V: CSR stvec = bl_trap_handler
      │
      └─ arch_irq_enable()          re-enable interrupts
              ARM64:  msr daifclr, #0xF
              ARM32:  CPSR I bit clear
              x86:    STI
              RISC-V: csrs sstatus, SIE
      │
      ▼
      │ ┌─────────────────────────────────────────────────────────────┐
      │ │ STAGE 3 — uiox_boot_main.c  Stage 3: "Storage"             │
      └─┤ STAGE 4 — Stage 4: "Load kernel"                           │
        └─────────────────────────────────────────────────────────────┘
      │
      ▼
③ Peripheral Init  ────────────────────────────── [BOOTLOADER — Stage 1–4]
      │  "Configure individual devices needed to load the kernel"
      │  Files: 02_FwHal/src/uiox_fw_uart.c, uiox_fw_sd.c, etc.
      │
      ├─ uiox_fw_uart_init()        PL011/16550/NS16550A debug console
      │       ARM64:  base=0x09000000, 115200 8N1, IRQ 33
      │       ARM32:  base=0x10009000, 115200 8N1, IRQ 44
      │       x86:    port=0x3F8,    115200 8N1, IRQ 4
      │       RISC-V: base=0x10000000,115200 8N1, IRQ 10
      │
      ├─ uiox_fw_timer_init()       SP804/PIT/CLINT — watchdog kick
      │
      ├─ uiox_fw_sd_init()          SD/eMMC — read kernel image
      │   OR uiox_fw_nvme_init()    NVMe — read kernel from SSD
      │
      ├─ uiox_boot_mem_probe()      DDR layout → uiox_mem_map_t
      │       ARM64:  reads DTB /memory node
      │       x86:    reads E820 map
      │       RISC-V: reads DTB /memory node
      │
      ├─ uiox_boot_fs_init()        FAT32 BPB parse → uiox_fat32_ctx_t
      │
      ├─ uiox_boot_fs_load()        read uiox_kernel.img into RAM
      │
      ├─ uiox_fw_secboot_verify()   SHA-256 + signature check
      │       File: 02_FwHal/src/uiox_fw_secboot.c
      │       └─▶ uiox_sha256()     hash kernel payload
      │       └─▶ verify signature  RSA/ECDSA against RoT key
      │       └─▶ anti-rollback     version ≥ OTP minimum
      │
      └─ uiox_boot_elf64_load()     copy ELF segments to load address
              └─▶ maps PT_LOAD → physical RAM
              └─▶ zeros BSS tail (memsz > filesz)
              └─▶ sets entry_pa from e_entry
      │
      ▼
      │ ┌─────────────────────────────────────────────────────────────┐
      │ │ STAGE 7 — uiox_boot_main.c  Stage 7: "Handoff"             │
      └─┤ uiox_boot_handoff(entry, dtb_pa, args_pa)                   │
        │ uiox_boot_arch_jump(entry, dtb_pa, args_pa)                 │
        └─────────────────────────────────────────────────────────────┘
      │
      │  ARM64:  mov x0,dtb_pa  mov x1,args_pa  DSB ISB  BR x4
      │  ARM32:  mov r2,dtb_pa  mov r3,args_pa  DSB ISB  BX r9
      │  x86:    mov rdi,args  mov rsi,dtb     MFENCE   JMP *rax
      │  RISC-V: mv a0,dtb_pa  mv a1,args_pa  fence    JR t0
      │
      ║  ══════════════════════════════════════════════════════════  ║
      ║   BOOTLOADER ENDS HERE — one-way jump — never returns        ║
      ║  ══════════════════════════════════════════════════════════  ║
      │
      ▼
╔══════════════════════════════════════════════════════════════════════════╗
║  KERNEL ENTRY POINT                                                     ║
║  Function: uiox_kernel_main()                                           ║
║  File:     33_ProcessControlSubsystem/uiox_kernel_main.c               ║
║  Address:  entry_pa (from ELF e_entry / uiox_image_hdr_t.entry_point)  ║
╚══════════════════════════════════════════════════════════════════════════╝
      │
      │  On entry (registers set by uiox_boot_arch_jump):
      │    ARM64:  x0=dtb_pa  x1=args_pa  x2=x3=0
      │    ARM32:  r2=dtb_pa  r3=args_pa  r0=r1=0
      │    x86:    rdi=args_pa  rsi=dtb_pa
      │    RISC-V: a0=dtb_pa   a1=args_pa
      │
      ├─ capture x0/x1 (a0/a1/rdi/rsi) into local variables
      │       BEFORE any C code can corrupt them
      │
      ├─ stack_setup()              set SP to _stack_top
      │
      ├─ bss_zero()                 clear _bss_start → _bss_end
      │
      ├─ g_dtb_pa    = dtb_pa       save for all subsystems
      ├─ g_boot_args = args_pa      save uiox_boot_args_t pointer
      │
      └─ early_puts()               UART output — MMU still OFF
              "[kernel] UIOX kernel entry (ARM64)"
      │
      ▼
④ Kernel arch_init()  ────────────────────────── [KERNEL — full CPU init]
      │  "Re-init CPU at OS level — MMU on, caches on, full IRQ routing"
      │  File: 10_Arch/arm64/src/arch_init.c  → arch_init()
      │
      ├─ uiox_soc_init()            full SoC reinit with MMU context
      │       └─▶ uiox_
=======================================
Rule of Thumb — 3 Questions to Decide
Ask these three questions for any init function:


1. Does the hardware need to work BEFORE the kernel is loaded?
   YES → Bootloader calls it
   NO  → Kernel calls it

2. Is it needed to FIND, LOAD, or VERIFY the kernel image?
   YES → Bootloader calls it  (storage, memory, secure boot, UART)
   NO  → Kernel calls it

3. Is it the full driver or a minimal bring-up stub?
   Minimal bring-up → Bootloader
   Full driver      → Kernel
==================================================
Power-On Reset
      │
      ▼
01_uBoot/src/uiox_boot_main.c
      │
      ├─ Stage 1: uiox_boot_hw_register()          ← register arch HAL vtable
      │               │
      │               └─ arch-specific:
      │                    ARM64:   GIC-400 init, PL011 UART, generic timer
      │                    ARM32:   GIC init, SP804 timer, PL011 UART
      │                    x86-64:  LAPIC, COM1 16550A, HPET/PIT
      │                    RISC-V:  CLINT, PLIC, NS16550A UART
      │
      ├─ Stage 2: uiox_soc_init()                  ← 02_FwHal SoC detect
      │               │
      │               ├─ uiox_soc_init_arm64()      ← identify SoC (MIDR_EL1)
      │               ├─ uiox_clk_init()            ← clock tree / PLL
      │               └─ uiox_pm_init()             ← power domains
      │
      ├─ Stage 2: uiox_boot_mem_probe()            ← DDR training / E820
      │
      ├─ Stage 3: uiox_boot_fs_init()              ← FAT32 / storage
      │
      ├─ Stage 4: uiox_boot_fs_load()              ← read kernel image
      │
      ├─ Stage 5: uiox_fw_secboot_verify()         ← 02_FwHal SHA-256 verify
      │
      ├─ Stage 6: uiox_boot_elf64_load()           ← copy ELF to RAM
      │
      └─ Stage 7: uiox_boot_arch_jump(entry, dtb, args)   ← JUMP TO KERNEL
============================
Great question — this is one of the most important distinctions in embedded systems firmware design. Here is the complete explanation.

SoC Hardware Init vs Peripheral Init
The One-Line Difference

| | What it is | Analogy |
|---|---|---|
| SoC Hardware Init | Bringing the silicon itself to life — clocks, power, reset, memory bus, interrupt fabric | Building the road and power grid |
| Peripheral Init | Configuring an attached device that uses the working silicon | Plugging in and setting up appliances |

1 · SoC Hardware Init

SoC hardware init is everything needed to make the chip itself functional. Without it, nothing else can run — not even a UART print.

What It Covers

``
┌─────────────────────────────────────────────────┐
│              SoC Die (single chip)              │
│                                                 │
│  ┌──────────┐   ┌──────────┐   ┌─────────────┐ │
│  │  CPU     │   │  PLL /   │   │  DDR        │ │
│  │  Cores   │   │  Clock   │   │  Controller │ │  ← SoC Init
│  │ (ARM64)  │   │  Tree    │   │  (DRAM PHY) │ │
│  └──────────┘   └──────────┘   └─────────────┘ │
│                                                 │
│  ┌──────────┐   ┌──────────┐   ┌─────────────┐ │
│  │  GIC /   │   │  Power   │   │  System Bus │ │
│  │  APIC /  │   │  Domains │   │  (AXI/AHB)  │ │  ← SoC Init
│  │  PLIC    │   │  (PMIC)  │   │  Interconnect│ │
│  └──────────┘   └──────────┘   └─────────────┘ │
│                                                 │
│  ┌──────────┐   ┌──────────┐   ┌─────────────┐ │
│  │  UART0   │   │  SPI     │   │  I2C        │ │
│  │  (PL011) │   │  (PL022) │   │  (DW-APB)   │ │  ← Peripheral Init
│  └──────────┘   └──────────┘   └─────────────┘ │
└─────────────────────────────────────────────────┘
`

Characteristics of SoC Hardware Init

| Property | Detail |
|---|---|
| Must happen first | Before any code can execute from RAM |
| Cannot be deferred | DDR must work before you can load anything |
| Affects the whole chip | A clock change affects every peripheral simultaneously |
| One-time setup | Done once at power-on; wrong config = nothing works |
| No driver needed | Done via direct register writes, not a driver stack |
| Examples in UIOX | uioxclkinit(), uioxpminit(), GIC distributor enable, DDR PHY training |

Examples

`c
/ ── CLOCK / PLL ─────────────────────────────── SoC Hardware Init ── /
// Set CPU PLL to 1.5 GHz, bus clock to 400 MHz
// Without this the CPU runs at the reset default (often 24 MHz)
uioxclkinit(&clkctx, socdesc);

/ ── POWER DOMAINS ───────────────────────────── SoC Hardware Init ── /
// Enable CPU, DRAM, peripheral power rails
// Without this DRAM cells have no power — reads return garbage
uioxpmdomainon(&pmctx, UIOXPDCPU0);
uioxpmdomainon(&pmctx, UIOXPDDRAM);

/ ── INTERRUPT CONTROLLER DISTRIBUTOR ────────── SoC Hardware Init ── /
// Enable GIC distributor — without this NO interrupts ever fire
mmiowrite32(GICDISTCTLR, 0x1u);

/ ── RESET CONTROLLER ────────────────────────── SoC Hardware Init ── /
// Deassert reset on peripheral subsystem
uioxrstdeassert(&pmctx, UIOXRSTPERIPH);
`

2 · Peripheral Init

Peripheral init configures an individual device — either a block inside the SoC or an external chip — after the SoC itself is working.

Characteristics of Peripheral Init

| Property | Detail |
|---|---|
| Requires working SoC | Needs clocks, power, and bus before it can be configured |
| Can be deferred | You can skip UART init and the CPU still runs |
| Device-specific | Different registers, different baud rates, different IRQs |
| Can be re-initialised | Driver can init, deinit, and re-init a peripheral at runtime |
| Uses a driver/HAL | Goes through uioxfwuartinit(), uioxfwspiinit() etc. |
| Examples in UIOX | uioxfwuartinit(), uioxfwspiinit(), uioxfwi2cinit(), uioxfwgpioinit() |

Examples

`c
/ ── UART ────────────────────────────────────── Peripheral Init ── /
// Configure PL011 for 115200 8N1
// This only works because:
//   - UART clock was enabled in SoC init
//   - UART power domain is on
//   - UART reset was deasserted
uioxfwuartinit(&uart, 0x09000000UL, true, 33, &UIOXFWUARTCFGDEFAULT);

/ ── SPI ─────────────────────────────────────── Peripheral Init ── /
// Configure PL022 SSP for Mode 0, 8-bit, 10 MHz
uioxfwspiinitpl022(&spi, 0x09050000UL, 24000000u, 35);

/ ── I2C ─────────────────────────────────────── Peripheral Init ── /
// Configure DesignWare I2C at 400 kHz Fast mode
uioxfwi2cinitdw(&i2c, 0x09040000UL, 24000000u, 36, UIOXI2CSPEEDFAST);

/ ── GPIO ────────────────────────────────────── Peripheral Init ── /
// Set pin 14 as output, pin 17 as interrupt input
uioxfwgpioinit(&gpio, 0x09030000UL, 52u, 128u);
uioxfwgpiosetdir(&gpio, 14, UIOXFWGPIOOUT);
uioxfwgpioirqen(&gpio, 17, UIOXFWGPIOIRQFALLING, mycb, NULL);
`

3 · Side-by-Side Comparison

| Dimension | SoC Hardware Init | Peripheral Init |
|---|---|---|
| Scope | Entire chip — clocks, power, buses, reset fabric, interrupt controller | Single device — UART, SPI, I2C, GPIO, USB, sensor |
| Dependency | No dependency — must come first | Depends on SoC init being complete |
| Who calls it | Bootloader Stage 1–2 | Bootloader Stage 1 (minimal) + Kernel (full) |
| If skipped | System hangs — nothing works | That specific device doesn't work; others unaffected |
| Can be deferred? | No — DDR must work before loading kernel | Yes — USB can init after shell is visible |
| Reversible? | No — you cannot un-init the clock tree at runtime | Yes — drivers can deinit and reinit |
| Done via | Direct MMIO register writes, CSR writes | Driver HAL (uioxfwuartinit etc.) |
| Examples | PLL, DDR PHY, GIC distributor, PMIC, reset deassert | UART baud rate, SPI clock polarity, I2C speed, GPIO direction |
| UIOX functions | uioxsocinit(), uioxclkinit(), uioxpminit(), uioxrstdeassert() | uioxfwuartinit(), uioxfwspiinit(), uioxfwi2cinit(), uioxfwgpioinit() |
| UIOX files | 02FwHal/src/uioxsoc.c, 02FwHal/include/uioxsocclk.h | 02FwHal/src/uioxfwuart.c, uioxfwspi.c, etc. |

4 · Dependency Chain — Why Order Matters

`
Power-On Reset
      │
      ▼
┌─────────────────────────────────────────────────────┐
│  SoC Hardware Init (MUST be in this order)          │
│                                                     │
│  1. Power domains ON     (PMIC / uioxpmdomainon) │
│       ↓ DRAM cells now have power                   │
│  2. PLL / Clock tree     (uioxclkinit)            │
│       ↓ CPU now at full speed, peripherals clocked  │
│  3. DDR PHY training     (uioxddrinit)            │
│       ↓ RAM is now usable                           │
│  4. Reset deassert       (uioxrstdeassert)        │
│       ↓ Peripheral registers now accessible         │
│  5. Interrupt controller (GIC/APIC/PLIC enable)     │
│       ↓ Interrupts can now be registered            │
└─────────────────────────────────────────────────────┘
      │
      ▼ ONLY NOW can peripheral init run
┌─────────────────────────────────────────────────────┐
│  Peripheral Init (order flexible, device-dependent) │
│                                                     │
│  ├─ UART init    → debug output working             │
│  ├─ Timer init   → delays and timeouts available    │
│  ├─ GPIO init    → button/LED control               │
│  ├─ I2C init     → sensor/PMIC communication        │
│  ├─ SPI init     → NOR flash, display               │
│  ├─ SD/eMMC init → storage access                   │
│  └─ USB init     → (often deferred to kernel)       │
└─────────────────────────────────────────────────────┘
      │
      ▼
Kernel entry → full driver stack
`

5 · Real UIOX Code Illustration

Both init types can appear in the same file (archinit.c) but they are conceptually different:

`c
/ 10Arch/arm64/src/archinit.c /

int archinit(void)
{
    / ════════════════════════════════════════════════════
      PART A — SoC Hardware Init
      These affect the chip itself, not just one device.
      ════════════════════════════════════════════════════ /

    / 1. Detect SoC and set up clocks + power domains /
    uioxsocinit();               / ← SoC Hardware Init /

    / 2. Enable GIC-400 distributor (interrupt fabric) /
    mmiowrite32(GICDISTCTLR, 0x0u);          / disable  /
    mmiowrite32(GICDISTISENABLER0, 0xFFFFFFFFu); / SPIs   /
    mmiowrite32(GICCPUCTLR, 0x1u);            / CPU iface /
    mmiowrite32(GICDISTCTLR, 0x1u);           / re-enable /
    / ↑ This is SoC init — GIC is the interrupt fabric for
        ALL devices, not just one peripheral               /

    / ════════════════════════════════════════════════════
      PART B — Peripheral Init
      These configure individual devices.
      ════════════════════════════════════════════════════ /

    / 3. Configure PL011 UART for 115200 8N1 /
    arm64uartinit();             / ← Peripheral Init   /
    irqregister(UART0IRQ, arm64uarthandler, NULL);
    irqenable(UART0IRQ);
    / ↑ Only affects UART0. All other devices unaffected  /

    / 4. Configure generic timer /
    arm64timerinit();            / ← Peripheral Init   /
    irqregister(TIMER0IRQ, arm64timerhandler, NULL);
    irqenable(TIMER0IRQ);

    / 5. Enable global CPU interrupts /
    archirqenable();             / ← SoC Hardware Init /
    / ↑ Affects ALL interrupts on this CPU core           /

    return 0;
}
``

Summary in One Sentence Each

SoC Hardware Init = "Turn the chip on and make the silicon work" — clocks, power, DDR, reset, interrupt fabric. Without it the CPU cannot run, RAM cannot be accessed, and nothing else can start.

Peripheral Init = "Configure one attached device to do its job" — set baud rate, clock polarity, IRQ number, pin direction. Without it that specific device doesn't work, but everything else is unaffected.====
=================================
Great question — these three are often confused because they overlap in timing and location. Here is the complete distinction.

Three Types of Initialization — Defined

| Type | What it initialises | Level | UIOX Location |
|---|---|---|---|
| SoC Hardware Init | The silicon — clocks, power, DDR, reset fabric | Chip-wide | 02FwHal/src/uioxsoc.c |
| Architecture Init | The CPU itself — exception vectors, MMU, cache, privilege levels | CPU-wide | 10Arch/<arch>/src/archinit.c |
| Peripheral Init | One individual device — UART, SPI, I2C, USB, sensor | Device-specific | 02FwHal/src/uioxfwuart.c etc. |

1 · SoC Hardware Init

Definition: Bringing the entire chip's infrastructure to a working state. This is about the silicon fabric — the wiring between CPU, memory, power, and all internal blocks.

What it covers

``
┌─────────────────────────────────────────────────────────────┐
│  SoC Hardware Init owns this layer:                         │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  PLL / CLK  │  │   Power     │  │   DDR Controller    │ │
│  │  Tree       │  │   Domains   │  │   PHY Training      │ │
│  │  (uioxclk) │  │   (uioxpm) │  │   (DDR must work)   │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  Reset      │  │  System Bus │  │   Interrupt Fabric  │ │
│  │  Controller │  │  AXI / AHB  │  │   GIC/APIC/PLIC     │ │
│  │  (deassert) │  │  Interconnect│  │   Distributor       │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
`

Key Properties
• Must happen before anything else — no RAM, no clocks, no interrupts until this runs
• Affects the whole chip simultaneously — a clock change affects every peripheral
• Done via direct MMIO register writes — no driver abstraction needed
• Performed by the bootloader (Stage 1–2)
• Cannot be skipped or deferred

UIOX Functions

`c
uioxsocinit()          / detect SoC, populate descriptor          /
uioxclkinit()          / PLL configuration, clock tree setup       /
uioxpminit()           / power domain enable (CPU, DRAM, periph)   /
uioxrstdeassert()      / deassert peripheral subsystem reset        /
uioxsocarm64init()    / GIC-600 redistributor wake, DSU, cache     /
`

2 · Architecture Init

Definition: Configuring the CPU core itself — its internal state machine, privilege level, exception handling, memory translation, and cache behaviour. This has nothing to do with external chips.

What it covers

`
┌─────────────────────────────────────────────────────────────┐
│  Architecture Init owns this layer:                         │
│                                                             │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  CPU Core Internal State                               │ │
│  │                                                        │ │
│  │  EL/Privilege   SCTLREL1  VBAREL1   DAIF            │ │
│  │  (EL3→EL1)      MMU bit    Exception  IRQ mask         │ │
│  │                            vectors                     │ │
│  │                                                        │ │
│  │  TTBR0/TTBR1    TCREL1    MAIREL1   Cache           │ │
│  │  Page table     VA range   Mem attrs  enable           │ │
│  │  base                                                  │ │
│  │                                                        │ │
│  │  FPU/NEON       CPACREL1  CPTREL3  Stack pointer    │ │
│  │  enable         FP access  FP trap   SPEL1           │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
`

Key Properties
• Configures registers inside the CPU — not MMIO, not external chips
• CPU-specific — ARM64 uses MRS/MSR, ARM32 uses MCR/MRC, x86 uses MOV CR0, RISC-V uses CSR instructions
• Runs twice — once in bootloader (minimal: set EL, stack), once in kernel (archinit() — full MMU, cache, IRQ routing)
• The MMU enable is an arch init operation, not a SoC or peripheral operation
• Affects only the current CPU core — other cores need their own arch init

UIOX Functions

`c
archinit()              / 10Arch/arm64/src/archinit.c             /
arm64gic400init()      / configure GIC CPU interface (per-core)    /
arm64uartinit()        / PL011 config (technically peripheral but  /
                         / called from archinit for convenience)    /
arm64smpenable()       / set CPUECTLREL1.SMPEN                    /
arm64cachetopology()   / read CLIDREL1, detect L1/L2/L3 sizes    /
uioxsocarm64init()    / GIC-600 redistributor, DSU, SMP           /
`

Architecture Init — Per Architecture

`
ARM64:                         ARM32:
  MSR SCTLREL1 (MMU on)        MCR p15,c1 (SCTLR)
  MSR TTBR0EL1 (page table)    MCR p15,c2 (TTBR0)
  MSR VBAREL1  (vectors)       MCR p15,c12 (VBAR)
  MSR DAIF      (IRQ mask)      CPSR I/F bits
  MSR TCREL1   (VA range)      MCR p15,c3 (DACR)
  MSR MAIREL1  (mem attrs)     ACTLR SMP bit

x86-64:                        RISC-V:
  MOV CR0 (PG + PE bits)         CSR satp (Sv39 mode)
  MOV CR3 (PML4 base)            CSR stvec (trap vector)
  MOV CR4 (PAE, PGE, PCID)       CSR sstatus (SIE, SPP)
  LGDT (GDT load)                CSR medeleg / mideleg
  LIDT (IDT load)                CSR mtvec (M-mode trap)
  WRMSR EFER (long mode)         SFENCE.VMA (TLB flush)
`

3 · Peripheral Init

Definition: Configuring one specific device — a block inside the SoC (UART, SPI, I2C, timer) or an external chip (sensor, PMIC, display) — after the SoC hardware and CPU architecture are both working.

What it covers

`
┌─────────────────────────────────────────────────────────────┐
│  Peripheral Init owns this layer:                           │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │  UART0   │  │  SPI     │  │  I2C     │  │  Timer    │  │
│  │  baud    │  │  CPOL    │  │  speed   │  │  period   │  │
│  │  parity  │  │  CPHA    │  │  addr    │  │  callback │  │
│  │  FIFO    │  │  CS pins │  │  IRQ     │  │  IRQ      │  │
│  └──────────┘  └──────────┘  └──────────┘  └───────────┘  │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │  USB     │  │  NVMe    │  │  Wi-Fi   │  │  Sensor   │  │
│  │  xHCI    │  │  queues  │  │  firmware│  │  gain     │  │
│  │  port    │  │  BAR0    │  │  SSID    │  │  itime    │  │
│  │  reset   │  │  opcodes │  │  connect │  │  thresh   │  │
│  └──────────┘  └──────────┘  └──────────┘  └───────────┘  │
└─────────────────────────────────────────────────────────────┘
`

Key Properties
• Depends on both SoC init AND arch init being complete
• Affects only that one device — UART init failure doesn't break SPI
• Goes through the HAL / driver layer — uioxfwuartinit(), uioxfwspiinit() etc.
• Can be deferred — USB can init after the shell is visible
• Can be re-initialised at runtime (deinit → reconfigure → re-init)
• Split across bootloader (minimal, what's needed to load the kernel) and kernel (full driver stack)

UIOX Functions

`c
uioxfwuartinit()      / PL011 / 16550 baud, parity, FIFO         /
uioxfwspiinit()       / PL022 CPOL, CPHA, speed, CS pins         /
uioxfwi2cinit()       / DW-APB speed, address, IRQ               /
uioxfwgpioinit()      / direction, pull, interrupt mode           /
uioxfwtimerinit()     / SP804/PIT/CLINT period, callback          /
uioxfwnvmeinit()      / BAR0, Admin+IO queue, identify            /
uioxfwusbinit()       / xHCI reset, port enable, enumerate        /
uioxfwalsinit()       / VEML7700 gain, integration time           /
`

4 · Complete Comparison Table

| Dimension | SoC Hardware Init | Architecture Init | Peripheral Init |
|---|---|---|---|
| What | Clock, power, DDR, reset, interrupt fabric | CPU privilege, MMU, cache, exception vectors | One device — UART, SPI, I2C, USB, sensor |
| Scope | Entire chip | One CPU core | One device |
| Register type | SoC MMIO registers | CPU system registers (MSR/MCR/CR0/CSR) | Device MMIO or I2C/SPI registers |
| If skipped | Nothing works at all | CPU runs in wrong mode; no MMU; no IRQs | That device doesn't work |
| Can defer? | No — must be first | No — must run before drivers | Yes — many can defer to kernel |
| Re-runnable? | No | Partially (archinit runs again in kernel) | Yes |
| Called by | Bootloader Stage 1–2 | Bootloader (minimal) + Kernel archinit | Bootloader (minimal) + Kernel (full) |
| UIOX module | 02FwHal/src/uioxsoc.c | 10Arch/<arch>/src/archinit.c | 02FwHal/src/uioxfw.c |
| Example ARM64 | uioxclkinit(), GIC distributor | MSR SCTLREL1 (MMU on), MSR VBAREL1 | uioxfwuartinit(), uioxfwi2cinit() |
| Example x86 | LAPIC enable, clock config | MOV CR0 (PG bit), LGDT, LIDT | COM1 baud rate, HPET period |
| Example RISC-V | CLINT init, PLIC threshold | CSR satp (Sv39), CSR stvec | NS16550A baud, PLIC IRQ enable |

5 · Execution Order — Why It Must Be This Sequence

`
Power-On Reset
      │
      ▼
① SoC Hardware Init  ──────────────────────────────── MUST BE FIRST
      │  "Power on the silicon"
      │
      │  uioxpminit()      power domains on
      │  uioxclkinit()     PLL → full CPU speed
      │  DDR PHY training    RAM now usable
      │  uioxrstdeassert() peripheral registers accessible
      │  GIC distributor on  interrupt fabric ready
      │
      ▼
② Architecture Init  ──────────────────────────────── SECOND
      │  "Prepare the CPU core"
      │
      │  archinit() (bootloader: minimal)
      │    set privilege level (EL1 / SVC / S-mode)
      │    install exception vectors (VBAR)
      │    configure stack pointer
      │    [kernel only]: enable MMU + caches
      │    [kernel only]: set up page tables
      │    enable global interrupts (DAIF / CPSR / sstatus)
      │
      ▼
③ Peripheral Init  ─────────────────────────────────── THIRD
      │  "Configure individual devices"
      │
      │  uioxfwuartinit()   debug console
      │  uioxfwtimerinit()  watchdog / tick
      │  uioxfwsdinit()     load kernel from storage
      │  uioxfwsecboot()     verify kernel image
      │
      ▼
④ Kernel archinit() ──────────────────────────────── KERNEL SIDE
      │  "CPU re-init at full OS level"
      │
      │  Full MMU on (page tables, TTBR/CR3/satp)
      │  Full cache on (D-cache + I-cache)
      │  Full GIC/APIC/PLIC (CPU interface + all IRQs)
      │  irqregister() for all devices
      │
      ▼
⑤ Full Peripheral Init  ────────────────────────────── KERNEL DRIVERS
      │
      │  USB, Wi-Fi, NVMe, sensors, display...
      │
      ▼
⑥ Normal Execution
      │
      │  uioxprocinit() → scheduler
      │  uioxshell_start() → first prompt
      │  User processes, system calls, apps
``

One-Line Summary for Each

SoC Hardware Init — "Power on the chip so RAM works and clocks run."

Architecture Init — "Configure the CPU core so it runs in the right mode with MMU, cache, and interrupts set up correctly."

Peripheral Init — "Configure one attached device so it can send/receive data."*
================
Great question — these three are often confused because they overlap in timing and location. Here is the complete distinction.

Three Types of Initialization — Defined

| Type | What it initialises | Level | UIOX Location |
|---|---|---|---|
| SoC Hardware Init | The silicon — clocks, power, DDR, reset fabric | Chip-wide | 02FwHal/src/uioxsoc.c |
| Architecture Init | The CPU itself — exception vectors, MMU, cache, privilege levels | CPU-wide | 10Arch/<arch>/src/archinit.c |
| Peripheral Init | One individual device — UART, SPI, I2C, USB, sensor | Device-specific | 02FwHal/src/uioxfwuart.c etc. |

1 · SoC Hardware Init

Definition: Bringing the entire chip's infrastructure to a working state. This is about the silicon fabric — the wiring between CPU, memory, power, and all internal blocks.

What it covers

``
┌─────────────────────────────────────────────────────────────┐
│  SoC Hardware Init owns this layer:                         │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  PLL / CLK  │  │   Power     │  │   DDR Controller    │ │
│  │  Tree       │  │   Domains   │  │   PHY Training      │ │
│  │  (uioxclk) │  │   (uioxpm) │  │   (DDR must work)   │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  Reset      │  │  System Bus │  │   Interrupt Fabric  │ │
│  │  Controller │  │  AXI / AHB  │  │   GIC/APIC/PLIC     │ │
│  │  (deassert) │  │  Interconnect│  │   Distributor       │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
`

Key Properties
• Must happen before anything else — no RAM, no clocks, no interrupts until this runs
• Affects the whole chip simultaneously — a clock change affects every peripheral
• Done via direct MMIO register writes — no driver abstraction needed
• Performed by the bootloader (Stage 1–2)
• Cannot be skipped or deferred

UIOX Functions

`c
uioxsocinit()          / detect SoC, populate descriptor          /
uioxclkinit()          / PLL configuration, clock tree setup       /
uioxpminit()           / power domain enable (CPU, DRAM, periph)   /
uioxrstdeassert()      / deassert peripheral subsystem reset        /
uioxsocarm64init()    / GIC-600 redistributor wake, DSU, cache     /
`

2 · Architecture Init

Definition: Configuring the CPU core itself — its internal state machine, privilege level, exception handling, memory translation, and cache behaviour. This has nothing to do with external chips.

What it covers

`
┌─────────────────────────────────────────────────────────────┐
│  Architecture Init owns this layer:                         │
│                                                             │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  CPU Core Internal State                               │ │
│  │                                                        │ │
│  │  EL/Privilege   SCTLREL1  VBAREL1   DAIF            │ │
│  │  (EL3→EL1)      MMU bit    Exception  IRQ mask         │ │
│  │                            vectors                     │ │
│  │                                                        │ │
│  │  TTBR0/TTBR1    TCREL1    MAIREL1   Cache           │ │
│  │  Page table     VA range   Mem attrs  enable           │ │
│  │  base                                                  │ │
│  │                                                        │ │
│  │  FPU/NEON       CPACREL1  CPTREL3  Stack pointer    │ │
│  │  enable         FP access  FP trap   SPEL1           │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
`

Key Properties
• Configures registers inside the CPU — not MMIO, not external chips
• CPU-specific — ARM64 uses MRS/MSR, ARM32 uses MCR/MRC, x86 uses MOV CR0, RISC-V uses CSR instructions
• Runs twice — once in bootloader (minimal: set EL, stack), once in kernel (archinit() — full MMU, cache, IRQ routing)
• The MMU enable is an arch init operation, not a SoC or peripheral operation
• Affects only the current CPU core — other cores need their own arch init

UIOX Functions

`c
archinit()              / 10Arch/arm64/src/archinit.c             /
arm64gic400init()      / configure GIC CPU interface (per-core)    /
arm64uartinit()        / PL011 config (technically peripheral but  /
                         / called from archinit for convenience)    /
arm64smpenable()       / set CPUECTLREL1.SMPEN                    /
arm64cachetopology()   / read CLIDREL1, detect L1/L2/L3 sizes    /
uioxsocarm64init()    / GIC-600 redistributor, DSU, SMP           /
`

Architecture Init — Per Architecture

`
ARM64:                         ARM32:
  MSR SCTLREL1 (MMU on)        MCR p15,c1 (SCTLR)
  MSR TTBR0EL1 (page table)    MCR p15,c2 (TTBR0)
  MSR VBAREL1  (vectors)       MCR p15,c12 (VBAR)
  MSR DAIF      (IRQ mask)      CPSR I/F bits
  MSR TCREL1   (VA range)      MCR p15,c3 (DACR)
  MSR MAIREL1  (mem attrs)     ACTLR SMP bit

x86-64:                        RISC-V:
  MOV CR0 (PG + PE bits)         CSR satp (Sv39 mode)
  MOV CR3 (PML4 base)            CSR stvec (trap vector)
  MOV CR4 (PAE, PGE, PCID)       CSR sstatus (SIE, SPP)
  LGDT (GDT load)                CSR medeleg / mideleg
  LIDT (IDT load)                CSR mtvec (M-mode trap)
  WRMSR EFER (long mode)         SFENCE.VMA (TLB flush)
`

3 · Peripheral Init

Definition: Configuring one specific device — a block inside the SoC (UART, SPI, I2C, timer) or an external chip (sensor, PMIC, display) — after the SoC hardware and CPU architecture are both working.

What it covers

`
┌─────────────────────────────────────────────────────────────┐
│  Peripheral Init owns this layer:                           │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │  UART0   │  │  SPI     │  │  I2C     │  │  Timer    │  │
│  │  baud    │  │  CPOL    │  │  speed   │  │  period   │  │
│  │  parity  │  │  CPHA    │  │  addr    │  │  callback │  │
│  │  FIFO    │  │  CS pins │  │  IRQ     │  │  IRQ      │  │
│  └──────────┘  └──────────┘  └──────────┘  └───────────┘  │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │  USB     │  │  NVMe    │  │  Wi-Fi   │  │  Sensor   │  │
│  │  xHCI    │  │  queues  │  │  firmware│  │  gain     │  │
│  │  port    │  │  BAR0    │  │  SSID    │  │  itime    │  │
│  │  reset   │  │  opcodes │  │  connect │  │  thresh   │  │
│  └──────────┘  └──────────┘  └──────────┘  └───────────┘  │
└─────────────────────────────────────────────────────────────┘
`

Key Properties
• Depends on both SoC init AND arch init being complete
• Affects only that one device — UART init failure doesn't break SPI
• Goes through the HAL / driver layer — uioxfwuartinit(), uioxfwspiinit() etc.
• Can be deferred — USB can init after the shell is visible
• Can be re-initialised at runtime (deinit → reconfigure → re-init)
• Split across bootloader (minimal, what's needed to load the kernel) and kernel (full driver stack)

UIOX Functions

`c
uioxfwuartinit()      / PL011 / 16550 baud, parity, FIFO         /
uioxfwspiinit()       / PL022 CPOL, CPHA, speed, CS pins         /
uioxfwi2cinit()       / DW-APB speed, address, IRQ               /
uioxfwgpioinit()      / direction, pull, interrupt mode           /
uioxfwtimerinit()     / SP804/PIT/CLINT period, callback          /
uioxfwnvmeinit()      / BAR0, Admin+IO queue, identify            /
uioxfwusbinit()       / xHCI reset, port enable, enumerate        /
uioxfwalsinit()       / VEML7700 gain, integration time           /
`

4 · Complete Comparison Table

| Dimension | SoC Hardware Init | Architecture Init | Peripheral Init |
|---|---|---|---|
| What | Clock, power, DDR, reset, interrupt fabric | CPU privilege, MMU, cache, exception vectors | One device — UART, SPI, I2C, USB, sensor |
| Scope | Entire chip | One CPU core | One device |
| Register type | SoC MMIO registers | CPU system registers (MSR/MCR/CR0/CSR) | Device MMIO or I2C/SPI registers |
| If skipped | Nothing works at all | CPU runs in wrong mode; no MMU; no IRQs | That device doesn't work |
| Can defer? | No — must be first | No — must run before drivers | Yes — many can defer to kernel |
| Re-runnable? | No | Partially (archinit runs again in kernel) | Yes |
| Called by | Bootloader Stage 1–2 | Bootloader (minimal) + Kernel archinit | Bootloader (minimal) + Kernel (full) |
| UIOX module | 02FwHal/src/uioxsoc.c | 10Arch/<arch>/src/archinit.c | 02FwHal/src/uioxfw.c |
| Example ARM64 | uioxclkinit(), GIC distributor | MSR SCTLREL1 (MMU on), MSR VBAREL1 | uioxfwuartinit(), uioxfwi2cinit() |
| Example x86 | LAPIC enable, clock config | MOV CR0 (PG bit), LGDT, LIDT | COM1 baud rate, HPET period |
| Example RISC-V | CLINT init, PLIC threshold | CSR satp (Sv39), CSR stvec | NS16550A baud, PLIC IRQ enable |

5 · Execution Order — Why It Must Be This Sequence

`
Power-On Reset
      │
      ▼
① SoC Hardware Init  ──────────────────────────────── MUST BE FIRST
      │  "Power on the silicon"
      │
      │  uioxpminit()      power domains on
      │  uioxclkinit()     PLL → full CPU speed
      │  DDR PHY training    RAM now usable
      │  uioxrstdeassert() peripheral registers accessible
      │  GIC distributor on  interrupt fabric ready
      │
      ▼
② Architecture Init  ──────────────────────────────── SECOND
      │  "Prepare the CPU core"
      │
      │  archinit() (bootloader: minimal)
      │    set privilege level (EL1 / SVC / S-mode)
      │    install exception vectors (VBAR)
      │    configure stack pointer
      │    [kernel only]: enable MMU + caches
      │    [kernel only]: set up page tables
      │    enable global interrupts (DAIF / CPSR / sstatus)
      │
      ▼
③ Peripheral Init  ─────────────────────────────────── THIRD
      │  "Configure individual devices"
      │
      │  uioxfwuartinit()   debug console
      │  uioxfwtimerinit()  watchdog / tick
      │  uioxfwsdinit()     load kernel from storage
      │  uioxfwsecboot()     verify kernel image
      │
      ▼
④ Kernel archinit() ──────────────────────────────── KERNEL SIDE
      │  "CPU re-init at full OS level"
      │
      │  Full MMU on (page tables, TTBR/CR3/satp)
      │  Full cache on (D-cache + I-cache)
      │  Full GIC/APIC/PLIC (CPU interface + all IRQs)
      │  irqregister() for all devices
      │
      ▼
⑤ Full Peripheral Init  ────────────────────────────── KERNEL DRIVERS
      │
      │  USB, Wi-Fi, NVMe, sensors, display...
      │
      ▼
⑥ Normal Execution
      │
      │  uioxprocinit() → scheduler
      │  uioxshell_start() → first prompt
      │  User processes, system calls, apps
``

One-Line Summary for Each

SoC Hardware Init — "Power on the chip so RAM works and clocks run."

Architecture Init — "Configure the CPU core so it runs in the right mode with MMU, cache, and interrupts set up correctly."

Peripheral Init — "Configure one attached device so it can send/receive data."*
=============================================
Great question. Here is the complete, precise difference between SoC and Architecture in the context of UIOX and embedded systems generally.

SoC vs Architecture — The Core Difference

| | SoC | Architecture |
|---|---|---|
| What it is | A specific chip product | An instruction set + CPU design rules |
| Who defines it | The chip manufacturer (Broadcom, NXP, SiFive, Intel) | The ISA owner (ARM Ltd, RISC-V International, Intel/AMD) |
| Examples | BCM2711, RK3588, i.MX8, SiFive U74, Intel Q35 | ARMv8-A (AArch64), ARMv7-A, AMD64, RV64IMAFDC |
| Scope | One specific product — peripherals, memory map, clock tree | All chips that implement the ISA |
| Changes per chip | UART base address, PLL divisors, IRQ numbers, DDR config | Never — the ISA is fixed across all chips that use it |
| UIOX module | 02FwHal/src/uioxsoc.c | 10Arch/<arch>/src/archinit.c |

The Clearest Way to Think About It

``
Architecture  =  The CPU instruction set + core design rules
                 (what instructions exist, how registers work,
                  how exceptions are taken, how the MMU works)

SoC           =  Architecture + everything else on the chip
                 (DRAM controller, clock tree, USB, PCIe,
                  UART, GPIO, interrupt controller, power domains)
`

Think of it this way:

`
Architecture (ARMv8-A)
      │
      │  defines:  A64 instruction set, EL0-EL3 privilege levels,
      │            SCTLREL1, TTBR0/TTBR1, VBAREL1, DAIF,
      │            generic timer (CNTPCTEL0/CNTFRQEL0),
      │            exception model, cache maintenance ops
      │
      ├──▶ SoC A:  BCM2711 (Raspberry Pi 4)
      │              UART0 @ 0xFE201000
      │              GIC-400 @ 0xFF841000
      │              PCIe @ 0x7D500000
      │              40-pin GPIO header
      │              VideoCore VI GPU
      │
      ├──▶ SoC B:  RK3588 (Rock Pi)
      │              UART0 @ 0xFEB50000
      │              GIC-600 @ 0xFE600000
      │              PCIe 3.0 @ 0xF5000000
      │              NPU, ISP, VPU
      │
      └──▶ SoC C:  QEMU virt (simulation)
                     UART0 @ 0x09000000
                     GIC-400 @ 0x08000000
                     VirtIO @ 0x0A000000
`

Same architecture (ARMv8-A), three completely different SoCs. All three use the exact same A64 instruction set, the same MRS SCTLREL1 to enable the MMU, the same TLBI VMALLE1IS to flush the TLB. But their UART is at a different address, their GIC is a different version, their clock tree is wired differently, and their peripherals are completely different.

Concrete Examples from UIOX
Architecture defines these — same on EVERY ARMv8-A chip:

`c
/ 10Arch/arm64/include/archdefs.h /

/ These are ARM Architecture constants — same on BCM2711, RK3588, IMX8, QEMU /
#define PSTATEEL1h          0x05u    / exception level 1, SPEL1           /
#define PSTATEIBIT         (1u<<7)  / IRQ mask bit in DAIF                /

/ These are Architecture barrier instructions — same on all ARMv8-A chips   /
#define archmb()   asm volatile("dmb sy"  ::: "memory")
#define archisb()  asm volatile("isb"     ::: "memory")
#define archwfi()  asm volatile("wfi")

/ Enable MMU — same instruction on every ARMv8-A chip /
/ MSR SCTLREL1, x0  (bit 0 = M = MMU enable)        /
`

SoC defines these — different on every chip:

`c
/ 02FwHal/include/uioxsocmap.h /

/ QEMU virt — these addresses ONLY apply to this specific SoC /
#define SOCDRAMBASE          0x40000000UL   / BCM2711 uses 0x00000000     /
#define SOCGICDISTBASE      0x08000000UL   / RK3588 uses 0xFE600000      /
#define SOCUART0BASE         0x09000000UL   / BCM2711 uses 0xFE201000     /
#define SOCCLINTBASE         0x02000000UL   / Only on RISC-V SoCs         /

/ BCM2711 (Raspberry Pi 4) — completely different from QEMU /
/ #define SOCDRAMBASE       0x00000000UL                                  /
/ #define SOCGICDISTBASE   0xFF841000UL                                  /
/ #define SOCUART0BASE      0xFE201000UL                                  /
`

The Five Key Differences Explained
1 · Instruction Set vs Silicon

Architecture is the contract between software and hardware — it specifies exactly what instructions exist and what they do. ADD X0, X1, X2 adds two registers. This is true on every ARMv8-A chip ever made.

SoC is the actual silicon that implements that contract, plus adds its own set of peripherals at its own addresses. Two SoCs can both be ARMv8-A but have completely different memory maps.

2 · Stable vs Variable

Architecture never changes for a given ISA version. ARMv8-A Cortex-A53 from 2013 and Cortex-A76 from 2018 both use the same MSR SCTLREL1 instruction to enable the MMU. The architecture is fixed.

SoC varies enormously between chips and even between chip revisions. BCM2711 rev B0 has different errata workarounds from rev C0.

3 · Who Knows What

Architecture init (archinit.c) needs to know:
• What privilege level to set (EL1, SVC, S-mode)
• What system registers control the MMU (SCTLREL1, CR0, satp)
• How to flush the TLB (TLBI VMALLE1IS, INVLPG, SFENCE.VMA)
• How to enable/disable interrupts (DAIF, CPSR, RFLAGS, sstatus)

SoC init (uioxsoc.c) needs to know:
• What physical address is the GIC/APIC/PLIC at
• What PLL multiplier gives 1.5 GHz on this chip
• What power domain register enables DDR
• What IRQ number does UART0 use on this SoC

4 · Universal vs Chip-Specific Code

`c
/ Architecture code — compiles identically for BCM2711, RK3588, IMX8 /
void enablemmu(void)
{
    uint64t sctlr;
    asm volatile("mrs %0, sctlrel1" : "=r"(sctlr));
    sctlr |= (1u << 0);   / M bit — same on ALL ARMv8-A chips /
    asm volatile("msr sctlrel1, %0" :: "r"(sctlr) : "memory");
    asm volatile("isb");
}

/ SoC code — completely different per chip /
void uartinitqemuvirt(void)
{
    mmiowrite32(0x09000000UL + 0x024u, 13u);  / IBRD — QEMU virt only    /
}

void uartinitrpi4(void)
{
    mmiowrite32(0xFE201000UL + 0x024u, 13u);  / IBRD — BCM2711 (RPi4)    /
}
`

5 · In UIOX Source Tree

`
10Arch/              ← ARCHITECTURE layer
│                        Same code for all SoCs that share the ISA
│
├── arm64/
│   ├── include/archdefs.h    ← ARMv8-A ISA constants (same on ALL ARM64 SoCs)
│   └── src/archinit.c        ← CPU core setup (MMU, cache, DAIF, VBAR, GIC CPU iface)
│
├── arm32/
│   ├── include/archdefs.h    ← ARMv7-A ISA constants
│   └── src/archinit.c
│
├── x8664/
│   ├── include/archdefs.h    ← AMD64 constants (CR0, CR3, EFER, RFLAGS)
│   └── src/archinit.c
│
└── riscv64/
    ├── include/archdefs.h    ← RV64 CSR names, CLINT/PLIC offsets
    └── src/archinit.c

02FwHal/             ← SoC layer
│                        Different per chip / board
│
├── include/
│   ├── uioxsocmap.h         ← MMIO addresses (CHANGE per SoC)
│   ├── uioxsocclk.h         ← Clock tree (CHANGE per SoC)
│   ├── uioxsocpm.h          ← Power domains (CHANGE per SoC)
│   └── uioxsoctypes.h       ← SoC ID enum (one entry per SoC product)
│
└── src/
    ├── uioxsocarm64.c       ← BCM2711 / RK3588 / IMX8 / QEMU detect + init
    ├── uioxsocarm32.c       ← BCM2836 / IMX6 / OMAP detect + init
    ├── uioxsocx86.c         ← QEMU Q35 / generic x86 detect + init
    └── uioxsocriscv64.c     ← SiFive U74 / QEMU virt detect + init
`

Summary in One Table

| Question | Architecture Answer | SoC Answer |
|---|---|---|
| What CPU instruction enables the MMU? | MSR SCTLREL1 (same on all ARM64) | N/A — SoC doesn't define instructions |
| Where is the UART? | Defined by the SoC, not the architecture | 0x09000000 (QEMU), 0xFE201000 (RPi4) |
| How do you flush the TLB? | TLBI VMALLE1IS (same on all ARM64) | N/A |
| What clock speed is the CPU? | Architecture defines max; actual speed is SoC | 1.5 GHz (BCM2711), 2.4 GHz (BCM2712) |
| What IRQ number is the UART? | Architecture defines IRQ model; actual number is SoC | 33 (QEMU virt), 153 (BCM2711) |
| How many privilege levels? | 4 (EL0–EL3 for ARM64) — architecture defines this | N/A |
| Can two chips share arch init code? | Yes — all ARMv8-A chips use the same archinit.c | No — BCM2711 and RK3588 need different uioxsoc*.c` |