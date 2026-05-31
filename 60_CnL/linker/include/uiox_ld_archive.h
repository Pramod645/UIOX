#ifndef UIOX_LD_ARCHIVE_H
#define UIOX_LD_ARCHIVE_H
/*
 * uiox_ld_archive.h - UIOX linker static archive (.a) reader
 * Implements the BSD/GNU ar format used by ar(1).
 */
#include "uiox_ld_types.h"
#include "uiox_ld_object.h"
#include "uiox_ld_diag.h"

#define ULD_AR_MAGIC      "!<arch>\n"
#define ULD_AR_MAGIC_LEN  8
#define ULD_AR_HDR_SIZE   60

/* -- Archive member header (on-disk layout) ----------------- */
typedef struct uld_ar_hdr {
    char  name[16];
    char  date[12];
    char  uid[6];
    char  gid[6];
    char  mode[8];
    char  size[10];
    char  fmag[2];   /* Must be "`\n"                           */
} uld_ar_hdr_t;

/* -- Archive member ----------------------------------------- */
typedef struct uld_ar_member {
    char              name[ULD_NAME_MAX];
    uld_u64_t         offset;    /* byte offset in archive file  */
    uld_u64_t         size;
    struct uld_ar_member *next;
} uld_ar_member_t;

/* -- Archive record ----------------------------------------- */
typedef struct uld_archive {
    char              path[ULD_PATH_MAX];
    uld_u8_t         *raw;
    uld_u64_t         raw_size;
    uld_ar_member_t  *members;
    uld_u32_t         member_count;
    /* symbol index: maps symbol name -> member offset */
    char            **sym_names;
    uld_u64_t        *sym_offsets;
    uld_u32_t         sym_count;
} uld_archive_t;

int  uld_archive_open        (uld_archive_t *ar, const char *path,
                               uld_diag_ctx_t *diag);
void uld_archive_free        (uld_archive_t *ar);
int  uld_archive_extract_all (uld_archive_t *ar,
                               uld_object_t *objs,
                               uld_u32_t *obj_count,
                               uld_u32_t max_objs,
                               uld_diag_ctx_t *diag);
int  uld_archive_extract_sym (uld_archive_t *ar,
                               const char *sym_name,
                               uld_object_t *out_obj,
                               uld_diag_ctx_t *diag);

#endif /* UIOX_LD_ARCHIVE_H */
