#include <stdio.h>
#include <string.h>
#include "fs_types.h"
#include "buffer.h"
#include "inode.h"
#include "superblock.h"
#include "bmap.h"
#include "namei.h"

static void banner(const char *s)
{
    printf("\n══════════════════════════════════════════\n");
    printf("  %s\n", s);
    printf("══════════════════════════════════════════\n");
}

/* ─────────────────────────────────────────────────────────────
 * Helper: create a regular file in a directory
 * ───────────────────────────────────────────────────────────── */
static InCoreInode *create_file(InCoreInode *dir, const char *name,
                                 uint16_t uid, uint16_t gid)
{
    InCoreInode *ip = ialloc(FT_REGULAR,
                              PERM_UR|PERM_UW|PERM_GR|PERM_OR,
                              uid, gid);
    if (!ip) return NULL;
    ip->nlink = 1;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);
    dir_add(dir, name, ip->ino);
    dir->nlink++;
    iupdate(dir);
    printf("[create] '%s' → ino=%u\n", name, ip->ino);
    return ip;
}

/* ─────────────────────────────────────────────────────────────
 * Helper: write bytes to a file using bmap_alloc + buffer cache
 * ───────────────────────────────────────────────────────────── */
static void file_write(InCoreInode *ip, const char *data, uint32_t len)
{
    uint32_t written = 0;
    while (written < len) {
        BmapResult bm = bmap_alloc(ip, ip->size + written);
        if (!bm.valid) break;

        BufEntry *buf = bread(bm.blkno);
        if (!buf) break;

        uint32_t n = len - written;
        if (n > bm.io_bytes) n = bm.io_bytes;
        memcpy(buf->data + bm.blk_offset, data + written, n);
        buf->dirty = true;
        bwrite(buf);
        brelse(buf);
        written += n;
    }
    ip->size  += written;
    ip->flags |= IFLAG_MODIFIED | IFLAG_ACCESSED;
    iupdate(ip);
    printf("[file_write] ino=%u  +%u bytes  size=%u\n",
           ip->ino, written, ip->size);
}

/* ─────────────────────────────────────────────────────────────
 * Helper: read bytes from a file
 * ───────────────────────────────────────────────────────────── */
static void file_read(InCoreInode *ip, uint32_t offset,
                      char *out, uint32_t len)
{
    uint32_t rd = 0;
    while (rd < len && offset + rd < ip->size) {
        BmapResult bm = bmap(ip, offset + rd);
        if (!bm.valid) break;

        BufEntry *buf = bread(bm.blkno);
        if (!buf) break;

        uint32_t n = len - rd;
        if (n > bm.io_bytes) n = bm.io_bytes;
        if (n > ip->size - (offset + rd)) n = ip->size - (offset + rd);
        memcpy(out + rd, buf->data + bm.blk_offset, n);
        brelse(buf);
        rd += n;
    }
    out[rd] = '\0';
    printf("[file_read] ino=%u  offset=%u  read=%u bytes: '%s'\n",
           ip->ino, offset, rd, out);
}

/* ─────────────────────────────────────────────────────────────
 * main
 * ───────────────────────────────────────────────────────────── */
int main(void)
{
    /* ── Initialise subsystems ──────────────────────────────── */
    banner("Filesystem Init");
    buf_init();
    inode_cache_init();
    sb_init();
    fs_mkfs();
    sb_print();

    /* ── ialloc / iget / iput ───────────────────────────────── */
    banner("ialloc / iget / iput  (Algorithms 7, 1, 2)");

    InCoreInode *root = iget(ROOT_INO);
    printf("Root ino=%u  type=%d  size=%u\n",
           root->ino, inode_type(root), root->size);

    /* Create a subdirectory 'home' */
    InCoreInode *home = ialloc(FT_DIR,
                                PERM_UR|PERM_UW|PERM_UX|PERM_GR|PERM_GX,
                                0, 0);
    home->nlink = 2;
    home->flags |= IFLAG_CHANGED;
    iupdate(home);
    dir_add(root, "home", home->ino);
    dir_add(home, ".",    home->ino);
    dir_add(home, "..",   root->ino);

    /* Create files */
    InCoreInode *f1 = create_file(home, "readme.txt", 1000, 1000);
    InCoreInode *f2 = create_file(home, "data.bin",   1000, 1000);

    /* ── bmap / file write / read ───────────────────────────── */
    banner("bmap / file I/O  (Algorithm 3)");

    const char *text = "Hello from the UIOX filesystem implementation!";
    file_write(f1, text, (uint32_t)strlen(text));

    char rbuf[128] = {0};
    file_read(f1, 0, rbuf, (uint32_t)strlen(text));

    /* ── namei ──────────────────────────────────────────────── */
    banner("namei  (Algorithm 4)");

    /* Look up absolute path */
    InCoreInode *found = namei("/home/readme.txt", NULL, 0, 0);
    if (found) {
        printf("namei('/home/readme.txt') → ino=%u  size=%u\n",
               found->ino, found->size);
        iput(found);
    }

    /* Look up relative path from home */
    InCoreInode *found2 = namei("data.bin", home, 1000, 1000);
    if (found2) {
        printf("namei('data.bin') → ino=%u\n", found2->ino);
        iput(found2);
    }

    /* Non-existent path */
    InCoreInode *notfound = namei("/home/ghost.txt", NULL, 0, 0);
    if (!notfound) printf("namei('/home/ghost.txt') → not found (correct)\n");

    /* ── alloc / free  (Algorithm 5 / 6) ───────────────────── */
    banner("alloc / free  (Algorithms 5, 6)");

    BufEntry *nb1 = fs_alloc();
    BufEntry *nb2 = fs_alloc();
    printf("alloc blk=%u\n", nb1 ? nb1->blkno : 0);
    printf("alloc blk=%u\n", nb2 ? nb2->blkno : 0);

    uint32_t saved1 = nb1 ? nb1->blkno : 0;
    uint32_t saved2 = nb2 ? nb2->blkno : 0;
    if (nb1) brelse(nb1);
    if (nb2) brelse(nb2);
    if (saved1) fs_free(saved1);
    if (saved2) fs_free(saved2);
    sb_print();

    /* ── ifree  (Algorithm 8) ───────────────────────────────── */
    banner("ifree  (Algorithm 8)");

    /* Unlink f2 (decrement link count → 0 triggers iput cleanup) */
    dir_remove(home, "data.bin");
    f2->nlink = 0;
    f2->flags |= IFLAG_CHANGED;
    iput(f2);  /* refcount → 0, nlink == 0 → blocks + inode freed */

    sb_print();

    /* ── iput on remaining inodes ──────────────────────────── */
    banner("Cleanup");
    iput(f1);
    iput(home);
    iput(root);

    buf_sync();
    buf_print();

    return 0;
}
