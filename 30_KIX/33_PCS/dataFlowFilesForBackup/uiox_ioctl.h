/*
 * 30_KIX/33_PCS/include/uiox_ioctl.h
 *
 * UIOX ioctl command definitions.
 * These are the userspace-visible API for reading
 * SoC / FwHal / driver data from kernel space.
 *
 * @version 1.0.0
 * @date    2026-07-28
 */

 #ifndef UIOX_IOCTL_H
 #define UIOX_IOCTL_H
 
 #include "uiox_soc.h"
 
 /* ── ioctl command encoding ────────────────────────────────────────── */
 /* [31:30] direction  00=none 01=write 10=read 11=read+write          */
 /* [29:16] size of arg struct                                          */
 /* [15:8]  magic type byte                                             */
 /* [7:0]   command number                                              */
 #define UIOX_IOC_NONE   0u
 #define UIOX_IOC_WRITE  1u
 #define UIOX_IOC_READ   2u
 #define UIOX_IOC_RW     3u
 
 #define UIOX_IOC(dir, type, nr, size) \
     (((dir)  << 30) | \
      (((size) & 0x3FFFu) << 16) | \
      ((type) << 8)  | \
      (nr))
 
 #define UIOX_IOR(type, nr, stype) \
     UIOX_IOC(UIOX_IOC_READ,  type, nr, sizeof(stype))
 
 #define UIOX_IOW(type, nr, stype) \
     UIOX_IOC(UIOX_IOC_WRITE, type, nr, sizeof(stype))
 
 #define UIOX_IOWR(type, nr, stype) \
     UIOX_IOC(UIOX_IOC_RW,    type, nr, sizeof(stype))

/* ── SoC info ──────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_SOC    'S'

typedef struct {
    char     name[32];       /* SoC name string                        */
    uint32_t cpu_hz;         /* CPU clock Hz from uiox_clk_get_hz()    */
    uint32_t arch_bits;      /* 32 or 64                               */
    uint32_t num_cores;      /* core count from MPIDR/MPIDR_EL1        */
    uint32_t dram_mb;        /* total DRAM MB from uiox_soc_mem_init() */
} uiox_soc_info_t;

#define UIOX_IOC_GET_SOC_INFO   UIOX_IOR(UIOX_IOC_MAGIC_SOC, 1, uiox_soc_info_t)

/* ── Clock ─────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_CLK    'C'

typedef struct {
    uint32_t clk_id;         /* uiox_clk_id_t from uiox_soc_clk.h     */
    uint32_t hz;             /* current frequency                       */
    uint8_t  enabled;        /* 1 = on, 0 = gated                      */
    uint8_t  _pad[3];
} uiox_clk_info_t;

#define UIOX_IOC_GET_CLK_HZ     UIOX_IOR (UIOX_IOC_MAGIC_CLK, 1, uiox_clk_info_t)
#define UIOX_IOC_CLK_ENABLE     UIOX_IOW (UIOX_IOC_MAGIC_CLK, 2, uint32_t)
#define UIOX_IOC_CLK_DISABLE    UIOX_IOW (UIOX_IOC_MAGIC_CLK, 3, uint32_t)

/* ── Power / PSCI ──────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_PWR    'P'

typedef struct {
    uint32_t state;          /* uiox_fw_pwr_state_t                    */
    uint32_t cpu_id;         /* MPIDR affinity for CPU_ON/OFF           */
} uiox_pwr_cmd_t;

#define UIOX_IOC_GET_PWR_STATE  UIOX_IOR (UIOX_IOC_MAGIC_PWR, 1, uiox_pwr_cmd_t)
#define UIOX_IOC_CPU_ON         UIOX_IOW (UIOX_IOC_MAGIC_PWR, 2, uiox_pwr_cmd_t)
#define UIOX_IOC_CPU_OFF        UIOX_IOW (UIOX_IOC_MAGIC_PWR, 3, uiox_pwr_cmd_t)
#define UIOX_IOC_SYSTEM_RESET   UIOX_IOC (UIOX_IOC_NONE, UIOX_IOC_MAGIC_PWR, 4, 0)
#define UIOX_IOC_SYSTEM_OFF     UIOX_IOC (UIOX_IOC_NONE, UIOX_IOC_MAGIC_PWR, 5, 0)

/* ── Thermal sensor ────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_THERM  'T'

typedef struct {
    uint8_t  zone;           /* thermal zone index                      */
    int16_t  temp_dc;        /* temperature in °C × 10                  */
    uint8_t  _pad;
} uiox_therm_data_t;

#define UIOX_IOC_GET_TEMP       UIOX_IOWR(UIOX_IOC_MAGIC_THERM, 1, uiox_therm_data_t)

