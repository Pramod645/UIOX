/**
 * @file  uiox_nvme_if.h
 * @brief UIOX NVMe interface driver — queues, identify, BAR0.
 * @date  2026-10-19
 */
#ifndef UIOX_NVME_IF_H
#define UIOX_NVME_IF_H

#include "uiox_nvme_hw.h"
#include "uiox_klibc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t blocks_read;
    uint64_t blocks_written;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint32_t cmds_submitted;
    uint32_t timeout_errors;
    uint32_t irq_count;
    uint32_t flush_count;
    uint32_t errors;
} uiox_nvme_if_stats_t;

typedef struct {
    uiox_nvme_hw_t      *hw;
    uiox_nvme_if_stats_t stats;
    bool                 primed;
} uiox_nvme_if_t;

int  uiox_nvme_if_config      (uiox_nvme_if_t *nif, uiox_nvme_hw_t *hw);
int  uiox_nvme_if_start       (uiox_nvme_if_t *nif);
void uiox_nvme_if_stop        (uiox_nvme_if_t *nif);

/* Block transfers — lba and count are 512-byte sectors */
int  uiox_nvme_if_read        (uiox_nvme_if_t *nif, uint64_t lba,
                               uint8_t *buf, uint32_t count);
int  uiox_nvme_if_write       (uiox_nvme_if_t *nif, uint64_t lba,
                               const uint8_t *buf, uint32_t count);

/* Identify — refreshes hw->ident and hw->nsid */
int  uiox_nvme_if_identify    (uiox_nvme_if_t *nif);
int  uiox_nvme_if_flush       (uiox_nvme_if_t *nif);

uiox_nvme_evt_t *uiox_nvme_if_irq_handle(uiox_nvme_if_t *nif,
                                          uint32_t now_ms);
void uiox_nvme_if_stats_get   (const uiox_nvme_if_t *nif,
                               uiox_nvme_if_stats_t *out);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_NVME_IF_H */
