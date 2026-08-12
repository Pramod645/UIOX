/*
 * 01_uBoot/src/unfs_boot_bridge.c
 *
 * Wires the UNFS reader (unfs.c) into the bootloader pipeline.
 * Replaces the FAT32 simulation in uiox_boot_fs.c Stage 3/4/5.
 *
 * Called from uiox_boot_main.c:
 *   Stage 3 — storage:  unfs_boot_probe()   mount UNFS volume
 *   Stage 4 — load:     unfs_boot_load()    find + load kernel ELF
 *   Stage 5 — verify:   unfs_boot_verify()  SHA-256 check (already in verify.c)
 *
 * Block device read is provided by the arch hw layer:
 *   ARM64/ARM32: eMMC SDMMC controller MMIO
 *   RISC-V:      VirtIO block device
 *   x86-64:      AHCI SATA (or VirtIO block)
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "uiox_boot.h"
 #include "unfs.h"
 
 /* ── Arch block-read callback — provided by hw layer ────────────────── */
 extern int uiox_boot_hw_read_block(uint32_t blkno, void *buf);
 
 /* ── UNFS mount context (static — no heap in bootloader) ────────────── */
 static unfs_mount_t s_mnt;
 
 /* ── Kernel file path on UNFS volume ────────────────────────────────── */
 #ifndef UNFS_KERNEL_PATH
 #  define UNFS_KERNEL_PATH  "/boot/uiox_kernel.elf"
 #endif
 
 #ifndef UNFS_DTB_PATH
 #  define UNFS_DTB_PATH     "/boot/uiox.dtb"
 #endif
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_boot_probe — Stage 3: mount UNFS volume
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_boot_probe(void)
 {
     int rc = unfs_mount(&s_mnt, uiox_boot_hw_read_block);
     if (rc != 0) {
         uiox_boot_puts("  [unfs] mount failed (");
         uiox_boot_printf("rc=%d)\n", rc);
         return rc;
     }
 
     uiox_boot_printf("  [unfs] mounted — %llu blocks, %u inodes\n",
                       (unsigned long long)s_mnt.sb.s_block_count,
                       s_mnt.sb.s_inode_count);
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_boot_load_dtb — load DTB from UNFS to dtb_pa
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_boot_load_dtb(uintptr_t dtb_pa, uint64_t max_bytes,
                         uint64_t *bytes_out)
 {
     unfs_inode_t inode;
     int rc = unfs_lookup(&s_mnt, UNFS_DTB_PATH, &inode);
     if (rc != 0) {
         /* DTB not found — use firmware-provided one (QEMU) */
         uiox_boot_puts("  [unfs] DTB not on volume — using firmware DTB\n");
         if (bytes_out) *bytes_out = 0u;
         return 0;
     }
 
     rc = unfs_read_file(&s_mnt, &inode, dtb_pa, max_bytes, bytes_out);
     if (rc != 0) {
         uiox_boot_puts("  [unfs] DTB load failed\n");
         return rc;
     }
 
     uiox_boot_printf("  [unfs] DTB loaded %llu bytes to 0x%llx\n",
                       (unsigned long long)*bytes_out,
                       (unsigned long long)dtb_pa);
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_boot_load — Stage 4: find and load kernel ELF from UNFS
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_boot_load(uintptr_t load_pa, uint64_t max_bytes,
                     uint64_t *bytes_out,
                     uiox_image_hdr_t *hdr_out)
 {
     /* Lookup kernel file in UNFS */
     unfs_inode_t inode;
     int rc = unfs_lookup(&s_mnt, UNFS_KERNEL_PATH, &inode);
     if (rc != 0) {
         uiox_boot_printf("  [unfs] kernel not found at '%s' (rc=%d)\n",
                           UNFS_KERNEL_PATH, rc);
         return rc;
     }
 
     uiox_boot_printf("  [unfs] kernel inode=%u size=%llu\n",
                       (uint32_t)inode.i_size, /* simplified */
                       (unsigned long long)inode.i_size);
 
     /* Read kernel image to load address */
     uint64_t bytes_read = 0u;
     rc = unfs_read_file(&s_mnt, &inode, load_pa, max_bytes, &bytes_read);
     if (rc != 0) {
         uiox_boot_puts("  [unfs] kernel read failed\n");
         return rc;
     }
 
     if (bytes_out) *bytes_out = bytes_read;
 
     /*
      * Parse UIOX image header at load_pa.
      * Header magic is UIOX_IMAGE_MAGIC (from uiox_boot_types.h).
      * If present, it carries the SHA-256 and entry point.
      */
     const uiox_image_hdr_t *hdr =
         (const uiox_image_hdr_t *)(uintptr_t)load_pa;
 
     if (hdr->magic == UIOX_IMAGE_MAGIC) {
         if (hdr_out) {
             /* Copy header — freestanding memcpy */
             const uint8_t *src = (const uint8_t *)hdr;
             uint8_t *dst = (uint8_t *)hdr_out;
             for (uint32_t i = 0u; i < sizeof(uiox_image_hdr_t); i++)
                 dst[i] = src[i];
         }
         uiox_boot_printf("  [unfs] UIOX header found entry=0x%llx\n",
                           (unsigned long long)hdr->entry_point);
     } else {
         /* ELF or flat binary — entry handled by handoff.c */
         uiox_boot_puts("  [unfs] raw ELF image\n");
         if (hdr_out) {
             /* Zero hdr so caller falls through to ELF parse */
             uint8_t *dst = (uint8_t *)hdr_out;
             for (uint32_t i = 0u; i < sizeof(uiox_image_hdr_t); i++)
                 dst[i] = 0u;
         }
     }
 
     uiox_boot_printf("  [unfs] loaded %llu bytes to 0x%llx\n",
                       (unsigned long long)bytes_read,
                       (unsigned long long)load_pa);
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_boot_unmount — called after kernel handoff preparation
  * ───────────────────────────────────────────────────────────────────── */
 void unfs_boot_unmount(void)
 {
     unfs_unmount(&s_mnt);
 }
 