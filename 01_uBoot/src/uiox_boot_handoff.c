/**
 * @file  uiox_boot_handoff.c
 * @brief UIOX Bootloader — ELF64 loader, boot-args builder, kernel jump.
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 /* =========================================================================
  * ELF64 loader
  * ====================================================================== */
 
 uiox_boot_err_t uiox_boot_elf64_load(const void *elf_buf, size_t elf_size,
                                        uint64_t *entry_pa)
 {
     if (!elf_buf || !entry_pa) return UIOX_BOOT_ERR_INVAL;
 
     const uint8_t *p = (const uint8_t *)elf_buf;
 
     /* Magic check */
     uint32_t magic;
     uiox_boot_memcpy(&magic, p, 4u);
     if (magic != ELF64_MAGIC) return UIOX_BOOT_ERR_BADMAGIC;
 
     /* Class / data encoding */
     if (p[4] != ELF_CLASS_64)   return UIOX_BOOT_ERR_UNSUP;
     if (p[5] != ELF_DATA_2LSB)  return UIOX_BOOT_ERR_UNSUP;
 
     const uiox_elf64_ehdr_t *eh = (const uiox_elf64_ehdr_t *)elf_buf;
     *entry_pa = eh->e_entry;
 
     /* Walk program headers and load PT_LOAD segments */
     const uint8_t *ph_base = p + eh->e_phoff;
     for (uint16_t i = 0u; i < eh->e_phnum; i++) {
         const uiox_elf64_phdr_t *ph =
             (const uiox_elf64_phdr_t *)(ph_base +
              (uint64_t)i * eh->e_phentsize);
 
         if (ph->p_type != PT_LOAD)   continue;
         if (ph->p_filesz == 0u)      continue;
 
         /* Bounds check against buffer */
         if (ph->p_offset + ph->p_filesz > (uint64_t)elf_size) {
             BOOT_ERR("ELF segment out of bounds");
             return UIOX_BOOT_ERR_BADMAGIC;
         }
 
         /* Copy segment to physical address */
         uiox_boot_memcpy((void *)(uintptr_t)ph->p_paddr,
                         (const uint8_t *)elf_buf + ph->p_offset,
                           (size_t)ph->p_filesz);
 
         /* Zero BSS tail (p_memsz > p_filesz) */
         if (ph->p_memsz > ph->p_filesz) {
             uiox_boot_memset(
                 (void *)(uintptr_t)(ph->p_paddr + ph->p_filesz),
                 0,
                 (size_t)(ph->p_memsz - ph->p_filesz));
         }
 
         uiox_boot_printf("  ELF seg: paddr=%016llx filesz=%llu memsz=%llu\n",
                           (unsigned long long)ph->p_paddr,
                           (unsigned long long)ph->p_filesz,
                           (unsigned long long)ph->p_memsz);
     }
 
     uiox_boot_hw_dcache_flush(0u, 0u);  /* platform flush */
     uiox_boot_hw_icache_inv();
     return UIOX_BOOT_OK;
 }
 
 uiox_boot_err_t uiox_boot_flat_load(const void *src, size_t size,
                                       uintptr_t dest_pa)
 {
     if (!src || size == 0u) return UIOX_BOOT_ERR_INVAL;
     uiox_boot_memcpy((void *)(uintptr_t)dest_pa, src, size);
     uiox_boot_hw_dcache_flush(dest_pa, size);
     uiox_boot_hw_icache_inv();
     uiox_boot_printf("  Flat binary load: %lu bytes → %p\n",
                       (unsigned long)size, (void *)dest_pa);
     return UIOX_BOOT_OK;
 }
 
 /* =========================================================================
  * Boot args builder
  * ====================================================================== */
 
 static void build_args(uiox_boot_args_t *args,
                         uint64_t kernel_entry,
                         uint64_t dtb_pa,
                         uint64_t args_pa,
                         const uiox_mem_map_t *mem_map,
                         const char *cmdline,
                         uiox_arch_t arch)
 {
     uiox_boot_memset(args, 0, sizeof(*args));
     args->magic         = UIOX_BOOT_ARGS_MAGIC;
     args->version       = UIOX_BOOT_ARGS_VERSION;
     args->kernel_entry  = kernel_entry;
     args->dtb_pa        = dtb_pa;
     args->initrd_pa     = 0u;
     args->initrd_size   = 0u;
     args->args_pa       = args_pa;
     args->arch          = arch;
     uiox_boot_memcpy(&args->mem_map, mem_map, sizeof(*mem_map));
 
     /* Copy cmdline */
     size_t clen = uiox_boot_strlen(cmdline);
     if (clen >= UIOX_IMAGE_CMDLINE_MAX)
         clen = UIOX_IMAGE_CMDLINE_MAX - 1u;
     uiox_boot_memcpy(args->cmdline, cmdline, clen);
     args->cmdline[clen] = '\0';
 }
 
 /* =========================================================================
  * Arch-specific final jump (defined in each arch hw file)
  * ====================================================================== */
 
 /* Declared extern; defined per-arch below */
 extern void __attribute__((noreturn))
 uiox_boot_arch_jump(uint64_t entry, uint64_t dtb_pa, uint64_t args_pa);
 
 void __attribute__((noreturn))
 uiox_boot_handoff(uint64_t kernel_entry,
                    uint64_t dtb_pa,
                    uint64_t args_pa,
                    const uiox_mem_map_t *mem_map,
                    const char *cmdline)
 {
     uiox_boot_args_t *args = (uiox_boot_args_t *)(uintptr_t)args_pa;
     build_args(args, kernel_entry, dtb_pa, args_pa,
                mem_map, cmdline,
 #if   defined(__aarch64__)
                UIOX_ARCH_ARM64
 #elif defined(__arm__)
                UIOX_ARCH_ARM32
#elif defined(__x86_64__)
                UIOX_ARCH_X86_64
#else
                UIOX_ARCH_RV64
 #endif
               );
 
     uiox_boot_printf("args@%016llx\nentry=%016llx\ndtb=%016llx\ncmd: %s\n",
                       (unsigned long long)args_pa,
                       (unsigned long long)kernel_entry,
                       (unsigned long long)dtb_pa,
                       args->cmdline);
     uiox_boot_puts("[BOOT] Jumping to kernel...\n");
 
     /* Flush console FIFO before jumping */
     uiox_boot_hw_barrier();
 
     uiox_boot_arch_jump(kernel_entry, dtb_pa, args_pa);
 }
 
 /* =========================================================================
  * Per-arch jump stubs (inline assembly, never returns)
  * ====================================================================== */
 
 #if defined(__aarch64__)
 
 void __attribute__((noreturn))
 uiox_boot_arch_jump(uint64_t entry, uint64_t dtb_pa, uint64_t args_pa)
 {
     /*
      * AArch64 Linux / UIOX boot convention:
      *   x0 = dtb_pa
      *   x1 = args_pa  (UIOX extension)
      *   x2–x3 = 0
      */
     __asm__ volatile(
         "mov  x4, %0\n"   /* entry  */
         "mov  x0, %1\n"   /* dtb_pa */
         "mov  x1, %2\n"   /* args_pa */
         "mov  x2, xzr\n"
         "mov  x3, xzr\n"
         "dsb  sy\n"
         "isb\n"
         "br   x4\n"
         :: "r"(entry), "r"(dtb_pa), "r"(args_pa)
         : "x0","x1","x2","x3","x4","memory");
     for (;;) __asm__ volatile("wfi");
 }
 
 #elif defined(__arm__)
 
 void __attribute__((noreturn))
 uiox_boot_arch_jump(uint64_t entry, uint64_t dtb_pa, uint64_t args_pa)
 {
     /*
      * ARMv7 Linux boot convention:
      *   r0 = 0 (machine type unused with DTB)
      *   r1 = 0
      *   r2 = dtb_pa / atag ptr
      */
     uint32_t e = (uint32_t)entry;
     uint32_t d = (uint32_t)dtb_pa;
     uint32_t a = (uint32_t)args_pa;
     __asm__ volatile(
         "mov  r9, %0\n"   /* entry    */
         "mov  r2, %1\n"   /* dtb_pa   */
         "mov  r3, %2\n"   /* args_pa  (UIOX ext) */
         "mov  r0, #0\n"
         "mov  r1, #0\n"
         "dsb\n"
         "isb\n"
         "bx   r9\n"
         :: "r"(e), "r"(d), "r"(a)
         : "r0","r1","r2","r3","r9","memory");
     for (;;) __asm__ volatile("wfi");
 }
 
 #elif defined(__x86_64__) /* x86_64 */
 
 void __attribute__((noreturn))
 uiox_boot_arch_jump(uint64_t entry, uint64_t dtb_pa, uint64_t args_pa)
 {
     /*
      * x86_64 SysV ABI: args in rdi, rsi, rdx.
      * Kernel receives: rdi=args_pa, rsi=dtb_pa (0 on x86)
      */
     __asm__ volatile(
         "movq %2, %%rdi\n"   /* args_pa */
         "movq %1, %%rsi\n"   /* dtb_pa  */
         "xorq %%rdx, %%rdx\n"
         "mfence\n"
         "jmpq *%0\n"
         :: "r"(entry), "r"(dtb_pa), "r"(args_pa)
         : "rdi","rsi","rdx","memory");
     for (;;) __asm__ volatile("hlt");
 }
 
 #else  /* riscv64 */

 void __attribute__((noreturn))
 uiox_boot_arch_jump(uint64_t entry, uint64_t dtb_pa, uint64_t args_pa)
 {
     /*
      * x86_64 SysV ABI: args in rdi, rsi, rdx.
      * Kernel receives: rdi=args_pa, rsi=dtb_pa (0 on x86)
      */
     __asm__ volatile(
        "mv   a0, %1\n\t"      /* a0 = dtb_pa   */
        "mv   a1, %2\n\t"      /* a1 = args_pa  */
        "jr   %0\n\t"          /* jump to entry (t0 used as base) */
        :
        : "r"((uintptr_t)entry),
          "r"((uintptr_t)dtb_pa),
          "r"((uintptr_t)args_pa)
        : "a0", "a1", "memory");
     for (;;);
 }

 #endif
 