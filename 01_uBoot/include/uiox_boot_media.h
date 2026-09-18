/*
 * 01_uBoot/include/uiox_boot_media.h
 * Boot-media abstraction — the bootloader asks for a UNFS block and does
 * not care whether it came from VirtIO, SDHCI, SPI-NOR, NVMe, or USB.
 */
#ifndef UIOX_BOOT_MEDIA_H
#define UIOX_BOOT_MEDIA_H

#include "uiox_boot_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UIOX_MEDIA_NONE    = 0,
    UIOX_MEDIA_VIRTIO  = 1,
    UIOX_MEDIA_SDMMC   = 2,
    UIOX_MEDIA_SPI_NOR = 3,
    UIOX_MEDIA_NVME    = 4,
    UIOX_MEDIA_AHCI    = 5,
    UIOX_MEDIA_USB     = 6,
} uiox_boot_media_kind_t;

#define UIOX_MEDIA_BLOCK_SIZE        4096u
#define UIOX_MEDIA_SECTOR_SIZE       512u
#define UIOX_MEDIA_SECTORS_PER_BLOCK (UIOX_MEDIA_BLOCK_SIZE / UIOX_MEDIA_SECTOR_SIZE)

typedef struct {
    const char *name;
    uiox_boot_media_kind_t kind;
    int  (*present)(void);
    uiox_boot_err_t (*init)(void);
    uiox_boot_err_t (*read_block)(uint32_t blk, uint32_t nblocks, void *buf);
} uiox_boot_media_ops_t;

void uiox_boot_media_register(const uiox_boot_media_ops_t *ops);
uiox_boot_media_kind_t uiox_boot_media_select(void);
uiox_boot_err_t uiox_boot_media_read_block(uint32_t blk, uint32_t nblocks, void *buf);
uiox_boot_media_kind_t uiox_boot_media_active(void);
const char *uiox_boot_media_active_name(void);

/* Driver constructors (defined in boot_media/*.c) */
const uiox_boot_media_ops_t *uiox_boot_media_fwstor(void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_BOOT_MEDIA_H */
