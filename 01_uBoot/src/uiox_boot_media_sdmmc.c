/*
 * 01_uBoot/src/boot_media/boot_media_sdmmc.c
 *
 * SDHCI/eMMC block driver — a REAL storage path for bare metal.
 * Uses the board descriptor's storage_base. Polled, no interrupts.
 *
 * Register offsets: SD Host Controller Simplified Spec (SDHCI).
 */
#include "uiox_boot.h"
#include "uiox_boot_media.h"
#include "uiox_boot_board.h"

#define SD_DMA_ADDR   0x00u
#define SD_BLK_SIZE   0x04u
#define SD_BLK_COUNT  0x06u
#define SD_ARG        0x08u
#define SD_XFER_MODE  0x0Cu
#define SD_COMMAND    0x0Eu
#define SD_BUF_DATA   0x20u
#define SD_PRESENT    0x24u
#define SD_INT_STATUS 0x30u
#define SD_ERR_STATUS 0x32u
#define SD_CMD17      17u
#define SD_CMD_DATA   (1u << 5)
#define SD_CMD_RESP_R1 (1u << 1)
#define SD_CMD_CRC    (1u << 3)
#define SD_CMD_IDX    (1u << 4)
#define SD_READ       (1u << 4)
#define SD_ST_CMD_INH (1u << 0)
#define SD_ST_DAT_INH (1u << 1)
#define SD_ST_BUF_RDY (1u << 11)
#define SD_INT_CMD_C  (1u << 0)
#define SD_INT_XFER   (1u << 1)
#define SD_INT_ERR    0xF8000FF0u

static inline void w32(uint64_t b, uint32_t o, uint32_t v)
{ *((volatile uint32_t *)(uintptr_t)(b + o)) = v; }
static inline uint32_t r32(uint64_t b, uint32_t o)
{ return *((volatile uint32_t *)(uintptr_t)(b + o)); }
static inline void w16(uint64_t b, uint32_t o, uint16_t v)
{ *((volatile uint16_t *)(uintptr_t)(b + o)) = v; }

static int sd_present(void)
{
    const uiox_board_t *bd = uiox_board_get();
    if (bd->storage != UIOX_STORAGE_SDMMC || !bd->storage_base) return 0;
    return r32(bd->storage_base, SD_PRESENT) != 0xFFFFFFFFu;
}

static uiox_boot_err_t sd_init(void) { return UIOX_BOOT_OK; }

static int sd_read_sector(uint32_t lba, void *dst)
{
    uint64_t b = uiox_board_get()->storage_base;
    for (int spin = 0; spin < 100000; spin++) {
        if (!(r32(b, SD_PRESENT) & (SD_ST_CMD_INH | SD_ST_DAT_INH))) break;
    }
    w32(b, SD_INT_STATUS, 0xFFFFFFFFu);
    w16(b, SD_BLK_SIZE, 512u);
    w16(b, SD_BLK_COUNT, 1u);
    w32(b, SD_ARG, lba);
    w16(b, SD_XFER_MODE, SD_READ);
    w16(b, SD_COMMAND, (uint16_t)((SD_CMD17 << 8) | SD_CMD_DATA |
                         SD_CMD_RESP_R1 | SD_CMD_CRC | SD_CMD_IDX));
    for (int spin = 0; spin < 100000; spin++)
        if (r32(b, SD_INT_STATUS) & SD_INT_CMD_C) break;
    w32(b, SD_INT_STATUS, SD_INT_CMD_C);
    if (r32(b, SD_ERR_STATUS) & SD_INT_ERR) return -1;

    uint8_t *p = (uint8_t *)dst;
    for (uint32_t i = 0; i < 512u / 4u; i++) {
        for (int spin = 0; spin < 100000; spin++)
            if (r32(b, SD_PRESENT) & SD_ST_BUF_RDY) break;
        uint32_t w = r32(b, SD_BUF_DATA);
        p[0] = (uint8_t)w; p[1] = (uint8_t)(w >> 8);
        p[2] = (uint8_t)(w >> 16); p[3] = (uint8_t)(w >> 24);
        p += 4;
    }
    for (int spin = 0; spin < 100000; spin++)
        if (r32(b, SD_INT_STATUS) & SD_INT_XFER) break;
    w32(b, SD_INT_STATUS, 0xFFFFFFFFu);
    return 0;
}

static uiox_boot_err_t sd_read_block(uint32_t blk, uint32_t nblocks, void *buf)
{
    uint8_t *d = (uint8_t *)buf;
    uint32_t base_sector = blk * UIOX_MEDIA_SECTORS_PER_BLOCK;
    for (uint32_t i = 0; i < nblocks * UIOX_MEDIA_SECTORS_PER_BLOCK; i++)
        if (sd_read_sector(base_sector + i, d + i * 512u) != 0)
            return UIOX_BOOT_ERR_IO;
    return UIOX_BOOT_OK;
}

static const uiox_boot_media_ops_t sd_ops = {
    .name = "sdhci", .kind = UIOX_MEDIA_SDMMC,
    .present = sd_present, .init = sd_init, .read_block = sd_read_block,
};

const uiox_boot_media_ops_t *uiox_boot_media_sdmmc(void) { return &sd_ops; }
