#ifndef EXEC_H
#define EXEC_H
#include "uiox_klibc.h"

#define EXEC_MAGIC      0x7F454C46u
#define EXEC_MAGIC_AOUT 0x0107u
#define MAX_ARGS        128
#define MAX_ARG_LEN     256
#define MAX_ENV         64

typedef enum seg_type {
    SEG_TEXT  = 0,
    SEG_DATA  = 1,
    SEG_BSS   = 2,
    SEG_STACK = 3
} seg_type_t;

typedef struct seg_desc {
    seg_type_t sd_type;
    uint32_t   sd_vaddr;
    uint32_t   sd_file_off;
    uint32_t   sd_file_sz;
    uint32_t   sd_mem_sz;
    uint32_t   sd_flags;
} seg_desc_t;

#define SEG_EXEC   0x01
#define SEG_WRITE  0x02
#define SEG_READ   0x04

#define MAX_SEGMENTS 8
typedef struct exec_hdr {
    uint32_t   eh_magic;
    uint32_t   eh_entry;
    uint32_t   eh_nseg;
    seg_desc_t eh_segs[MAX_SEGMENTS];
    uint32_t   eh_flags;
    uint8_t    eh_setuid;
    uint8_t    eh_setgid;
} exec_hdr_t;

typedef struct exec_args {
    char ea_argv[MAX_ARGS][MAX_ARG_LEN];
    int  ea_argc;
    char ea_envp[MAX_ENV][MAX_ARG_LEN];
    int  ea_envc;
    char ea_filename[MAX_ARG_LEN];
} exec_args_t;

struct proc;
struct u_area;
struct inode;
struct pregion;
struct region;

int  kernel_exec       (const char *filename,
                        char *const argv[], char *const envp[]);
int  exec_verify_file  (struct inode *ip, exec_hdr_t *hdr);
void exec_detach_all   (struct u_area *u, struct proc *p);
int  exec_load_segments(struct u_area *u, struct proc *p,
                        struct inode *ip, exec_hdr_t *hdr);
void exec_copy_args    (struct u_area *u, exec_args_t *ea);
void exec_handle_setuid(struct proc *p, exec_hdr_t *hdr);
void exec_init_registers(struct u_area *u, exec_hdr_t *hdr);
void xalloc            (struct inode *ip, struct proc *p,
                        struct u_area *u, exec_hdr_t *hdr);

#endif /* EXEC_H */
