#ifndef UIOX_BOOT_FS_H
#define UIOX_BOOT_FS_H
/*
 * uiox_boot_fs.h - Minimal FAT32 read-only driver.
 */
#include "uiox_boot_types.h"

/* -- Block device ops --------------------------------------- */
typedef struct {
    int (*read_sectors)(uboot_u64_t lba,
                         uboot_u32_t count,
                         void *buf);
} uboot_blk_ops_t;

/* -- FAT32 BPB offsets -------------------------------------- */
#define FAT32_OFS_BYTES_PER_SECT  0x0Bu
#define FAT32_OFS_SECTS_PER_CLUS  0x0Du
#define FAT32_OFS_RSVD_SECTS      0x0Eu
#define FAT32_OFS_NUM_FATS        0x10u
#define FAT32_OFS_FAT_SIZE32      0x24u
#define FAT32_OFS_ROOT_CLUS       0x2Cu
#define FAT32_DE_SIZE             32u
#define FAT32_DE_ATTR             0x0Bu
#define FAT32_DE_CLUS_HI          0x14u
#define FAT32_DE_CLUS_LO          0x1Au
#define FAT32_DE_FILE_SIZE        0x1Cu
#define FAT32_ATTR_LFN            0x0Fu
#define FAT32_ATTR_DIR            0x10u
#define FAT32_EOC                 0x0FFFFFF8u

/* -- FAT32 context ------------------------------------------ */
typedef struct {
    const uboot_blk_ops_t *blk;
    uboot_u64_t  part_start_lba;
    uboot_u16_t  bytes_per_sect;
    uboot_u8_t   sects_per_clus;
    uboot_u16_t  rsvd_sects;
    uboot_u32_t  fat_start_sect;
    uboot_u32_t  data_start_sect;
    uboot_u32_t  root_clus;
} uboot_fat32_t;

/* -- API ---------------------------------------------------- */
int  uboot_fat32_init  (uboot_fat32_t *fs,
                         const uboot_blk_ops_t *blk,
                         uboot_u64_t part_lba);
int  uboot_fat32_load  (uboot_fat32_t *fs,
                         const char *filename,
                         void *dest,
                         uboot_size_t max_bytes,
                         uboot_size_t *out_size);
int  uboot_fat32_exists(uboot_fat32_t *fs, const char *filename);

/* -- Block device probe stubs ------------------------------ */
int  uboot_blk_probe_emmc(uboot_blk_ops_t *ops);
int  uboot_blk_probe_nvme(uboot_blk_ops_t *ops, uboot_u32_t idx);
int  uboot_blk_probe_usb (uboot_blk_ops_t *ops);

#endif /* UIOX_BOOT_FS_H */
