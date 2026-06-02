/*
 * cpu_features.c - CPU feature detection for ARM64 / x86-64 / RISC-V
 */
#include "../include/cpu_features.h"
#include "../include/cpu_regs.h"
#include <string.h>
#include <stdio.h>

cpu_id_t g_cpu_id;

/* ── ARM Cortex-A76 detection ──────────────────────────────── */
#if defined(UIOX_ARCH_ARM64)
static void detect_arm64(cpu_id_t *id)
{
    cpu_u64_t midr;
    CPU_MRS(MIDR_EL1, midr);

    id->arch     = CPU_ARCH_ARM_CORTEX_A76;
    id->part_num = (cpu_u32_t)((midr >> 4)  & 0xFFFu);
    id->variant  = (cpu_u32_t)((midr >> 20) & 0xFu);
    id->revision = (cpu_u32_t)(midr & 0xFu);
    id->family   = (cpu_u32_t)((midr >> 16) & 0xFu);

    cpu_u64_t mpidr;
    CPU_MRS(MPIDR_EL1, mpidr);
    id->num_cores = 4; /* default; real count via device tree */

    /* feature detection via ID registers */
    cpu_u64_t id_aa64pfr0;
    CPU_MRS(ID_AA64PFR0_EL1, id_aa64pfr0);
    if ((id_aa64pfr0 >> 16) & 0xF) id->features |= CPU_FEAT_FPU;
    if ((id_aa64pfr0 >> 20) & 0xF) id->features |= CPU_FEAT_SIMD;
    if ((id_aa64pfr0 >> 32) & 0xF) id->features |= CPU_FEAT_SVE;
    if ((id_aa64pfr0 >> 40) & 0xF) id->features |= CPU_FEAT_EL2;
    if ((id_aa64pfr0 >> 44) & 0xF) id->features |= CPU_FEAT_EL3;

    cpu_u64_t id_aa64isar0;
    CPU_MRS(ID_AA64ISAR0_EL1, id_aa64isar0);
    if ((id_aa64isar0 >> 4)  & 0xF) id->features |= CPU_FEAT_AES;
    if ((id_aa64isar0 >> 8)  & 0xF) id->features |= CPU_FEAT_SHA;
    if ((id_aa64isar0 >> 16) & 0xF) id->features |= CPU_FEAT_CRC;
    if ((id_aa64isar0 >> 20) & 0xF) id->features |= CPU_FEAT_ATOMICS;

    cpu_u64_t id_aa64mmfr2;
    CPU_MRS(ID_AA64MMFR2_EL1, id_aa64mmfr2);
    if ((id_aa64mmfr2 >> 0) & 0xF) id->features |= CPU_FEAT_IOMMU;

    strncpy(id->vendor, "ARM Ltd", sizeof(id->vendor) - 1);
    if (id->part_num == 0xD0B)
        strncpy(id->model, "Cortex-A76", sizeof(id->model) - 1);
    else
        strncpy(id->model, "AArch64 CPU", sizeof(id->model) - 1);

    id->cache_line   = CPU_CACHE_LINE_ARM64;
    id->l1i_size_kb  = 64;
    id->l1d_size_kb  = 64;
    id->l2_size_kb   = 512;
    id->l3_size_kb   = 4096;
    id->features    |= CPU_FEAT_MULTICORE | CPU_FEAT_TRUSTZONE;
}

/* ── x86-64 detection ──────────────────────────────────────── */
#elif defined(UIOX_ARCH_X86_64)
static void detect_x86_64(cpu_id_t *id)
{
    cpu_u32_t eax, ebx, ecx, edx;

    id->arch = CPU_ARCH_X86_64;

    /* vendor string */
    cpu_cpuid(0, &eax, &ebx, &ecx, &edx);
    cpu_u32_t max_leaf = eax;
    char vend[13] = {0};
    memcpy(vend + 0, &ebx, 4);
    memcpy(vend + 4, &edx, 4);
    memcpy(vend + 8, &ecx, 4);
    strncpy(id->vendor, vend, sizeof(id->vendor) - 1);

    /* family / model / stepping */
    if (max_leaf >= 1) {
        cpu_cpuid(1, &eax, &ebx, &ecx, &edx);
        id->revision = eax & 0xF;
        id->part_num = (eax >> 4) & 0xF;
        id->family   = (eax >> 8) & 0xF;
        id->variant  = (eax >> 12) & 0x3;

        /* logical CPU count */
        id->num_cores = (ebx >> 16) & 0xFF;
        if (!id->num_cores) id->num_cores = 1;

        /* feature flags */
        if (edx & (1u << 0))  id->features |= CPU_FEAT_FPU;
        if (edx & (1u << 25)) id->features |= CPU_FEAT_SIMD;
        if (ecx & (1u << 25)) id->features |= CPU_FEAT_AES;
        if (ecx & (1u << 28)) id->features |= CPU_FEAT_AVX;
        if (ecx & (1u << 30)) id->features |= CPU_FEAT_RDRAND;
        if (ecx & (1u << 5))  id->features |= CPU_FEAT_EL2;
    }

    /* brand string */
    cpu_u32_t ext_max;
    cpu_cpuid(0x80000000u, &eax, &ebx, &ecx, &edx);
    ext_max = eax;
    if (ext_max >= 0x80000004u) {
        cpu_cpuid(0x80000002u, &eax, &ebx, &ecx, &edx);
        memcpy(id->model + 0,  &eax, 4);
        memcpy(id->model + 4,  &ebx, 4);
        memcpy(id->model + 8,  &ecx, 4);
        memcpy(id->model + 12, &edx, 4);
        cpu_cpuid(0x80000003u, &eax, &ebx, &ecx, &edx);
        memcpy(id->model + 16, &eax, 4);
        memcpy(id->model + 20, &ebx, 4);
        memcpy(id->model + 24, &ecx, 4);
        memcpy(id->model + 28, &edx, 4);
        cpu_cpuid(0x80000004u, &eax, &ebx, &ecx, &edx);
        memcpy(id->model + 32, &eax, 4);
        memcpy(id->model + 36, &ebx, 4);
        memcpy(id->model + 40, &ecx, 4);
        memcpy(id->model + 44, &edx, 4);
    }

    id->cache_line  = CPU_CACHE_LINE_X86_64;
    id->l1i_size_kb = 32;
    id->l1d_size_kb = 48;
    id->l2_size_kb  = 1280;
    id->l3_size_kb  = 12288;
    id->features   |= CPU_FEAT_MULTICORE;
}

