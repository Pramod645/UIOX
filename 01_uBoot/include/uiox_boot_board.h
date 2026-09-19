/*
 * 01_uBoot/include/uiox_boot_board.h
 *
 * Board description — the single place a physical board's addresses,
 * clocks, and storage live. Every arch hw file reads from here instead
 * of hardcoding QEMU bases.
 *
 * A board port = filling in one uiox_board_t. No other file changes.
 */
#ifndef UIOX_BOOT_BOARD_H
#define UIOX_BOOT_BOARD_H

#include "uiox_boot_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UIOX_STORAGE_NONE = 0,
    UIOX_STORAGE_SDMMC,     /* eMMC / SD card                */
    UIOX_STORAGE_SPI_NOR,   /* QSPI flash                    */
    UIOX_STORAGE_NVME,      /* PCIe NVMe                     */
    UIOX_STORAGE_AHCI,      /* SATA                          */
    UIOX_STORAGE_VIRTIO,    /* hypervisor — NOT real silicon */
} uiox_storage_kind_t;

typedef struct {
    const char *name;

    /* Console */
    uint64_t uart_base;

    /* Interrupt controller */
    uint64_t gic_dist_base;
    uint64_t gic_cpu_base;
    uint64_t vic_base;

    /* Timers */
    uint64_t timer_base;
    uint64_t clk_base;          /* clock/PLL controller       */

    /* Primary boot storage */
    uiox_storage_kind_t storage;
    uint64_t storage_base;
    uint32_t storage_irq;

    /* Load addresses */
    uint64_t kernel_load_pa;
    uint64_t kernel_entry_pa;
    uint64_t args_pa;
    uint64_t dtb_pa;

    /* DRAM */
    uint64_t dram_base;
    uint64_t dram_size;

    /* Pointer to the board's early init (clock/power/pinctrl). */
    uiox_boot_err_t (*bringup)(void);
} uiox_board_t;

/* The selected board — defined exactly once per board file. */
const uiox_board_t *uiox_board_get(void);

/* Board bring-up: clock/PLL, power domains, pin-mux.
 * Called by the HAL before the UART is touched. */
uiox_boot_err_t uiox_board_bringup(void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_BOOT_BOARD_H */
