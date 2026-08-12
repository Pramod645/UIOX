/*
 * 50_UIX/include/unfs_user.h
 *
 * UIOX Native Filesystem (UNFS) — userspace library header.
 *
 * Userspace accesses UNFS exclusively through kernel syscalls:
 *   open/close/read/write/lseek/stat/mkdir/unlink/readdir/mmap/fsync
 *
 * This header provides:
 *   1. UNFS-specific ioctl commands (snapshot, xattr, COW control)
 *   2. Userspace stat/dirent structures matching kernel layout
 *   3. Helper wrappers around standard syscalls for UNFS features
 *   4. UNFS volume format/mkfs utility structures
 *
 * Does NOT include any kernel headers — pure userspace.
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #ifndef UNFS_USER_H
 #define UNFS_USER_H
 
 /* Standard C types — userspace only */
 #include <stdint.h>
 #include <stddef.h>
 
 /* ── Magic and limits ───────────────────────────────────────────────── */
 #define UNFS_MAGIC            0x554E4653UL
 #define UNFS_BLOCK_SIZE       4096u
 #define UNFS_NAME_MAX         255u
 #define UNFS_PATH_MAX         4096u
 #define UNFS_SNAP_MAX         8u
 #define UNFS_XATTR_MAX        16u
 #define UNFS_XATTR_VAL_MAX    256u
 
 /* ── File type constants ─────────────────────────────────────────────── */
 #define UNFS_S_IFREG          0x8000u
 #define UNFS_S_IFDIR          0x4000u
 #define UNFS_S_IFLNK          0xA000u
 #define UNFS_S_IFMT           0xF000u
 
 #define UNFS_S_ISREG(m)  (((m) & UNFS_S_IFMT) == UNFS_S_IFREG)
 #define UNFS_S_ISDIR(m)  (((m) & UNFS_S_IFMT) == UNFS_S_IFDIR)
 #define UNFS_S_ISLNK(m)  (((m) & UNFS_S_IFMT) == UNFS_S_IFLNK)
 
 /* ── ioctl magic numbers ─────────────────────────────────────────────── */
 #define UNFS_IOC_MAGIC        'U'
 
 /* ── Snapshot ioctl structures ───────────────────────────────────────── */
 typedef struct {
     uint64_t  snap_id;
     char      name[32];
     uint64_t  created_ns;
     uint32_t  root_ino;
 } unfs_snap_info_t;
 
 #define UNFS_IOC_SNAP_CREATE  _IOW(UNFS_IOC_MAGIC, 1, char[32])
 #define UNFS_IOC_SNAP_DELETE  _IOW(UNFS_IOC_MAGIC, 2, uint64_t)
 #define UNFS_IOC_SNAP_LIST    _IOR(UNFS_IOC_MAGIC, 3, unfs_snap_info_t[8])
 
 /* ── Extended attribute ioctl structures ─────────────────────────────── */
 typedef struct {
     char     name[64];
     uint8_t  value[UNFS_XATTR_VAL_MAX];
     uint16_t val_len;
 } unfs_xattr_req_t;
 
 #define UNFS_IOC_XATTR_GET    _IOWR(UNFS_IOC_MAGIC, 10, unfs_xattr_req_t)
 #define UNFS_IOC_XATTR_SET    _IOW (UNFS_IOC_MAGIC, 11, unfs_xattr_req_t)
 #define UNFS_IOC_XATTR_LIST   _IOR (UNFS_IOC_MAGIC, 12, unfs_xattr_req_t[16])
 
 /* ── COW control ─────────────────────────────────────────────────────── */
 #define UNFS_IOC_COW_ENABLE   _IO  (UNFS_IOC_MAGIC, 20)
 #define UNFS_IOC_COW_DISABLE  _IO  (UNFS_IOC_MAGIC, 21)
 #define UNFS_IOC_COW_STATUS   _IOR (UNFS_IOC_MAGIC, 22, uint32_t)
 
 /* ── Filesystem info ─────────────────────────────────────────────────── */
 typedef struct {
     uint32_t  magic;
     uint32_t  version_major;
     uint32_t  version_minor;
     uint64_t  block_count;
     uint64_t  free_blocks;
     uint32_t  inode_count;
     uint32_t  free_inodes;
     uint32_t  block_size;
     uint32_t  group_count;
     uint8_t   volume_name[64];
     uint8_t   uuid[16];
     uint8_t   cow_enabled;
     uint8_t   clean;
 } unfs_fsinfo_t;
 
 #define UNFS_IOC_FSINFO       _IOR (UNFS_IOC_MAGIC, 30, unfs_fsinfo_t)
 
 /* ── Defragment request ──────────────────────────────────────────────── */
 typedef struct {
     uint32_t  ino;          /* inode to defragment (0 = whole volume)  */
     uint32_t  flags;
     uint64_t  extents_moved;
     uint64_t  blocks_freed;
 } unfs_defrag_req_t;
 
 #define UNFS_IOC_DEFRAG       _IOWR(UNFS_IOC_MAGIC, 40, unfs_defrag_req_t)
 
 /* ── Userspace stat structure (matches uiox_stat_t layout) ──────────── */
 typedef struct {
     uint32_t  st_ino;
     uint32_t  st_mode;
     uint32_t  st_nlink;
     uint32_t  st_uid;
     uint32_t  st_gid;
     uint64_t  st_size;
     uint64_t  st_blksize;
     uint64_t  st_blocks;
     uint64_t  st_atime;
     uint64_t  st_mtime;
     uint64_t  st_ctime;
 } unfs_stat_t;
 
 /* ── Userspace directory entry ───────────────────────────────────────── */
 typedef struct {
     uint32_t  d_ino;
     uint8_t   d_type;
     char      d_name[UNFS_NAME_MAX + 1u];
 } unfs_dirent_t;
 
 /* ── mkfs parameters ─────────────────────────────────────────────────── */
 typedef struct {
     uint64_t  volume_size_bytes;    /* total volume size               */
     uint32_t  block_size;           /* must be 4096                    */
     uint32_t  inodes_per_group;     /* default 512                     */
     uint32_t  journal_blocks;       /* default 256                     */
     uint8_t   enable_cow;           /* enable copy-on-write            */
     uint8_t   enable_checksum;      /* enable CRC32C checksums         */
     char      volume_name[64];
     uint8_t   uuid[16];
 } unfs_mkfs_params_t;
 
 /* ─────────────────────────────────────────────────────────────────────
  * Userspace helper API
  * ───────────────────────────────────────────────────────────────────── */
 
 /* Snapshot helpers */
 int  unfs_snapshot_create(const char *mountpoint, const char *snap_name);
 int  unfs_snapshot_delete(const char *mountpoint, uint64_t snap_id);
 int  unfs_snapshot_list  (const char *mountpoint,
                            unfs_snap_info_t *out, int max);
 
 /* Extended attribute helpers */
 int  unfs_xattr_get(const char *path, const char *name,
                     void *val, uint16_t *len);
 int  unfs_xattr_set(const char *path, const char *name,
                     const void *val, uint16_t len);
 
 /* Filesystem info */
 int  unfs_fsinfo(const char *mountpoint, unfs_fsinfo_t *out);
 
 /* MAC label helpers (wraps xattr "security.mac_label") */
 int  unfs_mac_label_get(const char *path, uint8_t label[16]);
 int  unfs_mac_label_set(const char *path, const uint8_t label[16]);
 
 /* mkfs — format a block device or file as UNFS */
 int  unfs_mkfs(const char *device, const unfs_mkfs_params_t *params);
 
 /* fsck — check and repair UNFS volume */
 int  unfs_fsck(const char *device, int repair);
 
 #endif /* UNFS_USER_H */
 