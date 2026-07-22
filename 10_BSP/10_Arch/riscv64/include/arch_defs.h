#ifndef UIOX_ARCH_DEFS_RISCV64_H
#define UIOX_ARCH_DEFS_RISCV64_H
/*
 * 10_Arch/riscv64/include/arch_defs.h
 * RISC-V RV64IMAFDC_Zicsr architecture definitions.
 * Targets QEMU virt and SiFive U74 (HiFive Unmatched).
 */

/* ── Architecture identity ────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "riscv64"
#define UIOX_ARCH_BITS          64
#define UIOX_ARCH_LITTLE_ENDIAN 1

/* ── Word / pointer sizes ─────────────────────────────────────────── */
#define UIOX_PTR_SIZE           8
#define UIOX_PAGE_SIZE          4096
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         64

/* ── Physical memory map (QEMU virt machine defaults) ────────────────
 * Overridden per SoC by 03_SoC/include/uiox_soc_map.h.
 * ──────────────────────────────────────────────────────────────────── */
#define PHYS_DRAM_BASE          0x80000000UL
#define PHYS_DRAM_SIZE          0x08000000UL   /* 128 MB                */
#define PHYS_MMIO_BASE          0x10000000UL
#define PHYS_MMIO_SIZE          0x10000000UL

/* ── CLINT (Core Local INTerruptor) ───────────────────────────────── */
#define CLINT_BASE              0x02000000UL
#define CLINT_SIZE              0x00010000UL
#define CLINT_MSIP(hart)        (CLINT_BASE + 0x0000UL + (unsigned long)(hart) * 4UL)
#define CLINT_MTIMECMP(hart)    (CLINT_BASE + 0x4000UL + (unsigned long)(hart) * 8UL)
#define CLINT_MTIME             (CLINT_BASE + 0xBFF8UL)

/* ── PLIC (Platform-Level Interrupt Controller) ───────────────────── */
#define PLIC_BASE               0x0C000000UL
#define PLIC_SIZE               0x04000000UL

/* PLIC register macros */
#define PLIC_PRIORITY(irq)      (PLIC_BASE + 4UL * (unsigned long)(irq))
#define PLIC_PENDING(word)      (PLIC_BASE + 0x1000UL + 4UL * (word))
#define PLIC_ENABLE(ctx, word)  (PLIC_BASE + 0x2000UL + 0x80UL*(ctx)  + 4UL*(word))
#define PLIC_THRESHOLD(ctx)     (PLIC_BASE + 0x200000UL + 0x1000UL*(ctx))
#define PLIC_CLAIM(ctx)         (PLIC_BASE + 0x200004UL + 0x1000UL*(ctx))

/* ── CSR numbers (Zicsr extension) ───────────────────────────────── */
#define CSR_MSTATUS             0x300U
#define CSR_MISA                0x301U
#define CSR_MEDELEG             0x302U
#define CSR_MIDELEG             0x303U
#define CSR_MIE                 0x304U
#define CSR_MTVEC               0x305U
#define CSR_MSCRATCH            0x340U
#define CSR_MEPC                0x341U
#define CSR_MCAUSE              0x342U
#define CSR_MTVAL               0x343U
#define CSR_MIP                 0x344U
#define CSR_MVENDORID           0xF11U
#define CSR_MARCHID             0xF12U
#define CSR_MIMPID              0xF13U
#define CSR_MHARTID             0xF14U

#define CSR_SSTATUS             0x100U
#define CSR_SIE                 0x104U
#define CSR_STVEC               0x105U
#define CSR_SSCRATCH            0x140U
#define CSR_SEPC                0x141U
#define CSR_SCAUSE              0x142U
#define CSR_STVAL               0x143U
#define CSR_SIP                 0x144U
#define CSR_SATP                0x180U

#define CSR_MTIME               0xC01U    /* read-only, use CLINT_MTIME */
#define CSR_MINSTRET            0xC02U

