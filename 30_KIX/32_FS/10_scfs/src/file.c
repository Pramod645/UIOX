#include "../include/file.h"
#include "../include/fs.h"
#include <string.h>
#include <stdio.h>

file_t   file_table[NFILE];
u_area_t u;

/* ── falloc ───────────────────────────────────────────────────────
 * Allocate a free entry in the system file table.
 */
file_t *falloc(void)
{
    for (int i = 0; i < NFILE; i++) {
        if (file_table[i].f_count == 0) {
            memset(&file_table[i], 0, sizeof(file_t));
            file_table[i].f_count = 1;
            return &file_table[i];
        }
    }
    fprintf(stderr, "falloc: file table full\n");
    return NULL;
}

/* ── fclose ───────────────────────────────────────────────────────
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

/* ── ufalloc ──────────────────────────────────────────────────────
 * Allocate a user file descriptor (lowest free fd).
 */
int ufalloc(void)
{
    for (int i = 0; i < NOFILE; i++) {
        if (u.u_ofile.ufd_file[i] == NULL)
            return i;
    }
    fprintf(stderr, "ufalloc: per-process fd table full\n");
    return -1;
}
