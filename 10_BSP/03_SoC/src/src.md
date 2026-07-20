File	Category	What It Implements
uiox_soc_arm64.c	SoC	ARM64 SoC detect (MIDR_EL1), GIC-400 init, PL011 clock config, PM init
uiox_soc_arm32.c	SoC	ARM32 SoC detect, GIC init, SP804 clock, PM init
uiox_soc_x86.c	SoC	x86-64 CPUID detect, LAPIC enable, COM1 init, HPET init
uiox_soc_riscv64.c	SoC	RISC-V SBI probe, CLINT init, PLIC threshold, NS16550A, S-mode delegation
uiox_fw_psci.c	SoC	PSCI 1.1 dispatch — CPU_ON/OFF, SYSTEM_OFF/RESET SMC handlers
uiox_fw_secboot.c	SoC	SHA-256, RSA/ECDSA verify, anti-rollback chain-of-trust
uiox_fw_power.c	SoC	PSCI ARM / ACPI x86 / SBI RISC-V power (reset, shutdown, CPU hot-plug)
uiox_fw_mem.c	SoC	no-libc memset/memcpy/memcmp, bump allocator, DDR region table
uiox_fw_riscv.c	SoC	RISC-V HAL vtable — uiox_fw_hw_ops_t for rv64: PLIC IRQ, SBI timer/power
uiox_fw_post.c	SoC	POST test runner — CPU sanity, DRAM walk, ROM CRC, UART smoke