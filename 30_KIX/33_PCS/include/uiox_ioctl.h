/*
 * 30_KIX/33_PCS/include/uiox_ioctl.h
 *
 * The ioctl command space — SoC / FwHal / driver data that is not a file.
 *
 *     SYS_IOCTL = 54            the syscall number        (uix_sys.h)
 *     UIOX_IOC_GET_TEMP         the command within ioctl  (this file)
 *
 * ── the encoding, which is Linux's ────────────────────────────────────
 *   [31:30]  direction   00 none  01 write  10 read  11 read+write
 *   [29:16]  size of the argument struct
 *   [15:8]   magic type byte
 *   [7:0]    command number
 *
 * @version 1.0.0  @date 2026-10-03
 */
#ifndef UIOX_IOCTL_H
#define UIOX_IOCTL_H

#include "uiox_klibc.h"

#define UIOX_IOC_NONE   0u
#define UIOX_IOC_WRITE  1u
#define UIOX_IOC_READ   2u
#define UIOX_IOC_RW     3u

#define UIOX_IOC(dir, type, nr, size)                       \
    ((((uint32_t)(dir)  & 3u)  << 30) |                     \
     ((((uint32_t)(size) & 0x3FFFu)) << 16) |               \
     ((((uint32_t)(type) & 0xFFu))   <<  8) |               \
     ( (uint32_t)(nr)   & 0xFFu))

#define UIOX_IOR(type, nr, stype) \
    UIOX_IOC(UIOX_IOC_READ,  (type), (nr), sizeof(stype))
#define UIOX_IOW(type, nr, stype) \
    UIOX_IOC(UIOX_IOC_WRITE, (type), (nr), sizeof(stype))
#define UIOX_IOWR(type, nr, stype) \
    UIOX_IOC(UIOX_IOC_RW,    (type), (nr), sizeof(stype))

/* ── SoC information ─────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_SOC    'S'

typedef struct {
    char     name[32];
    uint32_t cpu_hz;
    uint32_t arch_bits;
    uint32_t num_cores;
    uint32_t dram_mb;
    uint32_t _pad;
} uiox_soc_info_t;

#define UIOX_IOC_GET_SOC_INFO  UIOX_IOR(UIOX_IOC_MAGIC_SOC, 1, uiox_soc_info_t)

/* ── Clock ───────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_CLK    'C'

typedef struct {
    uint32_t clk_id;
    uint32_t hz;
    uint8_t  enabled;
    uint8_t  _pad[3];
} uiox_clk_info_t;

#define UIOX_IOC_GET_CLK_HZ    UIOX_IOR (UIOX_IOC_MAGIC_CLK, 1, uiox_clk_info_t)
#define UIOX_IOC_CLK_ENABLE    UIOX_IOW (UIOX_IOC_MAGIC_CLK, 2, uint32_t)
#define UIOX_IOC_CLK_DISABLE   UIOX_IOW (UIOX_IOC_MAGIC_CLK, 3, uint32_t)

/* ── Power / PSCI ────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_PWR    'P'

typedef struct {
    uint32_t state;
    uint32_t cpu_id;
} uiox_pwr_cmd_t;

#define UIOX_IOC_GET_PWR_STATE UIOX_IOR (UIOX_IOC_MAGIC_PWR, 1, uiox_pwr_cmd_t)
#define UIOX_IOC_CPU_ON        UIOX_IOW (UIOX_IOC_MAGIC_PWR, 2, uiox_pwr_cmd_t)
#define UIOX_IOC_CPU_OFF       UIOX_IOW (UIOX_IOC_MAGIC_PWR, 3, uiox_pwr_cmd_t)
#define UIOX_IOC_SYSTEM_RESET  UIOX_IOC (UIOX_IOC_NONE, UIOX_IOC_MAGIC_PWR, 4, 0)
#define UIOX_IOC_SYSTEM_OFF    UIOX_IOC (UIOX_IOC_NONE, UIOX_IOC_MAGIC_PWR, 5, 0)

/* ── Thermal ─────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_THERM  'T'

typedef struct {
    uint8_t  zone;
    uint8_t  _pad;
    int16_t  temp_dc;
} uiox_therm_data_t;

#define UIOX_IOC_GET_TEMP      UIOX_IOWR(UIOX_IOC_MAGIC_THERM, 1, uiox_therm_data_t)

/* ── BMS (battery) ───────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_BMS    'B'

typedef struct {
    uint16_t voltage_mv;
    int16_t  current_ma;
    uint8_t  soc_pct;
    int8_t   temp_dc;
    uint8_t  _pad[2];
} uiox_bms_data_t;

#define UIOX_IOC_GET_BMS_DATA  UIOX_IOR (UIOX_IOC_MAGIC_BMS, 1, uiox_bms_data_t)

/* ── WiFi ────────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_WIFI   'W'

typedef struct {
    uint8_t  mac[6];
    uint8_t  _pad0[2];
    uint8_t  ssid[33];
    int8_t   rssi_dbm;
    uint8_t  channel;
    uint8_t  associated;
    uint8_t  _pad1[3];
} uiox_wifi_status_t;

#define UIOX_IOC_GET_WIFI_STATUS UIOX_IOR(UIOX_IOC_MAGIC_WIFI, 1, uiox_wifi_status_t)

/* ── Camera ──────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_CAM    'K'

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
    uint64_t size;
    uint64_t _pad;
} uiox_cam_frame_info_t;

/* ── NO paddr FIELD, deliberately ──────────────────────────────────────
 * An earlier revision carried `uint64_t paddr` here and the documentation
 * told userspace to pass it back as mmap's OFFSET argument.  That is a
 * physical address handed to unprivileged code and then trusted on
 * return.  The mapping is reachable through the fd instead. */
