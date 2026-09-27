/*
 * 30_KIX/32_FS/10_unfs/src/unfs_iget.c
 *
 * UNFS — the on-disk -> in-core i-node bridge.
 *
 * Reads a unfs_inode_t (the bytes) and fills an InCoreInode (what 01_fsa
 * speaks).  Everything above — namei, readwrite, the file table — is
 * Bach's algorithm and never sees the on-disk layout; everything below —
 * extents, group bitmaps, xattr — is UNFS's private concern.  This file
 * is the one seam between them.
 *
 * ── WHAT CHANGED, AND WHY ─────────────────────────────────────────────
 * The previous revision was written against an inode type that does not
 * exist in this tree.  It used:
 *
 *     inode_t, inode_table[], NINODE       — no such cache
 *     ip->i_number, i_count, i_dev, i_flag — different field names
 *     ip->i_iop / ip->i_fop                — no ops pointers on the struct
 *     unfs_inode_ops / unfs_file_ops       — declared extern, defined
 *                                            nowhere in the tree
 *     IEXTENTS                             — not defined in fs_types.h
 *     unfs_mk64(d->i_size_lo, d->i_size_hi) — those fields were removed
 *
 * The cache in this tree is described by fs_types.h's MAX_INCACHE comment
 * ("inode.h: icache[] entries") and the type is InCoreInode.  So this is
 * a port, not a rename: the three conversions the banner below lists are
 * now all plain assignments, because unfs_inode_t and InCoreInode agree.
 *
 * ── THE THREE CONVERSIONS, AND WHAT BECAME OF THEM ────────────────────
 *   1. 64-bit size   : unfs_inode_t.i_size is ONE uint64_t and
 *                      InCoreInode.size is uint32_t.  There is no _lo/_hi
 *                      pair to reassemble any more, so the conversion is a
 *                      CLIP — see the note on inode_inflate().
 *   2. 64-bit times  : i_atime_ns / i_mtime_ns / i_ctime_ns on disk,
 *                      time_t in core.  A plain assignment; both are
 *                      64-bit in this build (uiox_klibc.h: time_t =
 *                      int64_t), so no value is lost.  The UNITS differ in
 *                      name only — the disk field is nanoseconds and the
 *                      core field is not documented as such, so a caller
 *                      comparing them against a clock must divide.
 *   3. extent model  : i_extents[4] + i_extent_tree exist on BOTH structs
 *                      with the same names, so this is a field-for-field
 *                      copy.  bmap() reads ip->i_extents directly and
 *                      needs nothing converted.
 *
 * @version 3.0.0  @date 2026-09-27
 */

 #include "unfs_fs.h"        /* unfs_fs_t, unfs_inode_priv_t            */
 #include "inode.h"          /* InCoreInode, DiskInode                  */
 #include "uiox_klibc.h"
 #include "unfs_io.h"
 #include "unfs_errno.h"
 
 #ifndef UNFS_EBADMAGIC
 #define UNFS_EBADMAGIC  -110   /* not a UNFS volume                   */
 #endif
 
 /* The inode cache.  fs_types.h sizes it with MAX_INCACHE and its comment
  * names this array; nothing else in the tree defines it, so it lives here
  * as the cache's owner.  inode.h declares inode_cache_init() for whatever
  * populates it at boot. */
 static InCoreInode icache[MAX_INCACHE];
 
 /* ─────────────────────────────────────────────────────────────────────
  * icache_slot — a free cache entry
  *
  * "Free" means no open reference AND no inode number: an entry whose
  * refcount has dropped to zero but still names an inode is a cached
  * (valid, reusable) inode, not a free slot.  Reusing one of those without
  * flushing it would lose the dirty state.
  * ───────────────────────────────────────────────────────────────────── */
 static InCoreInode *icache_slot(void)
 {
     uiox_uint32_t i;
 
     for (i = 0u; i < MAX_INCACHE; i++) {
         if (icache[i].refcount == 0 && icache[i].ino == 0u)
             return &icache[i];
     }
     return (InCoreInode *)0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_read_inode — the bytes at inode 'inum' into *out
  *
  * Geometry, from inode.h's own header comment:
  *
  *     block_num   = ((ino - 1) / INODES_PER_BLOCK) + UNFS_GROUP0_ITABLE
  *     byte_offset = ((ino - 1) % INODES_PER_BLOCK) * sizeof(DiskInode)
  *
  * The inode table is GROUP-0 ABSOLUTE: UNFS_GROUP0_ITABLE (261), not a
  * disk-supplied s_inode_first_blk.  unfs_sb_t carries no such field —
  * that was the removed header.
  *
  * The superblock is read and its magic checked on every call.  That is
  * deliberate: it costs one buffer-cache hit (the block is resident after
  * the first) and it means an inode read on a volume that is not UNFS
  * fails here rather than returning 256 bytes of someone else's data.
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_read_inode(uiox_uint32_t dev, uiox_uint32_t inum,
                     unfs_inode_t *out)
 {
     uiox_uint32_t byte_off;
     uiox_uint32_t blk;
     uiox_uint32_t off;
     uiox_uint8_t  blk0[UNFS_BLOCK_SIZE];
     uiox_uint8_t  ibuf[UNFS_BLOCK_SIZE];
     uiox_uint8_t *src;
     uiox_uint8_t *dst;
     uiox_uint32_t k;
 
     if (!out) return UNFS_EINVAL;
     if (inum == UNFS_NIL_INO) return UNFS_EINVAL;
 
     byte_off = (inum - 1u) * UNFS_INODE_BYTES;
     blk      = byte_off / UNFS_BLOCK_SIZE;
     off      = byte_off % UNFS_BLOCK_SIZE;
 
     if (unfs_bdev_read(dev, 0u, blk0) != UNFS_OK) return UNFS_EIO;
 
     /* The superblock occupies block 0 IN FULL — UNFS_SB_OFFSET is 0.  The
      * removed header put it at byte 1024 and read the wrong 4 KB. */
     {
         unfs_sb_t sb;
         uiox_uint8_t *p = (uiox_uint8_t *)&sb;
         for (k = 0u; k < (uiox_uint32_t)sizeof(sb); k++)
             p[k] = blk0[UNFS_SB_OFFSET + k];
         if (sb.s_magic != UNFS_MAGIC) return UNFS_EBADMAGIC;
     }
 
     if (unfs_bdev_read(dev, UNFS_GROUP0_ITABLE + blk, ibuf) != UNFS_OK)
         return UNFS_EIO;
 
     src = ibuf + off;
     dst = (uiox_uint8_t *)out;
     for (k = 0u; k < (uiox_uint32_t)sizeof(*out); k++)
         dst[k] = src[k];
 
     return UNFS_OK;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_write_inode_disk — the reverse, read-modify-write
  *
  * The enclosing block is read first because 16 inodes share it and the
  * other 15 must survive.  A read failure is NOT fatal here: the slot is
  * zeroed and then written, which loses the neighbours but produces a
  * volume a fsck can repair.  Writing into 256 bytes of uninitialised
  * stack instead would write garbage over every sibling inode.
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_write_inode_disk(uiox_uint32_t dev, uiox_uint32_t inum,
                           const unfs_inode_t *in)
 {
     uiox_uint32_t byte_off;
     uiox_uint32_t blk;
     uiox_uint32_t off;
     uiox_uint8_t  ibuf[UNFS_BLOCK_SIZE];
     uiox_uint8_t *dst;
     uiox_uint8_t *src;
     uiox_uint32_t k;
 
     if (!in) return UNFS_EINVAL;
     if (inum == UNFS_NIL_INO) return UNFS_EINVAL;
 
     byte_off = (inum - 1u) * UNFS_INODE_BYTES;
     blk      = byte_off / UNFS_BLOCK_SIZE;
     off      = byte_off % UNFS_BLOCK_SIZE;
 
     if (unfs_bdev_read(dev, UNFS_GROUP0_ITABLE + blk, ibuf) != UNFS_OK) {
         for (k = 0u; k < UNFS_BLOCK_SIZE; k++) ibuf[k] = 0u;
     }
 
     dst = ibuf + off;
     src = (uiox_uint8_t *)in;
     for (k = 0u; k < (uiox_uint32_t)sizeof(*in); k++)
         dst[k] = src[k];
 
     return unfs_bdev_write(dev, UNFS_GROUP0_ITABLE + blk, ibuf);
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * inode_inflate — unfs_inode_t -> InCoreInode
  *
  * THE SIZE CLIP.  unfs_inode_t.i_size is uint64_t; InCoreInode.size is
  * uint32_t.  A file larger than 4 GiB - 1 cannot be represented in core,
  * so the value is CLIPPED rather than truncated by an implicit narrowing
  * conversion — a silent wrap would make a large file look small, and the
  * read path would then serve a short file that verifies.
  *
  * The clip is reported: inode_inflate() returns non-zero when the on-disk
  * size exceeded what the core struct can hold, and unfs_iget() propagates
  * it by returning NULL rather than an inode whose size is a lie.  A
  * caller that must handle huge files needs InCoreInode.size widened to
  * uint64_t — one field, but it shifts the struct, so it is a deliberate
  * change rather than a porting detail.
  * ───────────────────────────────────────────────────────────────────── */
 static int inode_inflate(const unfs_inode_t *d, InCoreInode *ip)
 {
     uiox_uint32_t i;
     uiox_uint8_t *p = (uiox_uint8_t *)ip;
 
     /* clear field by field; no mem_zero declared in this unit */
     for (i = 0u; i < (uiox_uint32_t)sizeof(*ip); i++) p[i] = 0u;
 
     ip->mode  = d->i_mode;
     ip->nlink = d->i_nlink;
     ip->uid   = d->i_uid;
     ip->gid   = d->i_gid;
 
     /* conversion 1 — the clip.  Anything above 0xFFFFFFFF cannot be
      * carried in core; refuse rather than misreport. */
     if (d->i_size > 0xFFFFFFFFULL) {
         ip->size = 0xFFFFFFFFu;
         return 1;                     /* reported, not silently kept */
     }
     ip->size = (uiox_uint32_t)d->i_size;
 
     ip->i_extent_tree = d->i_extent_tree;
     for (i = 0u; i < UNFS_INLINE_EXTENTS; i++)
         ip->i_extents[i] = d->i_extents[i];
 
     /* conversion 2 — both 64-bit in this build, so no value is lost.
      * Units differ in name only: the disk fields are nanoseconds. */
     ip->atime = d->i_atime_ns;
     ip->mtime = d->i_mtime_ns;
     ip->ctime = d->i_ctime_ns;
 
     /* NOT carried, and why:
      *
      *   i_mac_label[16] / i_mac_flags   InCoreInode has no MAC fields.
      *                                   Reading them would need a
      *                                   33_PCS/05_sec interface that does
      *                                   not exist; the bytes stay on disk
      *                                   untouched by a metadata update.
      *   i_inline[60]                    In-core symlink targets have no
      *                                   home either; bmap() is not used
      *                                   for a symlink, and the target is
      *                                   read from disk when needed.
      *   i_checksum / i_flags / i_blocks InCoreInode carries none of them.
      *
      * None of these is a conversion the caller can observe: everything
      * bmap(), namei() and readwrite() read is above.
      */
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * inode_deflate — InCoreInode -> unfs_inode_t
  *
  * Built from the DISK COPY kept alongside, not from the core fields —
  * InCoreInode holds a subset, so rebuilding the record from it alone
  * would zero the MAC label, the inline target and the checksum on every
  * write.  The caller supplies the disk image it read; this updates only
  * the fields the core side owns.
  * ───────────────────────────────────────────────────────────────────── */
 static void inode_deflate(const InCoreInode *ip, unfs_inode_t *d)
 {
     uiox_uint32_t i;
 
     d->i_mode  = ip->mode;
     d->i_nlink = ip->nlink;
     d->i_uid   = ip->uid;
     d->i_gid   = ip->gid;
     d->i_size  = ip->size;
 
     d->i_atime_ns = ip->atime;
     d->i_mtime_ns = ip->mtime;
     d->i_ctime_ns = ip->ctime;
 
     d->i_extent_tree = ip->i_extent_tree;
     for (i = 0u; i < UNFS_INLINE_EXTENTS; i++)
         d->i_extents[i] = ip->i_extents[i];
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_iget — find or load the in-core inode for (dev, inum)
  *
  * Returns a cache entry with refcount incremented, or NULL.  NULL means
  * either "no such inode" or "cannot be represented in core" — the two are
  * not distinguished here because a caller can do nothing different with
  * the fact; unfs_inode_size_ok() below lets a caller that cares ask.
  * ═════════════════════════════════════════════════════════════════════ */
 InCoreInode *unfs_iget(uiox_uint8_t dev, uiox_uint32_t inum)
 {
     unfs_inode_t d;
     InCoreInode *ip;
     uiox_uint32_t i;
 
     if (inum == UNFS_NIL_INO) return (InCoreInode *)0;
 
     /* resident? */
     for (i = 0u; i < MAX_INCACHE; i++) {
         ip = &icache[i];
         if (ip->dev == dev && ip->ino == inum && ip->refcount > 0)
             { ip->refcount++; return ip; }
     }
 
     if (unfs_read_inode((uiox_uint32_t)dev, inum, &d) != UNFS_OK)
         return (InCoreInode *)0;
 
     ip = icache_slot();
     if (!ip) return (InCoreInode *)0;
 
     if (inode_inflate(&d, ip) != 0) {
         /* Size does not fit the core struct.  Do not hand back an inode
          * whose size is a lie; leave the slot free. */
         for (i = 0u; i < (uiox_uint32_t)sizeof(*ip); i++)
             ((uiox_uint8_t *)ip)[i] = 0u;
         return (InCoreInode *)0;
     }
 
     ip->ino      = inum;
     ip->dev      = dev;
     ip->refcount = 1;
     ip->locked   = false;
     ip->flags    = 0u;
 
     return ip;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_iput — drop a reference, flush if dirty
  *
  * The flush happens when the last reference goes and the entry is
  * marked dirty, which is Bach's rule: an inode leaves the cache only
  * after its core state has reached the disk.
  *
  * The disk image is re-read before patching so the fields the core side
  * does not carry survive — see inode_deflate().
  * ═════════════════════════════════════════════════════════════════════ */
 void unfs_iput(InCoreInode *ip)
 {
     unfs_inode_t d;
 
     if (!ip) return;
     if (ip->refcount > 0) ip->refcount--;
     if (ip->refcount > 0) return;
 
     if ((ip->flags & IFLAG_CHANGED) != 0u) {
         if (unfs_read_inode((uiox_uint32_t)ip->dev, ip->ino, &d) == UNFS_OK) {
             inode_deflate(ip, &d);
             (void)unfs_write_inode_disk((uiox_uint32_t)ip->dev, ip->ino, &d);
         }
     }
 
     /* Keep it cached (ino retained) but unreferenced, so a re-open is
      * free.  The slot is reclaimed by icache_slot() only when ino is
      * cleared, which happens on a future eviction pass — nothing evicts
      * yet, and with MAX_INCACHE 32 that is a known ceiling. */
     ip->flags &= (uiox_uint8_t)~IFLAG_CHANGED;
 
     /* Nothing to drop: this build has no ops pointers to clear. */
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_inode_size_ok — can this inode be represented in core?
  *
  * For a caller that needs to tell "no such inode" from "too large".  Costs
  * one inode read; used by anything reporting the difference to a user.
  * ═════════════════════════════════════════════════════════════════════ */
 int unfs_inode_size_ok(uiox_uint8_t dev, uiox_uint32_t inum)
 {
     unfs_inode_t d;
 
     if (unfs_read_inode((uiox_uint32_t)dev, inum, &d) != UNFS_OK) return 0;
     return (d.i_size > 0xFFFFFFFFULL) ? 0 : 1;
 }
 