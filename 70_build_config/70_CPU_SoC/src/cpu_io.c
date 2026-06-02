/*
 * cpu_io.c - Device access (MMIO + port I/O)
 */
#include "../include/cpu_io.h"
#include "../include/cpu_regs.h"
#include "../include/cpu_timer.h"
#include <string.h>
#include <stdio.h>

cpu_mmio_region_t g_mmio_regions[CPU_IO_MAX_REGIONS];
cpu_u32_t         g_mmio_count = 0;

int cpu_io_register_region(const char *name, cpu_addr_t base,
                             cpu_u64_t size, cpu_u32_t flags)
{
    if (g_mmio_count >= CPU_IO_MAX_REGIONS) return CPU_ERR;
    cpu_mmio_region_t *r = &g_mmio_regions[g_mmio_count++];
    strncpy(r->name, name, sizeof(r->name) - 1);
    r->base   = base;
    r->size   = size;
    r->flags  = flags;
    r->mapped = CPU_TRUE;
    return CPU_OK;
}

cpu_mmio_region_t *cpu_io_find_region(const char *name)
{
    for (cpu_u32_t i = 0; i < g_mmio_count; i++)
        if (strncmp(g_mmio_regions[i].name, name,
                    sizeof(g_mmio_regions[i].name)) == 0)
            return &g_mmio_regions[i];
    return NULL;
}

void cpu_io_print_regions(void)
{
    printf("[io] MMIO regions (%u):\n", g_mmio_count);
    for (cpu_u32_t i = 0; i < g_mmio_count; i++) {
        cpu_mmio_region_t *r = &g_mmio_regions[i];
        printf("  [%2u] %-16s  base=0x%016llx  size=0x%08llx\n",
               i, r->name,
               (unsigned long long)r->base,
               (unsigned long long)r->size);
    }
}

void cpu_io_write8 (cpu_addr_t a, cpu_u8_t  v)
{ cpu_mmio_write8(a, v); }
void cpu_io_write16(cpu_addr_t a, cpu_u16_t v)
{ *((volatile cpu_u16_t *)(cpu_addr_t)(a)) = v; cpu_dsb(); }
void cpu_io_write32(cpu_addr_t a, cpu_u32_t v)
{ cpu_mmio_write32(a, v); }
void cpu_io_write64(cpu_addr_t a, cpu_u64_t v)
{ cpu_mmio_write64(a, v); }

cpu_u8_t  cpu_io_read8 (cpu_addr_t a) { return cpu_mmio_read8(a); }
cpu_u16_t cpu_io_read16(cpu_addr_t a)
{ cpu_u16_t v = *((volatile cpu_u16_t *)(cpu_addr_t)(a));
  cpu_dsb(); return v; }
cpu_u32_t cpu_io_read32(cpu_addr_t a) { return cpu_mmio_read32(a); }
cpu_u64_t cpu_io_read64(cpu_addr_t a) { return cpu_mmio_read64(a); }

void cpu_io_set_bits32(cpu_addr_t a, cpu_u32_t mask)
{ cpu_io_write32(a, cpu_io_read32(a) | mask); }

void cpu_io_clr_bits32(cpu_addr_t a, cpu_u32_t mask)
{ cpu_io_write32(a, cpu_io_read32(a) & ~mask); }

void cpu_io_mod_bits32(cpu_addr_t a, cpu_u32_t mask, cpu_u32_t val)
{ cpu_io_write32(a, (cpu_io_read32(a) & ~mask) | (val & mask)); }

int cpu_io_poll_set32(cpu_addr_t a, cpu_u32_t mask, cpu_u32_t timeout_us)
{
    cpu_u32_t elapsed = 0;
    while ((cpu_io_read32(a) & mask) != mask) {
        cpu_timer_udelay(1);
        if (++elapsed >= timeout_us) return CPU_ETIMEOUT;
    }
    return CPU_OK;
}

int cpu_io_poll_clr32(cpu_addr_t a, cpu_u32_t mask, cpu_u32_t timeout_us)
{
    cpu_u32_t elapsed = 0;
    while (cpu_io_read32(a) & mask) {
        cpu_timer_udelay(1);
        if (++elapsed >= timeout_us) return CPU_ETIMEOUT;
    }
    return CPU_OK;
}

#if defined(UIOX_ARCH_X86_64)
void      cpu_io_port_write8 (cpu_u16_t p, cpu_u8_t  v) { cpu_outb(p,v); }
void      cpu_io_port_write16(cpu_u16_t p, cpu_u16_t v) { cpu_outw(p,v); }
void      cpu_io_port_write32(cpu_u16_t p, cpu_u32_t v) { cpu_outl(p,v); }
cpu_u8_t  cpu_io_port_read8  (cpu_u16_t p) { return cpu_inb(p); }
cpu_u16_t cpu_io_port_read16 (cpu_u16_t p) { return cpu_inw(p); }
cpu_u32_t cpu_io_port_read32 (cpu_u16_t p) { return cpu_inl(p); }
#endif
