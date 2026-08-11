# UIOX ISA Specification

## 1. Scope

This document defines the processor-architecture contract for UIOX-supported kernel targets:

- ARM64 (AArch64 / ARMv8-A)
- ARM32 (AArch32 / ARMv7-A)
- RISC-V 64 (RV64)
- x86-64 (AMD64 long mode)

This is a kernel-facing architecture specification. It does not replace vendor ISA manuals. Instead, it defines the subset of architectural behavior, privilege transitions, register conventions, memory-management controls, and low-level instructions required by UIOX.

## 2. Supported architectures

| UIOX name | ISA | Privilege model | Kernel privilege |
|---|---|---|---|
| arm64 | ARMv8-A AArch64 | EL0/EL1 | EL1 |
| arm32 | ARMv7-A AArch32 | User/SVC/IRQ/etc. | SVC/System |
| riscv64 | RV64IMAFDC + Zicsr + Zifencei | U/S/M | S-mode |
| x8664 | AMD64 long mode | Ring 3 / Ring 0 | Ring 0 |

## 3. Cross-architecture kernel contract

Every UIOX architecture port shall provide:

1. kernel entry from boot firmware or handoff code
2. early stack setup
3. exception/trap vector installation
4. timer interrupt support
5. device interrupt entry
6. syscall entry from user mode
7. return to user mode
8. page-table installation and address-space switching
9. TLB invalidation
10. context save/restore for scheduler switching
11. atomic synchronization primitives
12. CPU idle instruction

## 4. ARM64

### 4.1 Execution level
- User mode executes at EL0.
- Kernel executes at EL1.
- Return from exception uses `eret`.

### 4.2 Boot register contract
- `x0` = device-tree physical address
- `x1` = boot argument block physical address
- `x2` = reserved
- `x3` = reserved

### 4.3 Syscall ABI
- Trap instruction: `svc #0`
- Syscall number: `x8`
- Arguments: `x0-x5`
- Return value: `x0`

### 4.4 Exception state used by kernel
- `VBAR_EL1`
- `ELR_EL1`
- `SPSR_EL1`
- `ESR_EL1`
- `FAR_EL1`

### 4.5 MMU control
- `TTBR0_EL1` = user translation base
- `TTBR1_EL1` = kernel translation base
- `TCR_EL1`
- `MAIR_EL1`
- `SCTLR_EL1`

### 4.6 TLB and barriers
- TLB invalidation via `tlbi`
- completion barrier via `dsb`
- context synchronization via `isb`
- memory ordering via `dmb`

### 4.7 Idle and synchronization
- idle: `wfi`
- event wait: `wfe`
- atomics: LL/SC family (`ldxr/stxr`) or architected atomic instructions where enabled

## 5. ARM32

### 5.1 Execution model
- User mode for applications
- SVC/System mode for kernel execution
- IRQ/FIQ/Abort modes for exceptions
- return via exception restore path

### 5.2 Boot register contract
- `r0` = reserved
- `r1` = reserved
- `r2` = device-tree physical address
- `r3` = boot argument block physical address

### 5.3 Syscall ABI
- Trap instruction: `svc #0`
- Syscall number: `r7`
- Arguments: `r0-r5`
- Return value: `r0`

### 5.4 Exception state used by kernel
- `VBAR`
- `CPSR`
- `SPSR`
- `DFSR`, `IFSR`
- `DFAR`, `IFAR`

### 5.5 MMU control
- `TTBR0`
- `TTBR1`
- `DACR`
- `SCTLR`

### 5.6 TLB and barriers
- CP15 invalidation operations
- `dmb`
- `dsb`
- `isb`

### 5.7 Idle and synchronization
- idle: `wfi`
- atomics: `ldrex/strex`

## 6. RISC-V 64

### 6.1 Execution model
- User mode executes in U-mode
- Kernel executes in S-mode
- Machine mode is assumed to be firmware/runtime below the kernel
- return from trap uses `sret`

### 6.2 Required ISA profile
- `rv64i`
- `m`
- `a`
- `f`
- `d`
- `c`
- `zicsr`
- `zifencei`

### 6.3 Boot register contract
- `a0` = device-tree physical address
- `a1` = boot argument block physical address

### 6.4 Syscall ABI
- Trap instruction: `ecall`
- Syscall number: `a7`
- Arguments: `a0-a5`
- Return value: `a0`

### 6.5 Supervisor CSRs used by kernel
- `stvec`
- `sstatus`
- `sepc`
- `scause`
- `stval`
- `satp`
- `sscratch`
- `sie`
- `sip`

### 6.6 TLB and barriers
- address-space switch via `satp`
- TLB invalidation via `sfence.vma`
- memory ordering via `fence`
- instruction synchronization via `fence.i`

### 6.7 Idle and synchronization
- idle: `wfi`
- atomics: LR/SC and AMO instructions

## 7. x86-64

### 7.1 Execution model
- User mode executes at ring 3
- Kernel executes at ring 0
- syscall return uses `sysretq`
- interrupt/fault return uses `iretq`

### 7.2 Boot register contract
- `rdi` = boot argument block physical address
- `rsi` = device-tree or platform-description physical address

### 7.3 Syscall ABI
- Trap instruction: `syscall`
- Syscall number: `rax`
- Arguments: `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9`
- Return value: `rax`

### 7.4 Required control state
- `CR0`
- `CR2`
- `CR3`
- `CR4`
- `EFER`
- `STAR`
- `LSTAR`
- `SFMASK`
- `IDTR`
- `GDTR`

### 7.5 TLB and barriers
- address-space switch via `CR3`
- page invalidation via `invlpg`
- ordering via native x86 memory model plus `mfence/lfence/sfence` as required

### 7.6 Idle and synchronization
- idle: `hlt`
- atomics: `lock`-prefixed instructions, `cmpxchg`, `xadd`, `xchg`

## 8. Compiler and ABI constraints

### 8.1 arm64
- target: `armv8-a`
- ABI: LP64
- little-endian

### 8.2 arm32
- target: `armv7-a`
- ARM mode
- soft-float ABI
- little-endian

### 8.3 riscv64
- ISA string: `rv64imafdczicsr_zifencei`
- ABI: `lp64d`

### 8.4 x8664
- no red-zone
- no MMX/SSE/SSE2 assumptions in generic kernel code
- kernel code model

## 9. Architecture portability rules

1. Kernel code shall not assume pointer width beyond `uintptr_t`.
2. Pointer/integer conversion shall use portable mechanisms when strict warnings are enabled.
3. Floating-point shall not be required in generic kernel subsystems.
4. Architecture-specific barriers shall be wrapped by a common abstraction layer.
5. TLB invalidation shall be exposed through architecture hooks, not open-coded in generic code.
6. Trap-frame layout shall be architecture-specific but semantically equivalent across ports.

## 10. External references

- Armv8-A AArch64 Architecture Reference Manual
- ARMv7-A/R Architecture Reference Manual
- RISC-V Instruction Set Manual, Privileged Architecture
- AMD64 Architecture Programmer's Manual
