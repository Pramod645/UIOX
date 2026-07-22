/*
 * 10_BSP/include/uiox_bsp.h
 *
 * UIOX Board-Support Package — public API header.
 *
 * Self-contained: no system headers (<stdint.h>, <stddef.h>), no includes
 * from 01_uBoot/ or 02_FwHal/.
 *
 * Base types come from:
 *   10_BSP/03_SoC/include/uiox_base_types.h
 * which is already on the compiler include path via -I$(SOC_DIR)/include
 * in the BSP Makefile.  UIOX_BASETYPES_COMPAT is enabled here so the
 * standard aliases (uint8_t, uint32_t, uint64_t, uintptr_t, size_t, bool)
 * are available throughout this header and all BSP source files without
 * any renames — exactly as if <stdint.h> and <stddef.h> were included.
 *
 * Two build modes:
 *
 *   STATIC  (default)
 *     BSP is linked into the kernel image.
 *     uiox_kernel_main() calls uiox_bsp_init() before any subsystem init.
 *
 *   DYNAMIC (UIOX_BSP_DYNAMIC_BUILD)
 *     BSP is a standalone secondary-bootloader binary.
 *     Primary bootloader jumps to uiox_bsp_entry(); BSP loads the kernel
 *     ELF and hands off via uiox_bsp_jump_to_kernel().
 *
 * Include hierarchy (all BSP-internal, no sibling directories):
 *
 *   uiox_bsp.h
 *     └── uiox_base_types.h   (10_BSP/03_SoC/include/)
 *           └── (no further includes — fully freestanding)
 *
 * @version 1.2.0
 * @date    2026-07-21
 */

 #ifndef UIOX_BSP_H
 #define UIOX_BSP_H
 
 /* ── Base types ──────────────────────────────────────────────────────────────
  * Enable stdint-compatible aliases BEFORE including uiox_base_types.h so
  * that uint8_t / uint32_t / uint64_t / uintptr_t / size_t / bool resolve
  * to the UIOX bare-metal definitions throughout this header and all files
  * that include it.
  *
  * Rule: NEVER mix this with <stdint.h> or <stddef.h> in the same translation
  * unit — the aliases will conflict.  The BSP Makefile passes -nostdinc to
  * enforce this at build time.
  * ─────────────────────────────────────────────────────────────────────────── */
 #ifndef UIOX_BASETYPES_COMPAT
 #  define UIOX_BASETYPES_COMPAT
 #endif
 #include "uiox_base_types.h"   /* 10_BSP/03_SoC/include/uiox_base_types.h   */
 
 /* =========================================================================
  * § 1  Boot-argument types
  *      (previously in 01_uBoot/include/uiox_boot_types.h)
  *
  * Defined here so neither the kernel nor the BSP needs to reach into
  * 01_uBoot/ at compile time.
  * ====================================================================== */
 
 #ifndef UIOX_BOOT_CMDLINE_MAX
 #  define UIOX_BOOT_CMDLINE_MAX     256u
 #endif
 
 #ifndef UIOX_BOOT_MEM_REGIONS_MAX
 #  define UIOX_BOOT_MEM_REGIONS_MAX  16u
 #endif
 
 /* Single contiguous physical memory region */
 typedef struct uiox_mem_region {
     uint64_t  base;        /* physical base address                     */
     uint64_t  size;        /* size in bytes                             */
     uint32_t  type;        /* UIOX_MEM_TYPE_* below                     */
     uint32_t  reserved;
 } uiox_mem_region_t;
 
 #define UIOX_MEM_TYPE_RAM       0x01u   /* general-purpose DRAM          */
 #define UIOX_MEM_TYPE_RESERVED  0x02u   /* firmware / MMIO reserved      */
 #define UIOX_MEM_TYPE_ACPI      0x03u   /* ACPI reclaimable              */
 #define UIOX_MEM_TYPE_NVRAM     0x04u   /* persistent / battery-backed   */
 
 /* Physical memory map — passed from bootloader to kernel */
 typedef struct uiox_mem_map {
     uint32_t          count;
     uint32_t          reserved;
     uiox_mem_region_t regions[UIOX_BOOT_MEM_REGIONS_MAX];
 } uiox_mem_map_t;
 
 /* Full boot-argument block — placed at args_pa by the primary bootloader */
 typedef struct uiox_boot_args {
     uint32_t       magic;           /* UIOX_BOOT_ARGS_MAGIC               */
     uint32_t       version;         /* struct version (currently 1)       */
     uint64_t       kernel_entry;    /* kernel physical entry address      */
     uint64_t       dtb_pa;          /* physical address of DTB            */
     uint64_t       initrd_pa;       /* physical address of initrd (0=none)*/
     uint64_t       initrd_size;     /* initrd size in bytes               */
     uiox_mem_map_t mem_map;         /* physical memory map                */
     char           cmdline[UIOX_BOOT_CMDLINE_MAX]; /* kernel command line */
 } uiox_boot_args_t;
 
 #define UIOX_BOOT_ARGS_MAGIC  0x55494F58u   /* 'UIOX' */
 #define UIOX_BOOT_ARGS_VER    1u
 
 /* =========================================================================
  * § 2  SoC / clock / power types
  *      (previously in 02_FwHal/include/uiox_soc.h)
  *
  * Minimal set needed by the BSP to call uiox_soc_init().
  * Full SoC driver types live in 10_BSP/03_SoC/include/uiox_soc_types.h.
  * ====================================================================== */
 
 #define UIOX_SOC_OK          0
 #define UIOX_SOC_ERR_CLOCK  -1
 #define UIOX_SOC_ERR_PM     -2
 #define UIOX_SOC_ERR_RESET  -3
 
 /* Power-management state */
 typedef enum {
     UIOX_PM_STATE_ON      = 0,
     UIOX_PM_STATE_SUSPEND = 1,
     UIOX_PM_STATE_OFF     = 2,
 } uiox_pm_state_t;
 
 /* Minimal SoC descriptor — populated by uiox_soc_init() */
 typedef struct uiox_soc_info {
     uint32_t  vendor_id;
     uint32_t  chip_id;
     uint32_t  revision;
     uint32_t  cpu_freq_hz;    /* boot CPU frequency after PLL lock       */
     uint32_t  bus_freq_hz;
     uint32_t  reserved;
 } uiox_soc_info_t;
 
 /* =========================================================================
  * § 3  Early UART / console types
  *      (previously in 02_FwHal/include/uiox_fw_uart.h)
  *
  * Minimum needed for early_puts()-style output before the MMU is on.
  * Full UART driver lives in 10_BSP/03_SoC/src/.
  * ====================================================================== */
 
 typedef enum {
     UIOX_UART_PORT_0 = 0,
     UIOX_UART_PORT_1 = 1,
 } uiox_uart_port_t;
 
 #define UIOX_UART_BAUD_115200  115200u
 #define UIOX_UART_BAUD_921600  921600u
 
 /* =========================================================================
  * § 4  Fast-boot / milestone context
  *      (previously in 50_UIX/13_fboot)
  *
  * The BSP calls these as timing milestones.  Implementation lives in
  * 50_UIX/13_fboot; BSP only needs the context struct and mode enum.
  * ====================================================================== */
 
 typedef enum {
     UIOX_FB_MODE_COLD  = 0,   /* full cold-boot sequence                 */
     UIOX_FB_MODE_WARM  = 1,   /* warm reboot (no HW re-init)             */
     UIOX_FB_MODE_KEXEC = 2,   /* kexec handoff                           */
 } uiox_fb_mode_t;
 
 typedef struct uiox_fb_master_ctx {
     uint64_t       t_entry_ns;    /* timestamp: BSP/kernel entry          */
     uint64_t       t_shell_ns;    /* timestamp: first shell prompt ready  */
     uint64_t       budget_ns;     /* target shell-ready budget            */
     uiox_fb_mode_t mode;
     uint32_t       reserved;
 } uiox_fb_master_ctx_t;
 
 /* Implemented in 50_UIX/13_fboot — extern from BSP's perspective */
 extern void uiox_fb_init(uiox_fb_master_ctx_t *ctx,
                           uiox_fb_mode_t        mode,
                           uint64_t              budget_ns);
 extern void uiox_fb_shell_ready(uiox_fb_master_ctx_t *ctx);
 extern void uiox_fb_report(const uiox_fb_master_ctx_t *ctx);
 
 /* =========================================================================
  * § 5  BSP configuration and return codes
  * ====================================================================== */
 
 #define UIOX_BSP_OK           0
 #define UIOX_BSP_ERR_ARCH    -1
 #define UIOX_BSP_ERR_SOC     -2
 #define UIOX_BSP_ERR_MEM     -3
 #define UIOX_BSP_ERR_DTB     -4
 #define UIOX_BSP_ERR_LOAD    -5   /* dynamic build: kernel ELF load failed */
 
 /* BSP configuration block — filled by the kernel (static) or bsp_entry_c (dynamic) */
 typedef struct uiox_bsp_config {
     uint64_t  dtb_pa;          /* physical address of Device Tree Blob    */
     uint64_t  args_pa;         /* physical address of uiox_boot_args_t    */
     uint64_t  kernel_entry;    /* kernel entry PA (dynamic build)         */
     uint64_t  kernel_load_pa;  /* DMA destination for kernel ELF (dynamic)*/
     uint32_t  flags;           /* UIOX_BSP_FL_* below                     */
     uint32_t  reserved;
 } uiox_bsp_config_t;
 
 #define UIOX_BSP_FL_SILENT    (1u << 0)  /* suppress early UART output    */
 #define UIOX_BSP_FL_NO_MMU    (1u << 1)  /* skip MMU enable (debug)       */
 #define UIOX_BSP_FL_DYN_LOAD  (1u << 2)  /* dynamic: load kernel ELF      */
 
 /* =========================================================================
  * § 6  BSP public API
  * ====================================================================== */
 
 /*
  * uiox_bsp_init() — static-build entry point, called from uiox_kernel_main().
  * Runs arch_init() then uiox_soc_init() in the correct order.
  * Returns UIOX_BSP_OK or a negative error code.
  */
 int uiox_bsp_init(const uiox_bsp_config_t *cfg);
 
 /*
  * uiox_bsp_late_init() — optional second phase after MMU is enabled.
  */
 int uiox_bsp_late_init(const uiox_bsp_config_t *cfg);
 
 /*
  * uiox_bsp_entry() — dynamic-build entry point.
  * Primary bootloader jumps here with the same register convention as
  * uiox_kernel_main().  Never returns.
  */
 void __attribute__((noreturn)) uiox_bsp_entry(void);
 
 /*
  * uiox_bsp_jump_to_kernel() — final hand-off from BSP to kernel.
  * Drains write-buffers then branches to kernel_entry.  Never returns.
  */
 void __attribute__((noreturn))
 uiox_bsp_jump_to_kernel(uint64_t kernel_entry,
                          uint64_t dtb_pa,
                          uint64_t args_pa);
 
 /* Returns the active config after uiox_bsp_init() or uiox_bsp_entry(). */
 const uiox_bsp_config_t *uiox_bsp_get_config(void);
 
 #endif /* UIOX_BSP_H */
 