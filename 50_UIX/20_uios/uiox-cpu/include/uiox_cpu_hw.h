/**
 * @file    uiox_cpu_hw.h
 * @brief   UIOX CPU/SoC Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to CPU/SoC hardware. Supports:
 *   - ARM Cortex-A76 (AArch64, ARMv8.2-A)
 *   - x86-64 (AMD64 / Intel 64, SSE4.2 / AVX2 / AVX-512)
 *   - RISC-V RV64GC (SV39/SV48 MMU, M/S/U privilege modes)
 *
 * Owns:
 *   - System register access (MSR/MRS, RDMSR/WRMSR, CSR read/write)
 *   - MMIO access to SoC peripherals (GIC, PLIC, APIC, CLINT)
 *   - SMP bring-up (secondary core power-on sequence)
 *   - Exception / interrupt controller programming
 *   - Hardware performance counters (PMU)
 *   - Memory-mapped system timers (ARM CNTPCT, x86 TSC, RISC-V mtime)
 *
 * @version 1.0.0
 * @date    2026-06-02
 */

#ifndef UIOX_CPU_HW_H
#define UIOX_CPU_HW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Architecture detection
 * ====================================================================== */

#if defined(__aarch64__)
#  define UIOX_ARCH_ARM64   1
#elif defined(__x86_64__) || defined(_M_X64)
#  define UIOX_ARCH_X86_64  1
#elif defined(__riscv) && (__riscv_xlen == 64)
#  define UIOX_ARCH_RV64    1
#else
#  error "Unsupported architecture"
#endif

/* =========================================================================
 * CPU capability flags
 * ====================================================================== */

/* Common */
#define UIOX_CPU_CAP_SMP            (1u << 0)   /**< Multi-core SMP        */
#define UIOX_CPU_CAP_NEON           (1u << 1)   /**< NEON / SIMD           */
#define UIOX_CPU_CAP_FPU            (1u << 2)   /**< Hardware FPU          */
#define UIOX_CPU_CAP_ATOMIC         (1u << 3)   /**< Atomic instructions   */
#define UIOX_CPU_CAP_MMU            (1u << 4)   /**< Hardware MMU          */
#define UIOX_CPU_CAP_CACHE_L1       (1u << 5)   /**< L1 I/D cache          */
#define UIOX_CPU_CAP_CACHE_L2       (1u << 6)   /**< L2 unified cache      */
#define UIOX_CPU_CAP_CACHE_L3       (1u << 7)   /**< L3 shared cache       */
#define UIOX_CPU_CAP_PMU            (1u << 8)   /**< Performance counters  */
#define UIOX_CPU_CAP_VIRTUALIZATION (1u << 9)   /**< Hypervisor / VT-x     */
#define UIOX_CPU_CAP_CRYPTO         (1u << 10)  /**< HW crypto (AES/SHA)   */
#define UIOX_CPU_CAP_TRUSTZONE      (1u << 11)  /**< ARM TrustZone / SGX   */
#define UIOX_CPU_CAP_SVE            (1u << 12)  /**< ARM SVE (Scalable)    */
#define UIOX_CPU_CAP_AVX2           (1u << 13)  /**< x86 AVX2              */
#define UIOX_CPU_CAP_AVX512         (1u << 14)  /**< x86 AVX-512           */
#define UIOX_CPU_CAP_RVV            (1u << 15)  /**< RISC-V Vector (RVV)   */
#define UIOX_CPU_CAP_BTI            (1u << 16)  /**< ARM Branch Target ID  */
#define UIOX_CPU_CAP_MTE            (1u << 17)  /**< ARM Memory Tag Ext.   */
#define UIOX_CPU_CAP_CET            (1u << 18)  /**< x86 Control Flow Enf. */
#define UIOX_CPU_CAP_HYPERTHREAD    (1u << 19)  /**< x86 Hyperthreading    */
#define UIOX_CPU_CAP_HOTPLUG        (1u << 20)  /**< CPU hot-plug support  */

/* =========================================================================
 * Architecture type
 * ====================================================================== */

