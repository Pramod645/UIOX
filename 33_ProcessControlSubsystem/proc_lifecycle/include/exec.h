#ifndef EXEC_H
#define EXEC_H

#include <stdint.h>
#include <stddef.h>

/* ── Exec file header (simplified a.out/ELF-like) ───────────── */
#define EXEC_MAGIC      0x7F454C46  /* ELF magic: \x7fELF       */
#define EXEC_MAGIC_AOUT 0x0107      /* a.out magic              */

#define MAX_ARGS        128     /* max number of exec arguments */
#define MAX_ARG_LEN     256     /* max length of each argument  */
#define MAX_ENV         64      /* max environment variables    */

/* ── Program segment types ──────────────────────────────────── */
typedef enum seg_type {
    SEG_TEXT  = 0,  /* read-only executable code                */
    SEG_DATA  = 1,  /* read-write initialized data              */
    SEG_BSS   = 2,  /* uninitialized data (zero-filled)         */
    SEG_STACK = 3   /* stack segment                            */
} seg_type_t;

/* ── Segment descriptor in exec file header ─────────────────── */
typedef struct seg_desc {
    seg_type_t  sd_type;        /* segment type                 */
    uint32_t    sd_vaddr;       /* virtual load address         */
    uint32_t    sd_file_off;    /* offset in file               */
    uint32_t    sd_file_sz;     /* size in file                 */
    uint32_t    sd_mem_sz;      /* size in memory (>= file_sz)  */
    uint32_t    sd_flags;       /* permissions / flags          */
} seg_desc_t;

/* Segment permission flags */
#define SEG_EXEC    0x01
#define SEG_WRITE   0x02
#define SEG_READ    0x04

/* ── Exec file header ───────────────────────────────────────── */
#define MAX_SEGMENTS 8

typedef struct exec_hdr {
    uint32_t  eh_magic;             /* magic number             */
    uint32_t  eh_entry;             /* program entry point      */
    uint32_t  eh_nseg;              /* number of segments       */
    seg_desc_t eh_segs[MAX_SEGMENTS]; /* segment descriptors    */
    uint32_t  eh_flags;             /* header flags             */
    uint8_t   eh_setuid;            /* setuid bit               */
    uint8_t   eh_setgid;            /* setgid bit               */
} exec_hdr_t;

/* ── Exec arguments (saved in kernel space during exec) ─────── */
typedef struct exec_args {
    char   ea_argv[MAX_ARGS][MAX_ARG_LEN];  /* argument strings */
    int    ea_argc;                          /* argument count   */
    char   ea_envp[MAX_ENV][MAX_ARG_LEN];   /* environment vars */
    int    ea_envc;                          /* env var count    */
    char   ea_filename[MAX_ARG_LEN];        /* executable path  */
} exec_args_t;

/* ── Function prototypes ────────────────────────────────────── */
struct proc;
struct u_area;
struct inode;
struct pregion;
struct region;

int  kernel_exec(const char *filename,
                 char *const argv[],
                 char *const envp[]);
int  exec_verify_file(struct inode *ip, exec_hdr_t *hdr);
void exec_detach_all(struct u_area *u, struct proc *p);
int  exec_load_segments(struct u_area *u, struct proc *p,
                        struct inode *ip, exec_hdr_t *hdr);
void exec_copy_args(struct u_area *u, exec_args_t *ea);
void exec_handle_setuid(struct proc *p, exec_hdr_t *hdr);
void exec_init_registers(struct u_area *u, exec_hdr_t *hdr);

/* xalloc: allocate and initialize text region */
void xalloc(struct inode *ip, struct proc *p,
            struct u_area *u, exec_hdr_t *hdr);

#endif /* EXEC_H */
