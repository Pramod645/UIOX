00_Boot/
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
Build and run:
# Build all three
cd 00_Boot
make all

# ARM64 in QEMU
qemu-system-aarch64 -machine virt -cpu cortex-a53 -m 64M \
  -nographic -bios build/arm64/uiox_boot.bin

# ARM32 in QEMU
qemu-system-arm -machine versatilepb -cpu arm926 -m 16M \
  -nographic -bios build/arm32/uiox_boot.bin

# x86-64 in QEMU (multiboot2)
qemu-system-x86_64 -m 64M -nographic \
  -kernel build/x86_64/uiox_boot.bin