typedef enum {
    UIOX_CPU_ARCH_ARM64  = 0,
    UIOX_CPU_ARCH_X86_64 = 1,
    UIOX_CPU_ARCH_RV64   = 2,
} uiox_cpu_arch_t;

/* =========================================================================
 * CPU core state
 * ====================================================================== */

typedef enum {
    UIOX_CPU_STATE_OFFLINE  = 0,
    UIOX_CPU_STATE_ONLINE,
    UIOX_CPU_STATE_IDLE,
    UIOX_CPU_STATE_RUNNING,
    UIOX_CPU_STATE_HOTPLUG_OUT,
} uiox_cpu_core_state_t;

/* =========================================================================
 * Cache descriptor
 * ====================================================================== */

typedef struct {
    uint32_t  size_kb;        /**< Cache size in KiB                      */
    uint16_t  line_bytes;     /**< Cache line size (typically 64)         */
    uint8_t   associativity;  /**< Ways of set-associativity              */
    bool      inclusive;      /**< Inclusive of lower cache levels        */
} uiox_cpu_cache_t;

/* =========================================================================
 * Per-core hardware descriptor
 * ====================================================================== */

#define UIOX_CPU_MAX_CORES      16
#define UIOX_CPU_MAX_CLUSTERS   4
#define UIOX_CPU_ID_STR_LEN     64

typedef struct {
    uint8_t               core_id;       /**< Physical core ID             */
    uint8_t               cluster_id;    /**< Cluster / NUMA node          */
    uint8_t               thread_id;     /**< HW thread (SMT/HT)          */
    uiox_cpu_core_state_t state;
    uint32_t              freq_mhz;      /**< Current frequency            */
    uint32_t              max_freq_mhz;
    uint32_t              min_freq_mhz;
    uint8_t               voltage_mv;    /**< Supply voltage (× 10 mV)    */
    int8_t                temp_celsius;  /**< Current temperature          */
    uint64_t              cycles;        /**< Cycle counter snapshot       */
    uint64_t              instructions;  /**< Instructions retired         */
} uiox_cpu_core_t;

/* =========================================================================
 * SoC / platform descriptor
 * ====================================================================== */

typedef struct {
    uiox_cpu_arch_t   arch;
    char              model_str[UIOX_CPU_ID_STR_LEN]; /**< e.g. "Cortex-A76" */
    char              vendor_str[UIOX_CPU_ID_STR_LEN];/**< e.g. "ARM / Intel"*/
    uint32_t          caps;              /**< UIOX_CPU_CAP_* bitmask      */
    uint8_t           num_cores;
    uint8_t           num_clusters;
    uint8_t           num_threads;       /**< HW threads per core          */
    uiox_cpu_core_t   cores[UIOX_CPU_MAX_CORES];

    /* Cache hierarchy */
    uiox_cpu_cache_t  l1i, l1d;         /**< L1 instruction + data        */
    uiox_cpu_cache_t  l2;               /**< L2 unified                   */
    uiox_cpu_cache_t  l3;               /**< L3 shared (0 if absent)      */

    /* MMIO bases */
    uintptr_t         gic_base;          /**< ARM GIC / APIC / PLIC base  */
    uintptr_t         timer_base;        /**< System timer MMIO base       */
    uintptr_t         clint_base;        /**< RISC-V CLINT base (0=N/A)   */
    uint32_t          timer_freq_hz;     /**< Timer reference frequency    */

    /* SMP */
    volatile uint64_t *spin_table;       /**< ARM spin-table (SMP boot)   */
    uintptr_t          smp_mbox_base;    /**< Mailbox for secondary wake  */

    void              *priv;
} uiox_cpu_hw_t;

/* =========================================================================
 * Architecture-specific system register access
 * ====================================================================== */

