#ifndef UIOX_ARCH_MAIN_H
#define UIOX_ARCH_MAIN_H

/*
 * 10_BSP/uiox_arch_main.h
 *
 * Single header the kernel (or entry stub) includes.
 *
 * Build modes (selected by Makefile flag):
 *
 *   Static  (default, DYNAMIC_KERNEL not set):
 *     BSP + kernel compiled into one image.
 *     Stage 8 calls extern uiox_kernel_main() directly.
 *     Zero loader code compiled in.
 *
 *   Dynamic (DYNAMIC_KERNEL=1 → -DUIOX_DYNAMIC_KERNEL_LOAD):
 *     BSP is a standalone ELF.
 *     Stage 8 calls uiox_kernel_load() → verify() → jump().
 *     Full loader compiled in.
 *
 * The switch lives in uiox_soc_main.c Stage 8.
 * This header and uiox_arch_main.c are identical in both modes.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unified entry point called by uiox_kernel_main() or the
 * BSP entry stub.
 *   1. arch_init()       — ISA-level setup
 *   2. uiox_soc_init()   — SoC pipeline (Stage 0–8)
 *      Stage 8 is either a direct kernel call (static)
 *      or a load+jump sequence (dynamic).
 *
 * @param dtb_pa  Physical address of Device Tree Blob (0 if none).
 * @return        0 on success (static mode only — dynamic never returns).
 */
int uiox_arch_main(unsigned long dtb_pa);

/** DTB PA stored by uiox_arch_main for higher kernel layers. */
extern unsigned long uiox_arch_dtb_pa;

/** Accessor for uiox_arch_dtb_pa. */
unsigned long uiox_arch_dtb_get(void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_ARCH_MAIN_H */
