/*
 * 10_BSP/src/uiox_bsp_main.c
 *
 * UIOX Board-Support Package — main source.
 *
 * This file is the single entry-point for the BSP regardless of build mode.
 *
 * Static build  (UIOX_BSP_STATIC_BUILD):
 *   Kernel links this file in.  uiox_kernel_main() calls uiox_bsp_init().
 *   Flow:
 *     uiox_kernel_main()
 *       └─► uiox_bsp_init()        [this file]
 *             ├─► arch_init()       [10_BSP/10_Arch/<arch>/src/arch_init.c]
 *             └─► uiox_soc_init()   [10_BSP/03_SoC/src/uiox_soc_main.c]
 *
 * Dynamic build (UIOX_BSP_DYNAMIC_BUILD):
 *   BSP is a standalone ELF / binary.
 *   Primary bootloader (uiox_boot_arch_jump) jumps to uiox_bsp_entry().
 *   After board bring-up the BSP loads the kernel and calls
 *   uiox_bsp_jump_to_kernel().
 *   Flow:
 *     uiox_boot_arch_jump()         [01_uBoot/src/uiox_boot_handoff.c]
 *       └─► uiox_bsp_entry()        [this file — bsp_entry.S stub → here]
 *             ├─► arch_init()
 *             ├─► uiox_soc_init()
 *             ├─► load_kernel_elf() [dynamic only]
 *             └─► uiox_bsp_jump_to_kernel()
 *                   └─► uiox_kernel_main()
 *
 * @version 1.1.0
 * @date    2026-07-21
 */

 #include "../include/uiox_bsp.h"

 /* Subsystem init — implemented in sibling directories */
 extern int arch_init(void);           /* 10_BSP/10_Arch/<arch>/src/arch_init.c */
 extern int uiox_soc_init(void);       /* 10_BSP/03_SoC/src/uiox_soc_main.c    */
 
 /* BSS / stack symbols from linker script (dynamic build) */
 #if defined(UIOX_BSP_DYNAMIC_BUILD)
 extern uint8_t _bss_start[];
 extern uint8_t _bss_end[];
 extern uint8_t _stack_top[];
 #endif
 
 /* ── Module-private state ─────────────────────────────────────────────────── */
 static uiox_bsp_config_t g_bsp_cfg;
 static int                g_bsp_ready = 0;
 
 /* ── Minimal early UART (same pattern as uiox_kernel_main.c) ─────────────── */
 static void bsp_putc(char c)
 {
 #if defined(__aarch64__)
     volatile uint32_t *fr = (volatile uint32_t *)(0x09000000UL + 0x018u);
     volatile uint32_t *dr = (volatile uint32_t *)(0x09000000UL + 0x000u);
     while (*fr & (1u << 5u)) {}
     *dr = (uint32_t)(uint8_t)c;
 #elif defined(__arm__)
     volatile uint32_t *fr = (volatile uint32_t *)(0x10009000UL + 0x018u);
     volatile uint32_t *dr = (volatile uint32_t *)(0x10009000UL + 0x000u);
     while (*fr & (1u << 5u)) {}
     *dr = (uint32_t)(uint8_t)c;
 #elif defined(__x86_64__)
     while (!(({ uint8_t v;
                 __asm__ volatile("inb %1,%0":"=a"(v):"Nd"((uint16_t)0x3FDu));
                 v; }) & (1u << 5u))) {}
     __asm__ volatile("outb %0,%1" :: "a"((uint8_t)c), "Nd"((uint16_t)0x3F8u));
 #elif defined(__riscv)
     volatile uint32_t *lsr = (volatile uint32_t *)(0x10000000UL + 0x05u);
     volatile uint32_t *thr = (volatile uint32_t *)(0x10000000UL + 0x00u);
     while (!(*lsr & (1u << 5u))) {}
     *thr = (uint32_t)(uint8_t)c;
 #endif
 }
 
 static void bsp_puts(const char *s)
 {
     while (*s) {
         if (*s == '\n') bsp_putc('\r');
         bsp_putc(*s++);
     }
 }
 
 /* ── BSS zero (dynamic build — no libc yet) ──────────────────────────────── */
 #if defined(UIOX_BSP_DYNAMIC_BUILD)
 static void bsp_bss_zero(void)
 {
     uint8_t *p = _bss_start;
     while (p < _bss_end) *p++ = 0u;
 }
 #endif
 
 /* =========================================================================
  * uiox_bsp_init()
  *
  * Static-build path: called from uiox_kernel_main() before arch_init().
  * ====================================================================== */
 int uiox_bsp_init(const uiox_bsp_config_t *cfg)
 {
     int rc;
 
     if (!cfg) return UIOX_BSP_ERR_MEM;
 
     g_bsp_cfg = *cfg;
 
     if (!(cfg->flags & UIOX_BSP_FL_SILENT))
         bsp_puts("[bsp] uiox_bsp_init() — static build\n");
 
     /* 1. Architecture init: interrupt controller, MMU, caches */
     if (!(cfg->flags & UIOX_BSP_FL_SILENT))
         bsp_puts("[bsp]   arch_init()...\n");
     rc = arch_init();
     if (rc != UIOX_BSP_OK) {
         bsp_puts("[bsp] FATAL: arch_init() failed\n");
         return UIOX_BSP_ERR_ARCH;
     }
 
     /* 2. SoC init: clock tree, PM, peripheral reset */
     if (!(cfg->flags & UIOX_BSP_FL_SILENT))
         bsp_puts("[bsp]   uiox_soc_init()...\n");
     rc = uiox_soc_init();
     if (rc != UIOX_BSP_OK) {
         bsp_puts("[bsp] FATAL: uiox_soc_init() failed\n");
         return UIOX_BSP_ERR_SOC;
     }
 
     g_bsp_ready = 1;
 
     if (!(cfg->flags & UIOX_BSP_FL_SILENT))
         bsp_puts("[bsp] uiox_bsp_init() complete\n");
 
     return UIOX_BSP_OK;
 }
 
 /* =========================================================================
  * uiox_bsp_late_init()
  *
  * Optional second-phase init after MMU is enabled.
  * ====================================================================== */
 int uiox_bsp_late_init(const uiox_bsp_config_t *cfg)
 {
     (void)cfg;
     /* Placeholder: add per-board late-init (e.g. PCIe, USB, storage) here */
     bsp_puts("[bsp] uiox_bsp_late_init() — nothing to do\n");
     return UIOX_BSP_OK;
 }
 
 /* =========================================================================
  * Dynamic-build path
  * ====================================================================== */
 #if defined(UIOX_BSP_DYNAMIC_BUILD)
 
 /* ── Minimal ELF64 kernel loader ─────────────────────────────────────────── */
 /*
  * Base types for the ELF loader structs below.
  *
  * uiox_bsp.h (included at the top of this file) already pulls in
  * uiox_base_types.h with UIOX_BASETYPES_COMPAT active, so uint8_t,
  * uint16_t, uint32_t, uint64_t, and uintptr_t are already available.
  *
  * The guard below protects against a hypothetical standalone translation
  * unit that includes uiox_bsp_main.c without going through uiox_bsp.h.
  * In normal BSP builds this block is a no-op.
  *
  * Rule: do NOT add <stdint.h> or <stddef.h> here — the Makefile passes
  * -nostdinc to enforce the fully freestanding type system.
  */
 #ifndef UIOX_BASETYPES_COMPAT
 #  define UIOX_BASETYPES_COMPAT
 #endif
 #ifndef UIOX_BASE_TYPES_H
 #  include "uiox_base_types.h"   /* 10_BSP/03_SoC/include/uiox_base_types.h */
 #endif
 
 #define ELF_MAGIC   0x464C457Fu   /* 0x7F 'E' 'L' 'F' */
 
 typedef struct {
     uint8_t  e_ident[16];
     uint16_t e_type;
     uint16_t e_machine;
     uint32_t e_version;
     uint64_t e_entry;
     uint64_t e_phoff;
     uint64_t e_shoff;
     uint32_t e_flags;
     uint16_t e_ehsize;
     uint16_t e_phentsize;
     uint16_t e_phnum;
     uint16_t e_shentsize;
     uint16_t e_shnum;
     uint16_t e_shstrndx;
 } Elf64_Ehdr;
 
 typedef struct {
     uint32_t p_type;
     uint32_t p_flags;
     uint64_t p_offset;
     uint64_t p_vaddr;
     uint64_t p_paddr;
     uint64_t p_filesz;
     uint64_t p_memsz;
     uint64_t p_align;
 } Elf64_Phdr;
 
 #define PT_LOAD  1u
 
 static int load_kernel_elf(uint64_t elf_pa, uint64_t *out_entry)
 {
     const uint8_t    *base  = (const uint8_t *)(uintptr_t)elf_pa;
     const Elf64_Ehdr *ehdr  = (const Elf64_Ehdr *)base;
 
     /* Validate ELF magic */
     uint32_t magic;
     __builtin_memcpy(&magic, ehdr->e_ident, 4);
     if (magic != ELF_MAGIC) {
         bsp_puts("[bsp] load_kernel_elf: bad ELF magic\n");
         return UIOX_BSP_ERR_LOAD;
     }
 
     /* Walk PT_LOAD segments and copy them to their physical addresses */
     const Elf64_Phdr *phdr =
         (const Elf64_Phdr *)(base + ehdr->e_phoff);
 
     for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
         if (phdr[i].p_type != PT_LOAD) continue;
 
         uint8_t       *dst = (uint8_t *)(uintptr_t)phdr[i].p_paddr;
         const uint8_t *src = base + phdr[i].p_offset;
 
         /* Copy file bytes */
         for (uint64_t b = 0; b < phdr[i].p_filesz; b++)
             dst[b] = src[b];
 
         /* Zero remainder (BSS within this segment) */
         for (uint64_t b = phdr[i].p_filesz; b < phdr[i].p_memsz; b++)
             dst[b] = 0u;
     }
 
     *out_entry = ehdr->e_entry;
     return UIOX_BSP_OK;
 }
 
 /* ── uiox_bsp_entry() — called from bsp_entry.S after stack/BSS setup ────── */
 void __attribute__((noreturn)) uiox_bsp_entry_c(uint64_t dtb_pa,
                                                   uint64_t args_pa);
 
 /*
  * uiox_bsp_entry() is an alias for the assembly stub in bsp_entry.S.
  * The C implementation is uiox_bsp_entry_c(), entered after the stub
  * zeroes BSS and sets the stack pointer.
  */
 void __attribute__((noreturn))
 uiox_bsp_entry_c(uint64_t dtb_pa, uint64_t args_pa)
 {
     int      rc;
     uint64_t kernel_entry = 0;
 
     bsp_puts("\r\n[bsp] UIOX BSP entry — dynamic build\n");
 
     /* Populate config from boot args */
     g_bsp_cfg.dtb_pa         = dtb_pa;
     g_bsp_cfg.args_pa        = args_pa;
     g_bsp_cfg.flags          = UIOX_BSP_FL_DYN_LOAD;
 
     /* 1. Architecture init */
     bsp_puts("[bsp]   arch_init()...\n");
     rc = arch_init();
     if (rc != 0) {
         bsp_puts("[bsp] FATAL: arch_init failed\n");
         for (;;)
 #if defined(__x86_64__)
             __asm__ volatile("hlt");
 #else
             __asm__ volatile("wfi");
 #endif
     }
 
     /* 2. SoC init */
     bsp_puts("[bsp]   uiox_soc_init()...\n");
     rc = uiox_soc_init();
     if (rc != 0) {
         bsp_puts("[bsp] FATAL: uiox_soc_init failed\n");
         for (;;)
 #if defined(__x86_64__)
             __asm__ volatile("hlt");
 #else
             __asm__ volatile("wfi");
 #endif
     }
 
     /* 3. Load kernel ELF from flash / storage into DRAM */
     bsp_puts("[bsp]   load_kernel_elf()...\n");
     rc = load_kernel_elf(g_bsp_cfg.kernel_load_pa, &kernel_entry);
     if (rc != UIOX_BSP_OK) {
         bsp_puts("[bsp] FATAL: kernel ELF load failed\n");
         for (;;)
 #if defined(__x86_64__)
             __asm__ volatile("hlt");
 #else
             __asm__ volatile("wfi");
 #endif
     }
 
     g_bsp_cfg.kernel_entry = kernel_entry;
     g_bsp_ready = 1;
 
     bsp_puts("[bsp]   jumping to kernel...\n");
     uiox_bsp_jump_to_kernel(kernel_entry, dtb_pa, args_pa);
     /* never reached */
     for (;;);
 }
 
 /* ── uiox_bsp_jump_to_kernel() ───────────────────────────────────────────── */
 void __attribute__((noreturn))
 uiox_bsp_jump_to_kernel(uint64_t kernel_entry,
                          uint64_t dtb_pa,
                          uint64_t args_pa)
 {
     bsp_puts("[bsp] uiox_bsp_jump_to_kernel()\n");
 
 #if defined(__aarch64__)
     __asm__ volatile(
         "mov  x0,  %0\n"
         "mov  x1,  %1\n"
         "mov  x2,  xzr\n"
         "mov  x3,  xzr\n"
         "dsb  sy\n"
         "isb\n"
         "br   %2\n"
         :: "r"(dtb_pa), "r"(args_pa), "r"(kernel_entry)
         : "x0","x1","x2","x3","memory");
 
 #elif defined(__arm__)
     __asm__ volatile(
         "mov  r0,  #0\n"
         "mov  r1,  #0\n"
         "mov  r2,  %0\n"
         "mov  r3,  %1\n"
         "dsb\n"
         "isb\n"
         "bx   %2\n"
         :: "r"((uint32_t)dtb_pa), "r"((uint32_t)args_pa),
            "r"((uint32_t)kernel_entry)
         : "r0","r1","r2","r3","memory");
 
 #elif defined(__x86_64__)
     /* SysV ABI: rdi=args_pa, rsi=dtb_pa */
     __asm__ volatile(
         "mov  %0, %%rdi\n"
         "mov  %1, %%rsi\n"
         "xor  %%rdx, %%rdx\n"
         "jmpq *%2\n"
         :: "r"(args_pa), "r"(dtb_pa), "r"(kernel_entry)
         : "rdi","rsi","rdx","memory");
 
 #elif defined(__riscv)
     __asm__ volatile(
         "mv   a0, %0\n"
         "mv   a1, %1\n"
         "fence\n"
         "jr   %2\n"
         :: "r"(dtb_pa), "r"(args_pa), "r"(kernel_entry)
         : "a0","a1","memory");
 #endif
     /* suppress noreturn warning */
     for (;;);
 }
 
 #endif /* UIOX_BSP_DYNAMIC_BUILD */
 
 /* ── Public accessor ─────────────────────────────────────────────────────── */
 const uiox_bsp_config_t *uiox_bsp_get_config(void)
 {
     return g_bsp_ready ? &g_bsp_cfg : NULL;
 }