/* ── mstatus / sstatus bits ───────────────────────────────────────── */
#define MSTATUS_MIE             (1UL <<  3)   /* Machine IRQ enable     */
#define MSTATUS_MPIE            (1UL <<  7)   /* Previous MIE           */
#define MSTATUS_SPP             (1UL <<  8)   /* S-mode prev privilege  */
#define MSTATUS_MPP_M           (3UL << 11)   /* M-mode prev privilege  */
#define MSTATUS_MPP_S           (1UL << 11)
#define MSTATUS_MPP_U           (0UL << 11)
#define MSTATUS_SIE             (1UL <<  1)   /* Supervisor IRQ enable  */

/* ── mie / sie bits ───────────────────────────────────────────────── */
#define MIE_MSIE                (1UL << 3)    /* Machine SW interrupt   */
#define MIE_MTIE                (1UL << 7)    /* Machine timer interrupt*/
#define MIE_MEIE                (1UL << 11)   /* Machine ext interrupt  */
#define SIE_SSIE                (1UL << 1)    /* Supervisor SW          */
#define SIE_STIE                (1UL << 5)    /* Supervisor timer       */
#define SIE_SEIE                (1UL << 9)    /* Supervisor ext (PLIC)  */

/* ── satp modes ───────────────────────────────────────────────────── */
#define SATP_MODE_BARE          (0UL << 60)
#define SATP_MODE_SV39          (8UL << 60)
#define SATP_MODE_SV48          (9UL << 60)
#define SATP_ASID(a)            ((unsigned long)(a) << 44)
#define SATP_PPN(ppn)           ((unsigned long)(ppn))

/* ── Privilege levels ─────────────────────────────────────────────── */
#define ARCH_PRIV_U             0U
#define ARCH_PRIV_S             1U
#define ARCH_PRIV_M             3U

/* ── CSR read / write inline helpers ─────────────────────────────── */
/* These are inline functions (not statement-expression macros) so they
 * compile cleanly with -Wpedantic / -std=c11. */

static inline unsigned long arch_csrr_misa(void)
    { unsigned long v; __asm__ volatile("csrr %0, misa"      :"=r"(v)); return v; }
static inline unsigned long arch_csrr_mvendorid(void)
    { unsigned long v; __asm__ volatile("csrr %0, mvendorid" :"=r"(v)); return v; }
static inline unsigned long arch_csrr_marchid(void)
    { unsigned long v; __asm__ volatile("csrr %0, marchid"   :"=r"(v)); return v; }
static inline unsigned long arch_csrr_mhartid(void)
    { unsigned long v; __asm__ volatile("csrr %0, mhartid"   :"=r"(v)); return v; }
static inline unsigned long arch_csrr_sie(void)
    { unsigned long v; __asm__ volatile("csrr %0, sie"       :"=r"(v)); return v; }
static inline unsigned long arch_csrr_mstatus(void)
    { unsigned long v; __asm__ volatile("csrr %0, mstatus"   :"=r"(v)); return v; }
    static inline void arch_csrw_sie(unsigned long v)
    { __asm__ volatile("csrw sie, %0" :: "r"(v) : "memory"); }
static inline void arch_csrw_mstatus(unsigned long v)
    { __asm__ volatile("csrw mstatus, %0" :: "r"(v) : "memory"); }
static inline void arch_csrw_mtvec(unsigned long v)
    { __asm__ volatile("csrw mtvec, %0" :: "r"(v) : "memory"); }
static inline void arch_csrw_stvec(unsigned long v)
    { __asm__ volatile("csrw stvec, %0" :: "r"(v) : "memory"); }
static inline void arch_csrw_satp(unsigned long v)
    { __asm__ volatile("csrw satp, %0" :: "r"(v) : "memory"); }
static inline void arch_csrs_sie(unsigned long bits)
    { __asm__ volatile("csrs sie, %0" :: "r"(bits) : "memory"); }
static inline void arch_csrc_sie(unsigned long bits)
    { __asm__ volatile("csrc sie, %0" :: "r"(bits) : "memory"); }
static inline void arch_csrs_sstatus(unsigned long bits)
    { __asm__ volatile("csrs sstatus, %0" :: "r"(bits) : "memory"); }
