| #  | File                          | Purpose                                          |
|----|-------------------------------|--------------------------------------------------|
| 1  | include/arm64_types.h         | Base bit-width types                             |
| 2  | include/arm64_registers.h     | X0-X30, SP, PC, XZR, modes                      |
| 3  | include/arm64_psr.h           | PSTATE / SPSR bits, condition codes              |
| 4  | include/arm64_opcodes.h       | All opcode constants                             |
| 5  | include/arm64_instr_format.h  | 15 instruction encoding structs + union          |
| 6  | include/arm64_macros.h        | Per-instruction address macros + builder macros  |
| 7  | include/arm64_memory.h        | Memory map + access interface                    |
| 8  | include/arm64_exceptions.h    | Exception vectors + handler types                |
| 9  | include/arm64_sysregs.h       | System register (MRS/MSR) interface              |
| 10 | include/arm64_arch.h          | Master include + CPU state struct                |
| 11 | src/arm64_decode.c            | Instruction class decoder                        |
| 12 | src/arm64_execute.c           | Condition evaluator + executor                   |
| 13 | src/arm64_memory.c            | Flat memory read/write                           |
| 14 | src/arm64_exceptions.c        | Exception handler stubs                          |
| 15 | Makefile                      | Build system                                     |