/* ── RISC-V detection ──────────────────────────────────────── */
#elif defined(UIOX_ARCH_RISCV64)
static void detect_riscv64(cpu_id_t *id)
{
    cpu_u64_t misa;
    CPU_CSR_READ(misa, misa);

    id->arch = CPU_ARCH_RISCV64;
    strncpy(id->vendor, "RISC-V", sizeof(id->vendor) - 1);
    strncpy(id->model,  "RV64GC", sizeof(id->model) - 1);

    id->num_cores = 1;    /* hart count from device tree       */
    id->part_num  = (cpu_u32_t)(misa & 0xFFFF);

    if (misa & (1u << ('F'-'A'))) id->features |= CPU_FEAT_FPU;
    if (misa & (1u << ('D'-'A'))) id->features |= CPU_FEAT_FPU;
    if (misa & (1u << ('V'-'A'))) id->features |= CPU_FEAT_VECTOR;
    if (misa & (1u << ('C'-'A'))) id->features |= CPU_FEAT_COMPRESSED;
    if (misa & (1u << ('H'-'A'))) id->features |= CPU_FEAT_HYPER;
    if (misa & (1u << ('A'-'A'))) id->features |= CPU_FEAT_ATOMICS;
    id->features |= CPU_FEAT_MULTICORE;

    id->cache_line  = CPU_CACHE_LINE_RISCV64;
    id->l1i_size_kb = 32;
    id->l1d_size_kb = 32;
    id->l2_size_kb  = 512;
    id->l3_size_kb  = 0;
}
#endif

int cpu_features_detect(cpu_id_t *id)
{
    if (!id) return CPU_ERR;
    memset(id, 0, sizeof(*id));
#if   defined(UIOX_ARCH_ARM64)
    detect_arm64(id);
#elif defined(UIOX_ARCH_X86_64)
    detect_x86_64(id);
#elif defined(UIOX_ARCH_RISCV64)
    detect_riscv64(id);
#endif
    g_cpu_id = *id;
    return CPU_OK;
}

cpu_u32_t cpu_features_get(void) { return g_cpu_id.features; }

cpu_bool_t cpu_has_feature(cpu_u32_t feat)
{ return (g_cpu_id.features & feat) ? CPU_TRUE : CPU_FALSE; }

const char *cpu_arch_str(cpu_arch_t arch)
{
    switch (arch) {
        case CPU_ARCH_ARM_CORTEX_A76: return "ARM Cortex-A76";
        case CPU_ARCH_X86_64:         return "x86-64";
        case CPU_ARCH_RISCV64:        return "RISC-V RV64GC";
        default:                       return "Unknown";
    }
}

void cpu_id_print(const cpu_id_t *id)
{
    printf("[cpu] Arch     : %s\n", cpu_arch_str(id->arch));
    printf("[cpu] Vendor   : %s\n", id->vendor);
    printf("[cpu] Model    : %s\n", id->model);
    printf("[cpu] Cores    : %u\n", id->num_cores);
    printf("[cpu] PartNum  : 0x%03X  Rev=%u  Var=%u\n",
           id->part_num, id->revision, id->variant);
    printf("[cpu] Cache    : L1i=%uKB L1d=%uKB L2=%uKB L3=%uKB\n",
           id->l1i_size_kb, id->l1d_size_kb,
           id->l2_size_kb,  id->l3_size_kb);
    printf("[cpu] Features : 0x%08X\n", id->features);
    if (id->features & CPU_FEAT_FPU)       printf("         FPU ");
    if (id->features & CPU_FEAT_SIMD)      printf("SIMD ");
    if (id->features & CPU_FEAT_AES)       printf("AES ");
    if (id->features & CPU_FEAT_SVE)       printf("SVE ");
    if (id->features & CPU_FEAT_AVX)       printf("AVX ");
    if (id->features & CPU_FEAT_ATOMICS)   printf("ATOMICS ");
    if (id->features & CPU_FEAT_EL2)       printf("VMX/EL2 ");
    if (id->features & CPU_FEAT_TRUSTZONE) printf("TrustZone ");
    if (id->features & CPU_FEAT_RDRAND)    printf("RDRAND ");
    if (id->features & CPU_FEAT_VECTOR)    printf("RVV ");
    printf("\n");
}
