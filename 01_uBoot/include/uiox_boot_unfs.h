/*
 * 01_uBoot/include/uiox_boot_unfs.h
 *
 * UIOX Native Filesystem (UNFS) — bootloader read-only client.
 * Umbrella include for the 01_uBoot UNFS reader.
 *
 * The bootloader only reads from UNFS — no write, no journal replay.
 * Journal replay happens in the kernel (32_FS/02_journal).
 *
 * On-disk layout:
 *   Block 0         Superblock         (unfs_sb_t)
 *   Block 1         Journal superblock (reuse uiox_jr_sb_disk_t)
 *   Block 2..257    Journal log area   (256 × 4 KB = 1 MB)
 *   Block 258       Block group 0 descriptor
 *   Block 259       Block bitmap       (group 0)
 *   Block 260       Inode bitmap       (group 0)
 *   Block 261..268  Inode table        (group 0, 8 blocks = 512 inodes)
 *   Block 269+      Data blocks        (group 0)
 *   ...             Repeat for group 1, 2, …
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #ifndef UIOX_BOOT_UNFS_H
 #define UIOX_BOOT_UNFS_H
 
 #include "uiox_boot_types.h"
 
 /* ── Magic and version ─────────────────────────────────────────────── */
 #define UNFS_MAGIC            0x554E4653UL   /* "UNFS"                 */
 #define UNFS_VERSION_MAJOR    1u
 #define UNFS_VERSION_MINOR    0u
 
 /* ── Geometry ──────────────────────────────────────────────────────── */
 #define UNFS_BLOCK_SIZE       4096u          /* always 4 KB            */
 #define UNFS_INODE_SIZE       256u           /* bytes per inode        */
 #define UNFS_INODES_PER_BLOCK (UNFS_BLOCK_SIZE / UNFS_INODE_SIZE)
 #define UNFS_INODES_PER_GROUP 512u
 #define UNFS_BLOCKS_PER_GROUP 8192u
 #define UNFS_NAME_MAX         255u
 
 /* ── Fixed layout offsets (block numbers) ──────────────────────────── */
 #define UNFS_SB_BLOCK         0u    /* superblock                      */
 #define UNFS_JR_SB_BLOCK      1u    /* journal superblock              */
 #define UNFS_JR_LOG_START     2u    /* journal log first block         */
 #define UNFS_JR_LOG_BLOCKS    256u  /* journal log size                */
 #define UNFS_GDT_BLOCK        258u  /* group descriptor table          */
 #define UNFS_GRP0_BMAP        259u  /* group 0 block bitmap            */
 #define UNFS_GRP0_IMAP        260u  /* group 0 inode bitmap            */
 #define UNFS_GRP0_ITABLE      261u  /* group 0 inode table start       */
 #define UNFS_GRP0_ITABLE_BLKS 8u    /* inode table blocks per group    */
 #define UNFS_GRP0_DATA_START  269u  /* group 0 first data block        */
 
 /* ── Inode numbers ─────────────────────────────────────────────────── */
 #define UNFS_ROOT_INO         2u    /* root directory inode (like ext) */
 #define UNFS_RESERVED_INOS    10u   /* inodes 1-10 reserved            */
 
 /* ── File types (mode field top bits) ─────────────────────────────── */
 #define UNFS_IFMT             0xF000u
 #define UNFS_IFREG            0x8000u  /* regular file                 */
 #define UNFS_IFDIR            0x4000u  /* directory                    */
 #define UNFS_IFLNK            0xA000u  /* symbolic link                */
 #define UNFS_IFBLK            0x6000u  /* block device                 */
 #define UNFS_IFCHR            0x2000u  /* character device             */
 #define UNFS_IFIFO            0x1000u  /* FIFO / named pipe            */
 
 /* ── Permission bits ───────────────────────────────────────────────── */
 #define UNFS_ISUID            04000u
 #define UNFS_ISGID            02000u
 #define UNFS_ISVTX            01000u
 #define UNFS_IRUSR            00400u
 #define UNFS_IWUSR            00200u
 #define UNFS_IXUSR            00100u
 #define UNFS_IRGRP            00040u
 #define UNFS_IWGRP            00020u
 #define UNFS_IXGRP            00010u
 #define UNFS_IROTH            00004u
 #define UNFS_IWOTH            00002u
 #define UNFS_IXOTH            00001u
 
 /* ── Extent flags ──────────────────────────────────────────────────── */
 #define UNFS_EXT_LEAF         0x0001u  /* leaf extent (has data)       */
 #define UNFS_EXT_HOLE         0x0002u  /* sparse / hole — reads zero   */
 #define UNFS_EXT_COW          0x0004u  /* copy-on-write pending        */
 
 /* ── Error codes ───────────────────────────────────────────────────── */
 #define UNFS_OK               0
 #define UNFS_ENOENT          -2
 #define UNFS_EIO             -5
 #define UNFS_ENOMEM         -12
 #define UNFS_ENOTDIR        -20
 #define UNFS_EINVAL         -22
 #define UNFS_ENOTSUP        -95
 #define UNFS_ECORRUPT       -117  /* checksum mismatch                 */
 
 /* ── Forward declarations ──────────────────────────────────────────── */
 typedef struct unfs_sb        unfs_sb_t;
 typedef struct unfs_group_desc unfs_group_desc_t;
 typedef struct unfs_inode     unfs_inode_t;
 typedef struct unfs_extent    unfs_extent_t;
 typedef struct unfs_dirent    unfs_dirent_t;
 typedef struct unfs_mount     unfs_mount_t;
 
 /* ─────────────────────────────────────────────────────────────────────
  * On-disk structures
  * ───────────────────────────────────────────────────────────────────── */
 
 /* Extent — maps logical file blocks to physical device blocks */
 struct unfs_extent {
     uint32_t  e_logical;    /* first logical block in file            */
     uint32_t  e_physical;   /* first physical block on device         */
     uint16_t  e_len;        /* number of blocks in this extent        */
     uint16_t  e_flags;      /* UNFS_EXT_* flags                       */
 } __attribute__((packed));
 
 /* Superblock — block 0 */
 struct unfs_sb {
     uint32_t  s_magic;          /* UNFS_MAGIC = 0x554E4653              */
     uint16_t  s_version_major;
     uint16_t  s_version_minor;
     uint32_t  s_block_size;     /* always 4096                          */
     uint32_t  s_inode_size;     /* always 256                           */
     uint64_t  s_block_count;    /* total blocks on volume               */
     uint64_t  s_free_blocks;
     uint32_t  s_inode_count;    /* total inodes                         */
     uint32_t  s_free_inodes;
     uint32_t  s_inodes_per_group;
     uint32_t  s_blocks_per_group;
     uint32_t  s_group_count;    /* number of block groups               */
     uint32_t  s_jr_block;       /* journal superblock block number      */
     uint32_t  s_jr_size;        /* journal log size in blocks           */
     uint64_t  s_mount_time_ns;  /* last mount timestamp (ns)            */
     uint64_t  s_write_time_ns;  /* last write timestamp (ns)            */
     uint32_t  s_mount_count;    /* mounts since last fsck               */
     uint32_t  s_max_mount_count;
     uint8_t   s_clean;          /* 1 = cleanly unmounted                */
     uint8_t   s_cow_enabled;    /* 1 = copy-on-write active             */
     uint8_t   s_checksum_type;  /* 0=none 1=CRC32C                      */
     uint8_t   s_compress;       /* 0=none (reserved for future)         */
     uint8_t   s_volume_name[64];
     uint8_t   s_uuid[16];       /* volume UUID                          */
     uint32_t  s_sb_checksum;    /* CRC32C of bytes 0..s_sb_size-4       */
     uint8_t   _pad[UNFS_BLOCK_SIZE - 148u];
 } __attribute__((packed));
 
 /* Block group descriptor */
 struct unfs_group_desc {
     uint32_t  bg_block_bitmap;  /* block number of block bitmap         */
     uint32_t  bg_inode_bitmap;  /* block number of inode bitmap         */
     uint32_t  bg_inode_table;   /* first block of inode table           */
     uint32_t  bg_free_blocks;
     uint32_t  bg_free_inodes;
     uint32_t  bg_used_dirs;     /* number of directory inodes           */
     uint32_t  bg_checksum;      /* CRC32C of this descriptor            */
     uint8_t   _pad[8];
 } __attribute__((packed));
 
 /* Inode — 256 bytes on disk */
 struct unfs_inode {
     uint16_t  i_mode;           /* file type + permissions              */
     uint16_t  i_uid;
     uint16_t  i_gid;
     uint16_t  i_nlink;          /* hard link count                      */
     uint64_t  i_size;           /* file size in bytes                   */
     uint64_t  i_atime_ns;       /* last access (nanoseconds since epoch)*/
     uint64_t  i_mtime_ns;       /* last modification                    */
     uint64_t  i_ctime_ns;       /* last status change                   */
     uint32_t  i_blocks;         /* 512-byte blocks allocated            */
     uint32_t  i_flags;          /* misc flags                           */
     /* MAC security label — 33_PCS/05_sec */
     uint8_t   i_mac_label[16];  /* security label                       */
     uint32_t  i_mac_flags;      /* MAC policy flags                     */
     /* Extent tree — 4 inline extents */
     unfs_extent_t i_extents[4]; /* 4 × 8 = 32 bytes                    */
     uint32_t  i_extent_tree;    /* block# of overflow extent tree (0=none)*/
     /* Inline symlink target (if i_size <= 60) */
     uint8_t   i_inline[60];
     /* Checksum */
     uint32_t  i_checksum;       /* CRC32C of bytes 0..251               */
 } __attribute__((packed));
 
 /* Directory entry — variable length, 4-byte aligned */
 struct unfs_dirent {
     uint32_t  d_ino;            /* inode number                         */
     uint16_t  d_rec_len;        /* length of this record                */
     uint8_t   d_name_len;       /* length of name (no NUL)              */
     uint8_t   d_file_type;      /* file type (UNFS_FT_*)                */
     char      d_name[UNFS_NAME_MAX + 1u]; /* NUL-terminated name       */
 } __attribute__((packed));
 
 /* Directory entry file type codes */
 #define UNFS_FT_UNKNOWN   0u
 #define UNFS_FT_REG       1u
 #define UNFS_FT_DIR       2u
 #define UNFS_FT_CHRDEV    3u
 #define UNFS_FT_BLKDEV    4u
 #define UNFS_FT_FIFO      5u
 #define UNFS_FT_SYMLINK   6u
 
 /* ─────────────────────────────────────────────────────────────────────
  * In-memory mount context (bootloader)
  * ───────────────────────────────────────────────────────────────────── */
 struct unfs_mount {
     unfs_sb_t         sb;
     unfs_group_desc_t groups[16];   /* up to 16 block groups in boot    */
     uint32_t          n_groups;
     uint8_t           mounted;
 };
 
 /* ─────────────────────────────────────────────────────────────────────
  * Block device read callback — provided by arch hw layer
  * ───────────────────────────────────────────────────────────────────── */
 typedef int (*unfs_read_blk_fn)(uint32_t blkno, void *buf);
 
 /* ─────────────────────────────────────────────────────────────────────
  * Public API — bootloader read-only client
  * ───────────────────────────────────────────────────────────────────── */
 
 /* Mount UNFS volume — reads superblock + group descriptors */
 int   unfs_mount  (unfs_mount_t *mnt, unfs_read_blk_fn read_fn);
 
 /* Unmount — clears in-memory state */
 void  unfs_unmount(unfs_mount_t *mnt);
 
 /* Look up a file by absolute path — returns inode number or <0 */
 int   unfs_lookup (unfs_mount_t *mnt, const char *path,
                    unfs_inode_t *inode_out);
 
 /* Read file data into buffer at physical address load_pa */
 int   unfs_read_file(unfs_mount_t      *mnt,
                      const unfs_inode_t *inode,
                      uintptr_t           load_pa,
                      uint64_t            max_bytes,
                      uint64_t           *bytes_read_out);
 
 /* CRC32C checksum */
 uint32_t unfs_crc32c(const void *data, uint32_t len);
 
 #endif /* UIOX_BOOT_UNFS_H */
 