/* ARM64 system register access */
#ifdef UIOX_ARCH_ARM64
#  define uiox_cpu_read_sysreg(reg)  ({ uint64_t _v; __asm__ volatile("mrs %0," #reg : "=r"(_v)); _v; })
#  define uiox_cpu_write_sysreg(reg, val) __asm__ volatile("msr " #reg ",%0" :: "r"((uint64_t)(val)))
#  define uiox_cpu_isb()  __asm__ volatile("isb" ::: "memory")
#  define uiox_cpu_dsb()  __asm__ volatile("dsb sy" ::: "memory")
#  define uiox_cpu_dmb()  __asm__ volatile("dmb sy" ::: "memory")
#  define uiox_cpu_wfi()  __asm__ volatile("wfi")
#  define uiox_cpu_wfe()  __asm__ volatile("wfe")
#  define uiox_cpu_sev()  __asm__ volatile("sev")
#  define uiox_cpu_cycles() uiox_cpu_read_sysreg(pmccntr_el0)
#  define uiox_cpu_this_id() ((uint8_t)(uiox_cpu_read_sysreg(mpidr_el1) & 0xFF))
#endif

/* x86-64 system register access */
// this section need to be replaced with below
#ifdef UIOX_ARCH_X86_64
static inline uint64_t uiox_cpu_rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}
static inline void uiox_cpu_wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile("wrmsr" :: "c"(msr), "a"((uint32_t)val),
                     "d"((uint32_t)(val >> 32)));
}
static inline uint64_t uiox_cpu_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
static inline void uiox_cpu_mfence(void) { __asm__ volatile("mfence" ::: "memory"); }
static inline void uiox_cpu_pause(void)  { __asm__ volatile("pause"); }
static inline void uiox_cpu_hlt(void)    { __asm__ volatile("hlt"); }
#  define uiox_cpu_cycles()  uiox_cpu_rdtsc()
#  define uiox_cpu_this_id() uiox_cpu_apic_id()
static inline uint8_t uiox_cpu_apic_id(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "0"(1));
    return (uint8_t)((ebx >> 24) & 0xFF);
}
#endif
//////////////////
/* x86-64 system register access */

//#ifdef UIOX_ARCH_X86_64
#if 0
static inline uint8_t uiox_cpu_apic_id(void)   /* ← declare FIRST         */
{
    uint32_t eax, ebx, ecx, edx;
 //   __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "0"(1));
    return (uint8_t)((ebx >> 24) & 0xFF);
}

