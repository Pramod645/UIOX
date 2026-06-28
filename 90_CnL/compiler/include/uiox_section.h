#ifndef UIOX_SECTION_H
#define UIOX_SECTION_H
/*
 * uiox_section.h - UIOX linker section/segment management
 */

#define UIOX_SECT_NAME_MAX  32
#define UIOX_SECT_DATA_MAX  (16 * 1024 * 1024)  /* 16 MB max     */

typedef enum uiox_sect_type {
    SECT_NULL    = 0,
    SECT_TEXT    = 1,   /* executable code                     */
    SECT_DATA    = 2,   /* initialised read-write data         */
    SECT_RODATA  = 3,   /* read-only data                      */
    SECT_BSS     = 4,   /* zero-initialised (no file data)     */
    SECT_RELOC   = 5,   /* relocation entries                  */
    SECT_SYMTAB  = 6,   /* symbol table                        */
    SECT_STRTAB  = 7,   /* string table                        */
    SECT_DEBUG   = 8,   /* debug info                          */
    SECT_COMMENT = 9,   /* comment / version string            */
} uiox_sect_type_t;

typedef enum uiox_sect_flags {
    SECT_F_ALLOC   = (1 << 0),  /* allocated in memory image    */
    SECT_F_EXEC    = (1 << 1),  /* executable                   */
    SECT_F_WRITE   = (1 << 2),  /* writable                     */
    SECT_F_MERGE   = (1 << 3),  /* mergeable                    */
    SECT_F_STRINGS = (1 << 4),  /* null-terminated strings      */
    SECT_F_NOLOAD  = (1 << 5),  /* not loaded (BSS)            */
} uiox_sect_flags_t;

typedef struct uiox_section {
    char              name[UIOX_SECT_NAME_MAX];
    uiox_sect_type_t  type;
    unsigned int      flags;
    unsigned long long vaddr;   /* virtual address at link time  */
    unsigned long long offset;  /* offset in output file         */
    unsigned int       align;   /* alignment (power of 2)        */
    unsigned char     *data;    /* raw section bytes             */
    unsigned int       size;    /* current fill size             */
    unsigned int       cap;     /* allocated capacity            */
} uiox_section_t;

uiox_section_t *uiox_sect_create  (const char *name, uiox_sect_type_t type,
                                    unsigned int flags, unsigned int align);
void            uiox_sect_free    (uiox_section_t *s);
int             uiox_sect_write   (uiox_section_t *s, const void *data,
                                    unsigned int len);
int             uiox_sect_write8  (uiox_section_t *s, unsigned char v);
int             uiox_sect_write16 (uiox_section_t *s, unsigned short v);
int             uiox_sect_write32 (uiox_section_t *s, unsigned int v);
int             uiox_sect_write64 (uiox_section_t *s, unsigned long long v);
int             uiox_sect_pad     (uiox_section_t *s, unsigned int align);
unsigned int    uiox_sect_pos     (const uiox_section_t *s);
void            uiox_sect_patch32 (uiox_section_t *s, unsigned int off,
                                    unsigned int val);
void            uiox_sect_patch64 (uiox_section_t *s, unsigned int off,
                                    unsigned long long val);

#endif /* UIOX_SECTION_H */
