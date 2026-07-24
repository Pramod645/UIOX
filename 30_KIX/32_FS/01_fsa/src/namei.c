#include "namei.h"
#include "bmap.h"
#include "superblock.h"
#include "uiox_klibc.h"

/* ─────────────────────────────────────────────────────────────
 * dir_lookup
 * ───────────────────────────────────────────────────────────── */
uint32_t dir_lookup(InCoreInode *dir, const char *name)
{
    uint32_t offset;
    if (!dir || !name) return 0;
    if (inode_type(dir) != FT_DIR) return 0;

    offset = 0;
    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        if (!bm.valid) break;

        BufEntry *buf = bread(bm.blkno);
        if (!buf) break;

        DirEntry *entries = (DirEntry *)buf->data;
        int n = BLOCK_SIZE / (int)sizeof(DirEntry);
        int i;

        for (i = 0; i < n; i++) {
            if (entries[i].ino == 0) continue;
            if (strncmp(entries[i].name, name, MAX_NAME_LEN - 1) == 0) {
                uint32_t found_ino = entries[i].ino;
                brelse(buf);
                return found_ino;
            }
        }
        brelse(buf);
        offset += BLOCK_SIZE;
    }
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * dir_add
 * ───────────────────────────────────────────────────────────── */
int dir_add(InCoreInode *dir, const char *name, uint32_t ino)
{
    uint32_t offset;
    if (!dir || !name || ino == 0) return -1;
    if (inode_type(dir) != FT_DIR) return -1;

    offset = 0;
    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        if (!bm.valid) break;

        BufEntry *buf = bread(bm.blkno);
        if (!buf) return -1;

        DirEntry *entries = (DirEntry *)buf->data;
        int n = BLOCK_SIZE / (int)sizeof(DirEntry);
        int i;

        for (i = 0; i < n; i++) {
            if (entries[i].ino == 0) {
                entries[i].ino = ino;
                strncpy(entries[i].name, name, MAX_NAME_LEN - 1);
                buf->dirty = true;
                bwrite(buf);
                brelse(buf);
                dir->size += (uint32_t)sizeof(DirEntry);
                dir->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;
                printf("[dir_add] '%s' -> ino=%u in dir ino=%u\n",
                       name, ino, dir->ino);
                return 0;
            }
        }
        brelse(buf);
        offset += BLOCK_SIZE;
    }

    /* Extend directory by one block */
    {
        BmapResult bm = bmap_alloc(dir, dir->size);
        if (!bm.valid) return -1;

        BufEntry *buf = bread(bm.blkno);
        if (!buf) return -1;

        DirEntry *entries = (DirEntry *)buf->data;
        entries[0].ino = ino;
        strncpy(entries[0].name, name, MAX_NAME_LEN - 1);
        buf->dirty = true;
        bwrite(buf);
        brelse(buf);

        dir->size += BLOCK_SIZE;
        dir->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;
        printf("[dir_add] '%s' -> ino=%u (new block) in dir ino=%u\n",
               name, ino, dir->ino);
        return 0;
    }
}

/* ─────────────────────────────────────────────────────────────
 * dir_remove
 * ───────────────────────────────────────────────────────────── */
int dir_remove(InCoreInode *dir, const char *name)
{
    uint32_t offset;
    if (!dir || !name) return -1;

    offset = 0;
    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        if (!bm.valid) break;

        BufEntry *buf = bread(bm.blkno);
        if (!buf) return -1;

        DirEntry *entries = (DirEntry *)buf->data;
        int n = BLOCK_SIZE / (int)sizeof(DirEntry);
        int i;

        for (i = 0; i < n; i++) {
            if (entries[i].ino != 0 &&
                strncmp(entries[i].name, name, MAX_NAME_LEN - 1) == 0) {
                entries[i].ino  = 0;
                entries[i].name[0] = '\0';
                buf->dirty = true;
                bwrite(buf);
                brelse(buf);
                dir->flags |= IFLAG_MODIFIED;
                printf("[dir_remove] '%s' removed from dir ino=%u\n",
                       name, dir->ino);
                return 0;
            }
        }
        brelse(buf);
        offset += BLOCK_SIZE;
    }
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm namei  (§4)
 * ───────────────────────────────────────────────────────────── */
InCoreInode *namei(const char *path, InCoreInode *cwd,
                   uint16_t uid, uint16_t gid)
{
    InCoreInode *wip;
    char component[MAX_NAME_LEN];

    if (!path || path[0] == '\0') return NULL;

    if (path[0] == '/') {
        wip = iget(ROOT_INO);
        path++;
    } else {
        if (!cwd) { printf("[namei] ERROR: no cwd\n"); return NULL; }
        wip = iget(cwd->ino);
    }
    if (!wip) return NULL;

    while (*path) {
        uint32_t found_ino;
        int len = 0;

        while (*path == '/') path++;
        if (*path == '\0') break;

        while (*path && *path != '/' && len < MAX_NAME_LEN - 1)
            component[len++] = *path++;
        component[len] = '\0';

        if (inode_type(wip) != FT_DIR) {
            printf("[namei] ERROR: '%s' not a directory\n", component);
            iput(wip);
            return NULL;
        }
        if (!inode_access_ok(wip, uid, gid, 0, 0, 1)) {
            printf("[namei] ERROR: no execute perm on dir ino=%u\n", wip->ino);
            iput(wip);
            return NULL;
        }

        if (strcmp(component, "..") == 0 && wip->ino == ROOT_INO) {
            printf("[namei] '..' at root - staying\n");
            continue;
        }

        found_ino = dir_lookup(wip, component);
        iput(wip);

        if (!found_ino) {
            printf("[namei] '%s' not found\n", component);
            return NULL;
        }

        wip = iget(found_ino);
        if (!wip) return NULL;

        printf("[namei] component '%s' -> ino=%u\n", component, found_ino);
    }

    return wip;
}

/* ─────────────────────────────────────────────────────────────
 * fs_mkfs
 * ───────────────────────────────────────────────────────────── */
void fs_mkfs(void)
{
    InCoreInode *root;
    printf("[mkfs] creating filesystem\n");

    root = ialloc(FT_DIR,
                  PERM_UR|PERM_UW|PERM_UX|
                  PERM_GR|PERM_GX|
                  PERM_OR|PERM_OX,
                  0, 0);
    if (!root) { printf("[mkfs] ERROR: cannot alloc root inode\n"); return; }

    root->nlink = 2;
    root->flags |= IFLAG_CHANGED;

    dir_add(root, ".",  root->ino);
    dir_add(root, "..", root->ino);

    iupdate(root);
    iput(root);

    printf("[mkfs] root ino=%u created\n", ROOT_INO);
}
