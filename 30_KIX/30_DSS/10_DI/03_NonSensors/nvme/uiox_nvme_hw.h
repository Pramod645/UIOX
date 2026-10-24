/**
 * @file  uiox_nvme_hw.h
 * @brief UIOX NVMe Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - NVMe 1.4 over PCIe (M.2), BAR0 MMIO
 *   - Admin + I/O queue pairs, doorbell submission
 *   - Namespaces (nsid), LBA-addressed 512-byte sectors
 *
 * Owns:
 *   - BAR0 register access (CAP, CC, CSTS, AQA, ASQ/ACQ)
 *   - Controller enable/disable handshake (CC.EN ↔ CSTS.RDY)
 *   - Queue submission and completion
 *   - Identify (controller + namespace) → model/serial/fw_rev/nsid
 *
 * Deliberately absent, unlike uiox_emmc_hw_t: no CMD line, no CRC7/16,
 * no bus-width or clock programming, no tuning.  PCIe owns the link, so
 * those have no meaning here — the register map lives in
 * 02_FwHal/include/uiox_fw_nvme.h and the controller's BAR is assigned
 * by uiox_fw_pcie_assign_bars() before this layer sees it.
 *
 * @version 1.0.0  @date 2026-10-19
 */
#ifndef UIOX_NVME_HW_H
#define UIOX_NVME_HW_H

#include "uiox_klibc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Register offsets — BAR0 MMIO
 * Mirrors 02_FwHal/include/uiox_fw_nvme.h; declared here so 10_DI does
 * not include a FwHal header (the rule uiox_devops.h states).
 * ====================================================================== */
#define UIOX_NVME_REG_CAP        0x0000u
#define UIOX_NVME_REG_CC         0x0014u
#define UIOX_NVME_REG_CSTS       0x001Cu
#define UIOX_NVME_REG_AQA        0x0024u
#define UIOX_NVME_REG_ASQ_LO     0x0028u
#define UIOX_NVME_REG_ACQ_LO     0x0030u

#define UIOX_NVME_CC_EN          (1u << 0)
#define UIOX_NVME_CSTS_RDY       (1u << 0)
#define UIOX_NVME_CSTS_CFS       (1u << 1)

/* Opcodes — admin + I/O */
#define UIOX_NVME_ADMIN_IDENTIFY 0x06u
#define UIOX_NVME_IO_READ        0x02u
#define UIOX_NVME_IO_WRITE       0x01u
#define UIOX_NVME_IO_FLUSH       0x00u
#define UIOX_NVME_IO_DSM         0x09u

#define UIOX_NVME_BLOCK_SIZE     512u
#define UIOX_NVME_MODEL_LEN      41u   /* incl NUL — the FwHal field width */
#define UIOX_NVME_SERIAL_LEN     21u
#define UIOX_NVME_FWREV_LEN      9u

/* =========================================================================
 * Hardware capability flags
 * ====================================================================== */
#define UIOX_NVME_CAP_VWC        (1u << 0)  /**< Volatile write cache      */
#define UIOX_NVME_CAP_TRIM       (1u << 1)  /**< Dataset Management (DSM)  */
#define UIOX_NVME_CAP_FLUSH      (1u << 2)  /**< Explicit flush command    */
#define UIOX_NVME_CAP_MULTI_NS   (1u << 3)  /**< More than one namespace   */
#define UIOX_NVME_CAP_MSIX       (1u << 4)  /**< MSI-X interrupt vector(s) */

/* =========================================================================
 * Pending IRQ bits
 * ====================================================================== */
#define UIOX_NVME_IRQ_CMD_DONE   (1u << 0)
#define UIOX_NVME_IRQ_IO_DONE    (1u << 1)
#define UIOX_NVME_IRQ_ERROR      (1u << 2)

/* =========================================================================
 * Interrupt event — what the ISR reports when a queue completes
 *
 * uiox_nvme_if_irq_handle() returns one of these (declared in
 * uiox_nvme_if.h:48).  The three bits above are its `pending` field;
 * the HAL's own isr() signature is void(uiox_nvme_hw_t *), so the
 * event is assembled by the driver from the HAL's pending bits plus
 * the completion queue entry, not returned by the HAL.
 *
 * Same shape as the other classes' event records: what happened, and
 * which queue/command it happened to.
 * ====================================================================== */
typedef struct {
    uint32_t pending;     /* UIOX_NVME_IRQ_CMD_DONE / _IO_DONE / _ERROR */
    uint16_t cq_id;       /* completion queue the entry came from        */
    uint16_t status;      /* NVMe status field from the CQ entry         */
    uint32_t cid;         /* command identifier                          */
} uiox_nvme_evt_t;

/* =========================================================================
 * Device identity — what identify() fills
 * ====================================================================== */
typedef struct {
    char      model[UIOX_NVME_MODEL_LEN];
    char      serial[UIOX_NVME_SERIAL_LEN];
    char      fw_rev[UIOX_NVME_FWREV_LEN];
    uint32_t  nsid;              /**< Active namespace                 */
    uint64_t  capacity_lba;      /**< Namespace size, 512-byte units   */
    uint64_t  capacity_bytes;
    bool      volatile_wc;       /**< Volatile write cache present     */
    bool      trim_supported;    /**< DSM / TRIM available             */
} uiox_nvme_ident_t;

/* =========================================================================
 * Hardware descriptor
 * ====================================================================== */
