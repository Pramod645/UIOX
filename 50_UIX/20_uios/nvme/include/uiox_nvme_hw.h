/**
 * @file  uiox_nvme_hw.h
 * @brief UIOX NVMe SSD Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - NVMe 1.4 / 2.0 (NVM Express Base Specification)
 *   - PCIe 4.0 x4 (64 GT/s) and PCIe 5.0 x4 (128 GT/s)
 *   - M.2 2280 (Key M) form factor
 *   - Admin Queue (AQ) + up to 64 I/O Submission/Completion Queues
 *   - MSI-X interrupt routing (per-queue vectors)
 *   - Namespace management (up to 16 namespaces)
 *   - Autonomous Power State Transitions (APST)
 *   - NVM Command Set: Read, Write, Flush, DSM (Deallocate/TRIM)
 *   - Admin Command Set: Identify, Get/Set Features, SMART/Log
 *
 * Owns:
 *   - PCIe BAR0 MMIO register access (Controller Registers)
 *   - Submission Queue (SQ) and Completion Queue (CQ) management
 *   - Doorbell register writes (SQ tail, CQ head)
 *   - PRD (Physical Region Descriptor) / PRP (Physical Region Pages)
 *   - Controller reset (CC.EN = 0 → CC.EN = 1)
 *   - GPIO: PERST# (PCIe reset), CLKREQ# (clock request)
 *
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_NVME_HW_H
 #define UIOX_NVME_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PCIe generation
  * ====================================================================== */
 
 typedef enum {
     UIOX_NVME_PCIE_GEN3 = 0,  /**< PCIe 3.0 x4  (32 GT/s)              */
     UIOX_NVME_PCIE_GEN4,       /**< PCIe 4.0 x4  (64 GT/s)              */
     UIOX_NVME_PCIE_GEN5,       /**< PCIe 5.0 x4  (128 GT/s)             */
 } uiox_nvme_pcie_gen_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_NVME_CAP_64BIT_DMA     (1u << 0)  /**< 64-bit host memory  */
 #define UIOX_NVME_CAP_MSI_X         (1u << 1)  /**< MSI-X interrupts    */
 #define UIOX_NVME_CAP_MULTI_NS      (1u << 2)  /**< Multiple namespaces  */
 #define UIOX_NVME_CAP_NS_MGMT       (1u << 3)  /**< Namespace management */
 #define UIOX_NVME_CAP_TRIM          (1u << 4)  /**< Deallocate (DSM)     */
 #define UIOX_NVME_CAP_APST          (1u << 5)  /**< Auto power state     */
 #define UIOX_NVME_CAP_SMART         (1u << 6)  /**< SMART / Health log   */
 #define UIOX_NVME_CAP_FW_UPDATE     (1u << 7)  /**< Firmware update      */
 #define UIOX_NVME_CAP_VOLATILE_WC   (1u << 8)  /**< Volatile write cache */
 #define UIOX_NVME_CAP_CMB           (1u << 9)  /**< Controller mem buf   */
 #define UIOX_NVME_CAP_PMR           (1u << 10) /**< Persistent mem reg   */
 #define UIOX_NVME_CAP_CRYPTO_ERASE  (1u << 11) /**< Crypto erase         */
 #define UIOX_NVME_CAP_ZONED_NS      (1u << 12) /**< Zoned namespace      */
 #define UIOX_NVME_CAP_SGL           (1u << 13) /**< Scatter-gather list  */
 #define UIOX_NVME_CAP_PERST_GPIO    (1u << 14) /**< PERST# GPIO control  */
 
 /* =========================================================================
  * NVMe Controller Registers (BAR0 MMIO offsets — NVMe Spec §3.1)
  * ====================================================================== */
 
 /* Controller Capabilities */
 #define NVME_REG_CAP_LO             0x0000u  /**< CAP[31:0]               */
 #define NVME_REG_CAP_HI             0x0004u  /**< CAP[63:32]              */
 #define NVME_REG_VS                 0x0008u  /**< Version                 */
 #define NVME_REG_INTMS              0x000Cu  /**< Interrupt Mask Set      */
 #define NVME_REG_INTMC              0x0010u  /**< Interrupt Mask Clear    */
 #define NVME_REG_CC                 0x0014u  /**< Controller Configuration*/
 #define NVME_REG_CSTS               0x001Cu  /**< Controller Status       */
 #define NVME_REG_NSSR               0x0020u  /**< NVM Subsystem Reset     */
 #define NVME_REG_AQA                0x0024u  /**< Admin Queue Attributes  */
 #define NVME_REG_ASQ_LO             0x0028u  /**< Admin SQ Base (lo)      */
 #define NVME_REG_ASQ_HI             0x002Cu  /**< Admin SQ Base (hi)      */
 #define NVME_REG_ACQ_LO             0x0030u  /**< Admin CQ Base (lo)      */
 #define NVME_REG_ACQ_HI             0x0034u  /**< Admin CQ Base (hi)      */
 #define NVME_REG_CMBLOC             0x0038u  /**< CMB Location            */
 #define NVME_REG_CMBSZ              0x003Cu  /**< CMB Size                */
 #define NVME_REG_BPINFO             0x0040u  /**< Boot Partition Info     */
 #define NVME_REG_PMRCAP             0x0E00u  /**< PMR Capabilities        */
 #define NVME_REG_PMRCTL             0x0E04u  /**< PMR Control             */
 
 /* Doorbell base: 0x1000 + (QID * 2 + direction) * (4 << CAP.DSTRD) */
 #define NVME_DOORBELL_BASE          0x1000u
 #define NVME_DOORBELL_STRIDE        4u       /**< default: CAP.DSTRD=0   */
 
 /* CC (Controller Configuration) bits */
 #define NVME_CC_EN                  (1u << 0)   /**< Enable                */
 #define NVME_CC_CSS_NVM             (0u << 4)   /**< NVM command set       */
 #define NVME_CC_MPS_SHIFT           7u
 #define NVME_CC_AMS_RR              (0u << 11)  /**< Round robin arb       */
 #define NVME_CC_IOSQES_SHIFT        16u
 #define NVME_CC_IOCQES_SHIFT        20u
 #define NVME_CC_IOSQES_64B          (6u << NVME_CC_IOSQES_SHIFT)  /* 2^6 */
 #define NVME_CC_IOCQES_16B          (4u << NVME_CC_IOCQES_SHIFT)  /* 2^4 */
 
 /* CSTS (Controller Status) bits */
 #define NVME_CSTS_RDY               (1u << 0)   /**< Ready                 */
 #define NVME_CSTS_CFS               (1u << 1)   /**< Controller Fatal      */
 #define NVME_CSTS_SHST_MASK         (3u << 2)   /**< Shutdown status       */
 #define NVME_CSTS_SHST_NORMAL       (0u << 2)
 #define NVME_CSTS_SHST_OCCURRING    (1u << 2)
 #define NVME_CSTS_SHST_COMPLETE     (2u << 2)
 #define NVME_CSTS_NSSRO             (1u << 4)   /**< NVM Subsystem Reset   */
 
 /* CAP (Capabilities) bit fields */
 #define NVME_CAP_MQES_MASK          0xFFFFu     /**< Max Q entries - 1     */
 #define NVME_CAP_CQR                (1u << 16)  /**< Contiguous Q required */
 #define NVME_CAP_TO_SHIFT           24u         /**< Timeout (500 ms units)*/
 #define NVME_CAP_TO_MASK            (0xFFu << NVME_CAP_TO_SHIFT)
 #define NVME_CAP_DSTRD_SHIFT        32u         /**< Doorbell stride       */
 #define NVME_CAP_CSS_NVM            (1u << 37)  /**< NVM cmd set supported */
 #define NVME_CAP_MPSMIN_SHIFT       48u
 #define NVME_CAP_MPSMAX_SHIFT       52u
 
 /* =========================================================================
  * NVMe Admin Commands (opcode)
  * ====================================================================== */
 
 #define NVME_ADMIN_DELETE_SQ        0x00u
 #define NVME_ADMIN_CREATE_SQ        0x01u
 #define NVME_ADMIN_GET_LOG_PAGE     0x02u
 #define NVME_ADMIN_DELETE_CQ        0x04u
 #define NVME_ADMIN_CREATE_CQ        0x05u
 #define NVME_ADMIN_IDENTIFY         0x06u
 #define NVME_ADMIN_ABORT            0x08u
 #define NVME_ADMIN_SET_FEATURES     0x09u
 #define NVME_ADMIN_GET_FEATURES     0x0Au
 #define NVME_ADMIN_ASYNC_EVENT      0x0Cu
 #define NVME_ADMIN_NS_MGMT          0x0Du
 #define NVME_ADMIN_FW_COMMIT        0x10u
 #define NVME_ADMIN_FW_DOWNLOAD      0x11u
 #define NVME_ADMIN_NS_ATTACH        0x15u
 #define NVME_ADMIN_FORMAT_NVM       0x80u
 #define NVME_ADMIN_SECURITY_SEND    0x81u
 #define NVME_ADMIN_SECURITY_RECV    0x82u
 
 /* =========================================================================
  * NVMe I/O Commands (opcode)
  * ====================================================================== */
 
 #define NVME_IO_FLUSH               0x00u
 #define NVME_IO_WRITE               0x01u
 #define NVME_IO_READ                0x02u
 #define NVME_IO_WRITE_UNCORRECTABLE 0x04u
 #define NVME_IO_COMPARE             0x05u
 #define NVME_IO_WRITE_ZEROES        0x08u
 #define NVME_IO_DSM                 0x09u   /**< Dataset Management (TRIM)*/
 #define NVME_IO_VERIFY              0x0Cu
 #define NVME_IO_COPY                0x19u
 
 /* =========================================================================
  * Identify CNS (Controller or Namespace Structure) values
  * ====================================================================== */
 
 #define NVME_IDENTIFY_CNS_NS        0x00u   /**< Identify Namespace       */
 #define NVME_IDENTIFY_CNS_CTRL      0x01u   /**< Identify Controller      */
 #define NVME_IDENTIFY_CNS_NS_LIST   0x02u   /**< Active NS list           */
 #define NVME_IDENTIFY_CNS_NS_DESC   0x03u   /**< NS ID descriptor list    */
 
 /* =========================================================================
  * Feature IDs (Get/Set Features)
  * ====================================================================== */
 
 #define NVME_FEAT_ARBITRATION       0x01u
 #define NVME_FEAT_POWER_MGMT        0x02u
 #define NVME_FEAT_LBA_RANGE         0x03u
 #define NVME_FEAT_TEMP_THRESHOLD    0x04u
 #define NVME_FEAT_ERR_RECOVERY      0x05u
 #define NVME_FEAT_VOLATILE_WC       0x06u
 #define NVME_FEAT_NUM_QUEUES        0x07u
 #define NVME_FEAT_IRQ_COALESCING    0x08u
 #define NVME_FEAT_IRQ_CONFIG        0x09u
 #define NVME_FEAT_WRITE_ATOMICITY   0x0Au
 #define NVME_FEAT_ASYNC_EVENT       0x0Bu
 #define NVME_FEAT_AUTO_POWER_STATE  0x0Cu
 #define NVME_FEAT_HOST_MEM_BUF      0x0Du
 #define NVME_FEAT_TIMESTAMP         0x0Eu
 #define NVME_FEAT_KEEPALIVE         0x0Fu
 
 /* =========================================================================
  * Log Page IDs
  * ====================================================================== */
 
 #define NVME_LOG_ERROR              0x01u
 #define NVME_LOG_SMART              0x02u
 #define NVME_LOG_FW_SLOT            0x03u
 #define NVME_LOG_CHANGED_NS         0x04u
 #define NVME_LOG_CMD_EFFECTS        0x05u
 #define NVME_LOG_DEVICE_SELF_TEST   0x06u
 #define NVME_LOG_TELEMETRY_HOST     0x07u
 #define NVME_LOG_SANITIZE_STATUS    0x81u
 
 /* =========================================================================
  * NVMe Submission Queue Entry (64 bytes)
  * ====================================================================== */
 
 typedef struct __attribute__((packed, aligned(64))) {
     uint8_t  opc;        /**< Opcode                                      */
     uint8_t  fuse;       /**< Fused operation flags                       */
     uint16_t cid;        /**< Command identifier                          */
     uint32_t nsid;       /**< Namespace identifier                        */
     uint64_t rsvd;
     uint64_t mptr;       /**< Metadata pointer                            */
     uint64_t prp1;       /**< PRP entry 1 (or SGL segment)               */
     uint64_t prp2;       /**< PRP entry 2 (or SGL segment)               */
     uint32_t cdw10;      /**< Command-specific dword 10                   */
     uint32_t cdw11;
     uint32_t cdw12;
     uint32_t cdw13;
     uint32_t cdw14;
     uint32_t cdw15;
 } uiox_nvme_sqe_t;
 
 /* =========================================================================
  * NVMe Completion Queue Entry (16 bytes)
  * ====================================================================== */
 
 typedef struct __attribute__((packed, aligned(16))) {
     uint32_t dw0;        /**< Command-specific result                     */
     uint32_t dw1;        /**< Reserved                                    */
     uint16_t sqhd;       /**< SQ head pointer                            */
     uint16_t sqid;       /**< SQ identifier                              */
     uint16_t cid;        /**< Command identifier                          */
     uint16_t status;     /**< Status field + phase bit                   */
 } uiox_nvme_cqe_t;
 
 /* CQE status field: bits [15:1] = status, bit [0] = phase tag */
 #define NVME_CQE_STATUS_P           (1u << 0)   /**< Phase tag             */
 #define NVME_CQE_STATUS_SC_MASK     (0xFFu << 1)
 #define NVME_CQE_STATUS_SCT_MASK    (0x07u << 9)
 #define NVME_CQE_STATUS_SUCCESS     0x0000u
 #define NVME_CQE_SC(s)              (((s)->status >> 1u) & 0xFFu)
 #define NVME_CQE_SCT(s)             (((s)->status >> 9u) & 0x07u)
 
 /* =========================================================================
  * Queue parameters
  * ====================================================================== */
 
 #define UIOX_NVME_MAX_IO_QUEUES     64u
 #define UIOX_NVME_ADMIN_Q_DEPTH     32u
 #define UIOX_NVME_IO_Q_DEPTH        256u
 #define UIOX_NVME_MAX_NAMESPACES    16u
 #define UIOX_NVME_LBA_SIZE          512u    /**< Default LBA size         */
 #define UIOX_NVME_MAX_TRANSFER_PAGES 256u   /**< Max PRP pages per cmd    */
 
 /* =========================================================================
  * Namespace descriptor
  * ====================================================================== */
 
 typedef struct {
     uint32_t nsid;
     uint64_t nsze;          /**< Namespace size (LBAs)                   */
     uint64_t ncap;          /**< Namespace capacity (LBAs)               */
     uint64_t nuse;          /**< Namespace utilisation (LBAs)            */
     uint32_t lba_size;      /**< LBA data size (bytes)                   */
     bool     active;
 } uiox_nvme_ns_t;
 
 /* =========================================================================
  * Controller identity (from Identify Controller)
  * ====================================================================== */
 
 #define UIOX_NVME_MODEL_LEN         64u
 
 typedef struct {
     char     model[UIOX_NVME_MODEL_LEN];  /**< MN field (null-term)      */
     char     serial[21];                   /**< SN field (null-term)      */
     char     fw_rev[9];                    /**< FR field (null-term)      */
     uint16_t vid;                          /**< Vendor ID (PCIe)          */
     uint16_t ssvid;                        /**< Subsystem vendor ID       */
     uint32_t nn;                           /**< Number of namespaces      */
     uint32_t mdts;                         /**< Max data transfer size    */
     uint8_t  cntlid;                       /**< Controller ID             */
     uint8_t  ver_major;
     uint8_t  ver_minor;
     /* Feature support */
     bool     volatile_wc;
     bool     apst_supported;
     bool     trim_supported;
     bool     crypto_erase;
     bool     fw_update;
     /* SMART health thresholds */
     uint8_t  warn_composite_temp;          /**< Temperature warning (°C) */
     uint8_t  crit_composite_temp;          /**< Temperature critical     */
 } uiox_nvme_ctrl_id_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t            bar0;             /**< BAR0 MMIO base             */
     uint32_t             irq_base;         /**< MSI-X vector base          */
     uint32_t             num_irqs;         /**< MSI-X vectors allocated    */
     uint32_t             caps;
     uiox_nvme_pcie_gen_t pcie_gen;
     char                 model[UIOX_NVME_MODEL_LEN];
     /* Controller identity */
     uiox_nvme_ctrl_id_t  ctrl_id;
     /* Namespaces */
     uiox_nvme_ns_t       ns[UIOX_NVME_MAX_NAMESPACES];
     uint32_t             active_nsid;      /**< Primary namespace         */
     /* Queue state */
     uint32_t             sq_depth;         /**< I/O SQ depth              */
     uint32_t             cq_depth;         /**< I/O CQ depth              */
     uint32_t             num_io_queues;    /**< I/O queue pairs created   */
     /* Command ID counter (wrapping 16-bit) */
     volatile uint16_t    cmd_id;
     /* Controller state */
     bool                 ready;
     bool                 volatile_wc_enabled;
     /* Pending IRQ bitmask (per queue) */
     volatile uint32_t    pending_irq;
     /* Private (ops vtable) */
     void                *priv;
 } uiox_nvme_hw_t;
 
 /* Pending IRQ bits */
 #define UIOX_NVME_IRQ_ADMIN_CQ      (1u << 0)  /**< Admin CQ completion   */
 #define UIOX_NVME_IRQ_IO_CQ0        (1u << 1)  /**< I/O CQ 0 completion   */
 #define UIOX_NVME_IRQ_IO_CQ1        (1u << 2)  /**< I/O CQ 1 completion   */
 #define UIOX_NVME_IRQ_ERROR         (1u << 31) /**< Controller fatal error */
 
 /* GPIO pin IDs */
 #define UIOX_NVME_GPIO_PERST_N      0u         /**< PCIe reset (act-low)  */
 #define UIOX_NVME_GPIO_CLKREQ_N     1u         /**< Clock request         */
 #define UIOX_NVME_GPIO_DEVSLP       2u         /**< Device sleep          */
 
 /* =========================================================================
  * Hardware operations vtable (18-op table)
  * ====================================================================== */
 
 typedef struct {
     /* Lifecycle */
     int  (*init)          (uiox_nvme_hw_t *hw);
     void (*deinit)        (uiox_nvme_hw_t *hw);
     int  (*ctrl_reset)    (uiox_nvme_hw_t *hw);
     int  (*ctrl_enable)   (uiox_nvme_hw_t *hw);
 
     /* BAR0 MMIO access */
     uint32_t (*reg_read32)(uiox_nvme_hw_t *hw, uint32_t offset);
     uint64_t (*reg_read64)(uiox_nvme_hw_t *hw, uint32_t offset);
     void (*reg_write32)   (uiox_nvme_hw_t *hw,
                            uint32_t offset, uint32_t val);
     void (*reg_write64)   (uiox_nvme_hw_t *hw,
                            uint32_t offset, uint64_t val);
 
     /* Doorbell */
     void (*sq_doorbell)   (uiox_nvme_hw_t *hw,
                            uint16_t qid, uint16_t tail);
     void (*cq_doorbell)   (uiox_nvme_hw_t *hw,
                            uint16_t qid, uint16_t head);
 
     /* Admin command (blocking, timeout-based) */
     int  (*admin_cmd)     (uiox_nvme_hw_t *hw,
                            uiox_nvme_sqe_t *sqe,
                            uiox_nvme_cqe_t *cqe,
                            void *data, uint32_t data_len);
 
     /* I/O command submission */
     int  (*io_submit)     (uiox_nvme_hw_t *hw,
                            uint16_t qid, uiox_nvme_sqe_t *sqe);
     int  (*io_poll)       (uiox_nvme_hw_t *hw,
                            uint16_t qid, uiox_nvme_cqe_t *cqe);
 
     /* DMA */
     int  (*dma_read)      (uiox_nvme_hw_t *hw,
                            uint32_t nsid, uint64_t slba,
                            uint8_t *buf, uint32_t nlb);
     int  (*dma_write)     (uiox_nvme_hw_t *hw,
                            uint32_t nsid, uint64_t slba,
                            const uint8_t *buf, uint32_t nlb);
 
     /* GPIO */
     void (*gpio_write)    (uiox_nvme_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)     (uiox_nvme_hw_t *hw, uint32_t pin);
 
     /* ISR */
     void (*isr)           (uiox_nvme_hw_t *hw);
 } uiox_nvme_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_nvme_hw_init        (uiox_nvme_hw_t *hw,
                                     const uiox_nvme_hw_ops_t *ops);
 void     uiox_nvme_hw_deinit      (uiox_nvme_hw_t *hw);
 int      uiox_nvme_hw_ctrl_reset  (uiox_nvme_hw_t *hw);
 int      uiox_nvme_hw_ctrl_enable (uiox_nvme_hw_t *hw);
 uint32_t uiox_nvme_hw_reg_read32  (uiox_nvme_hw_t *hw, uint32_t offset);
 uint64_t uiox_nvme_hw_reg_read64  (uiox_nvme_hw_t *hw, uint32_t offset);
 void     uiox_nvme_hw_reg_write32 (uiox_nvme_hw_t *hw,
                                     uint32_t offset, uint32_t val);
 void     uiox_nvme_hw_reg_write64 (uiox_nvme_hw_t *hw,
                                     uint32_t offset, uint64_t val);
 void     uiox_nvme_hw_sq_doorbell (uiox_nvme_hw_t *hw,
                                     uint16_t qid, uint16_t tail);
 void     uiox_nvme_hw_cq_doorbell (uiox_nvme_hw_t *hw,
                                     uint16_t qid, uint16_t head);
 int      uiox_nvme_hw_admin_cmd   (uiox_nvme_hw_t *hw,
                                     uiox_nvme_sqe_t *sqe,
                                     uiox_nvme_cqe_t *cqe,
                                     void *data, uint32_t data_len);
 int      uiox_nvme_hw_io_submit   (uiox_nvme_hw_t *hw,
                                     uint16_t qid, uiox_nvme_sqe_t *sqe);
 int      uiox_nvme_hw_io_poll     (uiox_nvme_hw_t *hw,
                                     uint16_t qid, uiox_nvme_cqe_t *cqe);
 int      uiox_nvme_hw_dma_read    (uiox_nvme_hw_t *hw,
                                     uint32_t nsid, uint64_t slba,
                                     uint8_t *buf, uint32_t nlb);
 int      uiox_nvme_hw_dma_write   (uiox_nvme_hw_t *hw,
                                     uint32_t nsid, uint64_t slba,
                                     const uint8_t *buf, uint32_t nlb);
 
 //static inline bool uiox_nvme_is_ready(const uiox_nvme_hw_t *hw)
 //{ return hw && hw->ready; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NVME_HW_H */
 