#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H
/*
 * cpu_features.h - CPU feature detection and capability bits
 */
#include "cpu_types.h"

/* -- Feature flags ------------------------------------------ */
#define CPU_FEAT_FPU        (1u <<  0)  /* Floating-point unit       */
#define CPU_FEAT_SIMD       (1u <<  1)  /* SIMD / NEON / SSE / V ext */
#define CPU_FEAT_AES        (1u <<  2)  /* AES crypto acceleration   */
#define CPU_FEAT_SHA        (1u <<  3)  /* SHA crypto acceleration   */
#define CPU_FEAT_CRC        (1u <<  4)  /* CRC32 acceleration        */
#define CPU_FEAT_ATOMICS    (1u <<  5)  /* Large-system atomics      */
#define CPU_FEAT_PMU        (1u <<  6)  /* Performance monitor unit  */
#define CPU_FEAT_EL2        (1u <<  7)  /* Hypervisor (EL2 / VMX)    */
#define CPU_FEAT_EL3        (1u <<  8)  /* Secure monitor (EL3 / SMX)*/
#define CPU_FEAT_BTI        (1u <<  9)  /* Branch target identification*/
#define CPU_FEAT_MTE        (1u << 10)  /* Memory tagging extension  */
#define CPU_FEAT_SVE        (1u << 11)  /* Scalable vector extension */
#define CPU_FEAT_PAUTH      (1u << 12)  /* Pointer authentication    */
#define CPU_FEAT_AVX        (1u << 13)  /* AVX / AVX-512 (x86)       */
#define CPU_FEAT_RDRAND     (1u << 14)  /* Hardware RNG              */
#define CPU_FEAT_TSX        (1u << 15)  /* Transactional memory      */
#define CPU_FEAT_COMPRESSED (1u << 16)  /* RISC-V C extension        */
#define CPU_FEAT_HYPER      (1u << 17)  /* RISC-V H extension        */
#define CPU_FEAT_VECTOR     (1u << 18)  /* RISC-V V extension        */
#define CPU_FEAT_MULTICORE  (1u << 19)  /* SMP capable               */
#define CPU_FEAT_TRUSTZONE  (1u << 20)  /* ARM TrustZone             */
#define CPU_FEAT_IOMMU      (1u << 21)  /* SMMU / IOMMU present      */

/* -- CPU identification ------------------------------------- */
typedef struct cpu_id {
    cpu_arch_t  arch;
    char        vendor[32];    /* "ARM", "Intel", "AMD", "SiFive"   */
    char        model[64];     /* "Cortex-A76", "Core i7-12700H"    */
    cpu_u32_t   family;
    cpu_u32_t   variant;
    cpu_u32_t   revision;
    cpu_u32_t   part_num;      /* MIDR PartNum / CPUID Model        */
    cpu_u32_t   num_cores;
    cpu_u32_t   features;      /* bitmask of CPU_FEAT_* above       */
    cpu_u32_t   cache_line;
    cpu_u32_t   l1i_size_kb;
    cpu_u32_t   l1d_size_kb;
    cpu_u32_t   l2_size_kb;
    cpu_u32_t   l3_size_kb;
    cpu_u32_t   freq_mhz;
} cpu_id_t;

/* -- Global CPU ID (filled by cpu_features_detect) ---------- */
extern cpu_id_t g_cpu_id;

/* -- API ---------------------------------------------------- */
int         cpu_features_detect (cpu_id_t *id);
cpu_u32_t   cpu_features_get    (void);
cpu_bool_t  cpu_has_feature     (cpu_u32_t feat);
void        cpu_id_print        (const cpu_id_t *id);
const char *cpu_arch_str        (cpu_arch_t arch);

#endif /* CPU_FEATURES_H */
