#ifndef ARM_MEMORY_H
#define ARM_MEMORY_H

/*
 * arm_memory.h — ARM memory map and access interface
 * Reference: ARM Instruction Set, Section 2.2
 */

#include "arm_types.h"

/* ── UIX ARM memory map ──────────────────────────────────── */
#define ARM_MEM_RESET_VEC    0x00000000u  /* Reset vector           */
#define ARM_MEM_EXCEPT_BASE  0x00000000u  /* Exception vector table */
#define ARM_MEM_ROM_BASE     0x00000000u  /* ROM/Flash start        */
#define ARM_MEM_ROM_SIZE     0x00100000u  /* 1 MB ROM               */
#define ARM_MEM_RAM_BASE     0x00100000u  /* RAM start              */
#define ARM_MEM_RAM_SIZE     0x00F00000u  /* 15 MB RAM              */
#define ARM_MEM_STACK_TOP    0x01000000u  /* Initial SP             */
#define ARM_MEM_IO_BASE      0x10000000u  /* Memory-mapped I/O      */
#define ARM_MEM_IO_SIZE      0x10000000u  /* 256 MB I/O space       */
#define ARM_MEM_PERIPH_BASE  0x20000000u  /* Peripheral base        */

/* ── Alignment masks ─────────────────────────────────────── */
#define ARM_WORD_ALIGN_MASK  0xFFFFFFFCu
#define ARM_HALF_ALIGN_MASK  0xFFFFFFFEu

/* ── Memory access functions ─────────────────────────────── */
arm_word_t arm_mem_read_word  (arm_addr_t addr);
arm_word_t arm_mem_read_half  (arm_addr_t addr);
arm_word_t arm_mem_read_byte  (arm_addr_t addr);
void       arm_mem_write_word (arm_addr_t addr, arm_word_t val);
void       arm_mem_write_half (arm_addr_t addr, arm_word_t val);
void       arm_mem_write_byte (arm_addr_t addr, arm_word_t val);

/* ── PC-relative address computation ─────────────────────── */
/* ARM: PC points 8 bytes ahead of currently executing instr */
#define ARM_PC_OFFSET  8u
#define ARM_BRANCH_TARGET(pc, off24) \
    ((arm_addr_t)((pc) + ARM_PC_OFFSET + \
     (arm_int32_t)(((off24) & 0x800000u) ? \
     ((off24) | 0xFF000000u) : (off24)) * 4))

#endif /* ARM_MEMORY_H */