typedef struct {
    uintptr_t            bar0;    /**< BAR0 window — from PCIe, not fixed */
    uint32_t             irq;
    uint32_t             caps;
    /* Controller state */
    uint32_t             nsid;
    uint32_t             queue_depth;
    bool                 enabled; /**< CC.EN set, CSTS.RDY observed     */
    /* Device info */
    uiox_nvme_ident_t    ident;
    bool                 dev_ready;
    /* Pending IRQ */
    volatile uint32_t    pending_irq;
    /* Private (ops vtable) */
    const void                *ops;
    void                     *drv_priv;
} uiox_nvme_hw_t;

/* =========================================================================
 * Hardware operations vtable (12-op table)
 *
 * Slimmer than uiox_emmc_hw_ops_t's 18 by exactly the slots PCIe makes
 * meaningless: set_clock, set_bus_width, set_speed_mode, tuning, crc7,
 * crc16.  send_cmd survives as the doorbell submit — the one command
 * concept NVMe keeps.
 * ====================================================================== */
typedef struct {
    int  (*init)          (uiox_nvme_hw_t *hw);
    void (*deinit)        (uiox_nvme_hw_t *hw);
    int  (*power_on)      (uiox_nvme_hw_t *hw);
    void (*power_off)     (uiox_nvme_hw_t *hw);

    uint32_t (*reg_read)  (uiox_nvme_hw_t *hw, uint32_t offset);
    void (*reg_write)     (uiox_nvme_hw_t *hw,
                           uint32_t offset, uint32_t val);

    /* Controller enable/disable — the CC.EN ↔ CSTS.RDY handshake,
     * including the CSTS.CFS abort.  One slot because the sequence is
     * indivisible: a partial enable leaves the controller in an
     * undefined state the next call cannot recover from. */
    int  (*enable)        (uiox_nvme_hw_t *hw, bool on);

    /* Queue submission: one command, one doorbell write.  `opcode` is
     * UIOX_NVME_* above; lba/sectors are the I/O payload. */
    int  (*submit)        (uiox_nvme_hw_t *hw, uint8_t opcode,
                           uint64_t lba, uint8_t *buf, uint32_t sectors);

    int  (*read_blocks)   (uiox_nvme_hw_t *hw, uint64_t lba,
                           uint8_t *buf, uint32_t count);
    int  (*write_blocks)  (uiox_nvme_hw_t *hw, uint64_t lba,
                           const uint8_t *buf, uint32_t count);

    /* Identify controller + namespace → hw->ident and hw->nsid. */
    int  (*identify)      (uiox_nvme_hw_t *hw);

    /* Flush the volatile write cache.  NULL when caps has no VWC. */
    int  (*flush)         (uiox_nvme_hw_t *hw);

    void (*isr)           (uiox_nvme_hw_t *hw);
} uiox_nvme_hw_ops_t;

/* =========================================================================
 * HAL public API
 * ====================================================================== */
int      uiox_nvme_hw_init         (uiox_nvme_hw_t *hw,
                                    const uiox_nvme_hw_ops_t *ops);
void     uiox_nvme_hw_deinit       (uiox_nvme_hw_t *hw);
int      uiox_nvme_hw_power_on     (uiox_nvme_hw_t *hw);
void     uiox_nvme_hw_power_off    (uiox_nvme_hw_t *hw);
uint32_t uiox_nvme_hw_reg_read     (uiox_nvme_hw_t *hw, uint32_t offset);
void     uiox_nvme_hw_reg_write    (uiox_nvme_hw_t *hw,
                                    uint32_t offset, uint32_t val);
int      uiox_nvme_hw_enable       (uiox_nvme_hw_t *hw, bool on);
int      uiox_nvme_hw_submit       (uiox_nvme_hw_t *hw, uint8_t opcode,
                                    uint64_t lba, uint8_t *buf,
                                    uint32_t sectors);
int      uiox_nvme_hw_read_blocks  (uiox_nvme_hw_t *hw, uint64_t lba,
                                    uint8_t *buf, uint32_t count);
int      uiox_nvme_hw_write_blocks (uiox_nvme_hw_t *hw, uint64_t lba,
                                    const uint8_t *buf, uint32_t count);
int      uiox_nvme_hw_identify     (uiox_nvme_hw_t *hw);
int      uiox_nvme_hw_flush        (uiox_nvme_hw_t *hw);
void     uiox_nvme_hw_isr          (uiox_nvme_hw_t *hw);

/* =========================================================================
 * Firmware binding (02_FwHal / libuiox_fw_<arch>.a)
 *
 * Declared here, defined in uiox_nvme_fwops.c.  The i2c dev type is
 * forward-declared so this header does not pull FwHal headers into every
 * translation unit that includes it.
 *
 * Chips with a firmware entry: nvme (reached through the PCIe ECAM walk).
 * ====================================================================== */
struct uiox_i2c_dev;

int  uiox_nvme_fwbind(uiox_nvme_hw_t *hw, struct uiox_i2c_dev *i2c);

/* Decode an identify payload into hw->ident + hw->nsid.  Outside the
 * vtable because it touches no hardware — 10_DI decodes what the driver
 * fetched, as uiox_emmc_hw_parse_ext_csd() does. */
void uiox_nvme_hw_parse_ident(uiox_nvme_hw_t *hw, const uint8_t *id);

static inline uint32_t uiox_nvme_caps(const uiox_nvme_hw_t *hw)
{ return hw ? hw->caps : 0u; }


#ifdef __cplusplus
}
#endif
#endif /* UIOX_NVME_HW_H */
