/**
 * @file  uiox_sdmmc_hw.h
 * @brief UIOX SD/SDHC/SDXC Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - SDSC / SDHC / SDXC, 1-bit and 4-bit SD bus
 *   - Removable medium — presence and write-protect are runtime state
 *
 * Owns:
 *   - SD Host Controller MMIO register access (SD Host Spec v3 — the
 *     same map uiox_emmc_hw.h describes, since eMMC rides an SDHCI host)
 *   - Card detect and write-protect GPIO status
 *   - CMD0/CMD8/CMD17/CMD24/ACMD41 framing
 *   - Clock: ident (400 kHz) → HS (50 MHz) → SDR50/SDR104
 *
 * @version 1.0.0  @date 2026-10-20
 */
#ifndef UIOX_SDMMC_HW_H
#define UIOX_SDMMC_HW_H

#include "uiox_klibc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * SD bus speed mode
 * ====================================================================== */
typedef enum {
    UIOX_SDMMC_SPEED_IDENT = 0,   /**< 400 kHz identification           */
    UIOX_SDMMC_SPEED_DS,          /**< 25 MHz default speed             */
    UIOX_SDMMC_SPEED_HS,          /**< 50 MHz high speed                */
    UIOX_SDMMC_SPEED_SDR50,       /**< 100 MHz UHS-I                    */
    UIOX_SDMMC_SPEED_SDR104,      /**< 208 MHz UHS-I                    */
    UIOX_SDMMC_SPEED_DDR50,       /**< 50 MHz DDR                       */
} uiox_sdmmc_hw_speed_t;

/* Card generation — decides the addressing mode (byte vs block) */
typedef enum {
    UIOX_SDMMC_CARD_NONE = 0,
    UIOX_SDMMC_CARD_SDSC,         /**< standard capacity, byte-addressed */
    UIOX_SDMMC_CARD_SDHC,         /**< high capacity, block-addressed    */
    UIOX_SDMMC_CARD_SDXC,         /**< extended capacity                 */
} uiox_sdmmc_card_t;

/* =========================================================================
 * Hardware capability flags
 * ====================================================================== */
#define UIOX_SDMMC_CAP_4BIT        (1u << 0)  /**< 4-bit data bus        */
#define UIOX_SDMMC_CAP_SDR50       (1u << 1)  /**< UHS-I SDR50           */
#define UIOX_SDMMC_CAP_SDR104      (1u << 2)  /**< UHS-I SDR104          */
#define UIOX_SDMMC_CAP_DDR50       (1u << 3)  /**< UHS-I DDR50           */
#define UIOX_SDMMC_CAP_DMA         (1u << 4)  /**< ADMA2/SDMA            */
#define UIOX_SDMMC_CAP_CARD_DET    (1u << 5)  /**< Card-detect line      */
#define UIOX_SDMMC_CAP_WP_SWITCH   (1u << 6)  /**< Physical write-protect*/

/* =========================================================================
 * SD Host Controller registers (SD Host Spec v3)
 * Same map uiox_emmc_hw.h declares — eMMC and SD share the host, so the
 * offsets are deliberately identical.  Declared here too, per the rule
 * uiox_devops.h states: 10_DI names its own register map rather than
 * including another class's header.
 * ====================================================================== */
#define SDMMC_HC_DMA_ADDR        0x0000u
#define SDMMC_HC_BLK_SIZE        0x0004u
#define SDMMC_HC_BLK_COUNT       0x0006u
#define SDMMC_HC_ARG             0x0008u
#define SDMMC_HC_TRANS_MODE      0x000Cu
#define SDMMC_HC_CMD             0x000Eu
#define SDMMC_HC_RESP0           0x0010u
#define SDMMC_HC_RESP1           0x0014u
#define SDMMC_HC_RESP2           0x0018u
#define SDMMC_HC_RESP3           0x001Cu
#define SDMMC_HC_DATA            0x0020u
#define SDMMC_HC_PRESENT_STATE   0x0024u
#define SDMMC_HC_HOST_CTRL1      0x0028u
#define SDMMC_HC_PWR_CTRL        0x0029u
#define SDMMC_HC_CLK_CTRL        0x002Cu
#define SDMMC_HC_TIMEOUT_CTRL    0x002Eu
#define SDMMC_HC_SW_RESET        0x002Fu
#define SDMMC_HC_INT_STATUS      0x0030u
#define SDMMC_HC_INT_STATUS_EN   0x0034u
#define SDMMC_HC_INT_SIGNAL_EN   0x0038u
#define SDMMC_HC_HOST_CTRL2      0x003Eu
#define SDMMC_HC_CAPS0           0x0040u
#define SDMMC_HC_ADMA_ADDR_LO    0x0058u

