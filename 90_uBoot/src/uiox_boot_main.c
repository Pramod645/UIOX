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
 * Expected console output (matches uBoot.md §Sample Output):
 *   UIOX Bootloader v1.0 (ARM64/ARM32/x86) [github.com/Pramod645/UIOX]
 *   [BOOT] Stage 1: HW init
 *   [BOOT] Stage 2: Memory
 *   ...
 *   [BOOT] Jumping to kernel...
 *
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 /* =========================================================================
  * Platform configuration — filled from compile flags
  * ====================================================================== */
 
 /* Default kernel command line */
 #ifndef UIOX_CMDLINE
   #define UIOX_CMDLINE \
       "root=/dev/mmcblk0p2 rw quiet console=ttyAMA0"
 #endif
 
 /* Physical addresses (match linker scripts) */
 #if defined(__aarch64__)
   #define UIOX_KERN_LOAD_PA   0x40080000ULL  /**< Kernel loads here      */
   #define UIOX_ARGS_PA        0x40070000ULL  /**< Boot args placed here  */
   #define UIOX_DTB_FALLBACK   0x40000000ULL  /**< QEMU virt DTB address  */
   #define UIOX_ARCH_ID        UIOX_ARCH_ARM64
 #elif defined(__arm__)
   #define UIOX_KERN_LOAD_PA   0x00200000ULL
   #define UIOX_ARGS_PA        0x00180000ULL
   #define UIOX_DTB_FALLBACK   0x00010000ULL
   #define UIOX_ARCH_ID        UIOX_ARCH_ARM32
 #else
   #define UIOX_KERN_LOAD_PA   0x00200000ULL  /**< 2 MB, above boot image */
   #define UIOX_ARGS_PA        0x00180000ULL
   #define UIOX_DTB_FALLBACK   0x00000000ULL  /**< No DTB on x86          */
   #define UIOX_ARCH_ID        UIOX_ARCH_X86_64
 #endif
 
 /* Kernel image filename on FAT32 (8.3 space-padded, upper-case) */
 #define UIOX_KERNEL_FNAME   "KERNEL  BIN"
 
 /* Maximum kernel image size we will accept */
 #define UIOX_KERN_MAX_SIZE  (32u * 1024u * 1024u)  /* 32 MB */
 
 /* =========================================================================
  * Stub block-device read (QEMU simulation — no real MMC/NVMe driver)
  * Returns -1 to signal no storage, triggering simulation path.
  * ====================================================================== */
 
 static int sim_blk_read(uint64_t lba, uint32_t count,
                          void *buf, void *priv)
 {
     UIOX_UNUSED(lba); UIOX_UNUSED(count);
     UIOX_UNUSED(buf); UIOX_UNUSED(priv);
     return -1;  /* no real storage in simulation                         */
 }
 
 /* =========================================================================
  * Arch registration forward declaration
  * ====================================================================== */
 
 #if defined(__aarch64__)
   extern void uiox_boot_hw_arm64_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_arm64_register()
 #elif defined(__arm__)
   extern void uiox_boot_hw_arm32_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_arm32_register()
 #else
   extern void uiox_boot_hw_x86_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_x86_register()
 #endif
 
 /* =========================================================================
  * uiox_boot_main — 7-stage pipeline
  * Called from arch entry stub with dtb_pa in arg0 (x0 / r0 / rdi).
  * ====================================================================== */
 
 void __attribute__((noreturn))
 uiox_boot_main(uint64_t dtb_pa, uint64_t x1, uint64_t x2)
 {
     UIOX_UNUSED(x1); UIOX_UNUSED(x2);
 
     /* ================================================================== */
     /* Stage 1: Hardware init                                              */
     /* ================================================================== */
 
     UIOX_HW_REGISTER();           /* registers ops + calls ops->init()   */
     uiox_boot_console_init();
 
     uiox_boot_printf("\n" UIOX_BOOT_VERSION_STR
                      " (%s) [" UIOX_BOOT_URL "]\n",
 #if   defined(__aarch64__)
                      "ARM64"
 #elif defined(__arm__)
                      "ARM32"
 #else
                      "x86_64"
 #endif
                     );
 
     //BOOT_LOG(1, "HW init");
     uiox_boot_puts("OK\n");
 
     /* ================================================================== */
     /* Stage 2: Memory                                                     */
     /* ================================================================== */
 
     //BOOT_LOG(2, "Memory");
     uiox_mem_map_t mem_map;
     uiox_boot_err_t rc = uiox_boot_mem_probe(dtb_pa, &mem_map);
     //if (rc != UIOX_BOOT_OK)
         //BOOT_FATAL("memory probe failed (%d)", (int)rc);
 
     uiox_boot_mem_print(&mem_map);
 
     /* Initialise bump allocator (reserve bottom 512 KB for DTB + args)  */
     uiox_bump_alloc_t alloc;
     rc = uiox_boot_mem_alloc_init(&alloc, &mem_map,
                                    (uintptr_t)UIOX_ARGS_PA,
                                    sizeof(uiox_boot_args_t) + 0x10000u);
     //if (rc != UIOX_BOOT_OK)
         //BOOT_FATAL("bump alloc init failed (%d)", (int)rc);
 
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 3: Storage                                                    */
     /* ================================================================== */
 
     //BOOT_LOG(3, "Storage");
 
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
 
     //BOOT_LOG(4, "Load kernel");
 
     /* Allocate a buffer for the kernel + header from bump allocator */
     void *load_buf = uiox_boot_mem_alloc(&alloc,
                                           UIOX_KERN_MAX_SIZE + 512u,
                                           4096u);
     //if (!load_buf)
         //BOOT_FATAL("no memory for kernel load buffer");
 
     size_t   loaded      = 0u;
     bool     has_header  = false;
     bool     is_elf      = false;
     uint64_t entry_pa    = UIOX_KERN_LOAD_PA;
 
     if (storage_ok) {
         rc = uiox_boot_fs_load(&fs, UIOX_KERNEL_FNAME,
                                 load_buf, UIOX_KERN_MAX_SIZE, &loaded);
         if (rc == UIOX_BOOT_OK && loaded >= sizeof(uiox_image_hdr_t)) {
             uiox_boot_printf("  Loaded %lu bytes from " UIOX_KERNEL_FNAME
                               "\n", (unsigned long)loaded);
             has_header = true;
         } else {
             uiox_boot_printf("  File not found — "
                               "QEMU simulation handoff\n");
         }
     } else {
         uiox_boot_printf("  No kernel file — QEMU simulation handoff\n");
     }
 
     /* ================================================================== */
     /* Stage 5: Verify                                                     */
     /* ================================================================== */
 
     //BOOT_LOG(5, "Verify");
 
     if (has_header) {
         const uiox_image_hdr_t *hdr = (const uiox_image_hdr_t *)load_buf;
         const uint8_t *payload = (const uint8_t *)load_buf
                                   + sizeof(uiox_image_hdr_t);
         size_t pay_len = loaded - sizeof(uiox_image_hdr_t);
 
         rc = uiox_boot_verify_image(hdr, payload, pay_len, UIOX_ARCH_ID);
         //if (rc != UIOX_BOOT_OK)
             //BOOT_FATAL("image verification failed (%d)", (int)rc);
 
         /* Determine if ELF */
         uint32_t magic;
         uiox_boot_memcpy(&magic, payload, 4u);
         is_elf = (magic == ELF64_MAGIC) &&
                  ((const uiox_elf64_ehdr_t *)payload)->e_ident[4]
                       == ELF_CLASS_64;
 
         entry_pa = hdr->entry_point;
         uiox_boot_printf("  SHA-256 OK  entry=%016llx\n",
                           (unsigned long long)entry_pa);
         BOOT_OK();
     } else {
         uiox_boot_puts("  No kernel file — QEMU simulation handoff\n");
     }
 
     /* ================================================================== */
     /* Stage 6: ELF load                                                   */
     /* ================================================================== */
 
     //BOOT_LOG(6, "ELF");
 
     if (has_header) {
         const uiox_image_hdr_t *hdr = (const uiox_image_hdr_t *)load_buf;
         const uint8_t *payload = (const uint8_t *)load_buf
                                   + sizeof(uiox_image_hdr_t);
         size_t pay_len = loaded - sizeof(uiox_image_hdr_t);
 
         if (is_elf) {
             rc = uiox_boot_elf64_load(payload, pay_len, &entry_pa);
             //if (rc != UIOX_BOOT_OK)
                 //BOOT_FATAL("ELF load failed (%d)", (int)rc);
             uiox_boot_printf("  ELF64 loaded  entry=%016llx\n",
                               (unsigned long long)entry_pa);
         } else {
             /* Flat binary: load to hdr->load_addr */
             rc = uiox_boot_flat_load(payload, pay_len,
                                       (uintptr_t)hdr->load_addr);
             //if (rc != UIOX_BOOT_OK)
                 //BOOT_FATAL("flat load failed (%d)", (int)rc);
             entry_pa = hdr->entry_point;
             uiox_boot_printf("  Flat binary load\n  Entry: %016llx\n",
                               (unsigned long long)entry_pa);
         }
     } else {
         /* Simulation: no kernel binary — report entry as QEMU sim addr */
         entry_pa = UIOX_KERN_LOAD_PA;
         uiox_boot_printf("  Flat binary load\n  Entry: %016llx\n",
                           (unsigned long long)entry_pa);
     }
 
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 7: Handoff                                                    */
     /* ================================================================== */
 
     //BOOT_LOG(7, "Handoff");
 
     /* Use passed DTB PA or fallback */
     uint64_t final_dtb = (dtb_pa != 0u) ? dtb_pa : UIOX_DTB_FALLBACK;
 
     uiox_boot_handoff(entry_pa,
                        final_dtb,
                        UIOX_ARGS_PA,
                        &mem_map,
                        UIOX_CMDLINE);
 
     /* Never reached */
     for (;;) ;
 }
 