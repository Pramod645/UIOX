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
