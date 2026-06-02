#ifndef CPU_IO_H
#define CPU_IO_H
/*
 * cpu_io.h - Device access interface (MMIO + port I/O)
 * Abstraction layer over cpu_regs.h for higher-level drivers.
 */
#include "cpu_types.h"

/* -- MMIO region descriptor --------------------------------- */
typedef struct cpu_mmio_region {
    char       name[32];
    cpu_addr_t base;
    cpu_u64_t  size;
    cpu_u32_t  flags;        /* CPU_MMU_* attributes             */
    cpu_bool_t mapped;
} cpu_mmio_region_t;

#define CPU_IO_MAX_REGIONS  32
extern cpu_mmio_region_t g_mmio_regions[CPU_IO_MAX_REGIONS];
extern cpu_u32_t         g_mmio_count;

/* -- MMIO region management --------------------------------- */
int  cpu_io_register_region (const char *name, cpu_addr_t base,
                              cpu_u64_t size, cpu_u32_t flags);
cpu_mmio_region_t *cpu_io_find_region(const char *name);
void cpu_io_print_regions   (void);

/* -- Safe MMIO accessors (bounds-checked in debug builds) --- */
void      cpu_io_write8  (cpu_addr_t addr, cpu_u8_t  val);
void      cpu_io_write16 (cpu_addr_t addr, cpu_u16_t val);
void      cpu_io_write32 (cpu_addr_t addr, cpu_u32_t val);
void      cpu_io_write64 (cpu_addr_t addr, cpu_u64_t val);
cpu_u8_t  cpu_io_read8   (cpu_addr_t addr);
cpu_u16_t cpu_io_read16  (cpu_addr_t addr);
cpu_u32_t cpu_io_read32  (cpu_addr_t addr);
cpu_u64_t cpu_io_read64  (cpu_addr_t addr);

/* -- Bit-field helpers (read-modify-write) ------------------ */
void cpu_io_set_bits32   (cpu_addr_t addr, cpu_u32_t mask);
void cpu_io_clr_bits32   (cpu_addr_t addr, cpu_u32_t mask);
void cpu_io_mod_bits32   (cpu_addr_t addr, cpu_u32_t mask,
                           cpu_u32_t val);

/* -- Polling helpers ---------------------------------------- */
int  cpu_io_poll_set32   (cpu_addr_t addr, cpu_u32_t mask,
                           cpu_u32_t timeout_us);
int  cpu_io_poll_clr32   (cpu_addr_t addr, cpu_u32_t mask,
                           cpu_u32_t timeout_us);

/* -- x86 port I/O (no-ops on ARM/RISC-V) -------------------- */
#if defined(UIOX_ARCH_X86_64)
void      cpu_io_port_write8 (cpu_u16_t port, cpu_u8_t  val);
void      cpu_io_port_write16(cpu_u16_t port, cpu_u16_t val);
void      cpu_io_port_write32(cpu_u16_t port, cpu_u32_t val);
cpu_u8_t  cpu_io_port_read8  (cpu_u16_t port);
cpu_u16_t cpu_io_port_read16 (cpu_u16_t port);
cpu_u32_t cpu_io_port_read32 (cpu_u16_t port);
#else
#define cpu_io_port_write8(p,v)   do{}while(0)
#define cpu_io_port_write16(p,v)  do{}while(0)
#define cpu_io_port_write32(p,v)  do{}while(0)
#define cpu_io_port_read8(p)      (0u)
#define cpu_io_port_read16(p)     (0u)
#define cpu_io_port_read32(p)     (0u)
#endif

#endif /* CPU_IO_H */
