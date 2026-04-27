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