/**
 * @file  uiox_boot_main.c
 * @brief UIOX Bootloader — 7-stage boot pipeline.
 *
 * Stage 1: Hardware init   (UART, GIC/PIC, cache disable)
 * Stage 2: Memory          (DTB/E820 probe → region table)
 * Stage 3: Storage         (FAT32 BPB parse — simulation skips)
 * Stage 4: Load kernel     (read from FAT32 or QEMU simulation path)
 * Stage 5: Verify          (SHA-256 + UIOX image header check)
 * Stage 6: ELF load        (ELF64 segment copy or flat binary)
 * Stage 7: Handoff         (build boot-args → arch jump to kernel)
 *
 * Expected console output:
 *   UIOX Bootloader v1.0 (ARM64) [github.com/Pramod645/UIOX]
 *   [BOOT] Stage 1: HW init
 *   OK
 *   [BOOT] Stage 2: Memory
 *   Memory map (1 regions):
 *     base=0000000040000000 size=0000000004000000 USABLE
 *   Usable: 64 MB
 *   OK
 *   [BOOT] Stage 3: Storage
 *   No storage — simulation mode
 *   [BOOT] Stage 4: Load kernel
 *     No kernel file — QEMU simulation handoff
 *   [BOOT] Stage 5: Verify
 *     No kernel file — QEMU simulation handoff
 *   [BOOT] Stage 6: ELF
 *   Flat binary load
 *   Entry: 0000000040080000
 *   OK
 *   [BOOT] Stage 7: Handoff
 *   args@0000000040070000
 *   entry=0000000040080000
 *   dtb=0000000040000000
 *   cmd: root=/dev/mmcblk0p2 rw quiet console=ttyAMA0
 *   [BOOT] Jumping to kernel...
 *
 * @version 1.0.0
 * @date    2026-06-12
 */

 #include "uiox_boot.h"

 /* Ensure UIOX_UNUSED is defined even if uiox_boot_types.h is older */
 #ifndef UIOX_UNUSED
   #define UIOX_UNUSED(x) ((void)(x))
 #endif
 
 /* =========================================================================
  * Platform configuration
  * ====================================================================== */
 
 #ifndef UIOX_CMDLINE
   #define UIOX_CMDLINE \
       "root=/dev/mmcblk0p2 rw quiet console=ttyAMA0"
 #endif
 
 /* Physical addresses — must match linker scripts */
 #if defined(__aarch64__)
   #define UIOX_KERN_LOAD_PA   0x40080000ULL
   #define UIOX_ARGS_PA        0x40070000ULL
   #define UIOX_DTB_FALLBACK   0x40000000ULL
   #define UIOX_ARCH_ID        UIOX_ARCH_ARM64
   #define UIOX_ARCH_STR       "ARM64"
 #elif defined(__arm__)
   #define UIOX_KERN_LOAD_PA   0x00200000ULL
   #define UIOX_ARGS_PA        0x00180000ULL
   #define UIOX_DTB_FALLBACK   0x00010000ULL
   #define UIOX_ARCH_ID        UIOX_ARCH_ARM32
   #define UIOX_ARCH_STR       "ARM32"
 #else
   #define UIOX_KERN_LOAD_PA   0x00200000ULL
   #define UIOX_ARGS_PA        0x00180000ULL
   #define UIOX_DTB_FALLBACK   0x00000000ULL
   #define UIOX_ARCH_ID        UIOX_ARCH_X86_64
   #define UIOX_ARCH_STR       "x86_64"
 #endif
 
 /* FAT32 filename (8.3 space-padded upper-case, 11 chars + NUL) */
 #define UIOX_KERNEL_FNAME   "KERNEL  BIN"
 
 /* Maximum kernel image accepted */
 #define UIOX_KERN_MAX_SIZE  (32u * 1024u * 1024u)  /* 32 MB */
 
 /* =========================================================================
  * Stub block-device read
  * Returns -1 → signals "no real storage" → simulation path taken.
  * ====================================================================== */
 
 static int sim_blk_read(uint64_t lba, uint32_t count,
                          void *buf, void *priv)
 {
     UIOX_UNUSED(lba);
     UIOX_UNUSED(count);
     UIOX_UNUSED(buf);
     UIOX_UNUSED(priv);
     return -1;
 }
 
 /* =========================================================================
  * Arch HW registration
  * ====================================================================== */
 
 #if defined(__aarch64__)
   extern void uiox_boot_hw_arm64_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_arm64_register()
 #elif defined(__arm__)
   extern void uiox_boot_hw_arm32_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_arm32_register()
   #elif defined(__x86_64__)
   extern void uiox_boot_hw_x86_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_x86_register()
