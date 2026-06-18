/**
 * @file  uiox_boot_fs.h
 * @brief UIOX Bootloader — FAT32 BPB parser, cluster walk, file load.
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_BOOT_FS_H
 #define UIOX_BOOT_FS_H
 
 #include "uiox_boot_types.h"
 #include "uiox_boot_mem.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FS_SECTOR_SIZE     512u
 #define UIOX_FS_NAME_MAX        12u    /**< 8.3 + NUL                    */
 
 /* =========================================================================
  * FAT32 BPB (Bios Parameter Block) — packed layout
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint8_t  jmp[3];
     char     oem[8];
     uint16_t bytes_per_sec;
     uint8_t  sec_per_clus;
     uint16_t rsvd_sec_cnt;
     uint8_t  num_fats;
     uint16_t root_ent_cnt;    /* 0 for FAT32 */
     uint16_t total_sec16;
     uint8_t  media;
     uint16_t fat_sz16;        /* 0 for FAT32 */
     uint16_t sec_per_trk;
     uint16_t num_heads;
     uint32_t hidden_sec;
     uint32_t total_sec32;
     /* FAT32 extended */
     uint32_t fat_sz32;
     uint16_t ext_flags;
     uint16_t fs_ver;
     uint32_t root_clus;
     uint16_t fs_info;
     uint16_t bk_boot_sec;
     uint8_t  _rsvd[12];
     uint8_t  drv_num;
     uint8_t  _rsvd2;
     uint8_t  boot_sig;
     uint32_t vol_id;
     char     vol_lab[11];
     char     fs_type[8];     /* "FAT32   " */
 } uiox_fat32_bpb_t;
 
 /* FAT32 directory entry (32 bytes) */
 typedef struct __attribute__((packed)) {
     char     name[8];
     char     ext[3];
     uint8_t  attr;
     uint8_t  _rsvd[8];
     uint16_t clus_hi;
     uint8_t  _time[4];
     uint16_t clus_lo;
     uint32_t file_size;
 } uiox_fat32_dirent_t;
 
 #define FAT32_ATTR_DIR          0x10u
 #define FAT32_ATTR_LFN          0x0Fu
 #define FAT32_EOC               0x0FFFFFF8u
 
 /* =========================================================================
  * Storage block I/O callback (platform-supplied)
  * ====================================================================== */
 
 typedef int (*uiox_blk_read_fn)(uint64_t lba, uint32_t count,
                                   void *buf, void *priv);
 
 /* =========================================================================
  * FAT32 context
  * ====================================================================== */
 
 typedef struct {
     uiox_fat32_bpb_t bpb;
     uint32_t         fat_start_lba;
     uint32_t         data_start_lba;
     uint32_t         root_clus;
     uint32_t         sec_per_clus;
     uint32_t         bytes_per_clus;
     uiox_blk_read_fn read_fn;
     void            *read_priv;
     uint8_t          sector_buf[UIOX_FS_SECTOR_SIZE];
 } uiox_fat32_ctx_t;
 
 /* =========================================================================
  * FS API
  * ====================================================================== */
 
 uiox_boot_err_t uiox_boot_fs_init  (uiox_fat32_ctx_t *ctx,
                                      uiox_blk_read_fn read_fn,
                                      void *priv);
 
 uiox_boot_err_t uiox_boot_fs_load  (uiox_fat32_ctx_t *ctx,
                                      const char *name83,
                                      void *buf, size_t buf_size,
                                      size_t *bytes_loaded);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BOOT_FS_H */
 