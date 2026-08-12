/*
 * 50_UIX/src/unfs_lib.c
 *
 * UNFS userspace library — helper wrappers around kernel syscalls.
 *
 * All UNFS file operations go through the standard POSIX-like
 * syscall interface (open/read/write/ioctl/mmap).
 * This library provides convenience wrappers for UNFS-specific
 * features: snapshots, extended attributes, MAC labels, fsinfo.
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "unfs_user.h"
 #include <fcntl.h>
 #include <unistd.h>
 #include <sys/ioctl.h>
 #include <string.h>
 #include <errno.h>
 
 /* ─────────────────────────────────────────────────────────────────────
  * Snapshot helpers
  * ───────────────────────────────────────────────────────────────────── */
 
 int unfs_snapshot_create(const char *mountpoint, const char *snap_name)
 {
     int fd = open(mountpoint, O_RDONLY | O_DIRECTORY);
     if (fd < 0) return -errno;
 
     char name[32];
     size_t len = strlen(snap_name);
     if (len > 31u) len = 31u;
     memcpy(name, snap_name, len);
     name[len] = '\0';
 
     int rc = ioctl(fd, UNFS_IOC_SNAP_CREATE, name);
     close(fd);
     return rc;
 }
 
 int unfs_snapshot_delete(const char *mountpoint, uint64_t snap_id)
 {
     int fd = open(mountpoint, O_RDONLY | O_DIRECTORY);
     if (fd < 0) return -errno;
 
     int rc = ioctl(fd, UNFS_IOC_SNAP_DELETE, &snap_id);
     close(fd);
     return rc;
 }
 
 int unfs_snapshot_list(const char     *mountpoint,
                         unfs_snap_info_t *out,
                         int               max)
 {
     if (!out || max <= 0) return -1;
 
     int fd = open(mountpoint, O_RDONLY | O_DIRECTORY);
     if (fd < 0) return -errno;
 
     unfs_snap_info_t buf[UNFS_SNAP_MAX];
     int rc = ioctl(fd, UNFS_IOC_SNAP_LIST, buf);
     close(fd);
     if (rc < 0) return rc;
 
     int n = rc < max ? rc : max;
     memcpy(out, buf, (size_t)n * sizeof(unfs_snap_info_t));
     return n;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Extended attribute helpers
  * ───────────────────────────────────────────────────────────────────── */
 
 int unfs_xattr_get(const char *path, const char *name,
                     void *val, uint16_t *len)
 {
     int fd = open(path, O_RDONLY);
     if (fd < 0) return -errno;
 
     unfs_xattr_req_t req;
     memset(&req, 0, sizeof(req));
     size_t nlen = strlen(name);
     if (nlen > 63u) nlen = 63u;
     memcpy(req.name, name, nlen);
 
     int rc = ioctl(fd, UNFS_IOC_XATTR_GET, &req);
     close(fd);
     if (rc != 0) return rc;
 
     if (val && len) {
         uint16_t copy = req.val_len < *len ? req.val_len : *len;
         memcpy(val, req.value, copy);
         *len = copy;
     }
     return 0;
 }
 
 int unfs_xattr_set(const char *path, const char *name,
                     const void *val, uint16_t len)
 {
     int fd = open(path, O_WRONLY);
     if (fd < 0) return -errno;
 
     unfs_xattr_req_t req;
     memset(&req, 0, sizeof(req));
     size_t nlen = strlen(name);
     if (nlen > 63u) nlen = 63u;
     memcpy(req.name, name, nlen);
 
     uint16_t vlen = len < UNFS_XATTR_VAL_MAX ? len : UNFS_XATTR_VAL_MAX;
     memcpy(req.value, val, vlen);
     req.val_len = vlen;
 
     int rc = ioctl(fd, UNFS_IOC_XATTR_SET, &req);
     close(fd);
     return rc;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * MAC label helpers — wraps xattr "security.mac_label"
  * ───────────────────────────────────────────────────────────────────── */
 
 int unfs_mac_label_get(const char *path, uint8_t label[16])
 {
     uint16_t len = 16u;
     return unfs_xattr_get(path, "security.mac_label", label, &len);
 }
 
 int unfs_mac_label_set(const char *path, const uint8_t label[16])
 {
     return unfs_xattr_set(path, "security.mac_label", label, 16u);
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Filesystem info
  * ───────────────────────────────────────────────────────────────────── */
 
 int unfs_fsinfo(const char *mountpoint, unfs_fsinfo_t *out)
 {
     if (!out) return -1;
     int fd = open(mountpoint, O_RDONLY | O_DIRECTORY);
     if (fd < 0) return -errno;
 
     int rc = ioctl(fd, UNFS_IOC_FSINFO, out);
     close(fd);
     return rc;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * mkfs — format a block device as UNFS
  *
  * Writes the on-disk layout:
  *   Block 0:   Superblock
  *   Block 1:   Journal superblock
  *   Block 2..J Journal log area
  *   Block J+1: Group descriptor table
  *   Block J+2: Group 0 block bitmap
  *   Block J+3: Group 0 inode bitmap
  *   Block J+4..J+11: Group 0 inode table (8 blocks = 512 inodes)
  *   Block J+12+: Data blocks
  * ───────────────────────────────────────────────────────────────────── */
 
 /* CRC32C for mkfs (userspace) */
 static uint32_t crc32c_u(const void *data, uint32_t len)
 {
     const uint8_t *p = (const uint8_t *)data;
     uint32_t crc = 0xFFFFFFFFu;
     for (uint32_t i = 0u; i < len; i++) {
         crc ^= p[i];
         for (int b = 0; b < 8; b++)
             crc = (crc >> 1) ^ (0x82F63B78u & -(crc & 1u));
     }
     return crc ^ 0xFFFFFFFFu;
 }
 
 /* On-disk superblock layout (simplified — matches kernel unfs_sb_t) */
 typedef struct {
     uint32_t magic;
     uint16_t ver_maj, ver_min;
     uint32_t block_size, inode_size;
     uint64_t block_count, free_blocks;
     uint32_t inode_count, free_inodes;
     uint32_t inodes_per_group, blocks_per_group, group_count;
     uint32_t jr_block, jr_size;
     uint64_t mount_time_ns, write_time_ns;
     uint32_t mount_count, max_mount_count;
     uint8_t  clean, cow, csum_type, compress;
     uint8_t  vol_name[64];
     uint8_t  uuid[16];
     uint32_t sb_checksum;
     uint8_t  _pad[4096 - 148];
 } __attribute__((packed)) mkfs_sb_t;
 
 int unfs_mkfs(const char *device, const unfs_mkfs_params_t *params)
 {
     if (!device || !params) return -1;
     if (params->block_size != UNFS_BLOCK_SIZE) return -1;
 
     int fd = open(device, O_RDWR | O_CREAT | O_TRUNC, 0600);
     if (fd < 0) return -errno;
 
     uint64_t total_blocks = params->volume_size_bytes / UNFS_BLOCK_SIZE;
     uint32_t jr_blocks    = params->journal_blocks
                           ? params->journal_blocks : 256u;
     uint32_t ipg          = params->inodes_per_group
                           ? params->inodes_per_group : 512u;
     uint32_t bpg          = 8192u;
     uint32_t gdt_block    = 2u + jr_blocks;
     uint32_t grp0_bmap    = gdt_block + 1u;
     uint32_t grp0_imap    = grp0_bmap + 1u;
     uint32_t grp0_itable  = grp0_imap + 1u;
     uint32_t itable_blks  = (ipg * 256u) / UNFS_BLOCK_SIZE;
     uint32_t data_start   = grp0_itable + itable_blks;
     uint32_t n_groups     = (uint32_t)((total_blocks + bpg - 1u) / bpg);
     if (n_groups == 0u) n_groups = 1u;
 
     /* Allocate a zero block buffer */
     static uint8_t blk[UNFS_BLOCK_SIZE];
 
     /* Extend file to full size */
     if (ftruncate(fd, (off_t)params->volume_size_bytes) != 0) {
         close(fd);
         return -errno;
     }
 
     /* ── Block 0: Superblock ─────────────────────────────────────── */
     memset(blk, 0, sizeof(blk));
     mkfs_sb_t *sb = (mkfs_sb_t *)blk;
     sb->magic           = UNFS_MAGIC;
     sb->ver_maj         = 1u;
     sb->ver_min         = 0u;
     sb->block_size      = UNFS_BLOCK_SIZE;
     sb->inode_size      = 256u;
     sb->block_count     = total_blocks;
     sb->free_blocks     = total_blocks - data_start;
     sb->inode_count     = n_groups * ipg;
     sb->free_inodes     = n_groups * ipg - 11u; /* reserve first 10 */
     sb->inodes_per_group= ipg;
     sb->blocks_per_group= bpg;
     sb->group_count     = n_groups;
     sb->jr_block        = 1u;
     sb->jr_size         = jr_blocks;
     sb->clean           = 1u;
     sb->cow             = params->enable_cow;
     sb->csum_type       = params->enable_checksum;
     size_t vn = strlen(params->volume_name);
     if (vn > 63u) vn = 63u;
     memcpy(sb->vol_name, params->volume_name, vn);
     memcpy(sb->uuid, params->uuid, 16u);
     sb->sb_checksum = crc32c_u(blk, UNFS_BLOCK_SIZE - 4u);
 
     lseek(fd, 0, SEEK_SET);
     if (write(fd, blk, UNFS_BLOCK_SIZE) != UNFS_BLOCK_SIZE) {
         close(fd); return -5;
     }
 
     /* ── Block 1: Journal superblock ────────────────────────────── */
     memset(blk, 0, sizeof(blk));
     uint32_t *jr_magic = (uint32_t *)blk;
     *jr_magic = 0x6A626434UL;  /* journal magic */
     uint32_t *jr_bsize = (uint32_t *)(blk + 4);
     *jr_bsize = UNFS_BLOCK_SIZE;
     lseek(fd, UNFS_BLOCK_SIZE, SEEK_SET);
     write(fd, blk, UNFS_BLOCK_SIZE);
 
     /* ── Blocks 2..jr_blocks+1: Journal log (zeroed) ─────────────── */
     memset(blk, 0, sizeof(blk));
     for (uint32_t b = 0u; b < jr_blocks; b++) {
         lseek(fd, (off_t)(2u + b) * UNFS_BLOCK_SIZE, SEEK_SET);
         write(fd, blk, UNFS_BLOCK_SIZE);
     }
 
     /* ── Group descriptor table ──────────────────────────────────── */
     memset(blk, 0, sizeof(blk));
 
     /* Simple group descriptor: one group for now */
     typedef struct {
         uint32_t bmap, imap, itable;
         uint32_t free_blks, free_inos, used_dirs, csum;
         uint8_t  _pad[8];
     } __attribute__((packed)) gd_t;
 
     gd_t *gd = (gd_t *)blk;
     gd->bmap      = grp0_bmap;
     gd->imap      = grp0_imap;
     gd->itable    = grp0_itable;
     gd->free_blks = (uint32_t)(total_blocks - data_start);
     gd->free_inos = ipg - 11u;
     gd->used_dirs = 1u;  /* root dir */
     gd->csum      = crc32c_u(gd, sizeof(gd_t) - 4u);
 
     lseek(fd, (off_t)gdt_block * UNFS_BLOCK_SIZE, SEEK_SET);
     write(fd, blk, UNFS_BLOCK_SIZE);
 
     /* ── Block bitmaps ───────────────────────────────────────────── */
     memset(blk, 0, sizeof(blk));
     /* Mark pre-allocated blocks (0..data_start-1) as used */
     for (uint32_t b = 0u; b < data_start; b++)
         blk[b / 8u] |= (uint8_t)(1u << (b % 8u));
     lseek(fd, (off_t)grp0_bmap * UNFS_BLOCK_SIZE, SEEK_SET);
     write(fd, blk, UNFS_BLOCK_SIZE);
 
     /* ── Inode bitmap ────────────────────────────────────────────── */
     memset(blk, 0, sizeof(blk));
     /* Mark reserved inodes 1-10 + root inode 2 as used */
     blk[0] = 0x0Bu;  /* bits 0(ino1), 1(ino2=root), 3(ino4) reserved */
     blk[1] = 0x01u;  /* bit 0 = ino9 */
     lseek(fd, (off_t)grp0_imap * UNFS_BLOCK_SIZE, SEEK_SET);
     write(fd, blk, UNFS_BLOCK_SIZE);
 
     /* ── Inode table — write root inode (ino 2) ──────────────────── */
     memset(blk, 0, sizeof(blk));
     /* Root inode is inode 2, local index 1 in group 0 */
     typedef struct {
         uint16_t mode;
         uint16_t uid, gid, nlink;
         uint64_t size, atime, mtime, ctime;
         uint32_t blocks, flags;
         uint8_t  mac_label[16];
         uint32_t mac_flags;
         /* 4 extents × 8 bytes = 32 bytes */
         uint8_t  extents[32];
         uint32_t extent_tree;
         uint8_t  inline_data[60];
         uint32_t checksum;
     } __attribute__((packed)) ino_t;
 
     ino_t *root = (ino_t *)(blk + 256u);  /* slot 1 (0-indexed) */
     root->mode  = 0x4000u | 0755u;        /* directory + rwxr-xr-x */
     root->uid   = 0u;
     root->gid   = 0u;
     root->nlink = 2u;                      /* . and .. */
     root->size  = UNFS_BLOCK_SIZE;
     root->checksum = crc32c_u(root, 256u - 4u);
 
     lseek(fd, (off_t)grp0_itable * UNFS_BLOCK_SIZE, SEEK_SET);
     write(fd, blk, UNFS_BLOCK_SIZE);
 
     /* ── Root directory data block ──────────────────────────────── */
     memset(blk, 0, sizeof(blk));
     /* Write "." entry */
     typedef struct {
         uint32_t ino;
         uint16_t rec_len;
         uint8_t  name_len, file_type;
         char     name[256];
     } __attribute__((packed)) de_t;
 
     de_t *dot = (de_t *)blk;
     dot->ino       = 2u;
     dot->rec_len   = 12u;
     dot->name_len  = 1u;
     dot->file_type = 2u;  /* dir */
     dot->name[0]   = '.';
 
     /* Write ".." entry */
     de_t *dotdot = (de_t *)(blk + 12u);
     dotdot->ino       = 2u;
     dotdot->rec_len   = UNFS_BLOCK_SIZE - 12u;  /* fills rest of block */
     dotdot->name_len  = 2u;
     dotdot->file_type = 2u;
     dotdot->name[0]   = '.';
     dotdot->name[1]   = '.';
 
     lseek(fd, (off_t)data_start * UNFS_BLOCK_SIZE, SEEK_SET);
     write(fd, blk, UNFS_BLOCK_SIZE);
 
     close(fd);
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * fsck — basic consistency check
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_fsck(const char *device, int repair)
 {
     if (!device) return -1;
 
     int fd = open(device, repair ? O_RDWR : O_RDONLY);
     if (fd < 0) return -errno;
 
     /* Read superblock */
     static uint8_t sbuf[UNFS_BLOCK_SIZE];
     lseek(fd, 0, SEEK_SET);
     if (read(fd, sbuf, UNFS_BLOCK_SIZE) != UNFS_BLOCK_SIZE) {
         close(fd); return -5;
     }
 
     uint32_t *magic = (uint32_t *)sbuf;
     if (*magic != UNFS_MAGIC) {
         close(fd);
         return -117;  /* bad magic */
     }
 
     uint32_t stored_crc = *(uint32_t *)(sbuf + UNFS_BLOCK_SIZE - 4u);
     uint32_t computed   = crc32c_u(sbuf, UNFS_BLOCK_SIZE - 4u);
 
     if (stored_crc != computed) {
         if (repair) {
             /* Recompute and write superblock checksum */
             *(uint32_t *)(sbuf + UNFS_BLOCK_SIZE - 4u) = computed;
             lseek(fd, 0, SEEK_SET);
             write(fd, sbuf, UNFS_BLOCK_SIZE);
         } else {
             close(fd);
             return -117;  /* checksum mismatch */
         }
     }
 
     close(fd);
     return 0;   /* OK */
 }
 