/*
 * 10_BSP/10_Arch/riscv64/include/mmio.h
 *
 * RISC-V 64 Memory-Mapped I/O accessors.
 * All RISC-V peripheral access is MMIO — no port I/O.
 * 64-bit accessor uses natural alignment required by RV64 memory model.
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_MMIO_RISCV64_H
#define UIOX_MMIO_RISCV64_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void mmio_init(void)
{ __asm__ volatile("" ::: "memory"); }

/* 32-bit */
static inline void     mmio_write32(uintptr_t a, uint32_t v)
{ *((volatile uint32_t *)a) = v; }
static inline uint32_t mmio_read32 (uintptr_t a)
{ return *((volatile uint32_t *)a); }

/* 8-bit */
static inline void    mmio_write8(uintptr_t a, uint8_t v)
{ *((volatile uint8_t *)a) = v; }
static inline uint8_t mmio_read8 (uintptr_t a)
{ return *((volatile uint8_t *)a); }

/* 64-bit — naturally aligned on RV64 */
static inline void     mmio_write64(uintptr_t a, uint64_t v)
{ *((volatile uint64_t *)a) = v; }
static inline uint64_t mmio_read64 (uintptr_t a)
{ return *((volatile uint64_t *)a); }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MMIO_RISCV64_H */
