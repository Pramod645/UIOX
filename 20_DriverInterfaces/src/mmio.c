/*
 * src/mmio.c
 *
 * Memory-mapped I/O register access layer.
 *
 * On real hardware the mmio_readNN / mmio_writeNN functions emit
 * the correct load/store + barrier sequence for each ISA.
 *
 * On a hosted build (Linux / macOS) the physical address is resolved
 * to an offset inside a simulated byte array so the same code can be
 * unit-tested without a physical target.
 *
 * Architecture-specific paths:
 *   ARM64   — volatile pointer + dmb ish via cpu_mb()
 *   ARM32   — volatile pointer + dmb via cpu_mb()
 *   x86_64  — volatile pointer (TSO model) + compiler barrier
 *             port I/O via inb/outb inline asm
 */

 #include "mmio.h"
 #include "cpu.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 /* =============================================================
  * Simulated MMIO region table
  * ============================================================= */
 static mmio_region_t  regions[MMIO_NUM_REGIONS];
 static int            region_count = 0;
 
 /* x86_64 simulated port I/O space (64 KB) */
 static uint8_t port_space[0x10000];
 
 /* =============================================================
  * mmio_init
  * ============================================================= */
 void mmio_init(void)
 {
     memset(regions,    0, sizeof regions);
     memset(port_space, 0, sizeof port_space);
     region_count = 0;
 
     /* Pre-register well-known MMIO regions */
     mmio_region_register(MMIO_UART_BASE,  "uart");
     mmio_region_register(MMIO_TIMER_BASE, "timer");
     mmio_region_register(MMIO_INTC_BASE,  "intc");
     mmio_region_register(MMIO_DMA_BASE,   "dma");
     mmio_region_register(MMIO_DISK_BASE,  "disk");
 
     printf("[mmio] init: %d regions registered  arch=%s\n",
            region_count, UIOX_ARCH_NAME);
 }
 
 /* =============================================================
  * mmio_region_register
  * ============================================================= */
 int mmio_region_register(phys_addr_t base, const char *name)
 {
     if (region_count >= MMIO_NUM_REGIONS) {
         fprintf(stderr, "[mmio] region table full\n");
         return HW_ERR_RANGE;
     }
     regions[region_count].mr_base = base;
     regions[region_count].mr_name = name;
     memset(regions[region_count].mr_data, 0, MMIO_REGION_SIZE);
     region_count++;
     printf("[mmio] registered '%s' @ 0x%08lx\n",
            name, (unsigned long)base);
     return HW_OK;
 }
 
 /* =============================================================
  * mmio_region_find
  * ============================================================= */
 mmio_region_t *mmio_region_find(phys_addr_t addr)
 {
     int i;
     for (i = 0; i < region_count; i++) {
         phys_addr_t base = regions[i].mr_base;
         if (addr >= base && addr < base + MMIO_REGION_SIZE)
             return &regions[i];
     }
     return NULL;
 }
 
 /* =============================================================
  * Internal: resolve a physical address to a simulation pointer.
  * Returns NULL if the address is not covered by any region.
  * ============================================================= */
 static uint8_t *mmio_resolve(phys_addr_t addr)
 {
     mmio_region_t *r = mmio_region_find(addr);
     if (!r) {
         fprintf(stderr, "[mmio] unmapped address 0x%08lx\n",
                 (unsigned long)addr);
         return NULL;
     }
     return r->mr_data + (addr - r->mr_base);
 }
 
 /* =============================================================
  * 8-bit access
  * ============================================================= */
 void mmio_write8(phys_addr_t addr, uint8_t val)
 {
     uint8_t *p = mmio_resolve(addr);
     if (!p) return;
 
 #if defined(UIOX_ARCH_ARM64)
     /* On real ARM64: STR B + DMB ISH */
     __asm__ volatile("" ::: "memory");   /* compiler barrier */
 #elif defined(UIOX_ARCH_ARM32)
     /* On real ARM32: STRB + DMB */
     __asm__ volatile("" ::: "memory");
 #else
     /* x86_64: compiler barrier sufficient (TSO) */
     __asm__ volatile("" ::: "memory");
 #endif
 
     *p = val;
     cpu_wmb();
 
     printf("  [mmio] W8  [0x%08lx] ← 0x%02x\n",
            (unsigned long)addr, val);
 }
 
 uint8_t mmio_read8(phys_addr_t addr)
 {
     uint8_t *p = mmio_resolve(addr);
     if (!p) return 0xFF;
     cpu_rmb();
     uint8_t val = *p;
     printf("  [mmio] R8  [0x%08lx] → 0x%02x\n",
            (unsigned long)addr, val);
     return val;
 }
 
 /* =============================================================
  * 16-bit access
  * ============================================================= */
 void mmio_write16(phys_addr_t addr, uint16_t val)
 {
     uint8_t *p = mmio_resolve(addr);
     if (!p) return;
     if (addr & 1) { fprintf(stderr,"[mmio] misaligned W16\n"); return; }
     cpu_wmb();
     *(uint16_t *)p = val;
     printf("  [mmio] W16 [0x%08lx] ← 0x%04x\n",
            (unsigned long)addr, val);
 }
 
 uint16_t mmio_read16(phys_addr_t addr)
 {
     uint8_t *p = mmio_resolve(addr);
     if (!p) return 0xFFFF;
     if (addr & 1) { fprintf(stderr,"[mmio] misaligned R16\n"); return 0xFFFF; }
     cpu_rmb();
     uint16_t val = *(uint16_t *)p;
     printf("  [mmio] R16 [0x%08lx] → 0x%04x\n",
            (unsigned long)addr, val);
     return val;
 }
 
 /* =============================================================
  * 32-bit access  (most device registers)
  * ============================================================= */
 void mmio_write32(phys_addr_t addr, uint32_t val)
 {
     uint8_t *p = mmio_resolve(addr);
     if (!p) return;
     if (addr & 3) { fprintf(stderr,"[mmio] misaligned W32\n"); return; }
 
 #if defined(UIOX_ARCH_ARM64)
     __asm__ volatile("" ::: "memory");
 #elif defined(UIOX_ARCH_ARM32)
     __asm__ volatile("" ::: "memory");
 #else
     __asm__ volatile("" ::: "memory");
 #endif
 
     cpu_wmb();
     *(uint32_t *)p = val;
 
     printf("  [mmio] W32 [0x%08lx] ← 0x%08x\n",
            (unsigned long)addr, val);
 }
 
 uint32_t mmio_read32(phys_addr_t addr)
 {
     uint8_t *p = mmio_resolve(addr);
     if (!p) return 0xFFFFFFFFu;
     if (addr & 3) { fprintf(stderr,"[mmio] misaligned R32\n"); return 0; }
     cpu_rmb();
     uint32_t val = *(uint32_t *)p;
     printf("  [mmio] R32 [0x%08lx] → 0x%08x\n",
            (unsigned long)addr, val);
     return val;
 }
 
 /* =============================================================
  * 64-bit access
  * ============================================================= */
 void mmio_write64(phys_addr_t addr, uint64_t val)
 {
     uint8_t *p = mmio_resolve(addr);
     if (!p) return;
     if (addr & 7) { fprintf(stderr,"[mmio] misaligned W64\n"); return; }
     cpu_wmb();
     *(uint64_t *)p = val;
     printf("  [mmio] W64 [0x%08lx] ← 0x%016llx\n",
            (unsigned long)addr, (unsigned long long)val);
 }
 
 uint64_t mmio_read64(phys_addr_t addr)
 {
     uint8_t *p = mmio_resolve(addr);
     if (!p) return 0xFFFFFFFFFFFFFFFFull;
     if (addr & 7) { fprintf(stderr,"[mmio] misaligned R64\n"); return 0; }
     cpu_rmb();
     uint64_t val = *(uint64_t *)p;
     printf("  [mmio] R64 [0x%08lx] → 0x%016llx\n",
            (unsigned long)addr, (unsigned long long)val);
     return val;
 }
 
 /* =============================================================
  * x86_64 port I/O
  * ============================================================= */
 void port_outb(uint16_t port, uint8_t val)
 {
 #if defined(UIOX_ARCH_X86_64)
     __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
 #else
     port_space[port] = val;
 #endif
     printf("  [port] outb port=0x%04x val=0x%02x\n", port, val);
 }
 
 void port_outw(uint16_t port, uint16_t val)
 {
 #if defined(UIOX_ARCH_X86_64)
     __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
 #else
     *(uint16_t *)&port_space[port] = val;
 #endif
 }
 
 void port_outl(uint16_t port, uint32_t val)
 {
 #if defined(UIOX_ARCH_X86_64)
     __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
 #else
     *(uint32_t *)&port_space[port] = val;
 #endif
 }
 
 uint8_t port_inb(uint16_t port)
 {
     uint8_t val;
 #if defined(UIOX_ARCH_X86_64)
     __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
 #else
     val = port_space[port];
 #endif
     printf("  [port] inb  port=0x%04x → 0x%02x\n", port, val);
     return val;
 }
 
 uint16_t port_inw(uint16_t port)
 {
 #if defined(UIOX_ARCH_X86_64)
     uint16_t val;
     __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
     return val;
 #else
     return *(uint16_t *)&port_space[port];
 #endif
 }
 
 uint32_t port_inl(uint16_t port)
 {
 #if defined(UIOX_ARCH_X86_64)
     uint32_t val;
     __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
     return val;
 #else
     return *(uint32_t *)&port_space[port];
 #endif
 }
 
 /* =============================================================
  * Memory barriers (thin wrappers — real ISB/DSB/DMB on target)
  * ============================================================= */
 void hw_mb (void) { cpu_mb();  }
 void hw_rmb(void) { cpu_rmb(); }
 void hw_wmb(void) { cpu_wmb(); }
 
 /* =============================================================
  * DMA descriptor management
  * ============================================================= */
 void dma_desc_init(dma_desc_t *descs, int count)
 {
     memset(descs, 0, (size_t)count * sizeof(dma_desc_t));
 }
 
 int dma_submit(phys_addr_t dma_base, dma_desc_t *descs, int count)
 {
     int i;
     printf("[dma] submit %d descriptors to controller 0x%08lx\n",
            count, (unsigned long)dma_base);
 
     for (i = 0; i < count; i++) {
         /*
          * Program descriptor i into the DMA controller registers.
          * On a real SoC: write src/dst/len into MMIO registers,
          * then set the GO bit in the control register.
          *
          * Simulated: write descriptor fields via mmio_write32.
          */
         phys_addr_t desc_reg = dma_base + (phys_addr_t)(i * 0x10);
         mmio_write32(desc_reg + 0x00, (uint32_t)descs[i].dma_src);
         mmio_write32(desc_reg + 0x04, (uint32_t)descs[i].dma_dst);
         mmio_write32(desc_reg + 0x08, descs[i].dma_len);
         mmio_write32(desc_reg + 0x0C, descs[i].dma_flags);
 
         printf("  [dma] desc[%d] src=0x%08lx dst=0x%08lx len=%u\n",
                i,
                (unsigned long)descs[i].dma_src,
                (unsigned long)descs[i].dma_dst,
                descs[i].dma_len);
     }
 
     /* Kick GO bit in DMA control register (simulated) */
     mmio_write32(dma_base + 0x100, 0x1);
     return HW_OK;
 }
 
 int dma_poll_done(dma_desc_t *descs, int count, uint32_t timeout_us)
 {
     int i;
     uint32_t waited = 0;
 
     /* Simulated: mark all descriptors done immediately */
     for (i = 0; i < count; i++)
         descs[i].dma_flags |= DMA_FLAG_DONE;
 
     printf("[dma] poll_done: %d descriptors done (waited %u us)\n",
            count, waited);
     (void)timeout_us;
     return HW_OK;
 }
 
 void dma_print(const dma_desc_t *descs, int count)
 {
     int i;
     printf("[dma] descriptor chain (%d entries):\n", count);
     for (i = 0; i < count; i++) {
         printf("  [%d] src=0x%08lx dst=0x%08lx len=%-6u "
                "flags=0x%02x %s%s\n",
                i,
                (unsigned long)descs[i].dma_src,
                (unsigned long)descs[i].dma_dst,
                descs[i].dma_len,
                descs[i].dma_flags,
                (descs[i].dma_flags & DMA_FLAG_DONE)  ? "DONE "  : "",
                (descs[i].dma_flags & DMA_FLAG_ERROR) ? "ERROR" : "");
     }
 }
 