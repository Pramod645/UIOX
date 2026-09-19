/*
 * 01_uBoot/include/uiox_boot_board_select.h
 * One board per build. Define exactly one UIOX_BOARD_* flag on the
 * command line; the Makefile does this from BOARD=<name>.
 */
#ifndef UIOX_BOOT_BOARD_SELECT_H
#define UIOX_BOOT_BOARD_SELECT_H

#include "uiox_boot_board.h"

#if defined(UIOX_BOARD_QEMU_ARM64)
  /* board_qemu_arm64.c compiled */
#elif defined(UIOX_BOARD_QEMU_ARM32)
  /* board_qemu_arm32.c compiled */
#elif defined(UIOX_BOARD_QEMU_RISCV64)
  /* board_qemu_riscv64.c compiled */
#elif defined(UIOX_BOARD_QEMU_X86_64)
  /* board_qemu_x86_64.c compiled */
#elif defined(UIOX_BOARD_GENERIC_ARM64)
  /* board_generic_arm64.c compiled */
#elif defined(UIOX_BOARD_GENERIC_ARM32)
  /* board_generic_arm32.c compiled */
#elif defined(UIOX_BOARD_GENERIC_RISCV64)
  /* board_generic_riscv64.c compiled */
#elif defined(UIOX_BOARD_GENERIC_X86_64)
  /* board_generic_x86_64.c compiled */
#else
#  error "no board selected — define one UIOX_BOARD_* flag (make BOARD=...)"
#endif

#endif /* UIOX_BOOT_BOARD_SELECT_H */