static inline void arch_csrc_sstatus(unsigned long bits)
    { __asm__ volatile("csrc sstatus, %0" :: "r"(bits) : "memory"); }

/* ── Barrier macros ───────────────────────────────────────────────── */
#define arch_dsb_sy()     __asm__ volatile("fence"        ::: "memory")
#define arch_dsb_rw()     __asm__ volatile("fence rw,rw"  ::: "memory")
#define arch_dsb_r()      __asm__ volatile("fence r,r"    ::: "memory")
#define arch_dsb_w()      __asm__ volatile("fence w,w"    ::: "memory")
#define arch_isb()        __asm__ volatile("fence.i"      ::: "memory")
#define arch_wfi()        __asm__ volatile("wfi")
#define arch_nop()        __asm__ volatile("nop")

/* ── TLB invalidation (RISC-V sfence.vma) ─────────────────────────── */
#define arch_tlbi_all() \
    __asm__ volatile("sfence.vma" ::: "memory")
#define arch_tlbi_mva(va) \
    __asm__ volatile("sfence.vma %0, zero" :: "r"(va) : "memory")
#define arch_tlbi_asid(asid) \
    __asm__ volatile("sfence.vma zero, %0" :: "r"(asid) : "memory")
#define arch_tlbi_mva_asid(va, asid) \
    __asm__ volatile("sfence.vma %0, %1"   :: "r"(va), "r"(asid) : "memory")

/* ── Alignment helpers ────────────────────────────────────────────── */
#define ARCH_ALIGN_UP(v, a)     (((v) + ((a) - 1UL)) & ~((a) - 1UL))
#define ARCH_ALIGN_DOWN(v, a)   ((v) & ~((a) - 1UL))
#define ARCH_IS_ALIGNED(v, a)   (((v) & ((a) - 1UL)) == 0UL)
#define ARCH_PAGE_ALIGN(v)      ARCH_ALIGN_UP(v, UIOX_PAGE_SIZE)

/* ── SBI ecall helper ─────────────────────────────────────────────── */
static inline long arch_sbi_call(unsigned long ext,
                                   unsigned long fid,
                                   unsigned long a0,
                                   unsigned long a1,
                                   unsigned long a2)
{
    register unsigned long ra0 __asm__("a0") = a0;
    register unsigned long ra1 __asm__("a1") = a1;
    register unsigned long ra2 __asm__("a2") = a2;
    register unsigned long ra6 __asm__("a6") = fid;
    register unsigned long ra7 __asm__("a7") = ext;
    __asm__ volatile("ecall"
                     : "+r"(ra0)
                     : "r"(ra1), "r"(ra2), "r"(ra6), "r"(ra7)
                     : "memory");
    return (long)ra0;
}

/* SBI extension IDs */
#define SBI_EXT_BASE            0x10UL
#define SBI_EXT_TIME            0x54494D45UL  /* "TIME" */
#define SBI_EXT_IPI             0x735049UL    /* "sPI"  */
#define SBI_EXT_RFNC            0x52464E43UL  /* "RFNC" */
#define SBI_EXT_HSM             0x48534D55L   /* "HSMU" */
#ifndef SBI_EXT_SRST
#  define SBI_EXT_SRST          0x53525354UL  /* "SRST" */
#endif

/* SBI Base function IDs */
#define SBI_BASE_GET_SPEC_VER   0UL
#define SBI_BASE_GET_IMPL_ID    1UL
#define SBI_BASE_PROBE_EXT      3UL

/* SBI SRST function IDs */
#define SBI_SRST_SYSTEM_RESET   0UL
#define SBI_SRST_TYPE_SHUTDOWN  0UL
#define SBI_SRST_TYPE_COLD_REBOOT 1UL
#define SBI_SRST_TYPE_WARM_REBOOT 2UL

/* ── Compiler helpers ─────────────────────────────────────────────── */
#define ARCH_INLINE             static inline __attribute__((always_inline))
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_USED               __attribute__((used))
#define ARCH_UNUSED(x)          ((void)(x))

#endif /* UIOX_ARCH_DEFS_RISCV64_H */
