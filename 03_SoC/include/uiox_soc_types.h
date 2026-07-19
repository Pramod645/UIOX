/*
 * 02_FwHal/include/uiox_soc_types.h
 * UIOX SoC abstraction layer — SoC ID enumeration, capability flags,
 * and shared primitive types used across all SoC headers.
 */
#ifndef UIOX_SOC_TYPES_H
#define UIOX_SOC_TYPES_H

#include "uiox_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * SoC image layout constants
 * ====================================================================== */
#if defined(__aarch64__)
#  define UIOX_SOC_LOAD_PA     ((uiox_uintptr_t)0x40000000u)
#  define UIOX_SOC_IMAGE_SIZE  ((uiox_uint32_t) 0x00080000u)
#elif defined(__arm__)
#  define UIOX_SOC_LOAD_PA     ((uiox_uintptr_t)0x00100000u)
#  define UIOX_SOC_IMAGE_SIZE  ((uiox_uint32_t) 0x00040000u)
#else
#  define UIOX_SOC_LOAD_PA     ((uiox_uintptr_t)0x00100000u)
#  define UIOX_SOC_IMAGE_SIZE  ((uiox_uint32_t) 0x00080000u)
#endif

/* =========================================================================
 * SoC family identifiers
 * ====================================================================== */
typedef enum {
    UIOX_SOC_UNKNOWN        = 0,
    UIOX_SOC_QEMU_VIRT_A64  = 0x0100,
    UIOX_SOC_BCM2711        = 0x0101,
    UIOX_SOC_BCM2712        = 0x0102,
    UIOX_SOC_IMX8MP         = 0x0103,
    UIOX_SOC_RK3588         = 0x0104,
    UIOX_SOC_QEMU_VIRT_A32  = 0x0200,
    UIOX_SOC_BCM2836        = 0x0201,
    UIOX_SOC_IMX6Q          = 0x0202,
    UIOX_SOC_OMAP4430       = 0x0203,
    UIOX_SOC_X86_GENERIC    = 0x0300,
    UIOX_SOC_X86_QEMU_Q35   = 0x0301,
    UIOX_SOC_X86_QEMU_I440  = 0x0302,
    UIOX_SOC_QEMU_VIRT_RV64 = 0x0400,
    UIOX_SOC_SIFIVE_U74     = 0x0401,
    UIOX_SOC_TH1520         = 0x0402,
} uiox_soc_id_t;

/* =========================================================================
 * SoC capability flags — AUTHORITATIVE definitions.
 * Do NOT redefine any of these in uiox_soc_hw.h or any other header.
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
#define UIOX_SOC_CAP_SMMU           (1u <<  9)   /* NOT bit 13 */
#define UIOX_SOC_CAP_TRNG           (1u << 10)
#define UIOX_SOC_CAP_TRUSTZONE      (1u << 11)
#define UIOX_SOC_CAP_SECURE_BOOT    (1u << 12)
#define UIOX_SOC_CAP_PCIE           (1u << 13)
#define UIOX_SOC_CAP_USB            (1u << 14)
#define UIOX_SOC_CAP_EMMC           (1u << 15)
#define UIOX_SOC_CAP_ETH            (1u << 16)
#define UIOX_SOC_CAP_UART           (1u << 17)
#define UIOX_SOC_CAP_SBI            (1u << 18)
#define UIOX_SOC_CAP_PSCI           (1u << 19)   /* NOT bit 11 */
#define UIOX_SOC_CAP_ACPI           (1u << 20)   /* NOT bit 12 */
#define UIOX_SOC_CAP_DTB            (1u << 21)   /* NOT bit 15 */
#define UIOX_SOC_CAP_EFI            (1u << 22)

/* =========================================================================
 * CPU and platform limits — AUTHORITATIVE definitions.
 * Do NOT redefine UIOX_SOC_MAX_CPUS in uiox_soc_power.h or psci.h.
 * ====================================================================== */

/** Max CPUs tracked by power / PSCI subsystems. */
#define UIOX_SOC_MAX_CPUS        8u

