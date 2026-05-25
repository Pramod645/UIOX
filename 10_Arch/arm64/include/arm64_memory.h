#ifndef ARM64_MEMORY_H
#define ARM64_MEMORY_H
/*
 * arm64_memory.h — AArch64 memory map and access interface
 * Reference: ARM DDI 0487, Chapter D5 (Memory Model)
 */
#include "arm64_types.h"

/* ── UIOX AArch64 memory map ─────────────────────────────── */
#define ARM64_MEM_RESET_VEC     0x0000000000000000ULL  /* Reset / VBAR   */
#define ARM64_MEM_ROM_BASE      0x0000000000000000ULL  /* ROM / Flash     */
#define ARM64_MEM_ROM_SIZE      0x0000000000100000ULL  /* 1 MB            */
#define ARM64_MEM_RAM_BASE      0x0000000040000000ULL  /* DRAM start      */
#define ARM64_MEM_RAM_SIZE      0x0000000004000000ULL  /* 64 MB           */
#define ARM64_MEM_STACK_TOP     0x0000000044000000ULL  /* Initial SP_EL1  */
#define ARM64_MEM_IO_BASE       0x0000000010000000ULL  /* MMIO region     */
#define ARM64_MEM_IO_SIZE       0x0000000010000000ULL  /* 256 MB          */
#define ARM64_MEM_PERIPH_BASE   0x0000000020000000ULL  /* Peripheral base */
#define ARM64_MEM_GIC_DIST      0x0000000008000000ULL  /* GIC distributor */
#define ARM64_MEM_GIC_CPU       0x0000000008010000ULL  /* GIC CPU iface   */

/* ── Alignment masks ─────────────────────────────────────── */
#define ARM64_QWORD_ALIGN_MASK  0xFFFFFFFFFFFFFFF8ULL  /* 8-byte align   */
#define ARM64_DWORD_ALIGN_MASK  0xFFFFFFFFFFFFFFFCULL  /* 4-byte align   */
#define ARM64_WORD_ALIGN_MASK   0xFFFFFFFFFFFFFFFEULL  /* 2-byte align   */

/* ── Memory access functions ─────────────────────────────── */
arm64_word_t arm64_mem_read_qword (arm64_addr_t addr);  /* 64-bit read   */
arm64_word_t arm64_mem_read_dword (arm64_addr_t addr);  /* 32-bit read   */
arm64_word_t arm64_mem_read_word  (arm64_addr_t addr);  /* 16-bit read   */
arm64_word_t arm64_mem_read_byte  (arm64_addr_t addr);  /*  8-bit read   */

void arm64_mem_write_qword (arm64_addr_t addr, arm64_word_t val);
void arm64_mem_write_dword (arm64_addr_t addr, arm64_word_t val);
void arm64_mem_write_word  (arm64_addr_t addr, arm64_word_t val);
void arm64_mem_write_byte  (arm64_addr_t addr, arm64_word_t val);

/* ── PC-relative address computation ─────────────────────── */
/* AArch64: PC points to the current instruction (no +8 offset) */
#define ARM64_PC_OFFSET   0ULL
#if 0
#define ARM64_BRANCH_TARGET(pc, imm26) \
    ((arm64_addr_t)((pc) + ((arm64_int64_t)((arm64_int32_t)((imm26) << 6) >> 4))))

#define ARM64_BCOND_TARGET(pc, imm19) \
    ((arm64_addr_t)((pc) + ((arm64_int64_t)((arm64_int32_t)((imm19) << 13) >> 11))))
#endif
#define ARM64_CBZ_TARGET(pc, imm19) \
    ARM64_BCOND_TARGET(pc, imm19)

#define ARM64_PAGE_SIZE   4096ULL
#define ARM64_PAGE_SHIFT  12

#endif /* ARM64_MEMORY_H */
