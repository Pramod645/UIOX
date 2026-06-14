#ifndef UIOX_BOOT_HW_H
#define UIOX_BOOT_HW_H
/*
 * uiox_boot_hw.h - Hardware Abstraction Layer for the UIOX bootloader.
 * One ops vtable; each arch provides its own implementation.
 */
#include "uiox_boot_types.h"

/* -- UART types --------------------------------------------- */
typedef enum {
    UBOOT_UART_PL011  = 0,
    UBOOT_UART_16550  = 1,
    UBOOT_UART_SIFIVE = 2,
} uboot_uart_type_t;

/* -- HW ops vtable ------------------------------------------ */
typedef struct uboot_hw_ops {
    /* one-time platform init                                    */
    int  (*init)         (void);
    /* UART single-byte TX (spin until FIFO not full)            */
    void (*uart_putc)    (char c);
    /* return CPU ID (core number / hart ID / APIC ID)           */
    uboot_u32_t (*cpu_id)(void);
    /* data + instruction cache clean/invalidate                  */
    void (*cache_flush)  (uboot_addr_t start, uboot_size_t len);
    /* data memory barrier                                        */
    void (*mem_barrier)  (void);
    /* CPU idle (WFI / HLT)                                      */
    void (*cpu_idle)     (void);
    /* system reset                                               */
    void (*reset)        (void);
    /* MMIO 32-bit read / write                                   */
    uboot_u32_t (*mmio_read32) (uboot_addr_t addr);
    void        (*mmio_write32)(uboot_addr_t addr, uboot_u32_t val);
} uboot_hw_ops_t;

/* -- Global ops pointer (set in arch hw_init) --------------- */
extern const uboot_hw_ops_t *g_hw_ops;

/* -- Inline wrappers ---------------------------------------- */
static inline void uboot_uart_putc(char c)
{ if (g_hw_ops && g_hw_ops->uart_putc) g_hw_ops->uart_putc(c); }

static inline uboot_u32_t uboot_cpu_id(void)
{ return g_hw_ops ? g_hw_ops->cpu_id() : 0u; }

static inline void uboot_cache_flush(uboot_addr_t a, uboot_size_t n)
{ if (g_hw_ops && g_hw_ops->cache_flush) g_hw_ops->cache_flush(a,n); }

static inline void uboot_mem_barrier(void)
{ if (g_hw_ops && g_hw_ops->mem_barrier) g_hw_ops->mem_barrier(); }

static inline void uboot_cpu_idle(void)
{ if (g_hw_ops && g_hw_ops->cpu_idle) g_hw_ops->cpu_idle(); }

static inline void uboot_reset(void)
{ if (g_hw_ops && g_hw_ops->reset) g_hw_ops->reset(); for(;;); }

static inline uboot_u32_t uboot_mmio_read32(uboot_addr_t a)
{ return g_hw_ops ? g_hw_ops->mmio_read32(a) : 0u; }

static inline void uboot_mmio_write32(uboot_addr_t a, uboot_u32_t v)
{ if (g_hw_ops && g_hw_ops->mmio_write32) g_hw_ops->mmio_write32(a,v); }

/* -- Arch-specific hw_init prototypes ----------------------- */
int uboot_hw_init_arm64(void);
int uboot_hw_init_arm32(void);
int uboot_hw_init_x86  (void);

#endif /* UIOX_BOOT_HW_H */
