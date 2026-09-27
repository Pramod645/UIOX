What unfs_format.h now owns
Everything below is defined in exactly one place, and 01_fsa compiles against it:

Constant	Value

Constant	Value
UNFS_BLOCK_SIZE	4096
UNFS_INODE_SIZE	256
UNFS_INODES_PER_BLOCK	16
UNFS_SB_BLOCK	0
UNFS_ROOT_INO	2
UNFS_GROUP0_ITABLE	261
UNFS_ITABLE_BLOCKS	8
UNFS_NAME_MAX	255
UNFS_IFMT / UNFS_IF*	the on-disk type encoding
UNFS_DT_*	the dirent type encoding
unfs_sb_t, unfs_inode_t, unfs_extent_t, unfs_dirent_t	the structs
UNFS_INLINE_EXTENTS, UNFS_DIRENT_ALIGN	added by fs_types.h
UNFS_SECTORS_PER_BLOCK	8 — the buffer-cache conversion

And the 88-byte pad in unfs_inode_t stands as the right resolution: it closes a struct that declares 256 but measures 168, without moving a single inode offset. The bootloader's own header carries the same pad, so its assert passes too.
===============================
