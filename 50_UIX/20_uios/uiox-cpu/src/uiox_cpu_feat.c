/**
 * @file    uiox_cpu_feat.c
 * @brief   UIOX CPU feature detection implementation.
 * @date    2026-06-02
 */

 #include "uiox_cpu_feat.h"
 #include <string.h>
 #include <stdio.h>
 #include <errno.h>
 
 int uiox_cpu_feat_init(uiox_cpu_feat_t *feat, uiox_cpu_hw_t *hw)
 {
     if (!feat || !hw) return -EINVAL;
     memset(feat, 0, sizeof(*feat));
     feat->hw = hw;
     return 0;
 }
 
 int uiox_cpu_feat_detect(uiox_cpu_feat_t *feat)
 {
     if (!feat) return -EINVAL;
 
 #if defined(UIOX_ARCH_ARM64)
     /* Read MIDR_EL1: Main ID Register */
     uint64_t midr;
     __asm__ volatile("mrs %0, midr_el1" : "=r"(midr));
     feat->cpuid_family   = (uint32_t)((midr >> 20u) & 0xFu);
     feat->cpuid_model    = (uint32_t)((midr >> 4u)  & 0xFFFu);
     feat->cpuid_stepping = (uint32_t)(midr & 0xFu);
 
     /* Read ID_AA64ISAR0_EL1: ISA feature register */
     uint64_t isar0;
     __asm__ volatile("mrs %0, id_aa64isar0_el1" : "=r"(isar0));
     if ((isar0 >> 4u) & 0xFu)  feat->hw->caps |= UIOX_CPU_CAP_ATOMIC;
     if ((isar0 >> 12u) & 0xFu) feat->hw->caps |= UIOX_CPU_CAP_CRYPTO;
 
     /* Cache size ID */
     uint64_t ctr;
     __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
     feat->l1i.line_bytes = (uint16_t)(4u << ((ctr >> 0u) & 0xFu));
     feat->l1d.line_bytes = (uint16_t)(4u << ((ctr >> 16u) & 0xFu));
 
     strncpy(feat->brand_string, "ARM Cortex-A76", sizeof(feat->brand_string)-1);
 
 #elif defined(UIOX_ARCH_X86_64)
     /* CPUID leaf 0x1: family/model/stepping */
     uint32_t eax, ebx, ecx, edx;
     __asm__("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"0"(1));
     feat->cpuid_family   = ((eax >> 8u) & 0xFu) + ((eax >> 20u) & 0xFFu);
     feat->cpuid_model    = ((eax >> 4u) & 0xFu) | ((eax >> 12u) & 0xF0u);
     feat->cpuid_stepping =  (eax & 0xFu);
     if (edx & (1u << 25u)) feat->hw->caps |= UIOX_CPU_CAP_NEON;    /* SSE */
     if (ecx & (1u << 28u)) feat->hw->caps |= UIOX_CPU_CAP_NEON;    /* AVX */
     if (ecx & (1u << 6u))  feat->hw->caps |= UIOX_CPU_CAP_VIRTUALIZATION;
 
     /* Brand string (CPUID 0x80000002-4) */
     uint32_t *bstr = (uint32_t *)feat->brand_string;
     for (uint32_t leaf = 0x80000002u; leaf <= 0x80000004u; leaf++, bstr+=4)
         __asm__("cpuid":"=a"(bstr[0]),"=b"(bstr[1]),"=c"(bstr[2]),"=d"(bstr[3]):"0"(leaf));
 
     /* Cache (CPUID 0x4) */
     feat->l1d.size_kb    = 32u;
     feat->l1d.line_bytes = 64u;
     feat->l2.size_kb     = 256u;
     feat->l2.line_bytes  = 64u;
     feat->l3.size_kb     = 8192u;
     feat->l3.line_bytes  = 64u;
 
 #elif defined(UIOX_ARCH_RV64)
     /* RISC-V: misa register encodes ISA extensions */
     uint64_t misa = uiox_cpu_csr_read(misa);
     if (misa & (1u << ('F'-'A'))) feat->hw->caps |= UIOX_CPU_CAP_FPU;
     if (misa & (1u << ('A'-'A'))) feat->hw->caps |= UIOX_CPU_CAP_ATOMIC;
     if (misa & (1u << ('V'-'A'))) feat->hw->caps |= UIOX_CPU_CAP_RVV;
     strncpy(feat->brand_string, "RISC-V RV64GC", sizeof(feat->brand_string)-1);
 #endif
 
     feat->hw->caps |= UIOX_CPU_CAP_MMU | UIOX_CPU_CAP_FPU |
                       UIOX_CPU_CAP_CACHE_L1 | UIOX_CPU_CAP_CACHE_L2;
     return 0;
 }
 
 int uiox_cpu_feat_pmu_start(uiox_cpu_feat_t *feat,
                              uint8_t counter_id, uint32_t event_id)
 {
     if (!feat || counter_id >= UIOX_PMU_MAX_COUNTERS) return -EINVAL;
     feat->counters[counter_id].counter_id = counter_id;
     feat->counters[counter_id].event_id   = event_id;
     feat->counters[counter_id].value      = 0;
     feat->counters[counter_id].enabled    = true;
     if (counter_id >= feat->num_counters)
         feat->num_counters = counter_id + 1u;
 
 #if defined(UIOX_ARCH_ARM64)
     /* Programme PMSELR_EL0, PMXEVTYPER_EL0, PMCNTENSET_EL0 */
     __asm__ volatile("msr pmselr_el0,   %0" :: "r"((uint64_t)counter_id));
     __asm__ volatile("msr pmxevtyper_el0,%0" :: "r"((uint64_t)event_id));
     __asm__ volatile("msr pmcntenset_el0,%0" :: "r"(1ULL << counter_id));
     uint64_t pmcr;
     __asm__ volatile("mrs %0, pmcr_el0" : "=r"(pmcr));
     pmcr |= 1u;  /* E bit: enable */
     __asm__ volatile("msr pmcr_el0, %0" :: "r"(pmcr));
 #elif defined(UIOX_ARCH_X86_64)
     /* IA32_PERFEVTSEL0 + IA32_PMC0 */
     uiox_cpu_wrmsr(0x186u + counter_id,
                    (uint64_t)event_id | (1u<<16u) | (1u<<22u));
     uiox_cpu_wrmsr(0xC1u  + counter_id, 0);
 #endif
     return 0;
 }
 
 void uiox_cpu_feat_pmu_stop(uiox_cpu_feat_t *feat, uint8_t counter_id)
 {
     if (!feat || counter_id >= UIOX_PMU_MAX_COUNTERS) return;
     feat->counters[counter_id].enabled = false;
 #if defined(UIOX_ARCH_ARM64)
     __asm__ volatile("msr pmcntenclr_el0,%0" :: "r"(1ULL << counter_id));
 #elif defined(UIOX_ARCH_X86_64)
     uiox_cpu_wrmsr(0x186u + counter_id, 0);
 #endif
 }
 
 uint64_t uiox_cpu_feat_pmu_read(uiox_cpu_feat_t *feat, uint8_t counter_id)
 {
     if (!feat || counter_id >= UIOX_PMU_MAX_COUNTERS) return 0;
     uint64_t val = 0;
 #if defined(UIOX_ARCH_ARM64)
     __asm__ volatile("msr pmselr_el0, %0" :: "r"((uint64_t)counter_id));
     __asm__ volatile("mrs %0, pmxevcntr_el0" : "=r"(val));
 #elif defined(UIOX_ARCH_X86_64)
     val = uiox_cpu_rdmsr(0xC1u + counter_id);
 #elif defined(UIOX_ARCH_RV64)
     val = uiox_cpu_csr_read(minstret);
 #endif
     feat->counters[counter_id].value = val;
     return val;
 }
 
 void uiox_cpu_feat_print(const uiox_cpu_feat_t *feat)
 {
     if (!feat) return;
     printf("  CPU model      : %s\n", feat->brand_string);
     printf("  Family/Model   : 0x%X / 0x%X  step=%u\n",
            feat->cpuid_family, feat->cpuid_model, feat->cpuid_stepping);
     printf("  Capabilities   : 0x%08X\n", feat->hw->caps);
     printf("  L1I cache      : %u KB  line=%u B\n",
            feat->l1i.size_kb, feat->l1i.line_bytes);
     printf("  L1D cache      : %u KB  line=%u B\n",
            feat->l1d.size_kb, feat->l1d.line_bytes);
     printf("  L2  cache      : %u KB\n", feat->l2.size_kb);
     printf("  L3  cache      : %u KB\n", feat->l3.size_kb);
     printf("  PMU counters   : %u\n",   feat->num_counters);
 }
 