/** Max CPUs in the SoC descriptor (physical count). */
#define UIOX_SOC_DESC_MAX_CPUS  16u

/** PSCI-specific alias — same value, different name used in psci.c. */
#define UIOX_SOC_PSCI_MAX_CPUS   UIOX_SOC_MAX_CPUS

/* =========================================================================
 * CPU hot-plug / PSCI state — AUTHORITATIVE definition.
 * Shared by uiox_soc_power.h and uiox_soc_psci.h.
 * Do NOT redefine uiox_soc_cpu_state_t in either of those headers.
 * ====================================================================== */
typedef enum {
    UIOX_SOC_CPU_OFF     = 0,   /**< CPU is powered off                   */
    UIOX_SOC_CPU_ON      = 1,   /**< CPU is running                       */
    UIOX_SOC_CPU_PENDING = 2,   /**< CPU_ON issued, not yet running       */
    UIOX_SOC_CPU_SUSPEND = 3,   /**< CPU is suspended (WFI / C-state)     */
} uiox_soc_cpu_state_t;

/*
 * Aliases for the UIOX_SOC_CPU_STATE_* naming convention used in
 * uiox_soc_psci.c (inherited from the original uiox_fw_psci design).
 *
 * Fixes:
 *   'UIOX_SOC_CPU_STATE_ON' undeclared
 *   'UIOX_SOC_CPU_STATE_OFF' undeclared
 *   'UIOX_SOC_CPU_STATE_ON_PEND' undeclared
 */
#define UIOX_SOC_CPU_STATE_OFF      UIOX_SOC_CPU_OFF
#define UIOX_SOC_CPU_STATE_ON       UIOX_SOC_CPU_ON
#define UIOX_SOC_CPU_STATE_ON_PEND  UIOX_SOC_CPU_PENDING
#define UIOX_SOC_CPU_STATE_SUSPEND  UIOX_SOC_CPU_SUSPEND

/* =========================================================================
 * SoC descriptor
 * ====================================================================== */
#define UIOX_SOC_NAME_LEN   48u

typedef struct {
    uiox_soc_id_t    soc_id;
    char             name[UIOX_SOC_NAME_LEN];
    uiox_uint32_t    num_cpus;
    uiox_uint32_t    num_clusters;
    uiox_uint32_t    capabilities;
    uiox_uint64_t    dram_base;
    uiox_uint64_t    dram_size;
    uiox_uint32_t    cpu_freq_khz;
    uiox_uint32_t    l1_icache_kb;
    uiox_uint32_t    l1_dcache_kb;
    uiox_uint32_t    l2_cache_kb;
    uiox_uint32_t    l3_cache_kb;
    uiox_bool_t      initialized;
} uiox_soc_desc_t;

/* =========================================================================
 * SoC init / fini function pointer types
 * ====================================================================== */
typedef int  (*uiox_soc_init_fn_t)(uiox_soc_desc_t *desc);
typedef void (*uiox_soc_fini_fn_t)(void);

/* =========================================================================
 * Return / error codes
 * ====================================================================== */
typedef enum {
    UIOX_SOC_OK             =  0,
    UIOX_SOC_ERR_GENERIC    = -1,
    UIOX_SOC_ERR_INVAL      = -2,
    UIOX_SOC_ERR_NOMEM      = -3,
    UIOX_SOC_ERR_IO         = -4,
    UIOX_SOC_ERR_TIMEOUT    = -5,
    UIOX_SOC_ERR_BUSY       = -6,
    UIOX_SOC_ERR_NODEV      = -7,
    UIOX_SOC_ERR_UNSUP      = -8,
    UIOX_SOC_ERR_PERM       = -9,
    UIOX_SOC_ERR_OVERFLOW   = -10,
    UIOX_SOC_ERR_BADMAGIC   = -11,
    UIOX_SOC_ERR_POST       = -12,
    UIOX_SOC_ERR_SECBOOT    = -13,
    UIOX_SOC_ERR_SECURITY   = -14,
    UIOX_SOC_ERR_FULL       = -15,
    UIOX_SOC_ERR_NOTSUP     = -16,
    UIOX_SOC_ERR_NOTFOUND   = -17,
} uiox_soc_err_t;

