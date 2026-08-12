# UNFS Complete File Placement Guide

## 01_uBoot (bootloader — read-only)

| File | Destination | Purpose |
|---|---|---|
| unfs_boot_unfs.h | 01_uBoot/include/unfs.h | On-disk types + read API |
| unfs_boot_unfs.c | 01_uBoot/src/unfs.c | mount/lookup/read_file |
| unfs_boot_bridge.c | 01_uBoot/src/unfs_boot_bridge.c | Wires unfs into boot pipeline |

### Changes to existing 01_uBoot files

**uiox_boot_main.c** — Stage 3/4 replace FAT32 simulation:
```c
/* Stage 3 — Storage */
rc = unfs_boot_probe();
if (rc != 0) goto qemu_fallback;
BOOT_OK();

/* Stage 4 — Load kernel */
rc = unfs_boot_load(UIOX_KERN_LOAD_PA, 64*1024*1024,
                    &bytes_loaded, &img_hdr);
if (rc != 0) goto qemu_fallback;
BOOT_OK();
```

**uiox_boot_hw_arm64.c** — add block read:
```c
int uiox_boot_hw_read_block(uint32_t blkno, void *buf) {
    /* Read from eMMC SDMMC at SOC_EMMC_BASE */
    uintptr_t addr = SOC_EMMC_BASE + (uintptr_t)blkno * 4096u;
    memcpy(buf, (const void *)addr, 4096u);
    return 0;
}
```

---

## 30_KIX/32_FS (kernel — full read/write)

| File | Destination | Purpose |
|---|---|---|
| unfs_kix_unfs_disk.h | 32_FS/include/unfs_disk.h | Shared on-disk structs |
| unfs_kix_unfs_fs.h | 32_FS/include/unfs_fs.h | Kernel UNFS types + API |
| unfs_kix_unfs_vfs.c | 32_FS/10_unfs/unfs_vfs.c | VFS ops + mount/alloc/COW |
| unfs_xattr.c | 32_FS/10_unfs/unfs_xattr.c | Extended attributes |
| unfs_snap.c | 32_FS/10_unfs/unfs_snap.c | Snapshot + COW management |
| 32_FS_Makefile | 32_FS/Makefile | Updated Makefile |

### Changes to existing 32_FS files

**32_FS/src/uiox_fs_init.c** — register UNFS instead of SCFS:
```c
void uiox_fs_init(void) {
    uiox_pc_init();     /* page cache */
    vfs_init();         /* VFS layer  */
    unfs_register();    /* UNFS — replaces scfs_init() */
    uiox_jr_init();     /* journal    */
    vfs_mount_root();   /* mount /    */
}
```

---

## 50_UIX (userspace)

| File | Destination | Purpose |
|---|---|---|
| unfs_uix_unfs_user.h | 50_UIX/include/unfs_user.h | ioctl commands + API |
| unfs_uix_unfs_lib.c | 50_UIX/src/unfs_lib.c | Snapshot/xattr/mkfs/fsck |
| unfs_uix_unfs_shell.c | 50_UIX/src/unfs_shell.c | Shell built-in commands |

### Shell commands available after integration

```bash
unfs-info        /              # show volume info
unfs-snap-create / pre-upgrade  # create snapshot named "pre-upgrade"
unfs-snap-list   /              # list all snapshots
unfs-snap-delete / 1            # delete snapshot ID 1
unfs-getlabel    /etc/passwd    # get MAC label
unfs-setlabel    /etc/passwd 0000000000000001000000000000000f
unfs-mkfs        /dev/emmc0 256 # format 256 MB UNFS volume
unfs-fsck        /dev/emmc0     # check volume
unfs-fsck        /dev/emmc0 --repair  # check and repair
```

---

## unfs_disk.h — shared header

This file is IDENTICAL in both locations:
- 01_uBoot/include/unfs_disk.h
- 30_KIX/32_FS/include/unfs_disk.h

The easiest way to keep them in sync is a symlink:
```bash
ln -s ../../30_KIX/32_FS/include/unfs_disk.h 01_uBoot/include/unfs_disk.h
```

---

## Complete data flow

```
POWER ON
  │
  ▼
01_uBoot/src/unfs.c
  unfs_mount()         → reads block 0 (superblock) + block 258 (GDT)
  unfs_lookup("/boot/uiox_kernel.elf")
    → reads root inode (ino 2)
    → walks directory blocks
    → returns kernel inode
  unfs_read_file()     → reads extent blocks → DRAM at KERN_LOAD_PA
  unfs_unmount()
  uiox_boot_arch_jump(KERN_LOAD_PA, dtb_pa, args_pa)
  │
  ▼
30_KIX/32_FS/10_unfs/unfs_vfs.c
  unfs_kern_mount()    → reads superblock + GDT + root inode
  unfs_register_map()  → registers block-mapping callback with page cache
  │  (all file I/O goes through VFS + page cache + block cache)
  │
  ▼
50_UIX userspace
  open("/etc/config")  → sys_open → vfs_open → unfs_vfs_lookup
  read(fd, buf, n)     → sys_read → vfs_read → unfs_vfs_read
                          → uiox_pc_read → bread × 8
                          → copy_to_user(ubuf, kbuf, n)
  mmap(fd, ...)        → sys_mmap → vfs_mmap_page → unfs_vfs_mmap_page
                          → uiox_pc_get_page_pa → PA
                          → uiox_mm_map_user_phys → PTE → zero copy
  write(fd, buf, n)    → sys_write → copy_from_user → vfs_write
                          → unfs_vfs_write → uiox_pc_write → dirty page
  fsync(fd)            → sys_fsync → vfs_fsync → unfs_vfs_fsync
                          → uiox_pc_sync_inode → writeback → bwrite
                          → uiox_jr_force_commit → journal durable
```
