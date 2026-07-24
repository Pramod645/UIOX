/*
 *  30_KIX/32_FS/10_scfs/src/file.c
 *
 *  Freestanding fixes (v1.1):
 *    FIXED: #include "../../33_PCS/..."  →  #include "uiox_klibc.h"
 *    FIXED: fprintf(stderr, ...)         →  printf(...)  (2 occurrences)
 *    FIXED: for (int i = ...)            →  int i; before loop (strict C11)
 */
#include "../include/file.h"
#include "../include/fs.h"
#include "uiox_klibc.h"

file_t   file_table[NFILE];
u_area_t u;

/* ── falloc ─────────────────────────────────────────────────────
 * Allocate a free entry in the system file table.
 */
file_t *falloc(void)
{
    int i;
    for (i = 0; i < NFILE; i++) {
        if (file_table[i].f_count == 0) {
            memset(&file_table[i], 0, sizeof(file_t));
            file_table[i].f_count = 1;
            return &file_table[i];
        }
    }
    printf("[falloc] ERROR: file table full\n");
    return NULL;
}

/* ── f_close ─────────────────────────────────────────────────────
 * Decrement file table reference count; release if zero.
 */
void f_close(file_t *fp)
{
    if (!fp) return;
    fp->f_count--;
    if (fp->f_count == 0) {
        iput(fp->f_inode);
        fp->f_inode = NULL;
    }
}

/* ── ufalloc ─────────────────────────────────────────────────────
 * Allocate a free slot in the per-process file descriptor table.
 */
int ufalloc(void)
{
    int i;
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile.ufd_file[i] == NULL)
            return i;
    }
    printf("[ufalloc] ERROR: per-process fd table full\n");
    return -1;
}
