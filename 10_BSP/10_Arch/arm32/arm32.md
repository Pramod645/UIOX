| # | File | Purpose |
| --- | --- | --- |
| 1 | include/arm_types.h | Base bit-width types |
| 2 | include/arm_registers.h | R0-R15, SP, LR, PC, modes |
| 3 | include/arm_psr.h | CPSR/SPSR bits, condition codes |
| 4 | include/arm_opcodes.h | All opcode constants |
| 5 | include/arm_instr_format.h | 15 instruction encoding structs + union |
| 6 | include/arm_macros.h | Per-instruction address macros + builder macros |
| 7 | include/arm_memory.h | Memory map + access interface |
| 8 | include/arm_exceptions.h | Exception vectors + set macros |
| 9 | include/arm_coprocessor.h | CP0-CP15 interface + CP15 control bits |
| 10 | include/arm_arch.h | Master include + CPU state struct |
| 11 | src/arm_decode.c | Instruction class decoder |
| 12 | src/arm_execute.c | Condition evaluator + executor |
| 13 | src/arm_memory.c | Flat memory read/write |
| 14 | src/arm_exceptions.c | Exception handler stubs |
| 15 | Makefile | Build system |