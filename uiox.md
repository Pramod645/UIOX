| File | Purpose |
| --- | --- |
| arch/arm64/include/arch_defs.h | GIC, PL011, SP804, DAIF macros, barrier macros, IRQ numbers for AArch64 |
| arch/arm64/src/arch_init.c | GIC-400 init, UART init, SP804 timer, VBAR install, handler registration |
| arch/arm32/include/arch_defs.h | Same but ARMv7-A CPSR, Thumb, hard-float, versatilepb addresses |
| arch/arm32/src/arch_init.c | ARMv7-A GIC + PL011 + SP804 + IDE IRQ handlers |
| arch/x86_64/include/arch_defs.h | LAPIC, IOAPIC, 8259A, PIT, COM1, RFLAGS, MSR numbers, CLI/STI macros |
| arch/x86_64/src/arch_init.c | 8259A remap, PIT 100 Hz, 16550 UART, LAPIC enable, handler registration |
| build_config/common.mk | Shared CFLAGS, include paths to all three uiox layers, pattern rules |
| build_config/arm64.mk | aarch64-linux-gnu-gcc, -march=armv8-a, ELF→BIN, QEMU virt run |
| build_config/arm32.mk | arm-linux-gnueabihf-gcc, -march=armv7-a -mthumb -mfpu=vfpv3, versatilepb QEMU |
| build_config/x86_64.mk | gcc, -march=x86-64, native run + QEMU q35 |
| main.c | 8-stage integration: hw init → fs → dev → tty → irq/DMA/ctx → pty → sync → teardown |
| Makefile | Top-level: delegates to per-arch .mk files; make all builds all three |

//////////////////////////////////////////////////////////////////////////////////////////////////////////
File,Purpose
Makefile,"Top-level orchestrator — builds all 3 arches, QEMU, clean, help"
build_config/tools.mk,Auto-detects ARM64/ARM32/x86_64 cross-compilers
build_config/common.mk,"Shared -Wall, -ffreestanding, -nostdlib, include paths"
build_config/arm64.mk,"ARM64-only flags (-march=armv8-a, -mabi=lp64)"
build_config/arm32.mk,"ARM32-only flags (-march=armv7-a, -mfloat-abi=hard)"
build_config/x86_64.mk,"x86_64-only flags (-m64, -mno-red-zone, -mcmodel=kernel)"
linker/uiox_arm64.ld,"AArch64 — DRAM@0x40000000, vectors, stack, heap"
linker/uiox_arm32.ld,"ARM32 — ROM@0x0, RAM@0x100000, exception vectors"
linker/uiox_x86_64.ld,"x86-64 — kernel@0x100000, IDT/GDT/pgtable sections"
build_config/install_tools.sh,One-shot apt/brew installer for all toolchains + QEMU
build_config/uiox_compile_flags.txt,IDE/clangd compile flags reference
build_config/uiox_gdb.py,"GDB Python helper — connect, breakpoints, memmap"