static inline uint64_t uiox_cpu_rdmsr(uint32_t msr)
{
    uint32_t lo, hi;
 //   __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}
static inline void uiox_cpu_wrmsr(uint32_t msr, uint64_t val)
{
  //  __asm__ volatile("wrmsr" :: "c"(msr), "a"((uint32_t)val),
  //                            "d"((uint32_t)(val >> 32)));
}
static inline uint64_t uiox_cpu_rdtsc(void)
{
    uint32_t lo, hi;
  //  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
static inline void uiox_cpu_mfence(void){ __asm__ volatile("mfence":::"memory"); }
static inline void uiox_cpu_pause (void){ __asm__ volatile("pause"); }
static inline void uiox_cpu_hlt   (void){ __asm__ volatile("hlt"); }

#  define uiox_cpu_cycles()   uiox_cpu_rdtsc()
#  define uiox_cpu_this_id()  uiox_cpu_apic_id()  /* ← now safe to use    */
#endif


/////////////////////below section is separte
/* RISC-V CSR access */
#ifdef UIOX_ARCH_RV64
#  define uiox_cpu_csr_read(csr)  ({ uint64_t _v; __asm__ volatile("csrr %0," #csr : "=r"(_v)); _v; })
#  define uiox_cpu_csr_write(csr, val) __asm__ volatile("csrw " #csr ",%0" :: "rK"((uint64_t)(val)))
#  define uiox_cpu_csr_set(csr, bits)  __asm__ volatile("csrs " #csr ",%0" :: "rK"((uint64_t)(bits)))
#  define uiox_cpu_csr_clr(csr, bits)  __asm__ volatile("csrc " #csr ",%0" :: "rK"((uint64_t)(bits)))
#  define uiox_cpu_fence()  __asm__ volatile("fence" ::: "memory")
#  define uiox_cpu_wfi()    __asm__ volatile("wfi")
#  define uiox_cpu_cycles() uiox_cpu_csr_read(mcycle)
#  define uiox_cpu_this_id() ((uint8_t)(uiox_cpu_csr_read(mhartid) & 0xFF))
#endif

/* =========================================================================
 * Hardware operations vtable
 * ====================================================================== */

typedef struct {
    int  (*init)            (uiox_cpu_hw_t *hw);
    void (*deinit)          (uiox_cpu_hw_t *hw);

    /** Detect CPU capabilities and populate hw->caps, hw->cores[] */
    int  (*detect)          (uiox_cpu_hw_t *hw);

    /** Bring a secondary core online (SMP). */
    int  (*core_powerup)    (uiox_cpu_hw_t *hw, uint8_t core_id,
                             uintptr_t entry_point);

    /** Power down a secondary core (hot-plug out). */
    int  (*core_powerdown)  (uiox_cpu_hw_t *hw, uint8_t core_id);

    /** Set CPU frequency for a given core / cluster. */
    int  (*set_freq)        (uiox_cpu_hw_t *hw, uint8_t core_id,
                             uint32_t freq_mhz);

    /** Set CPU supply voltage (mV). */
    int  (*set_voltage)     (uiox_cpu_hw_t *hw, uint8_t cluster_id,
                             uint32_t mv);

    /** Read core temperature (Celsius). */
    int  (*read_temp)       (uiox_cpu_hw_t *hw, uint8_t core_id,
                             int8_t *temp_out);

    /** Read hardware performance counter. */
    uint64_t (*perf_read)   (uiox_cpu_hw_t *hw, uint8_t core_id,
                             uint8_t counter_id);

    /** Reset a performance counter. */
    void (*perf_reset)      (uiox_cpu_hw_t *hw, uint8_t core_id,
                             uint8_t counter_id);

    /** Flush and invalidate cache range [addr, addr+size). */
    void (*cache_flush)     (uiox_cpu_hw_t *hw,
                             uintptr_t addr, size_t size);

    /** Invalidate TLB for a virtual address. */
    void (*tlb_invalidate)  (uiox_cpu_hw_t *hw, uintptr_t vaddr);

    /** Send inter-processor interrupt (IPI). */
    int  (*ipi_send)        (uiox_cpu_hw_t *hw, uint8_t target_core,
                             uint8_t vector);

    /** ISR top-half: timer tick. */
    void (*isr_timer)       (uiox_cpu_hw_t *hw);

    /** ISR top-half: IPI received. */
    void (*isr_ipi)         (uiox_cpu_hw_t *hw, uint8_t vector);

    /** ISR top-half: fault (page fault / GPF / instruction fault). */
    void (*isr_fault)       (uiox_cpu_hw_t *hw, uint64_t fault_addr,
                             uint32_t fault_type);
} uiox_cpu_hw_ops_t;

/* =========================================================================
 * HAL public API
 * ====================================================================== */

int      uiox_cpu_hw_init         (uiox_cpu_hw_t *hw,
                                    const uiox_cpu_hw_ops_t *ops);
void     uiox_cpu_hw_deinit       (uiox_cpu_hw_t *hw);
int      uiox_cpu_hw_detect       (uiox_cpu_hw_t *hw);
int      uiox_cpu_hw_core_powerup (uiox_cpu_hw_t *hw, uint8_t core_id,
                                    uintptr_t entry);
int      uiox_cpu_hw_set_freq     (uiox_cpu_hw_t *hw, uint8_t core_id,
                                    uint32_t freq_mhz);
int      uiox_cpu_hw_read_temp    (uiox_cpu_hw_t *hw, uint8_t core_id,
                                    int8_t *temp);
void     uiox_cpu_hw_cache_flush  (uiox_cpu_hw_t *hw,
                                    uintptr_t addr, size_t size);
int      uiox_cpu_hw_ipi_send     (uiox_cpu_hw_t *hw, uint8_t target,
                                    uint8_t vector);
uint64_t uiox_cpu_hw_timestamp    (const uiox_cpu_hw_t *hw);

static inline uint32_t uiox_cpu_caps(const uiox_cpu_hw_t *hw)
{ return hw ? hw->caps : 0u; }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CPU_HW_H */
