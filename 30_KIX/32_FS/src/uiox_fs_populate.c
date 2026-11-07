/*
 * 30_KIX/32_FS/src/uiox_fs_populate.c
 * First-boot populate: put the userspace image and the package store
 * into the volume.
 *
 * Runs once, after the root is mounted and before the first exec.  The
 * volume mkfs produced holds a superblock, bitmaps and an empty root --
 * no /bin, no /pkg, no files -- so execve("/bin/uios") would return
 * ENOENT.
 *
 * Three things arrive here, and the order between the last two is
 * forced:
 *
 *   1. /bin/uios          from its own blob -- the kernel must carry
 *                         something to bootstrap with.
 *   2. /pkg/<name>.upkg   from the packed store, one file each.
 *   3. /pkg/index.upix    LAST -- its entries carry archive_lba, a UNFS
 *                         BLOCK number, knowable only after the file
 *                         that owns it exists.  Its OWN block goes back
 *                         to the caller in *out_index_blk, because the
 *                         store cannot locate an index it didn't write.
 *
 * The index is written block by block, never held whole: 256 entries is
 * 27 KB, and this runs on a kernel stack.
 */
#include "superblock.h"      /* ialloc_dev, iget, iput           */
#include "inode.h"           /* InCoreInode, iupdate, IFLAG_*    */
#include "namei.h"           /* dir_add, namei                   */
#include "readwrite.h"       /* writei                           */
#include "bmap.h"            /* bmap, BmapResult                 */
#include "fs_types.h"        /* PERM_*                           */
#include "unfs_format.h"     /* UNFS_IF*, UNFS_DT_*, ROOT_DEV,
                              * UNFS_BLOCK_SIZE                 */
#include "../04_pkg/include/uiox_pkg_store.h"  /* uiox_pkg_index_hdr_t, _entry_t,
                              * UIOX_PKG_INDEX_MAGIC,
                              * UIOX_PKG_INDEX_MAX_ENTRIES,
                              * UIOX_PKG_VERSION                */

extern const char _uios_blob_start[];
extern const char _uios_blob_end[];
extern const char _uix_pkg_start[];
extern const char _uix_pkg_end[];

#define UIOS_BIN_NAME   "uios"
#define UIOS_DIR_NAME   "bin"
#define PKG_DIR_NAME    "pkg"
#define PKG_IDX_NAME    "index.upix"

/* ── the packed store's container ─────────────────────────────────────
 * The host packer emits this manifest, then the .upkg files at the
 * offsets it names.  NOT uiox_pkg_hdr_t -- that describes one package;
 * this describes the container.
 *
 * Stride is 56 = UIOX_PKG_NAME_MAX(48) + off(4) + len(4); the manifest
 * totals 16 + 256*56 = 14352 bytes.  mkouix_pkg_all.sh writes the same
 * numbers and fails loudly on a mismatch. */
#define UIOX_PKG_BLOB_MAGIC  0x55495850u   /* 'UIXP' */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t count;
    uint32_t total;
    uint32_t reserved;
    struct {
        char     name[UIOX_PKG_NAME_MAX];
        uint32_t off;
        uint32_t len;
    } ent[UIOX_PKG_INDEX_MAX_ENTRIES];
} uiox_pkg_blob_t;

/* ── index blocks ─────────────────────────────────────────────────────
 * uiox_pkg_store.h states the shape: one block for the header, one block
 * per entry.  The padding IS the format -- the reader addresses entry n
 * at index_blk + 1 + n. */
typedef struct __attribute__((packed)) {
    uiox_pkg_index_hdr_t hdr;
    uint8_t pad[UNFS_BLOCK_SIZE - sizeof(uiox_pkg_index_hdr_t)];
} pkg_index_hdr_block_t;

typedef struct __attribute__((packed)) {
    uiox_pkg_index_entry_t ent;
    uint8_t pad[UNFS_BLOCK_SIZE - sizeof(uiox_pkg_index_entry_t)];
} pkg_index_ent_block_t;

/* ── helpers ──────────────────────────────────────────────────────── */

