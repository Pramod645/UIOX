#ifndef ARM64_MEMORY_H
#define ARM64_MEMORY_H
/*
 * arm64_memory.h — AArch64 memory map and access interface
 * Reference: ARMv8-A Architecture Reference Manual, Section D5
 *
 * AArch64 memory differences from ARM32:
 *   - 64-bit virtual address space (up to 52-bit physical)
 *   - Two translation regimes: TTBR0 (user) / TTBR1 (kernel)
 *   - 4-level page tables by default (4KB granule)
 *   - PC-relative addressing uses 0-offset (no +8 like ARM32)
 *   - VBAR_ELn holds exception vector base address
 */

#include "arm64_types.h"

/* ── UIX AArch64 memory map ──────────────────────────────── */
#define ARM64_MEM_RESET_VEC     0x0000000000000000ULL  /* Reset       */
#define ARM64_MEM_EXCEPT_BASE   0x0000000000000000ULL  /* VBAR_EL1    */
#define ARM64_MEM_ROM_BASE      0x0000000000000000ULL  /* ROM/Flash   */
#define ARM64_MEM_ROM_SIZE      0x0000000001000000ULL  /* 16 MB ROM   */
#define ARM64_MEM_RAM_BASE      0x0000000040000000ULL  /* 1 GB offset */
#define ARM64_MEM_RAM_SIZE      0x0000000040000000ULL  /* 1 GB RAM    */
#define ARM64_MEM_STACK_TOP     0x0000000080000000ULL  /* Initial SP  */
#define ARM64_MEM_IO_BASE       0x0000000009000000ULL  /* PL011 UART  */
#define ARM64_MEM_IO_SIZE       0x0000000001000000ULL  /* 16 MB I/O   */
#define ARM64_MEM_PERIPH_BASE   0x0000000010000000ULL  /* Peripherals */
#define ARM64_MEM_GIC_BASE      0x0000000008000000ULL  /* GICv3 dist  */

/* ── Page sizes ──────────────────────────────────────────── */
#define ARM64_PAGE_SIZE_4K      0x1000ULL
#define ARM64_PAGE_SIZE_16K     0x4000ULL
#define ARM64_PAGE_SIZE_64K     0x10000ULL

/* ── Alignment masks ─────────────────────────────────────── */
#define ARM64_DWORD_ALIGN_MASK  0xFFFFFFFFFFFFFFF8ULL
#define ARM64_WORD_ALIGN_MASK   0xFFFFFFFFFFFFFFFCULL
#define ARM64_HALF_ALIGN_MASK   0xFFFFFFFFFFFFFFFEULL

/* ── Exception Vector Base ───────────────────────────────── */
/* VBAR_ELn + offset for each exception type/source           */
#define ARM64_VBAR_SYNC_SP0     0x000ULL  /* Sync, SP_EL0           */
#define ARM64_VBAR_IRQ_SP0      0x080ULL  /* IRQ,  SP_EL0           */
#define ARM64_VBAR_FIQ_SP0      0x100ULL  /* FIQ,  SP_EL0           */
#define ARM64_VBAR_ERR_SP0      0x180ULL  /* SError, SP_EL0         */
#define ARM64_VBAR_SYNC_SPn     0x200ULL  /* Sync, SP_ELn           */
#define ARM64_VBAR_IRQ_SPn      0x280ULL  /* IRQ,  SP_ELn           */
#define ARM64_VBAR_FIQ_SPn      0x300ULL  /* FIQ,  SP_ELn           */
#define ARM64_VBAR_ERR_SPn      0x380ULL  /* SError, SP_ELn         */
#define ARM64_VBAR_SYNC_L64     0x400ULL  /* Sync, lower EL AArch64 */
#define ARM64_VBAR_IRQ_L64      0x480ULL  /* IRQ,  lower EL AArch64 */
#define ARM64_VBAR_FIQ_L64      0x500ULL  /* FIQ,  lower EL AArch64 */
#define ARM64_VBAR_ERR_L64      0x580ULL  /* SError, lower AArch64  */
#define ARM64_VBAR_SYNC_L32     0x600ULL  /* Sync, lower EL AArch32 */
#define ARM64_VBAR_IRQ_L32      0x680ULL  /* IRQ,  lower EL AArch32 */
#define ARM64_VBAR_FIQ_L32      0x700ULL  /* FIQ,  lower EL AArch32 */
#define ARM64_VBAR_ERR_L32      0x780ULL  /* SError, lower AArch32  */

/* ── PC-relative note ────────────────────────────────────── */
/* AArch64: PC points TO the current instruction (no +8 bias) */
#define ARM64_PC_OFFSET         0ULL

/* Branch target for B/BL with imm26 (sign-extended, *4) */
#define ARM64_BRANCH_TARGET(pc, imm26) \
    ((arm64_addr_t)((pc) + \
     (arm64_int64_t)((arm64_uint64_t)((imm26) & 0x3FFFFFFu) | \
     (((imm26) & 0x2000000u) ? 0xFFFFFFFFC0000000ULL : 0ULL)) * 4))

/* ── Memory access functions ─────────────────────────────── */
arm64_uint64_t arm64_mem_read_dword (arm64_addr_t addr);
arm64_uint32_t arm64_mem_read_word  (arm64_addr_t addr);
arm64_uint16_t arm64_mem_read_half  (arm64_addr_t addr);
arm64_uint8_t  arm64_mem_read_byte  (arm64_addr_t addr);

void arm64_mem_write_dword (arm64_addr_t addr, arm64_uint64_t val);
void arm64_mem_write_word  (arm64_addr_t addr, arm64_uint32_t val);
void arm64_mem_write_half  (arm64_addr_t addr, arm64_uint16_t val);
void arm64_mem_write_byte  (arm64_addr_t addr, arm64_uint8_t  val);

/* ── Simulated memory pool ───────────────────────────────── */
#define ARM64_SIM_MEM_SIZE  0x10000000UL   /* 256 MB simulation RAM */

#endif /* ARM64_MEMORY_H */
