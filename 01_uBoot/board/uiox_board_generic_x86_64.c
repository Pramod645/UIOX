/*
 * 01_uBoot/board/board_generic_x86_64.c
 * Generic x86-64 descriptor. Addresses from uiox_soc_map.h.
 */
#include "uiox_boot.h"
#include "uiox_boot_board.h"
#include "uiox_soc_map.h"

extern char _kern_load_base[];
extern char _args_base[];

static uiox_boot_err_t bringup(void) { return UIOX_BOOT_OK; }

static const uiox_board_t s_board = {
    .name          = UIOX_BOARD_STR,
    .uart_base     = SOC_COM1_BASE,        /* port I/O, not MMIO       */
    .vic_base      = SOC_IOAPIC_BASE,
    .timer_base    = SOC_PIT_BASE,
    .clk_base      = 0,
    .storage       = UIOX_STORAGE_AHCI,    /* TODO: set real kind      */
    .storage_base  = 0,                    /* TODO: AHCI ABAR          */
    .storage_irq   = 0,
    .kernel_load_pa  = 0,
    .kernel_entry_pa = 0,
    .args_pa         = 0,
    .dtb_pa          = 0,                  /* x86 has no DTB           */
    .dram_base       = SOC_DRAM_BASE,
    .dram_size       = 0,                  /* from E820                */
    .bringup       = bringup,
};

const uiox_board_t *uiox_board_get(void) { return &s_board; }
uiox_boot_err_t uiox_board_bringup(void) { return s_board.bringup(); }