/* Create one file in dir and fill it from bytes.  writei advances *off
 * by the bytes actually written (readwrite.c:265), so a short write
 * resumes on the next pass. */
static int write_one(InCoreInode *dir, const char *name, uint32_t namelen,
                     const char *bytes, uint32_t len)
{
    InCoreInode *file = ialloc_dev(ROOT_DEV, UNFS_IFREG,
                                   PERM_UR | PERM_UW | PERM_UX, 0u, 0u);
    uint32_t off = 0u;
    int32_t  n;

    if (!file) return -1;

    while (off < len) {
        n = writei(file, bytes + off, len - off, &off);
        if (n <= 0) { iput(file); return -1; }
    }
    iupdate(file);

    if (dir_add(dir, name, namelen, file->ino, UNFS_DT_REG) != 0) {
        iput(file); return -1;
    }
    iput(file);
    return 0;
}

static InCoreInode *mkdir_one(InCoreInode *parent, const char *name,
                              uint32_t namelen)
{
    InCoreInode *dir = ialloc_dev(ROOT_DEV, UNFS_IFDIR,
                                  PERM_UR | PERM_UW | PERM_UX, 0u, 0u);
    if (!dir) return 0;

    dir->nlink = 2u;
    dir->flags |= IFLAG_CHANGED;

    if (dir_add(parent, name, namelen, dir->ino, UNFS_DT_DIR) != 0) {
        iput(dir); return 0;
    }
    iupdate(dir);
    return dir;
}

/* "app_shell" -> "/pkg" entry name "app_shell.upkg"; returns its length
 * or 0 for an empty name */
static uint32_t upkg_name(char *dst, const char *base)
{
    uint32_t nl = 0u;

    while (nl < UIOX_PKG_NAME_MAX && base[nl]) {
        dst[nl] = base[nl];
        nl++;
    }
    if (nl == 0u) return 0u;

    dst[nl++] = '.'; dst[nl++] = 'u'; dst[nl++] = 'p';
    dst[nl++] = 'k'; dst[nl++] = 'g'; dst[nl]   = '\0';
    return nl;
}

/* the manifest's name field is NUL-padded, not necessarily terminated */
static void name_fill(char dst[UIOX_PKG_NAME_MAX], const char *src)
{
    uint32_t i = 0u;

    while (i < UIOX_PKG_NAME_MAX && src[i]) { dst[i] = src[i]; i++; }
    while (i < UIOX_PKG_NAME_MAX)            { dst[i] = '\0';    i++; }
}

/* ── uiox_fs_populate_root ──────────────────────────────────────────── */

