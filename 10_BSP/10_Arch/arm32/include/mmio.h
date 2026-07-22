/*
 * 10_BSP/10_Arch/arm32/include/mmio.h
 *
 * ARM32 Memory-Mapped I/O accessors.
 * Plain volatile pointer reads/writes — identical pattern to ARM64.
 * No port I/O on ARM.
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_MMIO_ARM32_H
#define UIOX_MMIO_ARM32_H

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
static inline void     mmio_write32(uiox_uintptr_t a, uiox_uint32_t v)
{ *((volatile uiox_uint32_t *)a) = v; }
static inline uiox_uint32_t mmio_read32 (uiox_uintptr_t a)
{ return *((volatile uiox_uint32_t *)a); }

/* 8-bit */
static inline void    mmio_write8(uiox_uintptr_t a, uiox_uint8_t v)
{ *((volatile uiox_uint8_t *)a) = v; }
static inline uiox_uint8_t mmio_read8 (uiox_uintptr_t a)
{ return *((volatile uiox_uint8_t *)a); }

/* 64-bit (LPAE capable, 64-bit aligned) */
static inline void     mmio_write64(uiox_uintptr_t a, uiox_uint64_t v)
{ *((volatile uiox_uint64_t *)a) = v; }
static inline uiox_uint64_t mmio_read64 (uiox_uintptr_t a)
{ return *((volatile uiox_uint64_t *)a); }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MMIO_ARM32_H */
