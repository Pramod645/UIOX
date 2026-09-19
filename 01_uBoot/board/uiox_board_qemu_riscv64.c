/*
 * 01_uBoot/board/board_qemu_riscv64.c
 * QEMU -machine virt RISC-V descriptor. Addresses from uiox_soc_map.h.
 */
#include "uiox_boot.h"
#include "uiox_boot_board.h"
#include "uiox_soc_map.h"

extern char _kern_load_base[];
extern char _args_base[];

static uiox_boot_err_t qemu_bringup(void) { return UIOX_BOOT_OK; }

static const uiox_board_t s_board = {
    .name          = UIOX_BOARD_STR,
    .uart_base     = SOC_UART0_BASE,
    .vic_base      = SOC_PLIC_BASE,
    .clk_base      = SOC_CLINT_BASE,
    .storage       = UIOX_STORAGE_VIRTIO,
    .storage_base  = SOC_VIRTIO_BASE,
    .storage_irq   = SOC_VIRTIO_IRQ,
    .kernel_load_pa  = 0,
    .kernel_entry_pa = 0,
    .args_pa         = 0,
    .dtb_pa          = 0,
    .dram_base       = SOC_DRAM_BASE,
    .dram_size       = SOC_DRAM_SIZE,
    .bringup       = qemu_bringup,
};

const uiox_board_t *uiox_board_get(void) { return &s_board; }