/* ── BMS (battery) ─────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_BMS    'B'

typedef struct {
    uint16_t voltage_mv;     /* battery voltage in mV                   */
    int16_t  current_ma;     /* charge/discharge current in mA          */
    uint8_t  soc_pct;        /* state of charge 0–100 %                 */
    int8_t   temp_dc;        /* temperature °C × 10                     */
    uint8_t  _pad[2];
} uiox_bms_data_t;

#define UIOX_IOC_GET_BMS_DATA   UIOX_IOR (UIOX_IOC_MAGIC_BMS, 1, uiox_bms_data_t)

/* ── WiFi ──────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_WIFI   'W'

typedef struct {
    uint8_t  mac[6];         /* MAC address                             */
    uint8_t  ssid[33];       /* current SSID (NUL-terminated)           */
    int8_t   rssi_dbm;       /* signal strength dBm                     */
    uint8_t  channel;        /* 802.11 channel                          */
    uint8_t  associated;     /* 1 = associated, 0 = not                 */
    uint8_t  _pad[2];
} uiox_wifi_status_t;

#define UIOX_IOC_GET_WIFI_STATUS UIOX_IOR(UIOX_IOC_MAGIC_WIFI, 1, uiox_wifi_status_t)
#define UIOX_IOC_WIFI_SCAN       UIOX_IOR(UIOX_IOC_MAGIC_WIFI, 2, uiox_wifi_status_t)

/* ── Camera ────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_CAM    'K'

typedef struct {
    uint32_t width;          /* frame width  in pixels                  */
    uint32_t height;         /* frame height in pixels                  */
    uint32_t stride;         /* row stride in bytes                     */
    uint32_t format;         /* pixel format (YUYV, NV12, RGB24 …)      */
    uint64_t paddr;          /* physical address of frame buffer        */
    uint64_t size;           /* frame buffer size in bytes              */
} uiox_cam_frame_info_t;

/* user calls mmap() after this to map the buffer zero-copy */
#define UIOX_IOC_CAM_GET_FRAME  UIOX_IOR(UIOX_IOC_MAGIC_CAM, 1, uiox_cam_frame_info_t)
#define UIOX_IOC_CAM_RELEASE    UIOX_IOW(UIOX_IOC_MAGIC_CAM, 2, uint64_t)

/* ── GPU buffer ────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_GPU    'G'

typedef struct {
    uint32_t buf_type;       /* VBO/IBO/UBO/CMD/TEX                     */
    uint32_t size;           /* buffer size in bytes                    */
    uint64_t paddr;          /* physical address — for mmap()           */
    uint64_t gpuva;          /* GPU virtual address                     */
} uiox_gpu_buf_info_t;

#define UIOX_IOC_GPU_ALLOC_BUF  UIOX_IOWR(UIOX_IOC_MAGIC_GPU, 1, uiox_gpu_buf_info_t)
#define UIOX_IOC_GPU_FREE_BUF   UIOX_IOW (UIOX_IOC_MAGIC_GPU, 2, uint64_t)

/* ── DMA ───────────────────────────────────────────────────────────── */
#define UIOX_IOC_MAGIC_DMA    'D'

typedef struct {
    uint32_t channel;        /* DMA channel 0–7                         */
    uint64_t src_pa;         /* source physical address                 */
    uint64_t dst_pa;         /* destination physical address            */
    uint32_t len;            /* transfer length in bytes                */
    uint8_t  status;         /* 0=idle 1=running 2=done 3=error         */
    uint8_t  _pad[3];
} uiox_dma_xfer_t;

#define UIOX_IOC_DMA_TRANSFER   UIOX_IOWR(UIOX_IOC_MAGIC_DMA, 1, uiox_dma_xfer_t)
#define UIOX_IOC_DMA_STATUS     UIOX_IOWR(UIOX_IOC_MAGIC_DMA, 2, uiox_dma_xfer_t)

/* ── Live patch status (33_PCS/06_kpatch) ──────────────────────────── */
#define UIOX_IOC_MAGIC_KP     'X'

typedef struct {
    uint32_t patch_id;
    uint32_t state;          /* 0=inactive 1=applied 2=reverted         */
    uint64_t target_va;      /* patched function VA                     */
    char     name[32];
} uiox_kpatch_status_t;

#define UIOX_IOC_KP_STATUS      UIOX_IOWR(UIOX_IOC_MAGIC_KP, 1, uiox_kpatch_status_t)
#define UIOX_IOC_KP_APPLY       UIOX_IOW (UIOX_IOC_MAGIC_KP, 2, uint32_t)
#define UIOX_IOC_KP_REVERT      UIOX_IOW (UIOX_IOC_MAGIC_KP, 3, uint32_t)

#endif /* UIOX_IOCTL_H */