/* =========================================================================
 * Magic numbers
 * ====================================================================== */
#define UIOX_SOC_MAGIC          0x55494F58u
#define UIOX_SOC_DEVSW_MAGIC    0x44455357u
#define UIOX_SOC_VERSION        0x00010000u

/* =========================================================================
 * Utility macros
 * ====================================================================== */
#define UIOX_SOC_UNUSED(x)        ((void)(x))
#define UIOX_SOC_ARRAY_SIZE(a)    (sizeof(a) / sizeof((a)[0]))
#define UIOX_SOC_ALIGN_UP(v, a)   (((v) + ((a) - 1u)) & ~((a) - 1u))
#define UIOX_SOC_ALIGN_DN(v, a)   ((v) & ~((a) - 1u))
#define UIOX_SOC_MIN(a, b)        ((a) < (b) ? (a) : (b))
#define UIOX_SOC_MAX(a, b)        ((a) > (b) ? (a) : (b))
#define UIOX_SOC_BIT(n)           (1u << (n))

/* =========================================================================
 * MMIO helpers (no-libc, inline)
 * ====================================================================== */
static inline void
soc_mmio_write32(uiox_uintptr_t addr, uiox_uint32_t val)
{ *((volatile uiox_uint32_t *)addr) = val; }

static inline uiox_uint32_t
soc_mmio_read32(uiox_uintptr_t addr)
{ return *((volatile uiox_uint32_t *)addr); }

static inline void
soc_mmio_write8(uiox_uintptr_t addr, uiox_uint8_t val)
{ *((volatile uiox_uint8_t *)addr) = val; }

static inline uiox_uint8_t
soc_mmio_read8(uiox_uintptr_t addr)
{ return *((volatile uiox_uint8_t *)addr); }

static inline void
soc_mmio_write64(uiox_uintptr_t addr, uiox_uint64_t val)
{ *((volatile uiox_uint64_t *)addr) = val; }

static inline uiox_uint64_t
soc_mmio_read64(uiox_uintptr_t addr)
{ return *((volatile uiox_uint64_t *)addr); }


//added form arch
/* 03_SoC/include/uiox_soc_types.h — add to the existing limits section */

/* =========================================================================
 * Cross-layer OS / driver policy constants
 *
 * These live in the SoC layer because they are tuned to the available
 * hardware resources (DRAM size, DMA controller, IRQ count) that vary
 * per SoC — not per ISA.
 * ====================================================================== */

/* ── Block device geometry ────────────────────────────────────────── */
#define UIOX_BLOCK_SIZE         512u    /**< Bytes per logical block        */
#define UIOX_MAX_BLOCKS         1024u   /**< Maximum blocks per device      */
#define UIOX_MAX_INODES         128u    /**< Maximum inodes in inode table  */

/* ── Device switch table limits ──────────────────────────────────── */
#define UIOX_MAJOR_BLK_MAX      8u      /**< Max block device major numbers */
#define UIOX_MAJOR_CHR_MAX      8u      /**< Max char  device major numbers */

/* ── Buffer cache ────────────────────────────────────────────────── */
#define UIOX_CBLOCK_POOL        256u    /**< Number of buffer-cache blocks  */

/* ── Interrupt subsystem ─────────────────────────────────────────── */
#define UIOX_IRQ_MAX            64u     /**< Maximum IRQ lines supported    */

/* ── DMA subsystem ───────────────────────────────────────────────── */
#define UIOX_DMA_MAX_DESC       16u     /**< Maximum DMA descriptors        */

/* ── MMIO region table ───────────────────────────────────────────── */
#define UIOX_MMIO_REGIONS       8u      /**< Max tracked MMIO regions       */





#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_TYPES_H */
