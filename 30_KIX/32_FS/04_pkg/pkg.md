Core layer

File	Role	Algorithm / mechanism
uiox_pkg_types.h	Base types	No algorithm. Declares the error enum, version packing macros (VER_MAJOR/MINOR/PATCH via shifts), uiox_pkg_hdr_t (packed, 512-byte aligned), uiox_pkg_dep_t, uiox_pkg_file_t, uiox_pkg_state_t, uiox_pkg_rec_t. Primitives come from uiox_klibc.h, not a local typedef block.

uiox_pkg_buf.h/.c	Record pool = the registry	Linear scan allocation. alloc walks 64 slots for in_use == 0, memsets it, sets the flag, decrements a counter. free asserts in_use > 0, clears the flag and next, increments. O(n) per alloc, no free list.


Store layer
File	Role	Algorithm / mechanism
uiox_pkg_store.h	Store contract	Types only: repo enum, uiox_pkg_index_hdr_t, uiox_pkg_index_entry_t, stats, context (index cache of 256 entries).

uiox_pkg_store.c	Index + archive I/O	Linear index search — index_find scans index[0..index_count) with a bounded strncmp. index_add appends at index_count++. index_remove does swap-with-last: overwrite the found slot with the final entry, shrink, zero the vacated tail — O(1) removal, so index order is not stable. load_pkg allocates from the pool and returns the pointer; caller frees. Two build modes: RAM archive table (default) or the UIOX_PKG_REAL_IO UNFS branch.


Resolver
File	Role	Algorithm / mechanism
uiox_pkg_resolve.h	Plan + graph types	uiox_pkg_plan_entry_t (name, version, op, is_auto), uiox_pkg_plan_t (128 entries + conflict msg), and the resolver context: adjacency matrix adj[64][16], nadj, in_degree, node_name/node_ver.

uiox_pkg_resolve.c	Dependency resolution	DFS graph build + Kahn's topological sort. build_graph recurses into required/optional deps, skipping already-installed ones and CONFLICT entries; add_node dedupes by linear name scan; add_edge records pkg → dep. topo_sort copies in_degree, seeds a FIFO with all zero-in-degree nodes, emits each one and decrements its successors' degrees. If processed ≠ node_count, a cycle exists → sets has_conflicts and returns UIOX_PKG_ERR_CONFLICT. Every loaded record is freed on both exit paths.


Executor
File	Role	Algorithm / mechanism
uiox_pkg_exec.h	Executor contract	Event enum (INSTALL_START…UPGRADE_FAIL), the callback typedef, executor state machine (OFF/INIT/READY/BUSY/ERROR), and the context holding store + resolver + stats.

uiox_pkg_exec.c	Runs the plan	Sequential plan walk. Each public operation resolves a plan, prints it, flips state to BUSY, then run_plan iterates plan->entries[] in order calling exec_plan_entry. That function is a switch on pe->op: INSTALL loads → extracts → marks installed → index_add + index_update; REMOVE loads → removes files → index_remove; UPGRADE removes files → marks UPGRADING → reloads → extracts → marks INSTALLED. A conflict short-circuits before any entry runs. Failure at entry i aborts the remaining entries — no rollback. Records are freed per entry, and events go out by callback on the same call.


The three-way split of work
Concern	File	Decides	Runs
What exists in the repo	store	—	index add/remove/update
What order to do it in	resolve	✓	—
Do it	exec	—	✓
That's the whole shape: types defines, buf stores, store reads and writes, resolve plans, exec executes.