#else //riscv64
   extern void uiox_boot_hw_riscv64_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_riscv64_register()
 #endif
 
 /* =========================================================================
  * uiox_boot_main — 7-stage pipeline
  *
  * @param dtb_pa  Physical address of DTB blob (x0/r0/rdi from entry stub).
  *                Zero on x86 (no DTB).
  * @param x1      Reserved (x1 on ARM64, unused).
  * @param x2      Reserved (x2 on ARM64, unused).
  * ====================================================================== */
 
 void __attribute__((noreturn))
 uiox_boot_main(uint64_t dtb_pa, uint64_t x1, uint64_t x2)
 {
     UIOX_UNUSED(x1);
     UIOX_UNUSED(x2);
 
     /* ================================================================== */
     /* Stage 1: Hardware init                                              */
     /* ================================================================== */
 
     UIOX_HW_REGISTER();        /* registers vtable + calls ops->init()   */
     uiox_boot_console_init();  /* UART already up; hook any extra init    */
 
     BOOT_BANNER(UIOX_ARCH_STR);
     BOOT_LOG(1, "HW init");
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 2: Memory                                                     */
     /* ================================================================== */
 
     BOOT_LOG(2, "Memory");
 
     uiox_mem_map_t  mem_map;
     uiox_boot_err_t rc = uiox_boot_mem_probe(dtb_pa, &mem_map);
     if (rc != UIOX_BOOT_OK)
         BOOT_FATAL("memory probe failed (err=%d)", (int)rc);
 
     uiox_boot_mem_print(&mem_map);
 
     /* Bump allocator — reserve space for boot-args and DTB shadow */
     uiox_bump_alloc_t alloc;
     rc = uiox_boot_mem_alloc_init(&alloc, &mem_map,
                                    (uintptr_t)UIOX_ARGS_PA,
                                    sizeof(uiox_boot_args_t) + 0x10000u);
     if (rc != UIOX_BOOT_OK)
         BOOT_FATAL("bump allocator init failed (err=%d)", (int)rc);
 
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 3: Storage                                                    */
     /* ================================================================== */
 
     BOOT_LOG(3, "Storage");
 
     uiox_fat32_ctx_t fs;
     bool storage_ok = false;
 
     rc = uiox_boot_fs_init(&fs, sim_blk_read, NULL);
     if (rc == UIOX_BOOT_OK) {
         storage_ok = true;
         uiox_boot_puts("FAT32 mounted\n");
     } else {
         uiox_boot_puts("No storage — simulation mode\n");
     }
 
     /* ================================================================== */
     /* Stage 4: Load kernel                                                */
     /* ================================================================== */
 
     BOOT_LOG(4, "Load kernel");
 
     void *load_buf = uiox_boot_mem_alloc(&alloc,
                                           UIOX_KERN_MAX_SIZE + 512u,
                                           4096u);
     if (!load_buf)
         BOOT_FATAL("no memory for kernel load buffer");
 
     size_t   loaded     = 0u;
     bool     has_header = false;
     bool     is_elf     = false;
     uint64_t entry_pa   = UIOX_KERN_LOAD_PA;
 
     if (storage_ok) {
         rc = uiox_boot_fs_load(&fs, UIOX_KERNEL_FNAME,
                                 load_buf, UIOX_KERN_MAX_SIZE, &loaded);
         if (rc == UIOX_BOOT_OK &&
             loaded >= sizeof(uiox_image_hdr_t)) {
             uiox_boot_printf("  Loaded %lu bytes from %s\n",
                               (unsigned long)loaded, UIOX_KERNEL_FNAME);
             has_header = true;
         } else {
             uiox_boot_puts("  File not found — QEMU simulation handoff\n");
         }
     } else {
         uiox_boot_puts("  No kernel file — QEMU simulation handoff\n");
     }
 
     /* ================================================================== */
     /* Stage 5: Verify                                                     */
     /* ================================================================== */
 
     BOOT_LOG(5, "Verify");
 
     if (has_header) {
         const uiox_image_hdr_t *hdr =
             (const uiox_image_hdr_t *)load_buf;
         const uint8_t *payload =
             (const uint8_t *)load_buf + sizeof(uiox_image_hdr_t);
         size_t pay_len = loaded - sizeof(uiox_image_hdr_t);
 
         rc = uiox_boot_verify_image(hdr, payload, pay_len, UIOX_ARCH_ID);
         if (rc != UIOX_BOOT_OK)
             BOOT_FATAL("image verification failed (err=%d)", (int)rc);
 
         /* Detect ELF by magic + class */
         uint32_t magic = 0u;
         uiox_boot_memcpy(&magic, payload, 4u);
         is_elf = (magic == ELF64_MAGIC) &&
                  (pay_len >= sizeof(uiox_elf64_ehdr_t)) &&
                  (((const uiox_elf64_ehdr_t *)payload)->e_ident[4]
                       == ELF_CLASS_64);
 
         entry_pa = hdr->entry_point;
         uiox_boot_printf("  SHA-256 OK  entry=%016llx\n",
                           (unsigned long long)entry_pa);
         BOOT_OK();
     } else {
         uiox_boot_puts("  No kernel file — QEMU simulation handoff\n");
     }
 
     /* ================================================================== */
     /* Stage 6: ELF load / flat binary load                                */
     /* ================================================================== */
 
     BOOT_LOG(6, "ELF");
 
     if (has_header) {
         const uiox_image_hdr_t *hdr =
             (const uiox_image_hdr_t *)load_buf;
         const uint8_t *payload =
             (const uint8_t *)load_buf + sizeof(uiox_image_hdr_t);
         size_t pay_len = loaded - sizeof(uiox_image_hdr_t);
 
         if (is_elf) {
             rc = uiox_boot_elf64_load(payload, pay_len, &entry_pa);
             if (rc != UIOX_BOOT_OK)
                 BOOT_FATAL("ELF64 load failed (err=%d)", (int)rc);
             uiox_boot_printf("  ELF64 loaded  entry=%016llx\n",
                               (unsigned long long)entry_pa);
         } else {
             /* Flat binary: copy directly to load address */
             rc = uiox_boot_flat_load(payload, pay_len,
                                       (uintptr_t)hdr->load_addr);
             if (rc != UIOX_BOOT_OK)
                 BOOT_FATAL("flat binary load failed (err=%d)", (int)rc);
             entry_pa = hdr->entry_point;
             uiox_boot_printf("  Flat binary load\n  Entry: %016llx\n",
                               (unsigned long long)entry_pa);
         }
     } else {
         /* Simulation: no image — hand off to QEMU-loaded kernel address */
         entry_pa = UIOX_KERN_LOAD_PA;
         uiox_boot_printf("  Flat binary load\n  Entry: %016llx\n",
                           (unsigned long long)entry_pa);
     }
 
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 7: Handoff                                                    */
     /* ================================================================== */
 
     BOOT_LOG(7, "Handoff");
 
     uint64_t final_dtb = (dtb_pa != 0u) ? dtb_pa : UIOX_DTB_FALLBACK;
 
     /* uiox_boot_handoff() never returns */
     uiox_boot_handoff(entry_pa,
                        final_dtb,
                        (uint64_t)UIOX_ARGS_PA,
                        &mem_map,
                        UIOX_CMDLINE);
 
     /* Unreachable — satisfy noreturn */
     for (;;)
         __asm__ volatile("" ::: "memory");
 }
 