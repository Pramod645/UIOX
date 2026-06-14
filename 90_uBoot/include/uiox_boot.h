#ifndef UIOX_BOOT_H
#define UIOX_BOOT_H
/*
 * uiox_boot.h  —  Master include for the UIOX bootloader.
 */
#include "uiox_boot_types.h"
#include "uiox_boot_hw.h"
#include "uiox_boot_mem.h"
#include "uiox_boot_console.h"
#include "uiox_boot_fs.h"
#include "uiox_boot_verify.h"
#include "uiox_boot_handoff.h"

/* C entry point called from every arch stub */
int uiox_boot_main(uboot_u64_t dtb_or_info, uboot_u32_t boot_flags);

#endif /* UIOX_BOOT_H */
