uiox-pkg/
├── include/
│   ├── uiox_pkg_types.h      # Layer 1 — Types: package header, deps, err codes
│   ├── uiox_pkg_buf.h        # Pool: package record / event queue
│   ├── uiox_pkg_store.h      # Layer 2 — Store: FAT32/ramfs package storage
│   ├── uiox_pkg_resolve.h    # Layer 3 — Resolver: dependency graph, SAT
│   ├── uiox_pkg_subsys.h     # Layer 4 — Subsystem: install/remove/upgrade
│   └── uiox_pkg_device.h     # Layer 5 — Application-facing API
└── src/
    ├── uiox_pkg_buf.c
    ├── uiox_pkg_store.c
    ├── uiox_pkg_resolve.c
    ├── uiox_pkg_subsys.c
    ├── uiox_pkg_device.c
    └── uiox_pkg_demo.c
===============================================================================
=== UIOX Package Manager Demo ===

--- Open ---
  open rc=OK

--- Start ---
  [store] mounted  repo=/pkg  type=1  entries=0
  [pkg] subsystem ready  repo=/pkg
  start rc=OK

--- Device info ---
  State        : READY
  Repo         : /pkg
  Installed    : 0
  Total in idx : 0
  Install ops  : 0
  Remove ops   : 0
  Upgrade ops  : 0
  Errors       : 0
  Rec pool free: 64 / 64
  Evt pool free: 32 / 32

--- Install libuiox-base (no deps) ---
  [pkg] install plan for 'libuiox-base':
  Resolution plan (1 steps):
  [ 0] INSTALL   libuiox-base                      v1.0.0
  [event] INSTALL_START        pkg=libuiox-base   status=OK
  [store] extract libuiox-base                     → /usr/pkg  (0 files)
  [event] INSTALL_DONE         pkg=libuiox-base   status=OK
  install rc=OK

--- Install libuiox-net (deps: libuiox-base already done) ---
  [pkg] install plan for 'libuiox-net':
  Resolution plan (1 steps):
  [ 0] INSTALL   libuiox-net                       v1.0.0
  [event] INSTALL_START        pkg=libuiox-net    status=OK
  [event] INSTALL_DONE         pkg=libuiox-net    status=OK
  install rc=OK

--- Install uiox-shell (deps: base + net) ---
  [pkg] install plan for 'uiox-shell':
  Resolution plan (1 steps):
  [ 0] INSTALL   uiox-shell                        v2.1.0
  ...
  install rc=OK

--- Re-install libuiox-base (should return ALREADY) ---
  install rc=ALREADY_INSTALLED  (expected ALREADY_INSTALLED)

--- Query uiox-shell ---
  name=uiox-shell  v2.1.0  deps=2

--- Is uiox-editor installed? ---
  installed=YES
  installed(nonexistent)=NO

--- List all installed packages ---
  libuiox-base                      v1.0.0  installed
  libuiox-net                       v1.0.0  installed
  uiox-shell                        v2.1.0  installed
  uiox-editor                       v1.3.0  installed
  uiox-devtools                     v1.0.0  installed

--- Try to remove libuiox-base (uiox-shell depends on it) ---
  remove rc=CONFLICT  (expected CONFLICT)

--- Remove uiox-shell first, then libuiox-base ---
  remove uiox-shell rc=OK
  remove libuiox-base rc=OK

=== UIOX Package Manager Demo complete ===
===============================================
File	Layer	Role
uiox_pkg_types.h	Types	uiox_pkg_hdr_t, uiox_pkg_dep_t, uiox_pkg_rec_t, error codes, version packing
uiox_pkg_buf.h/.c	Buffer pool	64 package records + 32 event records, scan-alloc, assert-free
uiox_pkg_store.h/.c	Store	In-memory index (uiox_pkg_index_entry_t[256]), simulated archive table, index_find/add/remove/update, load_pkg, extract, remove_files
uiox_pkg_resolve.h/.c	Resolver	DAG node/edge table, Kahn's topological sort, circular-dep detection, reverse-dep check for remove, install/remove/upgrade plan builder
uiox_pkg_subsys.h/.c	Subsystem	Plan execution (exec_plan_entry), event callback dispatch, install/remove/upgrade/upgrade_all/query/list, stats, name helpers
uiox_pkg_device.h/.c	Application API	Thin wrappers matching 40_SystemCallInterface syscall shape
uiox_pkg_demo.c	Demo	6 simulated packages with real dependency graph, 12 test scenarios
============================================
Integration points with UIOX repo:
UIOX folder	Package manager integration
32_FileSystem	uiox_pkg_store_extract() calls creat()/write() via FS; store_remove_files() calls unlink() via namei
31_BufferCache	store_index_load/save() uses bread()/bwrite() for the /pkg/index.upix sector
30_DeviceDrivers/01_block	The underlying block device (ramfs or SD) that holds /pkg/
40_SystemCallInterface	sys_pkg_install(name, ver) → uiox_pkg_install(&g_pkg_dev, name, ver)
33_ProcessControlSubsystem	Package install/remove runs in a kernel process context; uses wait()/signal() for concurrent-access locking
main.c (kernel entry)	uiox_pkg_open(&g_pkg_dev, &params) called during Stage 3 device init