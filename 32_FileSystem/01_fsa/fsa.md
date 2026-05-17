| File | Algorithm / concept |
| --- | --- |
| fs_types.h | All shared constants: block size, inode layout, file types, permission bits, dirty flags |
| buffer.h/c | Buffer cache — getblk, bread, bwrite, brelse, buf_sync; simulated disk array |
| inode.h/c | Algorithm 1 iget (cache lookup + disk read), Algorithm 2 iput (refcount, link-count=0 cleanup, write-back), iupdate, inode_access_ok |
| superblock.h/c | Algorithm 5 fs_alloc (free-block chain), Algorithm 6 fs_free, Algorithm 7 ialloc (free-inode list + disk scan), Algorithm 8 ifree, fs_free_inode_blocks |
| bmap.h/c | Algorithm 3 bmap (direct + single/double/triple indirect), bmap_alloc (allocating variant for writes) |
| namei.h/c | Algorithm 4 namei (path→inode walk), dir_lookup, dir_add, dir_remove, fs_mkfs (root directory creation) |