/* PRESENT_STATE bits — the two that make SD different from eMMC: a
 * soldered card is always inserted and never write-protected, so these
 * are constants there and runtime state here. */
#define SDMMC_PS_CMD_INHIBIT     (1u << 0)
#define SDMMC_PS_DAT_INHIBIT     (1u << 1)
#define SDMMC_PS_CARD_INSERTED   (1u << 16)
#define SDMMC_PS_WRITE_PROTECT   (1u << 19)

/* INT_STATUS bits */
#define SDMMC_INT_CMD_COMPLETE   (1u << 0)
#define SDMMC_INT_XFER_COMPLETE  (1u << 1)
#define SDMMC_INT_CMD_TIMEOUT    (1u << 16)
#define SDMMC_INT_CMD_CRC_ERR    (1u << 17)

/* HOST_CTRL1 / HOST_CTRL2 */
#define SDMMC_HC1_4BIT           (1u << 1)
#define SDMMC_HC1_HIGH_SPEED     (1u << 2)
#define SDMMC_HC2_SDR50          (0x02u)
#define SDMMC_HC2_SDR104         (0x03u)
#define SDMMC_HC2_DDR50          (0x04u)
#define SDMMC_HC2_1V8_EN         (1u << 3)

/* =========================================================================
 * SD command numbers
 * ====================================================================== */
#define SDMMC_CMD0_GO_IDLE       0u
#define SDMMC_CMD2_ALL_SEND_CID  2u
#define SDMMC_CMD3_SEND_RCA      3u
#define SDMMC_CMD7_SELECT_CARD   7u
#define SDMMC_CMD8_IF_COND       8u
#define SDMMC_CMD9_SEND_CSD      9u
#define SDMMC_CMD17_READ         17u
#define SDMMC_CMD24_WRITE        24u
#define SDMMC_CMD55_APP          55u
#define SDMMC_ACMD41_OP_COND     41u
#define SDMMC_ACMD41_HCS         (1u << 30)

/* =========================================================================
 * Pending IRQ bits
 * ====================================================================== */
#define UIOX_SDMMC_IRQ_CARD_INS  (1u << 0)
#define UIOX_SDMMC_IRQ_CARD_REM  (1u << 1)
#define UIOX_SDMMC_IRQ_XFER_DONE (1u << 2)
#define UIOX_SDMMC_IRQ_ERROR     (1u << 3)

/* =========================================================================
 * Device identity — what card_init() fills
 * ====================================================================== */
#define UIOX_SDMMC_MODEL_LEN     48u
#define UIOX_SDMMC_BLOCK_SIZE    512u

typedef struct {
    char      model[UIOX_SDMMC_MODEL_LEN];  /**< CID product name        */
    uiox_sdmmc_card_t card;
    uint16_t  rca;                          /**< Relative card address   */
    uint64_t  capacity_sectors;             /**< 512-byte units          */
    uint64_t  capacity_bytes;
    bool      card_present;
    bool      write_protect;
} uiox_sdmmc_ident_t;

/* =========================================================================
 * Hardware descriptor
 * ====================================================================== */
typedef struct {
    uintptr_t            base;    /**< SDHCI MMIO base — SDIO_HC_BASE_ARM64 */
    uint32_t             irq;
    uint32_t             caps;
    char                 model[UIOX_SDMMC_MODEL_LEN];
    /* Bus state */
    uiox_sdmmc_hw_speed_t speed;
    uint8_t              bus_width;   /**< 1 or 4 */
    uint32_t             clk_hz;
    /* Device info */
    uiox_sdmmc_ident_t   ident;
    bool                 dev_ready;
    /* Removable-medium state */
    bool                 card_present;
    bool                 write_protect;
    /* Pending IRQ */
    volatile uint32_t    pending_irq;
    /* Private (ops vtable) */
    const void          *ops;
    void                *drv_priv;
} uiox_sdmmc_hw_t;

/* =========================================================================
 * Hardware operations vtable (13-op table)
 *
 * send_cmd and the CRC helpers are kept because SD framing is still MMC
 * framing — unlike NVMe, which has no CMD line.  tuning and the eMMC
 * EXT_CSD slots are absent: SD has no EXT_CSD and no HS400 eye training.
 * detect() and card_init() are the two slots eMMC does not need, because
 * eMMC's card is soldered rather than seated.
 * ====================================================================== */