int uiox_fs_populate_root(uiox_uint32_t *out_index_blk)
{
    InCoreInode            *root, *bin, *pkgdir;
    const char             *img;
    const uiox_pkg_blob_t  *blob;
    uint32_t                n = 0u, i;

    if (out_index_blk) *out_index_blk = 0u;

    root = iget(UNFS_ROOT_INO);
    if (!root) return -1;

    /* idempotent: a second boot finds /bin/uios and stops */
    if (namei("/" UIOS_DIR_NAME "/" UIOS_BIN_NAME, root, 0u, 0u)) {
        iput(root);
        return 0;
    }

    /* ── /bin, then /bin/uios ─────────────────────────────────────── */
    bin = mkdir_one(root, UIOS_DIR_NAME, 3u);
    if (!bin) { iput(root); return -1; }

    if (write_one(bin, UIOS_BIN_NAME, 4u,
                  _uios_blob_start,
                  (uint32_t)(_uios_blob_end - _uios_blob_start)) != 0) {
        iput(bin); iput(root); return -1;
    }

    /* ── /pkg, and the store ────────────────────────────────────────
     * Absent or malformed, /bin still holds uios -- the previous
     * behaviour, not a failure. */
    img    = _uix_pkg_start;
    pkgdir = 0;
    blob   = 0;

    if ((uint32_t)(_uix_pkg_end - _uix_pkg_start) >= sizeof(uiox_pkg_blob_t)) {
        const uiox_pkg_blob_t *b = (const uiox_pkg_blob_t *)img;

        if (b->magic == UIOX_PKG_BLOB_MAGIC &&
            b->total <= (uint32_t)(_uix_pkg_end - _uix_pkg_start)) {
            pkgdir = mkdir_one(root, PKG_DIR_NAME, 3u);
            if (pkgdir) blob = b;
        }
    }

    if (blob && pkgdir) {
        InCoreInode *idx;
        uint32_t     written = 0u;

        n = blob->count;
        if (n > UIOX_PKG_INDEX_MAX_ENTRIES) n = UIOX_PKG_INDEX_MAX_ENTRIES;

        /* (2) every .upkg, one file each --------------------------- */
        for (i = 0u; i < n; i++) {
            char     nm[UIOX_PKG_NAME_MAX + 8];
            uint32_t nl = upkg_name(nm, blob->ent[i].name);

            if (nl == 0u) continue;
            if (blob->ent[i].off + blob->ent[i].len > blob->total)
                continue;                       /* corrupt entry */

            (void)write_one(pkgdir, nm, nl,
                            img + blob->ent[i].off, blob->ent[i].len);
            /* a failed package is skipped, not fatal: one bad archive
             * should not leave the volume half-built */
        }

        /* (3) the index, LAST — archive_lba needs the files to exist */
        idx = ialloc_dev(ROOT_DEV, UNFS_IFREG, PERM_UR | PERM_UW, 0u, 0u);

        if (idx) {
            pkg_index_hdr_block_t hb;

            hb.hdr.magic       = UIOX_PKG_INDEX_MAGIC;
            hb.hdr.version     = UIOX_PKG_VERSION;
            hb.hdr.entry_count = n;      /* known up front — no patch */
            hb.hdr._pad        = 0u;
            {
                uint32_t z;
                for (z = 0u; z < sizeof(hb.pad); z++) hb.pad[z] = 0u;
            }

            if (writei(idx, (const char *)&hb, UNFS_BLOCK_SIZE,
                       &written) > 0) {

                for (i = 0u; i < n; i++) {
                    pkg_index_ent_block_t eb;
                    BmapResult            m;
                    char                  nm[UIOX_PKG_NAME_MAX + 8];
                    uint32_t              nl = upkg_name(nm, blob->ent[i].name);
                    InCoreInode          *f;

                    if (nl == 0u) continue;

                    /* the file just written — namei is the only lookup
                     * available here */
                    f = namei(nm, pkgdir, 0u, 0u);
                    if (!f) continue;

                    m = bmap(f, 0u);         /* block holding offset 0 */
                    if (!m.valid) { iput(f); continue; }

                    name_fill(eb.ent.name, blob->ent[i].name);
                    eb.ent.version      = UIOX_PKG_VERSION;
                    eb.ent.state        = UIOX_PKG_STATE_AVAILABLE;
                    eb.ent.archive_lba  = m.blkno;
                    eb.ent.archive_size = blob->ent[i].len;
                    {
                        uint32_t z;
                        for (z = 0u; z < UIOX_PKG_SHA256_LEN; z++)
                            eb.ent.sha256[z] = 0u;
                        for (z = 0u; z < sizeof(eb.ent._pad); z++)
                            eb.ent._pad[z] = 0u;
                        for (z = 0u; z < sizeof(eb.pad); z++)
                            eb.pad[z] = 0u;
                    }
                    iput(f);

                    if (writei(idx, (const char *)&eb, UNFS_BLOCK_SIZE,
                               &written) <= 0)
                        break;
                }
            }

            iupdate(idx);

            /* the block the caller needs — the whole reason this
             * function has a parameter.  bmap on the file we just
             * created, offset 0. */
            {
                BmapResult mi = bmap(idx, 0u);
                if (mi.valid && out_index_blk) *out_index_blk = mi.blkno;
            }

            (void)dir_add(pkgdir, PKG_IDX_NAME, 10u,
                          idx->ino, UNFS_DT_REG);
            iput(idx);
        }
    }

    if (pkgdir) iput(pkgdir);
    iput(bin);
    iput(root);
    return 0;
}
