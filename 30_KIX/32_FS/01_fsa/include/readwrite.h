/*
 *  30_KIX/32_FS/01_fsa/include/readwrite.h
 *
 *  Read and write over an inode — Bach's I/O loop, declared.
 *
 *  Bach puts this loop inside Algorithm read and Algorithm write.  The
 *  syscall layer (10_scfs) is the wrapper around it; this is the loop.
 *
 *  v1.0: first cut against the real inode.h / bmap.h.
 */
#ifndef UIOX_READWRITE_H
#define UIOX_READWRITE_H

#include "inode.h"
#include "bmap.h"

/* ─────────────────────────────────────────────────────────────
 * The advancing forms
 *
 * *off is the file offset; it moves forward by the number of bytes
 * actually transferred.  These are the pair a syscall wrapper calls when
 * it holds an offset of its own.
 *
 * Both return the byte count, or a negative value on error.  A short
 * return means end of file (read) or out of space (write).
 * ───────────────────────────────────────────────────────────── */
int32_t readi (InCoreInode *ip, char       *kbuf, uint32_t  count, uint32_t *off);
int32_t writei(InCoreInode *ip, const char *kbuf, uint32_t  count, uint32_t *off);

/* ─────────────────────────────────────────────────────────────
 * The positioned forms
 *
 * The offset is passed BY VALUE, so nothing the caller owns moves.  This
 * is what pread/pwrite need, and it is also what keeps a file table entry
 * shared through dup() from having its f_offset displaced by a positioned
 * read.
 *
 * *moved reports how many bytes moved, because the return value cannot
 * carry it alone when the caller needs to distinguish "0 bytes, EOF" from
 * "0 bytes, error".
 * ───────────────────────────────────────────────────────────── */
int32_t readi_at (InCoreInode *ip, char       *kbuf, uint32_t  count,
                  uint32_t off, uint32_t *moved);
int32_t writei_at(InCoreInode *ip, const char *kbuf, uint32_t  count,
                  uint32_t off, uint32_t *moved);

#endif /* UIOX_READWRITE_H */
