UIOX NFS-lite covers:
  • NFS v3 client (UDP/TCP transport over uiox_fw_eth)
  • Plan 9 / 9P2000 client (simpler, used by QEMU virtfs)
  • VirtIO-FS client (FUSE-over-virtio, QEMU native)
  • Unified VFS mount layer (plugs into 32_FileSystem namei)
  • Remote file operations: open/read/write/close/stat/readdir
  • Attribute caching (30-second TTL)
  • Write-back page cache
  • Syscall shims: mount(2) / umount(2) / statfs(2)
========================================================
50_UIX/11_netfs/
├── include/
│   ├── uiox_nfs_types.h      # Types: FH, attr, dirent, error codes
│   ├── uiox_nfs_rpc.h        # RPC/XDR encode-decode (no libc)
│   ├── uiox_nfs_proto.h      # NFS v3 procedure call wrappers
│   ├── uiox_nfs_9p.h         # Plan 9 / 9P2000 client
│   ├── uiox_nfs_virtfs.h     # VirtIO-FS (FUSE over virtio)
│   ├── uiox_nfs_vfs.h        # VFS mount + inode cache bridge
│   ├── uiox_nfs_cache.h      # Attribute + page cache
│   └── uiox_netfs.h          # Master umbrella include
└── src/
    ├── uiox_nfs_rpc.c
    ├── uiox_nfs_proto.c
    ├── uiox_nfs_9p.c
    ├── uiox_nfs_virtfs.c
    ├── uiox_nfs_vfs.c
    ├── uiox_nfs_cache.c
    └── uiox_netfs_demo.c
=========================================
File	Layer	UIOX integration
uiox_nfs_types.h	Types — FH, attr, dirent, statfs, error codes	Shared by all layers
uiox_nfs_rpc.h/.c	Transport — ONC RPC/XDR encode/decode, UDP/TCP framing	uiox_fw_eth / uiox_fw_wifi for send/recv
uiox_nfs_proto.h/.c	NFS v3 — GETATTR/LOOKUP/READ/WRITE/READDIR/MKDIR/REMOVE/COMMIT	32_FileSystem inode operations
uiox_nfs_9p.h/.c	9P2000 — version/attach/walk/open/read/write/clunk/stat	QEMU -virtfs plan9 transport
uiox_nfs_virtfs.h/.c	VirtIO-FS — FUSE over VirtIO MMIO, nodeid-based ops	QEMU -device vhost-user-fs-pci
uiox_nfs_cache.h/.c	Cache — 30-second attribute TTL, write-back page cache	31_BufferCache design pattern
uiox_nfs_vfs.h/.c	VFS bridge — mount/umount/open/read/write/stat/readdir/syscalls	32_FileSystem namei hook, 40_SystemCallInterface SYS_MOUNT
uiox_netfs_demo.c	Demo — 15 scenarios: NFS3 + VirtFS mount, read/write/mkdir/readdir/rename/unlink	Full API coverage