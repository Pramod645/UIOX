/*
 * 01_uBoot/board/board_generic_arm64.c
 *
 * Generic ARM64 board descriptor.
 * Hardware addresses come from uiox_soc_map.h (SOC_* macros) — NOT
 * hardcoded here. Board bring-up only carries the init sequence.
 *
 * Load-layout addresses (_kern_load_base, _args_base) come from the
 * linker script. Neither belongs in this file.
 */
#include "uiox_boot.h"
#include "uiox_boot_board.h"
#include "uiox_soc_map.h"        /* SOC_UART0_BASE, SOC_GIC_*, ... */

extern char _boot_load_base[];
extern char _kern_load_base[];
extern char _args_base[];

static uiox_boot_err_t bringup_clocks(void)
{
    /* TODO(board): lock PLLs using SOC_CLK_BASE from the soc map. */
    return UIOX_BOOT_OK;
}
static uiox_boot_err_t bringup_dram(void)   { return UIOX_BOOT_OK; }
static uiox_boot_err_t bringup_power(void)  { return UIOX_BOOT_OK; }
static uiox_boot_err_t bringup_pinctrl(void){ return UIOX_BOOT_OK; }

uiox_boot_err_t uiox_board_bringup(void)
{
    if (bringup_clocks()  != UIOX_BOOT_OK) return UIOX_BOOT_ERR_IO;
    if (bringup_dram()    != UIOX_BOOT_OK) return UIOX_BOOT_ERR_IO;
    if (bringup_power()   != UIOX_BOOT_OK) return UIOX_BOOT_ERR_IO;
    if (bringup_pinctrl() != UIOX_BOOT_OK) return UIOX_BOOT_ERR_IO;
    return UIOX_BOOT_OK;
}

static const uiox_board_t s_board = {
    .name          = UIOX_BOARD_STR,
    .uart_base     = SOC_UART0_BASE,       /* macro, not a literal */
    .gic_dist_base = SOC_GIC_DIST_BASE,
    .gic_cpu_base  = SOC_GIC_CPU_BASE,
    .vic_base      = 0,
    .timer_base    = SOC_TIMER_BASE,
    .clk_base      = SOC_CLK_BASE,         /* SOC map TODO value   */
    .storage       = UIOX_STORAGE_SDMMC,   /* TODO: set real kind  */
    .storage_base  = SOC_STORAGE_BASE,
    .storage_irq   = 0,
    .kernel_load_pa  = 0,   /* from _kern_load_base */
    .kernel_entry_pa = 0,
    .args_pa         = 0,   /* from _args_base      */
    .dtb_pa          = 0,   /* from prior stage     */
    .dram_base       = SOC_DRAM_BASE,
    .dram_size       = SOC_DRAM_SIZE,
    .bringup       = uiox_board_bringup,
};

const uiox_board_t *uiox_board_get(void) { return &s_board; }
