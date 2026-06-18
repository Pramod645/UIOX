/**
 * @file  uiox_boot_fs.c
 * @brief UIOX Bootloader — FAT32 BPB parse, cluster walk, file load.
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 /* =========================================================================
  * Internal helpers
  * ====================================================================== */
 
 static uint32_t fat32_clus_to_lba(const uiox_fat32_ctx_t *ctx, uint32_t clus)
 {
     return ctx->data_start_lba +
            (clus - 2u) * ctx->sec_per_clus;
 }
 
 static uiox_boot_err_t fat32_read_fat(uiox_fat32_ctx_t *ctx,
                                        uint32_t clus,
                                        uint32_t *next_clus)
 {
     /* FAT entry is at byte offset clus*4 in the FAT */
     uint32_t fat_sector = ctx->fat_start_lba + (clus * 4u / UIOX_FS_SECTOR_SIZE);
     uint32_t fat_offset = (clus * 4u) % UIOX_FS_SECTOR_SIZE;
 
     if (ctx->read_fn(fat_sector, 1u, ctx->sector_buf, ctx->read_priv) != 0)
         return UIOX_BOOT_ERR_IO;
 
     uint32_t entry;
     uiox_boot_memcpy(&entry, ctx->sector_buf + fat_offset, 4u);
     *next_clus = entry & 0x0FFFFFFFu;
     return UIOX_BOOT_OK;
 }
 
 /* =========================================================================
  * FAT32 init
  * ====================================================================== */
 
 uiox_boot_err_t uiox_boot_fs_init(uiox_fat32_ctx_t *ctx,
                                     uiox_blk_read_fn read_fn,
                                     void *priv)
 {
     if (!ctx || !read_fn) return UIOX_BOOT_ERR_INVAL;
     uiox_boot_memset(ctx, 0, sizeof(*ctx));
     ctx->read_fn   = read_fn;
     ctx->read_priv = priv;
 
     /* Read sector 0 (BPB) */
     if (read_fn(0u, 1u, ctx->sector_buf, priv) != 0)
         return UIOX_BOOT_ERR_IO;
 
     uiox_boot_memcpy(&ctx->bpb, ctx->sector_buf, sizeof(ctx->bpb));
     uiox_fat32_bpb_t *b = &ctx->bpb;
 
     /* Verify FAT32 signature */
     if (b->bytes_per_sec != UIOX_FS_SECTOR_SIZE)
         return UIOX_BOOT_ERR_BADMAGIC;
     if (b->fat_sz16 != 0u || b->root_ent_cnt != 0u)
         return UIOX_BOOT_ERR_UNSUP;  /* Only FAT32 */
 
     ctx->fat_start_lba  = b->rsvd_sec_cnt;
     ctx->data_start_lba = ctx->fat_start_lba +
                           (uint32_t)b->num_fats * b->fat_sz32;
     ctx->root_clus      = b->root_clus;
     ctx->sec_per_clus   = b->sec_per_clus;
     ctx->bytes_per_clus = (uint32_t)b->sec_per_clus * UIOX_FS_SECTOR_SIZE;
 
     return UIOX_BOOT_OK;
 }
 
 /* =========================================================================
  * FAT32 file load by 8.3 name (upper-case, space-padded)
  * ====================================================================== */
 
 uiox_boot_err_t uiox_boot_fs_load(uiox_fat32_ctx_t *ctx,
                                     const char *name83,
                                     void *buf, size_t buf_size,
                                     size_t *bytes_loaded)
 {
     if (!ctx || !name83 || !buf) return UIOX_BOOT_ERR_INVAL;
     *bytes_loaded = 0u;
 
     /* Format name as 11-byte 8.3 padded with spaces */
     char sfn[11];
     uiox_boot_memset(sfn, ' ', 11u);
     size_t ni = 0u;
     for (size_t i = 0u; name83[i] && ni < 11u; i++) {
         char c = name83[i];
         if (c == '.') { ni = 8u; continue; }
         sfn[ni++] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
     }
 
     /* Walk root directory cluster chain */
     uint32_t clus = ctx->root_clus;
     bool     found = false;
     uint32_t file_start_clus = 0u;
     uint32_t file_size       = 0u;
 
     while (clus < FAT32_EOC && !found) {
         uint32_t lba = fat32_clus_to_lba(ctx, clus);
         for (uint32_t s = 0u; s < ctx->sec_per_clus && !found; s++) {
             if (ctx->read_fn(lba + s, 1u, ctx->sector_buf,
                              ctx->read_priv) != 0)
                 return UIOX_BOOT_ERR_IO;
 
             uiox_fat32_dirent_t *dir =
                 (uiox_fat32_dirent_t *)ctx->sector_buf;
             for (uint32_t e = 0u;
                  e < UIOX_FS_SECTOR_SIZE / sizeof(uiox_fat32_dirent_t);
                  e++) {
                 if (dir[e].name[0] == 0x00u) goto dir_end;  /* no more */
                 if ((uint8_t)dir[e].name[0] == 0xE5u) continue; /* deleted */
                 if (dir[e].attr == FAT32_ATTR_LFN)     continue; /* LFN */
                 if (dir[e].attr & FAT32_ATTR_DIR)       continue; /* subdir */
                 if (uiox_boot_memcmp(dir[e].name, sfn, 8u) == 0 &&
                     uiox_boot_memcmp(dir[e].ext,  sfn + 8u, 3u) == 0) {
                     file_start_clus =
                         ((uint32_t)dir[e].clus_hi << 16u) | dir[e].clus_lo;
                     file_size = dir[e].file_size;
                     found = true;
                     break;
                 }
             }
         }
         if (!found) {
             uint32_t next;
             if (fat32_read_fat(ctx, clus, &next) != UIOX_BOOT_OK) break;
             clus = next;
         }
     }
 dir_end:
     if (!found) return UIOX_BOOT_ERR_NOTFOUND;
     if (file_size > buf_size) return UIOX_BOOT_ERR_OVERFLOW;
 
     /* Read file data cluster by cluster */
     uint8_t *dst  = (uint8_t *)buf;
     uint32_t rem  = file_size;
     clus = file_start_clus;
 
     while (clus < FAT32_EOC && rem > 0u) {
         uint32_t lba  = fat32_clus_to_lba(ctx, clus);
         uint32_t csz  = UIOX_MIN(ctx->bytes_per_clus, rem);
         uint32_t secs = (csz + UIOX_FS_SECTOR_SIZE - 1u) / UIOX_FS_SECTOR_SIZE;
 
         for (uint32_t s = 0u; s < secs && rem > 0u; s++) {
             if (ctx->read_fn(lba + s, 1u, ctx->sector_buf,
                              ctx->read_priv) != 0)
                 return UIOX_BOOT_ERR_IO;
             uint32_t bytes = UIOX_MIN(rem, (uint32_t)UIOX_FS_SECTOR_SIZE);
             uiox_boot_memcpy(dst, ctx->sector_buf, bytes);
             dst  += bytes;
             rem  -= bytes;
             *bytes_loaded += bytes;
         }
         uint32_t next;
         if (fat32_read_fat(ctx, clus, &next) != UIOX_BOOT_OK) break;
         clus = next;
     }
     return UIOX_BOOT_OK;
 }
 