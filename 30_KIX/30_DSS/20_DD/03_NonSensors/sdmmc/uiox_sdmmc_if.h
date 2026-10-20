/**
 * @file  uiox_sdmmc_if.h
 * @brief UIOX SD interface driver — CMD/DAT, card detect, bus config.
 * @date  2026-10-20
 */
#ifndef UIOX_SDMMC_IF_H
#define UIOX_SDMMC_IF_H

#include "uiox_sdmmc_hw.h"
#include "uiox_klibc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t blocks_read;
    uint64_t blocks_written;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint32_t cmds_sent;
    uint32_t crc_errors;
    uint32_t timeout_errors;
    uint32_t insert_events;
    uint32_t remove_events;
    uint32_t irq_count;
    uint32_t errors;
} uiox_sdmmc_if_stats_t;

typedef struct {
    uiox_sdmmc_hw_t      *hw;
    uiox_sdmmc_if_stats_t stats;
    bool                  primed;
    uint8_t               bus_width;
    uiox_sdmmc_hw_speed_t speed;
} uiox_sdmmc_if_t;

int  uiox_sdmmc_if_config      (uiox_sdmmc_if_t *sif, uiox_sdmmc_hw_t *hw);
int  uiox_sdmmc_if_start       (uiox_sdmmc_if_t *sif);
void uiox_sdmmc_if_stop        (uiox_sdmmc_if_t *sif);

/* Medium — a card may arrive or leave at any time */
int  uiox_sdmmc_if_detect      (uiox_sdmmc_if_t *sif);
int  uiox_sdmmc_if_card_init   (uiox_sdmmc_if_t *sif);

/* CMD framing and bus reconfiguration */
int  uiox_sdmmc_if_send_cmd    (uiox_sdmmc_if_t *sif, uint8_t cmd,
                                uint32_t arg, uint32_t *resp);
int  uiox_sdmmc_if_set_clock   (uiox_sdmmc_if_t *sif, uint32_t hz);
int  uiox_sdmmc_if_set_bus_width(uiox_sdmmc_if_t *sif, uint8_t width);
int  uiox_sdmmc_if_set_speed   (uiox_sdmmc_if_t *sif,
                                uiox_sdmmc_hw_speed_t speed);

/* Block transfers — 512-byte sectors */
int  uiox_sdmmc_if_read        (uiox_sdmmc_if_t *sif, uint32_t lba,
                                uint8_t *buf, uint32_t count);
int  uiox_sdmmc_if_write       (uiox_sdmmc_if_t *sif, uint32_t lba,
                                const uint8_t *buf, uint32_t count);

uiox_sdmmc_evt_t *uiox_sdmmc_if_irq_handle(uiox_sdmmc_if_t *sif,
                                            uint32_t now_ms);
void uiox_sdmmc_if_stats_get   (const uiox_sdmmc_if_t *sif,
                                uiox_sdmmc_if_stats_t *out);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SDMMC_IF_H */