typedef struct {
    int  (*init)          (uiox_sdmmc_hw_t *hw);
    void (*deinit)        (uiox_sdmmc_hw_t *hw);
    int  (*power_on)      (uiox_sdmmc_hw_t *hw);
    void (*power_off)     (uiox_sdmmc_hw_t *hw);

    uint32_t (*reg_read)  (uiox_sdmmc_hw_t *hw, uint32_t offset);
    void (*reg_write)     (uiox_sdmmc_hw_t *hw,
                           uint32_t offset, uint32_t val);

    int  (*set_clock)     (uiox_sdmmc_hw_t *hw, uint32_t hz);
    int  (*set_bus_width) (uiox_sdmmc_hw_t *hw, uint8_t width);
    int  (*set_speed_mode)(uiox_sdmmc_hw_t *hw, uiox_sdmmc_hw_speed_t speed);

    /* Removable medium: is a card seated, and is it write-protected? */
    int  (*detect)        (uiox_sdmmc_hw_t *hw);

    /* The init sequence: CMD0 → CMD8 → ACMD41 → CMD2 → CMD3 → CMD7,
     * ending with rca and capacity filled into hw->ident. */
    int  (*card_init)     (uiox_sdmmc_hw_t *hw);

    int  (*send_cmd)      (uiox_sdmmc_hw_t *hw, uint8_t cmd,
                           uint32_t arg, uint32_t *resp);

    int  (*read_blocks)   (uiox_sdmmc_hw_t *hw, uint32_t lba,
                           uint8_t *buf, uint32_t count);
    int  (*write_blocks)  (uiox_sdmmc_hw_t *hw, uint32_t lba,
                           const uint8_t *buf, uint32_t count);

    void (*isr)           (uiox_sdmmc_hw_t *hw);
} uiox_sdmmc_hw_ops_t;

/* =========================================================================
 * HAL public API
 * ====================================================================== */
int      uiox_sdmmc_hw_init         (uiox_sdmmc_hw_t *hw,
                                     const uiox_sdmmc_hw_ops_t *ops);
void     uiox_sdmmc_hw_deinit       (uiox_sdmmc_hw_t *hw);
int      uiox_sdmmc_hw_power_on     (uiox_sdmmc_hw_t *hw);
void     uiox_sdmmc_hw_power_off    (uiox_sdmmc_hw_t *hw);
uint32_t uiox_sdmmc_hw_reg_read     (uiox_sdmmc_hw_t *hw, uint32_t offset);
void     uiox_sdmmc_hw_reg_write    (uiox_sdmmc_hw_t *hw,
                                     uint32_t offset, uint32_t val);
int      uiox_sdmmc_hw_set_clock    (uiox_sdmmc_hw_t *hw, uint32_t hz);
int      uiox_sdmmc_hw_set_bus_width(uiox_sdmmc_hw_t *hw, uint8_t width);
int      uiox_sdmmc_hw_set_speed    (uiox_sdmmc_hw_t *hw,
                                     uiox_sdmmc_hw_speed_t speed);
int      uiox_sdmmc_hw_detect       (uiox_sdmmc_hw_t *hw);
int      uiox_sdmmc_hw_card_init    (uiox_sdmmc_hw_t *hw);
int      uiox_sdmmc_hw_send_cmd     (uiox_sdmmc_hw_t *hw, uint8_t cmd,
                                     uint32_t arg, uint32_t *resp);
int      uiox_sdmmc_hw_read_blocks  (uiox_sdmmc_hw_t *hw, uint32_t lba,
                                     uint8_t *buf, uint32_t count);
int      uiox_sdmmc_hw_write_blocks (uiox_sdmmc_hw_t *hw, uint32_t lba,
                                     const uint8_t *buf, uint32_t count);
void     uiox_sdmmc_hw_isr          (uiox_sdmmc_hw_t *hw);

static inline uint32_t uiox_sdmmc_caps(const uiox_sdmmc_hw_t *hw)
{ return hw ? hw->caps : 0u; }

/* =========================================================================
 * Firmware binding (02_FwHal / libuiox_fw_<arch>.a)
 *
 * Declared here, defined in uiox_sdmmc_fwops.c.  The i2c dev type is
 * forward-declared so this header does not pull FwHal headers into every
 * translation unit that includes it.
 *
 * Chips with a firmware entry: sdhost.
 * ====================================================================== */
struct uiox_i2c_dev;

int uiox_sdmmc_fwbind(uiox_sdmmc_hw_t *hw, struct uiox_i2c_dev *i2c);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SDMMC_HW_H */
