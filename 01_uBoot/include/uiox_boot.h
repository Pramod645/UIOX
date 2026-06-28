/**
 * @file  uiox_boot.h
 * @brief UIOX Bootloader — single master include for all boot APIs.
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_BOOT_H
 #define UIOX_BOOT_H
 
 #include "uiox_boot_types.h"
 #include "uiox_boot_hw.h"
 #include "uiox_boot_mem.h"
 #include "uiox_boot_console.h"
 #include "uiox_boot_fs.h"
 #include "uiox_boot_verify.h"
 #include "uiox_boot_handoff.h"
 
 #define UIOX_BOOT_VERSION_STR   "UIOX Bootloader v1.0"
 #define UIOX_BOOT_URL           "github.com/Pramod645/UIOX"
 
 #endif /* UIOX_BOOT_H */
 