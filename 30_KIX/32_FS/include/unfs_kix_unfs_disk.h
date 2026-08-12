/*
 * 30_KIX/32_FS/include/unfs_disk.h
 *
 * UNFS on-disk structures — shared between 01_uBoot and 30_KIX/32_FS.
 * This file is identical in both locations (copy or symlink).
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #ifndef UNFS_DISK_H
 #define UNFS_DISK_H
 
 #include "uiox_base_types.h"
 
 #define UNFS_MAGIC            0x554E4653UL
 #define UNFS_VERSION_MAJOR    1u
 #define UNFS_VERSION_MINOR    0u
 #define UNFS_BLOCK_SIZE       4096u
 #define UNFS_INODE_SIZE       256u
 #define UNFS_INODES_PER_BLOCK (UNFS_BLOCK_SIZE / UNFS_INODE_SIZE)
 #define UNFS_INODES_PER_GROUP 512u
 #define UNFS_BLOCKS_PER_GROUP 8192u
 #define UNFS_NAME_MAX         255u
 
 #define UNFS_SB_BLOCK         0u
 #define UNFS_JR_SB_BLOCK      1u
 #define UNFS_JR_LOG_START     2u
 #define UNFS_JR_LOG_BLOCKS    256u
 #define UNFS_GDT_BLOCK        258u
 #define UNFS_GRP0_BMAP        259u
 #define UNFS_GRP0_IMAP        260u
 #define UNFS_GRP0_ITABLE      261u
 #define UNFS_GRP0_ITABLE_BLKS 8u
 #define UNFS_GRP0_DATA_START  269u
 
 #define UNFS_ROOT_INO         2u
 #define UNFS_RESERVED_INOS    10u
 
 #define UNFS_IFMT             0xF000u
 #define UNFS_IFREG            0x8000u
 #define UNFS_IFDIR            0x4000u
 #define UNFS_IFLNK            0xA000u
 #define UNFS_IFBLK            0x6000u
 #define UNFS_IFCHR            0x2000u
 #define UNFS_IFIFO            0x1000u
 
 #define UNFS_EXT_LEAF         0x0001u
 #define UNFS_EXT_HOLE         0x0002u
 #define UNFS_EXT_COW          0x0004u
 
 #define UNFS_FT_UNKNOWN       0u
 #define UNFS_FT_REG           1u
 #define UNFS_FT_DIR           2u
 #define UNFS_FT_CHRDEV        3u
 #define UNFS_FT_BLKDEV        4u
 #define UNFS_FT_FIFO          5u
 #define UNFS_FT_SYMLINK       6u
 
 typedef struct {
     uint32_t  e_logical;
     uint32_t  e_physical;
     uint16_t  e_len;
     uint16_t  e_flags;
 } __attribute__((packed)) unfs_extent_t;
 
 typedef struct {
     uint32_t  s_magic;
     uint16_t  s_version_major;
     uint16_t  s_version_minor;
     uint32_t  s_block_size;
     uint32_t  s_inode_size;
     uint64_t  s_block_count;
     uint64_t  s_free_blocks;
     uint32_t  s_inode_count;
     uint32_t  s_free_inodes;
     uint32_t  s_inodes_per_group;
     uint32_t  s_blocks_per_group;
     uint32_t  s_group_count;
     uint32_t  s_jr_block;
     uint32_t  s_jr_size;
     uint64_t  s_mount_time_ns;
     uint64_t  s_write_time_ns;
     uint32_t  s_mount_count;
     uint32_t  s_max_mount_count;
     uint8_t   s_clean;
     uint8_t   s_cow_enabled;
     uint8_t   s_checksum_type;
     uint8_t   s_compress;
     uint8_t   s_volume_name[64];
     uint8_t   s_uuid[16];
     uint32_t  s_sb_checksum;
     uint8_t   _pad[UNFS_BLOCK_SIZE - 148u];
 } __attribute__((packed)) unfs_sb_t;
 
 typedef struct {
     uint32_t  bg_block_bitmap;
     uint32_t  bg_inode_bitmap;
     uint32_t  bg_inode_table;
     uint32_t  bg_free_blocks;
     uint32_t  bg_free_inodes;
     uint32_t  bg_used_dirs;
     uint32_t  bg_checksum;
     uint8_t   _pad[8];
 } __attribute__((packed)) unfs_group_desc_t;
 
 typedef struct {
     uint16_t  i_mode;
     uint16_t  i_uid;
     uint16_t  i_gid;
     uint16_t  i_nlink;
     uint64_t  i_size;
     uint64_t  i_atime_ns;
     uint64_t  i_mtime_ns;
     uint64_t  i_ctime_ns;
     uint32_t  i_blocks;
     uint32_t  i_flags;
     uint8_t   i_mac_label[16];
     uint32_t  i_mac_flags;
     unfs_extent_t i_extents[4];
     uint32_t  i_extent_tree;
     uint8_t   i_inline[60];
     uint32_t  i_checksum;
 } __attribute__((packed)) unfs_inode_t;
 
 typedef struct {
     uint32_t  d_ino;
     uint16_t  d_rec_len;
     uint8_t   d_name_len;
     uint8_t   d_file_type;
     char      d_name[UNFS_NAME_MAX + 1u];
 } __attribute__((packed)) unfs_dirent_t;
 
 #endif /* UNFS_DISK_H */
 