#define UIOX_IOC_CAM_GET_FRAME  UIOX_IOR(UIOX_IOC_MAGIC_CAM, 1, uiox_cam_frame_info_t)
#define UIOX_IOC_CAM_RELEASE    UIOX_IOW(UIOX_IOC_MAGIC_CAM, 2, uint64_t)

/* ── GPU buffer ──────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_GPU    'G'

typedef struct {
    uint32_t buf_type;
    uint32_t size;
    uint64_t gpuva;
    uint64_t _pad;
} uiox_gpu_buf_info_t;

#define UIOX_IOC_GPU_ALLOC_BUF UIOX_IOWR(UIOX_IOC_MAGIC_GPU, 1, uiox_gpu_buf_info_t)
#define UIOX_IOC_GPU_FREE_BUF  UIOX_IOW (UIOX_IOC_MAGIC_GPU, 2, uint64_t)

/* ── DMA ─────────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_DMA    'D'

typedef struct {
    uint32_t channel;
    uint8_t  status;
    uint8_t  _pad[3];
    uint32_t len;
    uint32_t _pad2;
    uint64_t src_pa;
    uint64_t dst_pa;
} uiox_dma_xfer_t;

#define UIOX_IOC_DMA_TRANSFER  UIOX_IOWR(UIOX_IOC_MAGIC_DMA, 1, uiox_dma_xfer_t)
#define UIOX_IOC_DMA_STATUS    UIOX_IOWR(UIOX_IOC_MAGIC_DMA, 2, uiox_dma_xfer_t)

/* ── Live patch (33_PCS/06_kpatch) ───────────────────────────────────── */
#define UIOX_IOC_MAGIC_KP     'X'

typedef struct {
    uint32_t patch_id;
    uint32_t state;
    uint64_t target_va;
    char     name[32];
} uiox_kpatch_status_t;

#define UIOX_IOC_KP_STATUS     UIOX_IOWR(UIOX_IOC_MAGIC_KP, 1, uiox_kpatch_status_t)
#define UIOX_IOC_KP_APPLY      UIOX_IOW (UIOX_IOC_MAGIC_KP, 2, uint32_t)
#define UIOX_IOC_KP_REVERT     UIOX_IOW (UIOX_IOC_MAGIC_KP, 3, uint32_t)

/* ── The entry point.  fd is FIRST and is not optional: it is what makes
 * the command a request against a device the caller opened, rather than
 * against whatever magic byte the command happens to carry. ─────────── */
long uiox_ioctl_soc_dispatch(int fd, unsigned long cmd, unsigned long uarg);

#endif /* UIOX_IOCTL_H */
