#ifndef UIOX_MMIO_H
#define UIOX_MMIO_H

/*
 * mmio.h
 *
 * Memory-mapped I/O register access for ARM64, ARM32, and x86_64.
 *
 * On real hardware these inline functions emit the correct load/store
 * barrier instructions.  On the hosted simulation they read/write the
 * backing byte array inside mmio_region_t.
 *
 * ARM64:  ldr/str with appropriate memory barrier (dmb ish)
 * ARM32:  ldr/str with dsb / dmb
 * x86_64: volatile pointer read/write (x86 TSO makes barriers implicit
 *         for ordinary stores; compiler barrier via asm volatile)
 */

#include "hw_types.h"

/* =============================================================
 * Simulated MMIO region table API
 * ============================================================= */

/* Initialise all simulated MMIO regions (call once at startup) */
void mmio_init(void);

/*
 * mmio_region_register
 * Add a simulated MMIO region at physical address 'base'.
 * Returns HW_OK or HW_ERR_RANGE if the table is full.
 */
int  mmio_region_register(phys_addr_t base, const char *name);

/*
 * mmio_region_find
 * Locate the simulated region that covers 'addr'.
 * Returns a pointer to the region or NULL if not found.
 */
mmio_region_t *mmio_region_find(phys_addr_t addr);

/* =============================================================
 * 8-bit MMIO access
 * ============================================================= */
void    mmio_write8 (phys_addr_t addr, uint8_t  val);
uint8_t mmio_read8  (phys_addr_t addr);

/* =============================================================
 * 16-bit MMIO access
 * ============================================================= */
void     mmio_write16(phys_addr_t addr, uint16_t val);
uint16_t mmio_read16 (phys_addr_t addr);

/* =============================================================
 * 32-bit MMIO access  (most common for device registers)
 * ============================================================= */
void     mmio_write32(phys_addr_t addr, uint32_t val);
uint32_t mmio_read32 (phys_addr_t addr);

/* =============================================================
 * 64-bit MMIO access
 * ============================================================= */
void     mmio_write64(phys_addr_t addr, uint64_t val);
uint64_t mmio_read64 (phys_addr_t addr);

/* =============================================================
 * Architecture-specific port I/O  (x86_64 only)
 *
 * On ARM targets these are no-ops because ARM uses MMIO exclusively.
 * On x86_64 they emit inb / outb instructions (or their simulation).
 * ============================================================= */
void    port_outb(uint16_t port, uint8_t  val);
void    port_outw(uint16_t port, uint16_t val);
void    port_outl(uint16_t port, uint32_t val);
uint8_t  port_inb(uint16_t port);
uint16_t port_inw(uint16_t port);
uint32_t port_inl(uint16_t port);

/* =============================================================
 * Memory barriers
 *
 * Wrappers that emit the correct barrier instruction per ISA.
 *   hw_mb()   — full memory barrier (read + write)
 *   hw_rmb()  — read  memory barrier
 *   hw_wmb()  — write memory barrier
 * ============================================================= */
void hw_mb (void);
void hw_rmb(void);
void hw_wmb(void);

/* =============================================================
 * DMA descriptor management
 * ============================================================= */

/* Initialise a descriptor array */
void dma_desc_init(dma_desc_t *descs, int count);

/*
 * dma_submit
 * Program the DMA controller with the descriptor chain and start
 * the transfer.  Returns HW_OK or a negative HW_ERR_* code.
 */
int  dma_submit(phys_addr_t dma_base,
                dma_desc_t *descs, int count);

/*
 * dma_poll_done
 * Busy-wait until all descriptors in the chain are marked DONE
 * or until 'timeout_us' microseconds elapse.
 * Returns HW_OK on success, HW_ERR_TIMEOUT on expiry.
 */
int  dma_poll_done(dma_desc_t *descs, int count,
                   uint32_t timeout_us);

/* Debug: print descriptor chain state */
void dma_print(const dma_desc_t *descs, int count);

#endif /* UIOX_MMIO_H */
