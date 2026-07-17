/*
 * 02_FwHal/include/uiox_soc_types.h
 * UIOX SoC abstraction layer — SoC ID enumeration, capability flags,
 * and shared primitive types used across all SoC headers.
 *
 * Integrates with:
 *   02_FwHal/uiox_fw_secboot.h  — boot verification context
 *   10_Arch/include/arch_defs.h — per-arch MMIO base addresses
 *   20_DriverInterfaces/include/hw_types.h — phys_addr_t, reg32_t
 */
#ifndef UIOX_SOC_TYPES_H
#define UIOX_SOC_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * SoC family identifiers
 * ====================================================================== */
typedef enum {
    UIOX_SOC_UNKNOWN        = 0,

    /* ── ARM64 (ARMv8-A / Cortex-A) ────────────────────────────────── */
    UIOX_SOC_QEMU_VIRT_A64  = 0x0100,  /**< QEMU virt (arm64 sim)       */
    UIOX_SOC_BCM2711        = 0x0101,  /**< Raspberry Pi 4 (Cortex-A72) */
    UIOX_SOC_BCM2712        = 0x0102,  /**< Raspberry Pi 5 (Cortex-A76) */
    UIOX_SOC_IMX8MP         = 0x0103,  /**< NXP i.MX 8M Plus            */
    UIOX_SOC_RK3588         = 0x0104,  /**< Rockchip RK3588             */

    /* ── ARM32 (ARMv7-A / Cortex-A) ────────────────────────────────── */
    UIOX_SOC_QEMU_VIRT_A32  = 0x0200,  /**< QEMU virt (arm32 sim)       */
    UIOX_SOC_BCM2836        = 0x0201,  /**< Raspberry Pi 2 (Cortex-A7)  */
    UIOX_SOC_IMX6Q          = 0x0202,  /**< NXP i.MX 6Quad              */
    UIOX_SOC_OMAP4430       = 0x0203,  /**< TI OMAP4430                 */

    /* ── x86-64 ─────────────────────────────────────────────────────── */
    UIOX_SOC_X86_GENERIC    = 0x0300,  /**< Generic x86-64 PC platform  */
    UIOX_SOC_X86_QEMU_Q35   = 0x0301,  /**< QEMU Q35 machine            */
    UIOX_SOC_X86_QEMU_I440  = 0x0302,  /**< QEMU i440FX machine         */

    /* ── RISC-V ─────────────────────────────────────────────────────── */
    UIOX_SOC_QEMU_VIRT_RV64 = 0x0400,  /**< QEMU virt (rv64 sim)        */
    UIOX_SOC_SIFIVE_U74     = 0x0401,  /**< SiFive U74 / HiFive Unmatched */
    UIOX_SOC_TH1520         = 0x0402,  /**< T-Head TH1520               */
} uiox_soc_id_t;

/* =========================================================================
 * SoC capability flags  (OR-combined bitmask)
 * ====================================================================== */
#define UIOX_SOC_CAP_NONE           0x00000000u
#define UIOX_SOC_CAP_MMU            (1u <<  0)
#define UIOX_SOC_CAP_CACHE_L1       (1u <<  1)
#define UIOX_SOC_CAP_CACHE_L2       (1u <<  2)
#define UIOX_SOC_CAP_CACHE_L3       (1u <<  3)
#define UIOX_SOC_CAP_SMP            (1u <<  4)
#define UIOX_SOC_CAP_GIC_V2         (1u <<  5)
#define UIOX_SOC_CAP_GIC_V3         (1u <<  6)
#define UIOX_SOC_CAP_PLIC           (1u <<  7)
#define UIOX_SOC_CAP_APIC           (1u <<  8)
#define UIOX_SOC_CAP_SMMU           (1u <<  9)
#define UIOX_SOC_CAP_TRNG           (1u << 10)
#define UIOX_SOC_CAP_TRUSTZONE      (1u << 11)
#define UIOX_SOC_CAP_SECURE_BOOT    (1u << 12)
#define UIOX_SOC_CAP_PCIE           (1u << 13)
#define UIOX_SOC_CAP_USB            (1u << 14)
#define UIOX_SOC_CAP_EMMC           (1u << 15)
#define UIOX_SOC_CAP_ETH            (1u << 16)
#define UIOX_SOC_CAP_UART           (1u << 17)
#define UIOX_SOC_CAP_SBI            (1u << 18)
#define UIOX_SOC_CAP_PSCI           (1u << 19)
#define UIOX_SOC_CAP_ACPI           (1u << 20)
#define UIOX_SOC_CAP_DTB            (1u << 21)
#define UIOX_SOC_CAP_EFI            (1u << 22)

/* =========================================================================
 * SoC descriptor — one instance per physical platform
 * ====================================================================== */
#define UIOX_SOC_NAME_LEN   48u
#define UIOX_SOC_MAX_CPUS   16u

typedef struct {
    uiox_soc_id_t   soc_id;
    char            name[UIOX_SOC_NAME_LEN];
    uint32_t        num_cpus;           /**< Physical CPU count          */
    uint32_t        num_clusters;       /**< CPU cluster count           */
    uint32_t        capabilities;       /**< UIOX_SOC_CAP_* bitmask      */
    uint64_t        dram_base;          /**< Physical DRAM start address */
    uint64_t        dram_size;          /**< DRAM size in bytes          */
    uint32_t        cpu_freq_khz;       /**< Boot CPU frequency (kHz)    */
    uint32_t        l1_icache_kb;       /**< L1 instruction cache (KB)   */
    uint32_t        l1_dcache_kb;       /**< L1 data cache (KB)          */
    uint32_t        l2_cache_kb;        /**< Unified L2 (KB); 0 = none   */
    uint32_t        l3_cache_kb;        /**< Unified L3 (KB); 0 = none   */
    bool            initialized;
} uiox_soc_desc_t;

/* =========================================================================
 * SoC init / fini function pointer types
 * Called by uiox_soc_init_arm64() / uiox_soc_init_x86() / etc.
 * ====================================================================== */
typedef int  (*uiox_soc_init_fn_t)(uiox_soc_desc_t *desc);
typedef void (*uiox_soc_fini_fn_t)(void);

/* =========================================================================
 * Return codes (compatible with HW_OK / HW_ERR_* from hw_types.h)
 * ====================================================================== */
#define UIOX_SOC_OK             0
#define UIOX_SOC_ERR_INVAL     -1
#define UIOX_SOC_ERR_NOTFOUND  -2
#define UIOX_SOC_ERR_NOMEM     -3
#define UIOX_SOC_ERR_TIMEOUT   -4
#define UIOX_SOC_ERR_NODEV     -5

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_TYPES_H */
